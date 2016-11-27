eonsim {#mainpage}
======

eonsim is an event-driven traffic simulator for dynamic shared-path protected provisioning in elastic optical networks. It is designed for testing different provisioning heuristics.
It has been developed as part of a master thesis project. For background information, see the thesis document at [urn:nbn:se:kth:diva-155684](http://urn.kb.se/resolve?urn=urn%3Anbn%3Ase%3Akth%3Adiva-155684)

Building
--------

eonsim is known to compile with the GNU g++ 4.8.3 compiler on a Debian "Jessie" amd64 GNU/Linux system, but is expected to compile on most platforms where a compiler supporting C++11 is available.

To build the release version, do
~~~Bash
mkdir Release && cd Release/
cmake -DCMAKE_BUILD_TYPE=Release ..
make
~~~

Running
-------
eonsim is a command-line program. A typical call might look like

~~~Bash
eonsim -p "load=150:10:250,k=4" -a "ksq(c_cut=1,c_algn=1,c_sep=1,mode=3),pfmbl(c1=0:0.88:0.88)" -t 2 -i inputfile -o outputfile
~~~

Numeric parameters can be given either as a single value (`c_cut=1`) or as a range in the form `start:step:end`.
Global parameters are given with the `-p` option, algorithms to be run and their specific options are given with `-a`.
There is a `-h` option to show a help text listing all algorithms and their parameters.

The example runs a simulation for load values from 150-250 Erlang, including both limits, in steps of 10. The parameter k=4 is passed to all heuristics as the default for k-shortest path searches. The "k-squared" heuristic will be run with the given weights; "PF-MBL" will be run in the PF-MBL-0 variant and in the hybrid variant with weight c1=0.88. eonsim always executes the cartesian product of all specified parameter ranges and algorithms, i.e. each heuristic with each parameter combination for each load value. Run `eonsim -h` to get information about the specific options.

File formats
------------

The network graph is read as a matrix from the input text files. Two slightly different file formats are supported:
The first variant contains the number of nodes $n$ on the first line, followed by a symmetric $n\times n$ distance matrix of floating-point values $d_{ij}$ separated by spaces. These values represent the length of the fiber between the nodes $i$ and $j$. If there is no connection or if $i=j$, set $d_{ij}=0$.
The other variant contains the number of nodes $n$ on the first line, the number of bidirectional links $m$ on the second line, then an $n\times n$ adjacency matrix, an $n\times n$ distance matrix and possibly further matrices. The adjacency matrix is ignored since the same information is contained in the distance matrix; only $n$, $m$ and the distance matrix are parsed.
Example files can be found in the `input/` directory.

The output file format is an ASCII table where columns are separated by ';' and rows by line breaks. They contain comment lines that start with # and specify the algorithm's name and the column titles, which may differ for different algorithms. A part of an output file where the columns do not change and where the comment line is removed can be read into e.g. Octave or Matlab using the function
`dlmread(FILENAME, ";")`

Code structure
--------------

The main() function parses the program arguments, loads the network structure and then starts sending work packages to a thread pool. The JobIterator class is used to generate all requested algorithm-parameter combinations that need to be run, these are the work packages. The worker() threads pass their results back to the main loop, which takes care of printing them in the correct order.

The main loop of a single simulation round can be found in Simulation::run(). The Simulation object holds a reference to the read-only network structure in a NetworkGraph object (which is shared by all simulation threads) and its own thread-local representation of the spectrum state in a NetworkState object. It also keeps track of the performance metrics and other statistics in a StatCounter object.
The simulation main loop processes terminations first, then advances the simulation time and creates a new random connection Request. It calls the given subclass of ProvisioningScheme to provision it. After each termination and provisioning, the corresponding method of a StatCounter is called.

The provisioning algorithms are defined in the `provisioning_schemes` subdirectory. Each algorithm is a subclass of the ProvisioningScheme class. It also needs to have a static const member of type ProvisioningSchemeFactory::Registrar<> to register it with a factory class. This is used to create algorithm objects from their names passed as command-line parameters or to iterate over all supported algorithms to print the help information.


Network representation
----------------------

The network graph is represented as a compressed sparse row graph, which is the preferred graph structure in the Boost Graph Library for graphs that change rarely. A directed graph is used and between two nodes, there is typically a separate edge for each direction. Compared to an undirected graph, this has the advantage that an edge can directly be identified with a spectrum state.
The network state representation in the NetworkState class closely follows the mathematical formulation in the thesis.
Spectrum use is represented as bitsets. There are arrays with one bitset per network link representing whether a slot is used by a primary connection (NetworkState::primaryUse) and whether it is used by any connection at all (NetworkState::anyUse). A two-dimensional array of bitsets is used to represent which backup slots in link i are currently protecting primaries going through link j. This allows to easily validate link-disjointness constraints by logically OR-ing these bitsets together.

The NetworkState class offers comfortable member functions for evaluating where spectrum is available for a primary connection or for a backup connection, considering all constraints. It also has member functions for some cost metrics. It would be nicer from a design point of view if cost metrics could be implemented by the algorithms without changing the NetworkState class, but that would require access to many private members. Contributors are encouraged to come up with better solutions here.

Adding new heuristics
---------------------

Heuristics are implemented in separate classes in the `provisioning_schemes` directory. Each of these classes is a subclass of the `ProvisioningScheme` interface and has a member of type ProvisioningSchemeFactory::Registrar<>.
Implement the constructor to accept parameters for your algorithm in a ProvisioningScheme::ParameterSet object and
~~~cpp
operator()(const NetworkGraph& g, const NetworkState& s,const NetworkGraph::DijkstraData &data,const Request& r)
~~~
to implement your new algorithm. It is best to look at an existing algorithm class to see how this is done and copy it to create a new algorithm.

