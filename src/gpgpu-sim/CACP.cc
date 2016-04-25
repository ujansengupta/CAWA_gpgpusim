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


enum cache_request_status tag_array_CACP::probe( new_addr_type addr, unsigned &idx, bool critical ) const {
    //assert( m_config.m_write_policy == READ_ONLY );
    //*****David-4/21*******************************************/
	//change index assingment based on criticality
    unsigned set_index = m_config.set_index(addr);
	if(critical)
		if(set_index>(m_config.m_nset)/2-1)
			set_index-=(m_config.m_nset)/2;
	else
		if(set_index<(m_config.m_nset)/2)
			set_index+=(m_config.m_nset)/2; 
	//*****David-4/21*******************************************/
	
	new_addr_type tag = m_config.tag(addr);

    unsigned invalid_line = (unsigned)-1;
    unsigned valid_line = (unsigned)-1;
    unsigned valid_timestamp = (unsigned)-1;

    bool all_reserved = true;

    // check for hit or pending hit
    for (unsigned way=0; way<m_config.m_assoc; way++) {
		
		unsigned index = set_index*m_config.m_assoc+way;
        cache_block_t *line = &m_lines[index];
        if (line->m_tag == tag) {
            if ( line->m_status == RESERVED ) {
                idx = index;
                return HIT_RESERVED;
				//*****David-4/21*******************************************/
				//call CACP HIT function in extended object.
				//call SRRIP HIT function in extended object.
				//*****David-4/21*******************************************/
		
            } else if ( line->m_status == VALID ) {
                idx = index;
                return HIT;
				//*****David-4/21*******************************************/
				//call CACP HIT function in extended object.
				//call SRRIP HIT function in extended object.
				//*****David-4/21*******************************************/
            } else if ( line->m_status == MODIFIED ) {
                idx = index;
                return HIT;
				//*****David-4/21*******************************************/
				//call CACP HIT function in extended object.
				//call SRRIP HIT function in extended object.
				//*****David-4/21*******************************************/
            } else {
                assert( line->m_status == INVALID );
            }
        }
        if (line->m_status != RESERVED) {
            all_reserved = false;
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
				//*****David-4/21*******************************************/
				//add condition for SRRIP miss
				//*****David-4/21*******************************************/
		
		
            }
        }
    }
    if ( all_reserved ) {
        assert( m_config.m_alloc_policy == ON_MISS ); 
        return RESERVATION_FAIL; // miss and not enough space in cache to allocate on miss
    }

    if ( invalid_line != (unsigned)-1 ) {
        idx = invalid_line;
    } else if ( valid_line != (unsigned)-1) {
        idx = valid_line;
    } else abort(); // if an unreserved block exists, it is either invalid or replaceable 
	//	check and add this follwoing to probe. COver all
		//*****David-4/21*******************************************/
		//call CACP Insertion/Fill function in extended object.
		 //*****David-4/21*******************************************/
		
				//*****David-4/21*******************************************/
				//call CACP eviction function in extended object.
    return MISS;
}