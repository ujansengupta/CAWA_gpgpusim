#!/bin/bash
set -e
#configs
SRC_PATH=gpgpusim
OUT=results.txt
FIRST=no
LOAD=yes
CACP=yes
#for oracle counter
ORACLE_TYPE=1
#for actual counter
AVG_CPI=1
STALL=1
#bench_pool=(AES BFS CP DG LIB LPS MUM NN NQU RAY STO WP)
bench_pool=(AES BFS LPS MUM NN NQU RAY STO WP)

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
ACTUAL=no
if [ -n "$1" ]
then
    ACTUAL=yes
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
if [ "$FIRST" = "yes" ]
then
    if [ "$ORACLE_TYPE" = "1" ]
    then
	ACTUAL=yes
    fi
fi
config ${SRC_PATH} ${SCHED} ${FIRST} ${CACP} ${ACTUAL} ${AVG_CPI} ${STALL} ${ORACLE_TYPE}

#generate all oracle counters
if [ "$FIRST" = "yes" ]
then
    source scripts/generate.sh
    generate ${bench_pool} ${SCHED} ${ORACLE_TYPE}
else
    source scripts/cpl.sh
    load_cpl ${bench_pool} ${SCHED} ${ORACLE_TYPE}
    #run the benchmark
    source scripts/run_bench.sh
    run_bench $BENCH $OUT $DEBUG $SCHED $ACTUAL ${AVG_CPI} ${STALL}
fi