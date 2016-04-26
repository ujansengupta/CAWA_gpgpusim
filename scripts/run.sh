#!/bin/bash
set -e
#configs
SRC_PATH=gpgpusim
OUT=results.txt
FIRST=yes
LOAD=yes
CACP=no

#take parameters
BENCH=AES
if [ -n "$1" ]
then
    BENCH=$1
    shift
fi
SCHED=gto
if [ -n "$1" ]
then 
    SCHED=$1
    shift
fi
DEBUG=no
if [ -n "$1" ]
then
    DEBUG=$1
    shift
fi

cd ${SRC_PATH}
make
cd ..

if [ "$BENCH" = "compile" ]
then
    exit 0
fi

#setup configuration files
cd ${SRC_PATH}/configs/GTX480
if [ ! -f gpgpusim.config.${SCHED} ]
then
    echo "your scheduling policy not supported, or use small letters"
    exit 1
fi
pwd
cp gpgpusim.config.${SCHED} gpgpusim.config
if [ "$FIRST" = "no" ]
then
    printf "\n-gpgpu_load_oracle_counter 1" >> gpgpusim.config
else
    printf "\n-gpgpu_store_oracle_counter 1" >> gpgpusim.config
fi
if [ "$CACP" = "yes" ]
then
    printf "\n-gpgpu_with_cacp 1" >> gpgpusim.config
fi
cd ../../../

#generate all oracle counters
if [ "$FIRST" = "yes" ]
then
    cd ispass2009-benchmarks
    bench_pool=(AES BFS CP DG LIB LPS NUM NN NQU RAY STO WP)
    for bench in ${bench_pool[@]};
    do
	cd $bench
	OUT=results.$SCHED.txt
	if [ -f $OUT ]
        then
            rm $OUT
        fi
	echo "generating oracle counter for $bench with $SCHED"
        sh README.GPGPU-Sim > $OUT
	cd ..
    done
else
#run the benchmark
    cd ispass2009-benchmarks
    cd $BENCH
    if [ "$DEBUG" = "no" ]
    then
	echo "Running $BENCH"
	if [ "$OUT" = "" ]
	then
	    sh README.GPGPU-Sim
	else
	    OUT=results.$SCHED.txt
	    if [ -f $OUT ]
	    then
		rm $OUT
	    fi
	    sh README.GPGPU-Sim > $OUT
	    ../../scripts/sum.py $OUT
	fi
    else
	gdb --args `cat README.GPGPU-Sim`
    fi
fi