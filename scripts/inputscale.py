#!/usr/bin/env python

import sys, math;

nan=float('nan')
if len(sys.argv)!=2 or not float(sys.argv[1])>0.0:
    sys.exit("Need a scale factor as argument.")
scale=float(sys.argv[1])

n_node=int(sys.stdin.readline())
print n_node
firstline=sys.stdin.readline().split()
if len(firstline)==1:
    print firstline[0] #This is the link number. Discard the following matrix.
    for n in range(n_node):
        print sys.stdin.readline(),
    firstline=sys.stdin.readline().split()
elif len(firstline)!=n_node: #This is the first line of the distance matrix.
    sys.exit("Unknown file format")

print ' '.join(str(float(f)*scale) if float(f)>0.0 else '0' for f in firstline)
for n in range(n_node-1):
    print ' '.join(str(float(f)*scale) if float(f)>0.0 else '0' for f in sys.stdin.readline().split())

#print all following data unchanged
for l in sys.stdin:
    print l,
