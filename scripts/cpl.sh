#!/bin/bash

load_cpl(){
    if [ ! -d gpgpusim/oracleCPLs ]
    then
	echo "The oracleCPLs folder not downloaded from GitHub"
	exit 1
    fi
    bench_pool=$1
    if [ "${2}" = "lrr" ]
    then
	oracle=lrr
    else
	oracle=gto
    fi
    for bench in ${bench_pool[@]};
    do
	if [ -f gpgpusim/oracleCPLs/${bench}.${oracle}.cpl${3} ]
	then
	    if [ -f ispass2009-benchmarks/${bench}/${oracle}.cpl ]
	    then
		rm ispass2009-benchmarks/${bench}/${oracle}.cpl
	    fi
	    cp gpgpusim/oracleCPLs/${bench}.${oracle}.cpl${3} ispass2009-benchmarks/${bench}/${oracle}.cpl
	else
	    echo "oracle CPL loading failed"
	    exit 1
	fi
    done
}

store_cpl(){
    if [ ! -d gpgpusim/oracleCPLs ]
    then
	mkdir gpgpusim/oracleCPLs
    fi
    bench_pool=$1
    for bench in ${bench_pool[@]};
    do
    if [ -f gpgpusim/oracleCPLs/${bench}.${2}.cpl${3} ]
        then
            rm gpgpusim/oracleCPLs/${bench}.${2}.cpl${3}
        fi
        cp ispass2009-benchmarks/${bench}/${2}.cpl gpgpusim/oracleCPLs/${bench}.${2}.cpl${3}
    done
}