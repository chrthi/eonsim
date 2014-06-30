/**
 * @file NetworkGraph.h
 *
 */

/*
 * This file is part of eonsim.
 *
 * eonsim is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * eonsim is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with eonsim.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NETWORKGRAPH_H_
#define NETWORKGRAPH_H_

#include <boost/graph/compressed_sparse_row_graph.hpp>
#include <boost/graph/graph_selectors.hpp>
#include <boost/pending/property.hpp>
#include <stddef.h>
#include <fstream>
#include <map>
#include <utility>
#include <vector>

#include "globaldef.h"

/**
 * \brief Holds the network graph structure and supports path search.
 *
 * There is one instance of NetworkGraph in the program that is shared by all
 * threads. The graph is read from a text file and then remains constant,
 * so there are no concurrency issues.
 */
class NetworkGraph {
public:
	static NetworkGraph loadFromMatrix(std::istream &s);
	virtual ~NetworkGraph();
	const distance_t* const link_lengths;

	typedef boost::compressed_sparse_row_graph<
			boost::directedS, //Graph type: Directed, Undirected, Bidirectional.
			boost::no_property, //Vertex properties
			boost::no_property, //Edge properties
			boost::no_property, //Graph properties
			nodeIndex_t, //vertex index type
			linkIndex_t  //edge index type
			> Graph;
	Graph g;
	/**
	 * \brief Distance and predecessor matrices used by the Dijkstra algorithm.
	 *
	 * To avoid the memory allocation cost, a simulation object keeps a
	 * DijkstraData object during the whole simulation run.
	 */
	class DijkstraData {
	public:
		DijkstraData(const NetworkGraph &g);
		~DijkstraData();
		distance_t *const weights, *const tmpWeights, *const dists;
		Graph::vertex_descriptor *const preds;
		unsigned char *const colors;
		void resetWeights() const;
	private:
		const distance_t *const link_lengths;
		size_t wSize;
		DijkstraData(const DijkstraData &);
	};

	typedef std::vector<Graph::edge_descriptor> Path;

	void printAsDot(std::ostream &s) const;
	Path dijkstra(Graph::vertex_descriptor s, Graph::vertex_descriptor d, const DijkstraData &data) const;

	/**
	 * \brief Holds k-shortest path search state so that additional paths can be calculated later.
	 */
	class YenKShortestSearch{
	public:
		YenKShortestSearch(const NetworkGraph &g, Graph::vertex_descriptor s, Graph::vertex_descriptor d, const DijkstraData &data);
		std::vector<Path> &getPaths(unsigned int k);
		void reset();
		void reset(Graph::vertex_descriptor s, Graph::vertex_descriptor d);
	private:
		typedef std::vector<Graph::vertex_descriptor> VertexPath;
		typedef std::multimap<distance_t,VertexPath> yen_path_buffer;

		const NetworkGraph &g;
		Graph::vertex_descriptor s, d;
		const DijkstraData &data;
		std::vector<Path> A;
		yen_path_buffer B;
	};
private:
	typedef std::vector<std::pair<nodeIndex_t, nodeIndex_t> >::iterator edgeIterator;
	NetworkGraph(edgeIterator edge_begin, edgeIterator edge_end,
			Graph::vertices_size_type numverts, Graph::edges_size_type numedges, const std::vector<distance_t> &dists);
};

std::ostream & operator<<(std::ostream &os, const NetworkGraph::Path& p);

#endif /* NETWORKGRAPH_H_ */
