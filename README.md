# gpgpusim

****************************************************
Changes to the gpgpusim code:
CPL: all the newly defined functions are in CPL.cc
     Also made changes to:
      	  abstract_hardware_model.h
	  gpgpusim_entrypoint.cc
	  shader.h
	  shader.cc
	  gpu-sim.cc
     	 *Just search "TW:" or "tw_" in those files you can find all the changes
gCAWS/CAWS: all the newly defined function are in CAWS.cc
	    Also made changes to:
	    shader.h
            shader.cc
         *Just search "US" or "CAWS" in those files you can find all the changes
CACP: all the newly defined function are in CACP.cc
      Also made changes to:
      	   shader.h
           shader.cc
	   gpu-cache.h
	   gpu-cache.cc
	   mem_fetch.h
	   mem_fetch.cc
	 *Just search "david" or "CACP" in those files you can find all the changes
****************************************************

The CAWA paper : https://pdfs.semanticscholar.org/0ca2/b92a4f992b35683c7fffcd49b4c883772a29.pdf

To run with scripts: (run at one level above gpgpu-sim_distribution)
cp -r CAWA/scripts .
# (04/26/16) caws contains oracle_predictor and caws, should be the newest branch.
# (04/26/16) only AES and BFS have oracle counters generated and more are coming...
To run with my scripts:
  copy the whole scripts folder to ~/
  change SRC_PATH to the compiling gpgpusim folder (e.g. gpgpu-sim_distribution)
  go to your $SRC_PATH folder and do source setup_environment
  under ~/ directory, type:
  ./scripts/run.sh [BENCHMARK NAME] [SCHEDULING POLICY]
  e.g.
  ./scripts/run.sh BFS gto

Before you run the script:
  If you want to debug with gdb, edit the script to define DEBUG to DEBUG=yes

What will the script do?
  1. It will go to the SRC_PATH folder and compile the code
  2. It will generate the config file according to your config (will add more configs according to further development)
  3. It will load the oracle counters from oracleCPLs and copy it to the according folders
  4. It will go to the according benchmark folder and run the benchmark and save it to results.[SCHEDULING POLICY].txt
  5. It will call my python code to calculate the total number of simulation cycles and print it out
