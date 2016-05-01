/*********************************************************
 ****  Code for Criticality Prediction Logic          ****
 ****  Contributed by Tiancong Wang                   ****
 ****  Also made change to:                           ****
 ****     ../gpgpusim_entrypoint.cc                   ****
 ****     gpu-sim.cc                                  ****
 ****     shader.cc                                   ****
 ****  04/07/16 04/20/16 04/22/16 04/25/16            ****
 ****  04/26/16                                       ****
 *********************************************************/

#include <float.h>
#include "gpu-sim.h"
#include "../cuda-sim/ptx_sim.h"
#include "shader.h"
#include <string.h>
#include <algorithm>
//#define TW_DEBUG
//#define TW_DEBUG_NEW

//***************** class gpgpu_sim *********************/
void gpgpu_sim::tw_store_oracle_cpl() const
{
  char cpl_name[50];
  strcpy(cpl_name, m_shader_config->gpgpu_scheduler_string);
  strcat(cpl_name, ".cpl");
  if (m_shader_config->tw_gpgpu_store_oracle_counter){
    FILE* fp = fopen(cpl_name, "w");
    m_shader_stats->tw_store_oracle_cpl(fp);
    fclose(fp);
  }
}
void gpgpu_sim::tw_load_oracle_cpl()
{
  char cpl_name[50];
  strcpy(cpl_name, m_shader_config->tw_gpgpu_oracle_scheduler_string);
  strcat(cpl_name, ".cpl");
  FILE* fp = fopen(cpl_name, "r");
  if (m_shader_config->tw_gpgpu_load_oracle_counter){
    if (fp == NULL){
      printf("The oracle counter for scheduler %s not exists\n", m_shader_config->tw_gpgpu_oracle_scheduler_string);
      assert(0);
    }
    printf("Load criticality counter from file %s\n", cpl_name);
    m_shader_stats->tw_load_oracle_cpl(fp);
    fclose(fp);
  }
}
void gpgpu_sim::tw_print_cpl_accuracy() const
{
  if (m_shader_config->tw_calculate_cpl_accuracy){
    m_shader_stats->tw_print_cpl_accuracy(stdout);
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
void shader_core_stats::tw_print_cpl_accuracy(FILE* fp) const
{
  fprintf(fp, "TW: number of total cpl samples = %d\n", tw_total_cpl_for_accuracy);
  fprintf(fp, "TW: number of accurace cpl samples = %d\n", tw_accurate_cpl_for_accuracy);
  fprintf(fp, "TW: cpl accuracy = %.2f%%\n", 100.0*tw_accurate_cpl_for_accuracy / tw_total_cpl_for_accuracy);
}
//********************** class shader_core_config ****************/
void shader_core_config::tw_cawa_reg_options(class OptionParser * opp)
{
  option_parser_register(opp, "-gpgpu_with_oracle_cpl", OPT_BOOL, &tw_gpgpu_oracle_cpl,
			 "Use oracle CPL info or not (default=on)",
			 "1");
  option_parser_register(opp, "-gpgpu_load_oracle_counter", OPT_BOOL, &tw_gpgpu_load_oracle_counter,
			 "Load oracle CPL info or not (default=off)",
			 "0");
  option_parser_register(opp, "-gpgpu_store_oracle_counter", OPT_BOOL, &tw_gpgpu_store_oracle_counter,
			 "Store oracle CPL info or not (default=off)",
			 "0");
  option_parser_register(opp, "-gpgpu_oracle_counter_from_scheduler", OPT_CSTR, &tw_gpgpu_oracle_scheduler_string,
			 "oracle counter from which previous scheduler",
			 "lrr");
  option_parser_register(opp, "-caws_oracle_cpl_exec_cycles", OPT_BOOL, &tw_oracle_cpl_exec_cycles,
			 "on = use execution cycles from enter to complete, off = use execution cycle distribution",
			 "0");
  option_parser_register(opp, "-caws_actual_cpl_static_ninst", OPT_BOOL, &tw_actual_cpl_static_ninst,
			 "in caws, use static instruction count as initial counter for actual counter or not",
			 "0");
  option_parser_register(opp, "-caws_actual_cpl_real_cpi", OPT_BOOL, &tw_actual_cpl_real_cpi,
			 "in caws, use calculated average CPI per warp for actual counter or not",
			 "0");
  option_parser_register(opp, "-caws_actual_cpl_stall", OPT_BOOL, &tw_actual_cpl_stall,
			 "in caws, use stall cycles in actual counter or not",
			 "0");
  option_parser_register(opp, "-caws_calculate_cpl_accuracy", OPT_BOOL, &tw_calculate_cpl_accuracy,
			 "calculate CPL accuracy of the actual counter with the oracle counter or not",
			 "1");
  option_parser_register(opp, "-gpgpu_with_cacp", OPT_BOOL, &dj_gpgpu_with_cacp,
			 "Use CACP or not (default=off)",
			 "0");
  option_parser_register(opp, "-gpgpu_with_cacp_stats", OPT_BOOL, &dj_gpgpu_with_cacp_stats,
			 "Calculate CACP stats on normal cache or not (default=off)",
			 "0");
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
  if (m_stats->tw_with_oracle_cpl)
    tw_print_CPL_counters(start_warp_id, end_warp_id);
#endif
#ifdef TW_DEBUG_NEW
  printf("TW: Kernel %d core %d cta %d (%d in kernel)\'s warp criticalities:", m_kernel->get_uid(), m_sid, cta_num, tw_cta_num_in_kernel[cta_num]);
#endif
  for (unsigned i = start_warp_id; i < end_warp_id; i++){
#ifdef TW_DEBUG_NEW
    printf("\t W%d: %d", i, m_stats->tw_warp_cta_cycle_dist[m_sid][i]);
#endif
    if (m_config->tw_oracle_cpl_exec_cycles){
      m_stats->tw_cpl_actual[m_kernel->get_uid()-1][tw_cta_num_in_kernel[cta_num]+1][i-start_warp_id] = m_warp[i].tw_get_warp_execution_cycles();
    }
    else{
      m_stats->tw_cpl_actual[m_kernel->get_uid()-1][tw_cta_num_in_kernel[cta_num]+1][i-start_warp_id] = m_stats->tw_warp_cta_cycle_dist[m_sid][i];
    }
  }
#ifdef TW_DEBUG_NEW
  printf("\n");
#endif
}

std::vector<float> shader_core_ctx::tw_get_current_CPL_counters() const
{
  std::vector<float> ret;
  for (unsigned i = 0; i < m_warp.size(); i++){
    ret.push_back(m_warp[i].tw_get_CPL());
  }
  return ret;
}
void shader_core_ctx::tw_print_CPL_counters(unsigned start_id, unsigned end_id) const
{
  printf("TW: CPL counters for warp %d - warp %d:\n", start_id, end_id);
  std::vector<float> counters = tw_get_current_CPL_counters();
  for (unsigned i = start_id; i < end_id; i++){
    printf("W%d: %.2f, ", i, counters[i]);
  }
  printf("\n");
}

void shader_core_ctx::tw_oracle_CPL_sanity_check(unsigned warp_id, int actual_counter) const
{
  assert(m_config->tw_gpgpu_oracle_cpl);
  if (m_warp[warp_id].tw_get_oracle_CPL() != actual_counter){
    printf("The actual oracle CPL is not the same as the previous one on shader %d warp %d: %d vs %d\n", m_sid, warp_id, m_warp[warp_id].tw_get_oracle_CPL(), actual_counter);
    assert(0);
  }
}

bool shader_core_ctx::tw_if_use_oracle_cpl() const
{  
  if (m_config->tw_gpgpu_oracle_cpl == true){
    assert (m_stats->tw_with_oracle_cpl);
    return true;
  }
  else
    return false;
}

void shader_core_ctx::tw_calculate_cpl_accuracy() const
{
  //  assert(m_stats->tw_with_oracle_cpl);
  if (m_kernel == NULL)
    return;
  for (unsigned i = 0; i < kernel_max_cta_per_shader; i++){
    if (m_cta_status[i] == 0)
      break;
    //Find critical warp of the thread block
    unsigned start_warp_id, end_warp_id;
    tw_get_start_end_warp_id(&start_warp_id, &end_warp_id, i);
    assert(start_warp_id < m_warp.size() && end_warp_id <= m_warp.size());
    //    assert(end_warp_id - start_warp_id == 8);
    unsigned crit_warp = start_warp_id;
    for (unsigned j = start_warp_id; j < end_warp_id; j++){
      if (m_warp[j].tw_get_oracle_CPL() > m_warp[crit_warp].tw_get_oracle_CPL()){
	crit_warp = j;
      }
    }
    //Check if its actual counter is larger than 50% of the other warps
    unsigned num_larger = 0;
    for (unsigned j = start_warp_id; j < end_warp_id; j++){
      if (m_warp[crit_warp].tw_get_CPL() >= m_warp[j].tw_get_CPL()){
	num_larger++;
      }
    }
    if (num_larger >= (end_warp_id-start_warp_id) / 2){
      m_stats->tw_accurate_cpl_for_accuracy++;
    }
#ifdef TW_DEBUG_NEW
    else{
      printf("Found an inaccurate CPL\n");
      for (unsigned j = start_warp_id; j < end_warp_id; j++){
	printf("TW:[W%d] Oracle: %d Actual: %f\n", j, m_warp[j].tw_get_oracle_CPL(), m_warp[j].tw_get_CPL());
      }
    }
#endif
    m_stats->tw_total_cpl_for_accuracy++;
  }
}
void shader_core_ctx::tw_calculate_cpl(unsigned cycle)
{
  assert(!m_config->tw_gpgpu_oracle_cpl);
#ifdef TW_DEBUG_NEW
  printf("TW: CPL calculation for core %d @ cycle %d:\n", m_sid, cycle);
#endif
  for (unsigned i = 0; i < m_warp.size(); i++){
    m_warp[i].tw_cpl_calculate(m_config->tw_actual_cpl_real_cpi, m_config->tw_actual_cpl_stall, cycle);
#ifdef TW_DEBUG_NEW
    printf("W%d %.2f, ", i, m_warp[i].tw_get_CPL());
#endif
  }
#ifdef TW_DEBUG_NEW
  printf("\n");
#endif
  if (m_config->tw_calculate_cpl_accuracy){
    tw_calculate_cpl_accuracy();
  }
}
unsigned shader_core_ctx::tw_get_static_ninst(unsigned warp_id)
{
  if (!m_config->tw_actual_cpl_static_ninst)
    return 0;
  else{
    printf("cannot support static instruction count\n");
    assert (0);
    //TODO: get static instruction count for each warp
  }
}

static unsigned tw_find_next_pc(unsigned cur_pc, addr_vector_t vec)
{
  assert(vec.size() > 0);
  unsigned min = vec[0];
  for (unsigned i = 0; i < vec.size(); i++){
    if (vec[i] < min)
      min = vec[i];
  }
  unsigned max = vec[0];
  for (unsigned i = 0; i < vec.size(); i++){
    if (vec[i] > max)
      max = vec[i];
  }
  if (min < cur_pc && max > cur_pc){
    min = max;
    for (unsigned i = 0; i < vec.size(); i++){
      if (vec[i] < min && vec[i] > cur_pc)
	min = vec[i];
    }
  }
  return min;
}
address_type shader_core_ctx::tw_calculate_npc_per_warp(unsigned warp_id)
{
  unsigned wtid = warp_id * m_warp_size;
  addr_vector_t next_pc;
  for (unsigned i = 0; i < m_warp_size; i++) {
    if( !ptx_thread_done(wtid+i) ) {
      next_pc.push_back( m_thread[wtid+i]->get_pc() );
    }
  }
  if (next_pc.size() > 0){
    return tw_find_next_pc(m_warp[warp_id].get_pc(), next_pc);
  }
  else{
    return (address_type) -1;
  }
}
//***************** class shd_warp_t **************/
float shd_warp_t::tw_get_CPL() const
{
  assert(m_shader);
  return m_shader->tw_if_use_oracle_cpl() ? 1.0*tw_cpl_oracle : tw_cpl_actual;
}
void shd_warp_t::tw_warp_enter(unsigned cycle, unsigned ninst)
{
  tw_nInst = ninst;
  tw_warp_entered_cycle = cycle;
}
void shd_warp_t::tw_warp_issue(unsigned cycle, address_type npc, unsigned isize)
{
  //save stall info
  assert(cycle > tw_last_schedule_cycle);
  tw_nStall += (tw_last_schedule_cycle != 0) ? (cycle - tw_last_schedule_cycle) : 0;
  tw_last_schedule_cycle = cycle;
  //calculate nInst changes
  if (npc != (address_type) -1){
    if (npc < m_next_pc){
#ifdef TW_DEBUG_NEW
      printf("Backedges: TW: warp %d: curr PC: %d, next PC: %d, new nInst: %d\n", m_warp_id, m_next_pc, npc, tw_nInst);
#endif
      tw_nInst += (m_next_pc - npc) / isize + 1;
    }
    else{
#ifdef TW_DEBUG_NEW
      if ((npc - m_next_pc) / isize - 1 != 0){
	printf("Skips: TW: warp %d: curr PC: %d, next PC: %d, new nInst: %d\n", m_warp_id, m_next_pc, npc, tw_nInst);
      }
#endif
      tw_nInst -= (npc - m_next_pc) / isize - 1;
    }
#ifdef TW_DEBUG_NEW
    printf("TW: warp %d: curr PC: %d, next PC: %d, new nInst: %d\n", m_warp_id, m_next_pc, npc, tw_nInst);
#endif
  }
}
void shd_warp_t::tw_warp_complete()
{
  tw_num_completed_inst++;
  tw_nInst--;
}
void shd_warp_t::tw_cpl_calculate(bool avg_cpi, bool stall, unsigned cycle)
{
  float cpi = avg_cpi ? 1.0 * (cycle - tw_warp_entered_cycle) / tw_num_completed_inst : 1.0;
  tw_cpl_actual = cpi * tw_nInst;
  tw_cpl_actual += stall ? 1.0 * tw_nStall : 0.0;
#ifdef TW_DEBUG_NEW
  printf("TW: W%d: %d %.2f %d %.2f\n", m_warp_id, tw_nInst, cpi, tw_nStall, tw_cpl_actual);
#endif
}
void shd_warp_t::tw_warp_exit(unsigned cycle)
{
#ifdef TW_DEBUG_NEW
  printf("Complete W%d @ Cycle %d, nInst: %d, nStall: %d, enter: %d, complete_insts: %d\n", m_warp_id, cycle, tw_nInst, tw_nStall, tw_warp_entered_cycle, tw_num_completed_inst);
  //  assert(0);
#endif
  tw_warp_completed_cycle = cycle;
}
