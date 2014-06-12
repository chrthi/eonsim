#!/usr/bin/env python

import sys, math;

adj=[]
nan=float('nan')

scalefac=0.01;
if len(sys.argv)>1:
    scalefac=float(sys.argv[1]);

n_node=int(sys.stdin.readline())
firstline=sys.stdin.readline().split()
if len(firstline)==1: #This is the link number. Discard the following matrix.
    for n in range(n_node):
        sys.stdin.readline()
    firstline=sys.stdin.readline().split()
elif len(firstline)!=n_node: #the first line of the distance matrix.
    sys.exit("Unknown file format")

adj.append([float(f) if float(f)>0.0 else nan for f in firstline])
for n in range(n_node-1):
    adj.append([float(f) if float(f)>0.0 else nan for f in sys.stdin.readline().split()])
sys.stdout.write("graph {\n")
for m in range(n_node):
    for n in range(m):
        if not math.isnan(adj[m][n]):
            sys.stdout.write("{:d} -- {:d} [label=\"{:g}\" len={:g}];\n".format(m,n,adj[m][n],adj[m][n]*scalefac))
sys.stdout.write("}\n")
