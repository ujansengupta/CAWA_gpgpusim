#!/bin/bash
set -e
#configs
SRC_PATH=gpgpu-sim_distribution
OUT=results.txt
FIRST=no
LOAD=yes
CACP=no
#bench_pool=(AES BFS CP DG LIB LPS MUM NN NQU RAY STO WP)
bench_pool=(AES BFS CP LIB LPS MUM NN NQU RAY STO WP)

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

source scripts/compile.sh
compile

if [ "$BENCH" = "compile" ]
then
    exit 0
fi

#setup configuration files
source scripts/config.sh
config ${SRC_PATH} ${SCHED} ${FIRST} ${CACP}

#generate all oracle counters
if [ "$FIRST" = "yes" ]
then
    source scripts/generate.sh
    generate ${bench_pool} ${SCHED}
else
    source scripts/cpl.sh
    load_cpl ${bench_pool} ${SCHED}
    #run the benchmark
    source scripts/run_bench.sh
    run_bench $BENCH $DEBUG $SCHED
fi
