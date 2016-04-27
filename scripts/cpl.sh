#!/bin/bash

load_cpl(){
    if [ ! -d gpgpusim/oracleCPLs ]
    then
	echo "The oracleCPLs folder not downloaded from GitHub"
	exit 1
    fi
    bench_pool=$1
    for bench in ${bench_pool[@]};
    do
	if [ -f ispass2009-benchmarks/${bench}/${2}.cpl ]
	then
	    rm ispass2009-benchmarks/${bench}/${2}.cpl
	fi
	if [ -f gpgpusim/oracleCPLs/${bench}.${2}.cpl ]
	then
	    cp gpgpusim/oracleCPLs/${bench}.${2}.cpl ispass2009-benchmarks/${bench}/${2}.cpl
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
    if [ -f ${1}/oracleCPLs/${bench}.${2}.cpl ]
        then
            rm ${1}/oracleCPLs/${bench}.${2}.cpl
        fi
        cp ispass2009-benchmarks/${bench}/${2}.cpl gpgpusim/oracleCPLs/${bench}.${2}.cpl
    done
}