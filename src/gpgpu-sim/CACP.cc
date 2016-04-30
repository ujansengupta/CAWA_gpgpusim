/*********************************************************
 ****  Code for CACP			              		  ****
 ****  Contributed by David Joshy                     ****
 ****  Changes made in:                               ****
 ****     gpu-cache.cc                                ****
 ****     shader.cc                                   ****
 ****     gpu-cache.h                                 ****
 ****     shader.h									  **** 	
 ****     mem_fetch.h								  **** 	
 ****  04/07/16                                       ****
 *********************************************************/
#include "shader.h"

//***************** class shader_core_ctx *********************/
 void shader_core_ctx:: calc_warp_criticality()
 {
   int max=0;
   //need to add check for first run.
   for (long index=0; index<(long)m_warp.size(); ++index) 
     if(max<m_warp.at(index).tw_get_CPL())
       max=m_warp.at(index).tw_get_CPL();
   for (long index=0; index<(long)m_warp.size(); ++index) 
     if(m_warp.at(index).tw_get_CPL()/max>0.75)
       m_warp.at(index).criticality=true;
     else
       m_warp.at(index).criticality=false;
 }
 
 bool shader_core_ctx::get_warp_critical(unsigned warpid){
	 calc_warp_criticality();
	 return m_warp.at(warpid).criticality;
 }

 
//*****David-4/24*******************************************/
//adding cacp access copy
enum cache_request_status
l1_cache_cacp::access( new_addr_type addr,
		       mem_fetch *mf,
		       unsigned time,
		       std::list<cache_event> &events )
{
  assert( mf->get_data_size() <= m_config.get_line_sz());
  bool wr = mf->get_is_write();
  new_addr_type block_addr = m_config.block_addr(addr);
  unsigned cache_index = (unsigned)-1;
  enum cache_request_status probe_status
    = m_tag_array->probe( block_addr, cache_index , mf->req_criticality, mf->get_pc());  
  enum cache_request_status access_status
    = process_tag_probe( wr, probe_status, addr, cache_index, mf, time, events );
  m_stats.inc_stats(mf->get_access_type(),
		    m_stats.select_stats_status(probe_status, access_status));
    return access_status;
    // return data_cache::access( addr, mf, time, events );  
}

//***************** class tag_array_CACP *********************/
enum cache_request_status tag_array_CACP::probe( new_addr_type addr, unsigned &idx, bool critical, unsigned pc )  {
    //assert( m_config.m_write_policy == READ_ONLY );
    unsigned signature=(addr & 255)^(pc & 255);
    //unsigned signature=last 8 bits of PC and request addrees. Needs PC address sent here.
    //change index assingment based on predicted criticality
    unsigned set_index = m_config.set_index(addr);
    new_addr_type tag = m_config.tag(addr);
    int way;
    // check for hit or pending hit
    for (way=0; way<(int)m_config.m_assoc; way++) {	  
      unsigned index = set_index*m_config.m_assoc+way;
      cache_block_t *line = &m_lines[index];
      if (line->m_tag == tag) {
	if ( line->m_status == RESERVED ) {
	  idx = index;	
	  //call CACP HIT function in extended object.
	  cacp_hit(critical,idx);
	  //call SRRIP HIT function
	  SHiP[line->sig]=0;
	  return HIT_RESERVED;
	} else if ( line->m_status == VALID ) {
	  idx = index;
	  cacp_hit(critical,idx);
	  SHiP[line->sig]=0;
	  return HIT;
	} else if ( line->m_status == MODIFIED ) {
	  idx = index;
	  cacp_hit(critical,idx);
	  SHiP[line->sig]=0;
	  return HIT;
	} else {
	  assert( line->m_status == INVALID );
	}
      }
    }
    int start_way, end_way;
    if(CCBP[signature]>=2){
      start_way = 0;
      end_way = (int)m_config.m_assoc / 2;
    }
    else{
      start_way = (int)m_config.m_assoc / 2;
      end_way = (int)m_config.m_assoc;
    }
    unsigned invalid_line = (unsigned)-1;
    unsigned valid_line = (unsigned)-1;
    bool all_reserved = true;
    for (way=start_way; way<end_way; way++) {
      unsigned index = set_index*m_config.m_assoc+way;
      cache_block_t *line = &m_lines[index];
      assert(SHiP[line->sig] <= 3);
      if (line->m_status != RESERVED) {
	all_reserved = false;
	if (line->m_status == INVALID || SHiP[line->sig] == 3) {
          invalid_line = index;
	}
      }
      if(way==end_way-1 && invalid_line == (unsigned) -1){
	if (all_reserved)
	  break;
	//means no lines with RRPV=3 were found
	for (int i=start_way; i<end_way; i++){
	  unsigned index1 = set_index*m_config.m_assoc+i;
	  cache_block_t *line1 = &m_lines[index1];
	  SHiP[line1->sig] += (SHiP[line1->sig] == 3) ? 0 : 1;
	}
	//Restarting loop, with incremented RRPV values.
	way=start_way-1;
      }
    }
    if ( all_reserved ) {
        assert( m_config.m_alloc_policy == ON_MISS ); 
        return RESERVATION_FAIL; // miss and not enough space in cache to allocate on miss
    }
    
    if (invalid_line == (unsigned) -1){
      for (way=start_way; way<end_way; way++) {
	unsigned index = set_index*m_config.m_assoc+way;
	cache_block_t *line = &m_lines[index];
	printf("%d ", SHiP[line->sig]);
	printf("(%d), ", line->m_status);
      }
      printf("\n");
    }
    assert(invalid_line != (unsigned)-1);
    //valid timestamp not used anywhere. GG. David 4/24************
    if ( invalid_line != (unsigned)-1 ) {
	//line with RRPV=3 found, replace it.
        idx = invalid_line;
	SHiP[signature]=2; //Add this also maybe? Need to confirm/experiment with signature behaviour.
	m_lines[idx].sig=signature;
    } else if ( valid_line != (unsigned)-1) {
        idx = valid_line;
    } else abort(); // if an unreserved block exists, it is either invalid or replaceable 
	
    //call CACP eviction function in extended object.
    if( m_lines[idx].m_status == MODIFIED )
      cacp_eviction(idx, set_index);	 
    return MISS;
}
void tag_array_CACP::cacp_hit(bool critical, unsigned &idx){
	int signature=m_lines[idx].sig;
	if(critical)
	{
		m_lines[idx].c_reuse=1;
		if(CCBP[signature]!=3)
			CCBP[signature]++;
		if(SHiP[signature]!=3)
			SHiP[signature]++;
	}
	else
	{
		m_lines[idx].nc_reuse=1;
		if(SHiP[signature]!=3)
			SHiP[signature]++;
	}
	
}
void tag_array_CACP::cacp_eviction(unsigned &idx, unsigned set_index){
	cache_block_t &evicted=m_lines[idx];
	int signature=evicted.sig;
	if(!evicted.c_reuse && evicted.nc_reuse && set_index<=(m_config.m_nset)/2-1)
	{	
		if(CCBP[signature]!=0)
			CCBP[evicted.sig]--;
	}
	else if(!evicted.c_reuse && !evicted.nc_reuse){
		if(SHiP[signature]!=0)
			SHiP[evicted.sig]--;
	}
			
}
/*
enum cache_request_status  tag_array::probe( new_addr_type addr, unsigned &idx, bool critical, unsigned pc)
{}*/
