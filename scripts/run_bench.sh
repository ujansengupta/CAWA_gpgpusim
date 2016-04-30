#!/bin/bash

run_bench()
{
    cd ispass2009-benchmarks
    cd $1
    if [ "$3" = "no" ]
    then
        echo "Running $BENCH..."
        if [ "$2" = "" ]
        then
            sh README.GPGPU-Sim
        else
	    if [ "$5" = "yes" ]
	    then
		OUT=results.$4.actual.{$6$7}.txt
	    else
		OUT=results.$4.oracle.txt
	    fi
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