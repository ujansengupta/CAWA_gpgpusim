#!/bin/bash

run_bench()
{
    cd ispass2009-benchmarks
    cd $1
    if [ "$3" = "no" ]
    then
        echo "Running $1..."
        if [ "$2" = "" ]
        then
            sh README.GPGPU-Sim
        else
	    if [ "$4" = "gto" ] || [ "$4" = "lrr" ]
	    then
		if [ "$6" = "yes" ]
		then
		    if [ "$9" = "yes" ]
		    then
			OUT=results.$4.\(cacp\).txt
		    else
			OUT=results.$4.cacp.txt
		    fi
		else
		    OUT=results.$4.txt
		fi
	    else
		if [ "$5" = "yes" ]
		then
		    OUT=results.$4.actual.{$7$8}.txt
		else
		    OUT=results.$4.oracle.txt
		fi
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