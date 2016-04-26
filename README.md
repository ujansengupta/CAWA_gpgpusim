# gpgpusim
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
