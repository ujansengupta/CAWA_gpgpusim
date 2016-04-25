#!/usr/bin/python
import re
def find_pattern(contents, pattern):
    cycle_str = re.compile(pattern).findall(contents)
    cycles = [int(re.compile('[\d]+').findall(x)[0]) for x in cycle_str]
    return sum(cycles)

import sys
f=open(sys.argv[1])
contents=f.read()
f.close()
print find_cycle(contents, 'gpu_sim_cycle = [\d]+')
