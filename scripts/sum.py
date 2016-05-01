#!/usr/bin/python
import re
def find_sum(contents, pattern, type_str):
    string = re.compile(pattern).findall(contents)
    if len(string) > 0:
        if type_str == 'int':
            res = re.compile('[\d]+').findall(string[-1])
            sums = [int(re.compile('[\d]+').findall(x)[0]) for x in string]
        elif type_str == 'float':
            res = re.compile('[\d]+\.[\d]+').findall(string[-1])
            sums = [float(re.compile('[\d]+\.[\d]+').findall(x)[0]) for x in string]
        return sum(sums)
    else:
#        print pattern, "not found"
        return 0

def find_last(contents, pattern, type_str):
    string = re.compile(pattern).findall(contents)
    if len(string) > 0:
        if type_str == 'int':
            res = re.compile('[\d]+').findall(string[-1])
            return int(res[-1])
        elif type_str == 'float':
            res = re.compile('[\d]+\.[\d]+').findall(string[-1])
            return float(res[-1])
    else:
#        print pattern, "not found"
        return 0

import sys
for i in range(len(sys.argv)-1):
    f=open(sys.argv[i+1])
    contents=f.read()
    f.close()
    #print IPC
    IPC = find_last(contents, 'gpu_tot_ipc[\s]+=[\s]+[\d]+\.[\d]+', 'float')
    print "IPC %.3f" % IPC
    #print MPKI
    misses = find_last(contents, 'L1D_total_cache_misses[\s]+=[\s]+[\d]+', 'int')
    insns = find_last(contents, 'gpu_tot_sim_insn[\s]+=[\s]+[\d]+', 'int')
    print "MPKI %.3f" % (1000.0*misses/insns)
    #print critical hits
    crit_hits = find_sum(contents, 'Critical Hit: [\d]+,', 'int')
    crit_access = find_sum(contents, 'Critical Access: [\d]+', 'int')
    if crit_access > 0:
        print "critical hits %.3f%%" % (100.0*crit_hits/crit_access)
    #print zero reuse
    zero_evict = find_sum(contents, 'Zero eviction: [\d]+', 'int')
    crit_evict = find_sum(contents, 'Critical eviction: [\d]+', 'int')
    if crit_evict > 0:
        print "zero eviction %.3f%%" % (100.0*zero_evict/crit_evict)
    #CPL accuracy
    cpl_accuracy = find_last(contents, 'TW: cpl accuracy[\s]+=[\s]+[\d]+\.[\d]+%', 'float')
    if cpl_accuracy > 0:
        print "CPL accuracy", cpl_accuracy
    #CCBP accuracy
    ccbp_accuracy = find_sum(contents, 'CCBP correct: [\d]+', 'int')
    total_access = find_sum(contents, 'Total CCBP Access: [\d]+', 'int')
    if total_access > 0:
        print "CCBP accuracy %.3f%%" % (100.0*ccbp_accuracy/total_access)
    
