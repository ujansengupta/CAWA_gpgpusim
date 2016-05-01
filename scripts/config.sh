#!/bin/bash

config()
{
    cd ${1}/configs/GTX480
    if [ ! -f gpgpusim.config.${2} ]
    then
	echo "your scheduling policy not supported, or use small letters"
	exit 1
    fi

    cp gpgpusim.config.${2} gpgpusim.config
    if [ "$3" = "no" ]
    then
	printf "\n-gpgpu_load_oracle_counter 1" >> gpgpusim.config
	printf "\n-gpgpu_store_oracle_counter 1" >> gpgpusim.config
    else
	printf "\n-gpgpu_store_oracle_counter 1" >> gpgpusim.config
	printf "\n-caws_calculate_cpl_accuracy 0" >> gpgpusim.config
    fi
    if [ "$4" = "yes" ]
    then
	printf "\n-gpgpu_with_cacp 1" >> gpgpusim.config
    fi
    if [ "$5" = "yes" ]
    then
	printf "\n-gpgpu_with_oracle_cpl 0" >> gpgpusim.config
    else
	#record CPLs for cawa oracle
	if [ "$2" = "cawa" ]
	then
	    printf "\n-gpgpu_store_oracle_counter 1" >> gpgpusim.config
	fi
    fi
    if [ "$6" = "1" ]
    then
	printf "\n-caws_actual_cpl_real_cpi 1" >> gpgpusim.config
    fi
    if [ "$7" = "1" ]
    then
	printf "\n-caws_actual_cpl_stall 1" >> gpgpusim.config
    fi
    if [ "$8" = "1" ]
    then
	printf "\n-caws_oracle_cpl_exec_cycles 1" >> gpgpusim.config
    fi
    if [ "$9" = "1" ]
    then
	printf "\n-gpgpu_with_cacp_stats 1" >> gpgpusim.config
    fi
    cd ../../../
}