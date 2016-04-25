# gpgpusim
To run with my scripts:
  copy the whole scripts folder to ~/
  change SRC_PATH to the compiling gpgpusim folder (e.g. gpgpu-sim_distribution)
  go to your $SRC_PATH folder and do source setup_environment
  under ~/ directory, type:
  ./scripts/run.sh [BENCHMARK NAME] [SCHEDULING POLICY]
  e.g.
  ./scripts/run.sh BFS gto

Before you run the script:
  If it's the first run, open the scripts/run.sh with any text editor and change the define the variable FIRST to FIRST=yes.
  If you want to debug with gdb, edit the script to define DEBUG to DEBUG=yes

What will the script do?
  1. It will go to the SRC_PATH folder and compile the code
  2. It will generate the config file according to your config (will add more configs according to further development)
  3. It will go to the according benchmark folder and run the benchmark and save it to results.[SCHEDULING POLICY].txt
  4. It will call my python code to calculate the total number of simulation cycles and print it out
