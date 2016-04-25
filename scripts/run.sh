#!/bin/bash
set -e
SRC_PATH=gpgpusim
OUT=results.txt
FIRST=yes
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
fi
if [ "$CACP" = "yes" ]
then
    printf "\n-gpgpu_with_cacp 1" >> gpgpusim.config
fi
cd ../../../
#run the benchmark
cd ispass2009-benchmarks
cd $BENCH
if [ "$DEBUG" = "no" ]
then
    if [ "$FIRST" = "yes" ]
    then
	if [ -f "$SCHED.cpl" ]
	then
	    rm $SCHED.cpl
	fi
    fi
    echo "Running $BENCH"
    if [ "$OUT" = "" ]
    then
	sh README.GPGPU-Sim
    else
	if [ -f $OUT ]
	then
	    rm $OUT
	fi
	sh README.GPGPU-Sim > $OUT || true
	less $OUT
    fi
else
    gdb --args `cat README.GPGPU-Sim`
fi