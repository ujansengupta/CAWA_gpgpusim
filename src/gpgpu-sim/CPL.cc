/*********************************************************
 ****  Code for Criticality Prediction Logic          ****
 ****  Contributed by Tiancong Wang                   ****
 ****  Also made change to:                           ****
 ****     ../gpgpusim_entrypoint.cc                   ****
 ****     gpu-sim.cc                                  ****
 ****     shader.cc                                   ****
 ****  04/07/16                                       ****
 *********************************************************/

#include <float.h>
#include "gpu-sim.h"
#include "shader.h"
#include <string.h>

#define TW_DEBUG

//***************** class gpgpu_sim *********************/
void gpgpu_sim::tw_store_oracle_cpl() const
{
  char cpl_name[50];
  strcpy(cpl_name, m_shader_config->gpgpu_scheduler_string);
  strcat(cpl_name, ".cpl");
  if (!m_shader_stats->tw_if_with_oracle_cpl()){
    FILE* fp = fopen(cpl_name, "w");
    m_shader_stats->tw_store_oracle_cpl(fp);
    fclose(fp);
  }
}
void gpgpu_sim::tw_load_oracle_cpl()
{
  char cpl_name[50];
  strcpy(cpl_name, m_shader_config->gpgpu_scheduler_string);
  strcat(cpl_name, ".cpl");
  FILE* fp = fopen(cpl_name, "r");
  if (fp){// Only load oracle cpl file if exists
    printf("Load criticality counter from file %s\n", cpl_name);
    m_shader_stats->tw_load_oracle_cpl(fp);
    fclose(fp);
  }
}

//****************** class shader_core_stats *****************/
void shader_core_stats::tw_launch_kernel(unsigned kid, unsigned total_cta, unsigned num_warps_per_cta)
{
  for (unsigned i = tw_cpl_actual.size(); i < kid; i++){
    unsigned** cpl_actual = (unsigned**)calloc(total_cta+1, sizeof(unsigned*));
    cpl_actual[0] = (unsigned*)calloc(2, sizeof(unsigned));
    cpl_actual[0][0] = total_cta;
    cpl_actual[0][1] = num_warps_per_cta;
    for (unsigned j = 1; j <= total_cta; j++){
      cpl_actual[j] = (unsigned*)calloc(num_warps_per_cta, sizeof(unsigned));
    }
    tw_cpl_actual.push_back(cpl_actual);
  }
  assert(tw_cpl_actual[kid-1] != NULL);
  tw_num_launched_kernels++;
}

void shader_core_stats::tw_store_oracle_cpl(FILE* fp) const
{
  fprintf(fp, "%d\n", tw_num_launched_kernels);
  for (unsigned i = 0; i < tw_num_launched_kernels; i++){
    fprintf(fp, "%d %d %d\n", i, tw_cpl_actual[i][0][0], tw_cpl_actual[i][0][1]);
    for (unsigned j = 1; j <= tw_cpl_actual[i][0][0]; j++){
      for (unsigned k = 0; k < tw_cpl_actual[i][0][1]; k++){
	fprintf(fp, "%d ", tw_cpl_actual[i][j][k]);
      }
    }
    fprintf(fp, "\n");
  }
}
void shader_core_stats::tw_load_oracle_cpl(FILE* fp)
{
  unsigned num_kernel;
  fscanf(fp, "%d", &num_kernel);
  tw_cpl_oracle = (unsigned***)calloc(num_kernel, sizeof(unsigned**));
  for (unsigned i = 0; i < num_kernel; i++){
    unsigned kid, num_cta, num_warps_per_cta;
    fscanf(fp, "%d %d %d", &kid, &num_cta, &num_warps_per_cta);
    tw_cpl_oracle[i] = (unsigned**)calloc(num_cta+1, sizeof(unsigned*));
    tw_cpl_oracle[i][0] = (unsigned*)calloc(2, sizeof(unsigned));
    tw_cpl_oracle[i][0][0] = num_cta;
    tw_cpl_oracle[i][0][1] = num_warps_per_cta;
    for (unsigned j = 1; j <= num_cta; j++){
      tw_cpl_oracle[i][j] = (unsigned*)calloc(num_warps_per_cta, sizeof(unsigned));
      for (unsigned k = 0; k < num_warps_per_cta; k++){
	fscanf(fp, "%d ", &tw_cpl_oracle[i][j][k]);
      }
    }
  }
  tw_with_oracle_cpl = true;
#ifdef TW_DEBUG
  printf("TW: oracle critical warp info loaded (%d kernels)\n", num_kernel);
  for (unsigned i = 0; i < num_kernel; i++){
    printf("Kernel %d has %d CTAs with %d warps in each:\n", i, tw_cpl_oracle[i][0][0], tw_cpl_oracle[i][0][1]);
    for (unsigned j = 1; j <= tw_cpl_oracle[i][0][0]; j++){
      printf("CTA %d:\n", j-1);
      for (unsigned k = 0; k < tw_cpl_oracle[i][0][1]; k++){
	printf("%d,", tw_cpl_oracle[i][j][k]);
      }
      printf("\n");
    }
    printf("\n");
  }
#endif
}

//********************** class shader_core_ctx *******************/
void shader_core_ctx::tw_get_start_end_warp_id(unsigned* start_warp_id, unsigned* end_warp_id, unsigned cta_num) const{
  unsigned cta_size = m_kernel->threads_per_cta();
  unsigned padded_cta_size = cta_size;
  if (cta_size%m_config->warp_size)
    padded_cta_size = ((cta_size/m_config->warp_size)+1)*(m_config->warp_size);
  assert(padded_cta_size % m_config->warp_size == 0);
  *start_warp_id = cta_num * padded_cta_size / m_config->warp_size;
  *end_warp_id = *start_warp_id + padded_cta_size / m_config->warp_size;
}

void shader_core_ctx::tw_rank_oracle_cpl(unsigned cta_num, unsigned** ranked_oracle_cpl) const{
#ifdef TW_DEBUG
  printf("TW: Kernel %d core %d cta %d (%d in kernel)\'s criticalital warp:", m_kernel->get_uid(), m_sid, cta_num, tw_cta_num_in_kernel[cta_num]);
#endif
  assert(m_stats->tw_if_with_oracle_cpl());
  unsigned start_warp_id, end_warp_id;
  tw_get_start_end_warp_id(&start_warp_id, &end_warp_id, cta_num);
  unsigned* cpl_oracle = m_stats->tw_cpl_oracle[m_kernel->get_uid()-1][tw_cta_num_in_kernel[cta_num]+1];
  unsigned num_warps_per_cta = m_stats->tw_cpl_oracle[m_kernel->get_uid()-1][0][1];
  *ranked_oracle_cpl = (unsigned*)calloc(num_warps_per_cta, sizeof(unsigned));
  bool* ranked = (bool*)calloc(num_warps_per_cta, sizeof(bool));
  for (unsigned i = 0; i < num_warps_per_cta; i++){
    unsigned max_counter = 0, max_j = -1;
    for (unsigned j = 0; j < num_warps_per_cta; j++){
      if (!ranked[j]){
	if (cpl_oracle[j] > max_counter){
	  max_counter = cpl_oracle[j];
	  max_j = j;
	}
      }
    }
    assert(max_j != (unsigned)-1);
    ranked[max_j] = true;
    (*ranked_oracle_cpl)[i] = max_j + start_warp_id;
  }
}

void shader_core_ctx::tw_record_oracle_cpl(unsigned cta_num)
{
#ifdef TW_DEBUG
  printf("TW: Kernel %d core %d cta %d (%d in kernel)\'s warp criticalities:", m_kernel->get_uid(), m_sid, cta_num, tw_cta_num_in_kernel[cta_num]);
#endif
  unsigned start_warp_id, end_warp_id;
  tw_get_start_end_warp_id(&start_warp_id, &end_warp_id, cta_num); // calculate start and end warp_id
  for (unsigned i = start_warp_id; i < end_warp_id; i++){
#ifdef TW_DEBUG
    printf("\t W%d: %d", i, m_stats->tw_warp_cta_cycle_dist[m_sid][i]);
#endif
    m_stats->tw_cpl_actual[m_kernel->get_uid()-1][tw_cta_num_in_kernel[cta_num]+1][i-start_warp_id] = m_stats->tw_warp_cta_cycle_dist[m_sid][i];
  }
  printf("\n");
}
