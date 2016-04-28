/*********************************************************
 ****  Code for Criticality Aware Warp Scheduler      ****
 ****  Contributed by Ujan Sengupta                   ****
 ****  Also made change to:                           ****
 ****     shader.cc                                   ****
 ****     shader.h                                    ****
 ****  04/20/16                              	      ****
 *********************************************************/
 
#include "shader.h"
 
//***************** class shader.h *********************/ 
bool sort_warps_by_criticality(shd_warp_t* lhs, shd_warp_t* rhs);

void cawa_scheduler::order_warps()
{
  assert(m_supervised_warps.size());
  /*
  printf("TW: %d %x %x\n", m_supervised_warps.size(), *m_supervised_warps.begin(), *m_supervised_warps.end());
  for (unsigned i = 0; i < m_supervised_warps.size(); i++){
    printf("%x ", m_supervised_warps[i]);
  }
  printf("\n");*/
    order_by_priority( m_next_cycle_prioritized_warps,
                       m_supervised_warps,
                       m_last_supervised_issued,
                       m_supervised_warps.size() );
}

void cawa_scheduler::order_by_priority(std::vector<shd_warp_t*>& result_list,
				       const std::vector<shd_warp_t*>& input_list,
				       const std::vector<shd_warp_t*>::const_iterator& last_issued_from_input,
				       unsigned num_warps_to_add)
{
  assert( num_warps_to_add <= input_list.size() );
  result_list.clear();
  std::vector<shd_warp_t*> temp = input_list;
  /*
  for (unsigned i = 0; i < temp.size(); i++){
    printf("%d ", temp[i]->get_warp_id());
  }
  printf("\n");*/
  sort_warps(temp);
  /*
  for (unsigned i = 0; i < temp.size(); i++){
    printf("%d ", temp[i]->get_warp_id());
  }
  printf("\n");*/
  //  assert(0);
  std::vector<shd_warp_t*>::iterator iter = temp.begin();
  for ( unsigned count = 0; count < num_warps_to_add; ++count, ++iter ){
    result_list.push_back( *iter );
  }
}
void cawa_scheduler::sort_warps(std::vector<shd_warp_t*>& temp){
  for (unsigned i = 0; i < temp.size()-1; i++){
    unsigned jMax = i;
    for (unsigned j = i+1; j < temp.size(); j++){
      if (sort_warps_by_criticality(temp[j], temp[jMax])) {
	jMax = j;
      }
    }
    if (jMax != i) {
      shd_warp_t* tmp = temp[i];
      temp[i] = temp[jMax];
      temp[jMax] = tmp;
    }
  }
}
bool sort_warps_by_criticality(shd_warp_t* lhs, shd_warp_t* rhs)
{
  /*
  printf("%x %x\n", lhs, rhs);
  printf("%d %d %d\n", lhs->get_cta_id(), lhs->get_warp_id(), lhs->get_dynamic_warp_id());
  printf("%d %d %d\n", rhs->get_cta_id(), rhs->get_warp_id(), rhs->get_dynamic_warp_id());*/
  if (rhs && lhs )
    {
        if ( lhs->done_exit() || lhs->waiting() ) 
        {
            return false;
        } 
        else if ( rhs->done_exit() || rhs->waiting() ) 
        {
            return true;
        } 
        else 
        {
            if (lhs->tw_get_CPL() == rhs->tw_get_CPL())
	      {
            	return lhs->get_dynamic_warp_id() < rhs->get_dynamic_warp_id();
            }
            else
            {
            	return lhs->tw_get_CPL() > rhs->tw_get_CPL();
            }
        }
    } 
    else 
    {
      assert(0);
        return lhs > rhs;
    }
}


scheduler_unit::scheduler_unit()
{
    count = 0;
    flag = 0;
}

