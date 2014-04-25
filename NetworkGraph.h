/**
 * @file NetworkGraph.h
 *
 */

/*
 * This file is part of SPP EON Simulator.
 *
 * SPP EON Simulator is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SPP EON Simulator is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SPP EON Simulator.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NETWORKGRAPH_H_
#define NETWORKGRAPH_H_

#include <boost/graph/compressed_sparse_row_graph.hpp>
#include <boost/graph/graph_selectors.hpp>
#include <boost/graph/properties.hpp>
#include <boost/pending/property.hpp>
#include <fstream>
#include <utility>
#include <vector>

class NetworkGraph:
		public boost::compressed_sparse_row_graph<
		boost::directedS, //Graph type: Directed, Undirected, Bidirectional.
		boost::no_property, //Vertex properties
		boost::property<boost::edge_weight_t,unsigned int>, //Edge properties
		boost::no_property, //Graph properties
		unsigned int, //vertex index type
		unsigned int  //edge index type
		>
{
public:
	static NetworkGraph loadFromMatrix(std::istream &s);
	virtual ~NetworkGraph();
private:
	typedef std::vector<std::pair<unsigned int, unsigned int> >::iterator edge_iterator;
	typedef std::vector<unsigned int>::iterator link_distance_iterator;
	NetworkGraph(edge_iterator edge_begin, edge_iterator edge_end,link_distance_iterator ep_iter,
            vertices_size_type numverts,
            edges_size_type numedges);
};

#endif /* NETWORKGRAPH_H_ */
