eonsim
======

eonsim is an event-driven traffic simulator for dynamic shared-path protected provisioning in elastic optical networks. It is designed for testing different provisioning heuristics.
It has been developed as part of a master thesis project. For background information, see the thesis document which will be linked here as soon as it is made public.

Building
--------

eonsim is known to compile with the GNU g++ 4.8.3 compiler on a Debian "Jessie" amd64 GNU/Linux system, but is expected to compile on most platforms where a compiler supporting C++11 is available.

To build the release version, do
```Bash
mkdir Release && cd Release/
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

Running
-------
eonsim is a command-line program. A typical call might look like

```Bash
eonsim -p "load=150:10:250,k=4" -a "ksq(c_cut=1,c_algn=1,c_sep=1,mode=1:1:3),pfmbl(c1=0:0.88:0.88)" -t 2 -i inputfile -o outputfile
```

This runs a simulation for load values from 150-250 Erlang, including both limits, in steps of 10. The parameter k=4 is passed to all heuristics as as the default for k-shortest path searches. The "k-squared" heuristic will be run with the given weights and in 3 different modes; similarly, "PF-MBL" will be run in the PF-MBL-0 variant and in the hybrid variant with weight c1=0.88. eonsim always executes the cartesian product of all specified parameter ranges and algorithms, i.e. each heuristic with each parameter combination for each load value.

Run ````eonsim -h```` to get information about the specific options.

File formats
------------

The network graph is read as a matrix from the input text files. Two slightly different file formats are supported:
The first variant contains the number of nodes $n$ on the first line, followed by a symmetric $n\times n$ distance matrix of floating-point values $d_{ij}$ separated by spaces. These values represent the length of the fiber between the nodes $i$ and $j$. If there is no connection or if $i=j$, set $d_{ij}=0$.
The other variant contains the number of nodes $n$ on the first line, the number of bidirectional links $m$ on the second line, then an $n\times n$ adjacency matrix, an $n\times n$ distance matrix and possibly further matrices. The adjacency matrix is ignored since the same information is contained in the distance matrix; only $n$, $m$ and the distance matrix are parsed.
Example files can be found in the `input/` directory.

The output file format is an ASCII table where columns are separated by ';' and rows by line breaks. They contain comment lines that start with # and specify the column titles, which may differ for different algorithms. A part of an output file where the columns do not change and where the comment line is removed can be read into e.g. Octave or Matlab using the function
````dlmread(FILENAME, ";")````

Code structure
--------------

To do.

Network representation
----------------------

To do.


Adding new heuristics
---------------------

Heuristics are implemented in separate classes in the `provisioning_schemes`
directory. Each of these classes is a subclass of the `ProvisioningScheme`
interface.
Implement the constructor to accept parameters for your heuristic and
```C++
operator()(const NetworkGraph& g, const NetworkState& s,const NetworkGraph::DijkstraData &data,const Request& r)
```
to implement your new heuristic. Copying one of the existing heuristics may
be the best start.