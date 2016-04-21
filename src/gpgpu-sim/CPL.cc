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
    int** cpl_actual = (int**)calloc(total_cta+1, sizeof(int*));
    cpl_actual[0] = (int*)calloc(2, sizeof(int));
    cpl_actual[0][0] = total_cta;
    cpl_actual[0][1] = num_warps_per_cta;
    for (unsigned j = 1; j <= total_cta; j++){
      cpl_actual[j] = (int*)calloc(num_warps_per_cta, sizeof(int));
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
    for (int j = 1; j <= tw_cpl_actual[i][0][0]; j++){
      for (int k = 0; k < tw_cpl_actual[i][0][1]; k++){
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
  tw_cpl_oracle = (int***)calloc(num_kernel, sizeof(int**));
  for (unsigned i = 0; i < num_kernel; i++){
    unsigned kid, num_cta, num_warps_per_cta;
    fscanf(fp, "%d %d %d", &kid, &num_cta, &num_warps_per_cta);
    tw_cpl_oracle[i] = (int**)calloc(num_cta+1, sizeof(int*));
    tw_cpl_oracle[i][0] = (int*)calloc(2, sizeof(int));
    tw_cpl_oracle[i][0][0] = num_cta;
    tw_cpl_oracle[i][0][1] = num_warps_per_cta;
    for (unsigned j = 1; j <= num_cta; j++){
      tw_cpl_oracle[i][j] = (int*)calloc(num_warps_per_cta, sizeof(int));
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
    for (int j = 1; j <= tw_cpl_oracle[i][0][0]; j++){
      printf("CTA %d:\n", j-1);
      for (int k = 0; k < tw_cpl_oracle[i][0][1]; k++){
	printf("%d,", tw_cpl_oracle[i][j][k]);
      }
      printf("\n");
    }
    printf("\n");
  }
#endif
}
int shader_core_stats::tw_get_oracle_CPL_counter(unsigned kernel_id, unsigned block_num, unsigned warp_id_within_block) const
{
  return tw_with_oracle_cpl ? tw_cpl_oracle[kernel_id][block_num+1][warp_id_within_block] : 0;
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

void shader_core_ctx::tw_record_oracle_cpl(unsigned cta_num)
{
  unsigned start_warp_id, end_warp_id;
  tw_get_start_end_warp_id(&start_warp_id, &end_warp_id, cta_num); // calculate start and end warp_id
#ifdef TW_DEBUG
  tw_print_CPL_counters(start_warp_id, end_warp_id);
  printf("TW: Kernel %d core %d cta %d (%d in kernel)\'s warp criticalities:", m_kernel->get_uid(), m_sid, cta_num, tw_cta_num_in_kernel[cta_num]);
#endif
  for (unsigned i = start_warp_id; i < end_warp_id; i++){
    tw_oracle_CPL_sanity_check(i, m_stats->tw_warp_cta_cycle_dist[m_sid][i]);
#ifdef TW_DEBUG
    printf("\t W%d: %d", i, m_stats->tw_warp_cta_cycle_dist[m_sid][i]);
#endif
    m_stats->tw_cpl_actual[m_kernel->get_uid()-1][tw_cta_num_in_kernel[cta_num]+1][i-start_warp_id] = m_stats->tw_warp_cta_cycle_dist[m_sid][i];
  }
  printf("\n");
}

std::vector<int> shader_core_ctx::tw_get_current_CPL_counters() const
{
  std::vector<int> ret;
  for (unsigned i = 0; i < m_warp.size(); i++){
    if (m_config->tw_gpgpu_oracle_cpl){
      assert(m_stats->tw_with_oracle_cpl);
      ret.push_back(m_warp[i].tw_get_oracle_CPL());
    }
    else{
      ret.push_back(m_warp[i].tw_get_actual_CPL());
    }
  }
  return ret;
}
void shader_core_ctx::tw_print_CPL_counters(unsigned start_id, unsigned end_id) const
{
#ifdef TW_DEBUG
  printf("TW: oracle counters loaded for warp %d - warp %d:\n", start_id, end_id);
  std::vector<int> counters = tw_get_current_CPL_counters();
  for (unsigned i = start_id; i < end_id; i++){
    printf("W%d: %d, ", i, counters[i]);
  }
  printf("\n");
#endif
}

void shader_core_ctx::tw_oracle_CPL_sanity_check(unsigned warp_id, int actual_counter) const
{
  assert(m_config->tw_gpgpu_oracle_cpl);
  if (m_warp[warp_id].tw_get_oracle_CPL() != actual_counter){
    printf("The actual oracle CPL is not the same as the previous one on shader %d warp %d: %d vs %d\n", m_sid, warp_id, m_warp[warp_id].tw_get_oracle_CPL(), actual_counter);
    assert(0);
  }
}
