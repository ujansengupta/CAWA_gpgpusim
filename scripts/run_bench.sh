#!/bin/bash

run_bench()
{
    cd ispass2009-benchmarks
    cd $1
    if [ "$2" = "no" ]
    then
        echo "Running $BENCH..."
        if [ "$OUT" = "" ]
        then
            sh README.GPGPU-Sim
        else
            OUT=results.$3.txt
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
}