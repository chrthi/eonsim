#!/usr/bin/env python

import sys;
import csv;

eondata=csv.reader(sys.stdin,delimiter=';',doublequote=False,escapechar='\\',skipinitialspace=True)
params=eondata.next()[2:]
algorithms=list()
tables=[dict() for p in params]

specialStyles={
    'Primary as blocking reason':'set noautoscale y\nset yrange [0:1]',
}

#reading
for row in eondata:
    a=row[0]
    if a not in algorithms:
        algorithms.append(a)
        for t in tables:
            t[a]=list()
    l=row[1]
    for vidx, val in enumerate(row[2:]):
        tables[vidx][a].append((l,val))

#output
print 'set key inside left top vertical Left reverse'
print 'set style data lp'
print 'set multiplot layout 2,3'
for pidx, pname in enumerate(params):
#    print 'set term wxt %d'%pidx
    print 'set title "%s"' % pname
    print 'set autoscale xy'
    if pname in specialStyles:
        print specialStyles[pname]
    print 'set xlabel "Network Load (Erlang)"'
    print 'plot', ', '.join('"-" u 1:2 title "%s"' % a for a in algorithms)
    for a in algorithms:
#        tables[pidx][a].sort()
        for (l, v) in tables[pidx][a]:
            print l, v
        print 'e'

