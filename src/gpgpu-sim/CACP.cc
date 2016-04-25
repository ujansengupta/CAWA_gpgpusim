/*********************************************************
 ****  Code for CACP			              		  ****
 ****  Contributed by David Joshy                     ****
 ****  Changes made in:                               ****
 ****     gpu-cache.cc                                ****
 ****     shader.cc                                   ****
 ****     gpu-cache.h                                 ****
 ****     shader.h 				      				  ****
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
				m_warp.at(index).criticality=1;
	 
	 
 }


enum cache_request_status tag_array_CACP::probe( new_addr_type addr, unsigned &idx, bool critical )  {
    //assert( m_config.m_write_policy == READ_ONLY );
    //*****David-4/21*******************************************/
	unsigned signature;
	//unsigned signature=last 8 bits of PC and request addrees. Needs PC address sent here.
	//change index assingment based on predicted criticality
    unsigned set_index = m_config.set_index(addr);
	if(CCBP[signature]>=2){
		if(set_index>(m_config.m_nset)/2-1)
			set_index-=(m_config.m_nset)/2;
	}
	else{
		if(set_index<(m_config.m_nset)/2)
			set_index+=(m_config.m_nset)/2; 
	}
	//*****David-4/21*******************************************/
	new_addr_type tag = m_config.tag(addr);

    unsigned invalid_line = (unsigned)-1;
    unsigned valid_line = (unsigned)-1;
    unsigned valid_timestamp = (unsigned)-1;
	
    bool all_reserved = true;
    unsigned way;
    // check for hit or pending hit
    for (way=0; way<m_config.m_assoc; way++) {
		
		unsigned index = set_index*m_config.m_assoc+way;
        cache_block_t *line = &m_lines[index];
        if (line->m_tag == tag) {
            if ( line->m_status == RESERVED ) {
                idx = index;	
               
				//*****David-4/21*******************************************/
				//call CACP HIT function in extended object.
				cacp_hit(critical,idx);
				//call SRRIP HIT function
				SHiP[line->sig]=0;
				//*****David-4/21*******************************************/
				 return HIT_RESERVED;
		
            } else if ( line->m_status == VALID ) {
                idx = index;
                
				//*****David-4/21*******************************************/
				//cacp_hit(critical,idx);
				SHiP[line->sig]=0;
				//*****David-4/21*******************************************/
				return HIT;
            } else if ( line->m_status == MODIFIED ) {
                idx = index;
                
				//*****David-4/21*******************************************/
				cacp_hit(critical,idx);
				SHiP[line->sig]=0;
				//*****David-4/21*******************************************/
				return HIT;
            } else {
                assert( line->m_status == INVALID );
            }
        }
        if (line->m_status != RESERVED) {
            all_reserved = false;
			/*
			Commenting out other replacement policies for now. -David 4/24.
            if (line->m_status == INVALID) {
                invalid_line = index;
            } else {
                // valid line : keep track of most appropriate replacement candidate
				
				
                if ( m_config.m_replacement_policy == LRU ) {
                    if ( line->m_last_access_time < valid_timestamp ) {
                        valid_timestamp = line->m_last_access_time;
                        valid_line = index;
                    }
                } else if ( m_config.m_replacement_policy == FIFO ) {
                    if ( line->m_alloc_time < valid_timestamp ) {
                        valid_timestamp = line->m_alloc_time;
                        valid_line = index;
                    }
                }
			*/	
			//*****David-4/24*******************************************/
			//Modelling SRRIP Miss- David 4/24.
			if(SHiP[line->sig]==3)
			{
				//line with RRPV=3 found, replace it.
				invalid_line=index;
				//SHiP[line->sig]=2;
				SHiP[signature]=2; //Add this also maybe? Need to confirm/experiment with signature behaviour.
				m_lines[index].sig=signature;
				break;
			}
			if(way==m_config.m_assoc-1){
				//means no lines with RRPV=3 were found
				for (unsigned i=0; i<m_config.m_assoc; i++){
					unsigned index = set_index*m_config.m_assoc+i;
					cache_block_t *line = &m_lines[index];
					SHiP[line->sig]++;
				}
				//Restarting loop, with incremented RRPV values.
				way=-1;
            }
			//*****David-4/24*******************************************/
        }
    }
    if ( all_reserved ) {
        assert( m_config.m_alloc_policy == ON_MISS ); 
        return RESERVATION_FAIL; // miss and not enough space in cache to allocate on miss
    }
	//valid timestamp not used anywhere. GG. David 4/24************
    if ( invalid_line != (unsigned)-1 ) {
        idx = invalid_line;
    } else if ( valid_line != (unsigned)-1) {
        idx = valid_line;
    } else abort(); // if an unreserved block exists, it is either invalid or replaceable 
	
	//call CACP eviction function in extended object.
	//*****David-4/24*******************************************/
	 if( m_lines[idx].m_status == MODIFIED )
		 cacp_eviction(idx, set_index);
		 
	 
		//*****David-4/24*******************************************/ 
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