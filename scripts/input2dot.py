#!/usr/bin/env python

import sys, math;

n_node=int(sys.stdin.readline())
#n_link=int(sys.stdin.readline())
adj=[]
nan=float('nan')
#for n in range(n_node):
#    sys.stdin.readline()
for n in range(n_node):
    adj.append([float(f) if float(f)>0.0 else nan for f in sys.stdin.readline().split()])
sys.stdout.write("graph {\n")
for m in range(n_node):
    for n in range(m):
        if not math.isnan(adj[m][n]):
            sys.stdout.write("{:d} -- {:d} [label=\"{:g}\"];\n".format(m,n,adj[m][n]))
sys.stdout.write("}\n")
