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

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_selectors.hpp>
#include <boost/graph/properties.hpp>
#include <boost/pending/property.hpp>
#include <fstream>

class NetworkGraph:
		public boost::adjacency_list<
		boost::vecS, //Container type to store outgoing edges of each vertex
		boost::vecS, //Container type to store vertices
		boost::directedS, //Graph type: Directed, Undirected, Bidirectional.
		boost::no_property, //Vertex properties
		boost::property<boost::edge_weight_t,unsigned int>, //Edge properties
		boost::no_property, //Graph properties
		boost::vecS //Container type to store the edge list of the graph
		>
{
public:
	NetworkGraph();
	void loadFromMatrix(std::istream &s);
	virtual ~NetworkGraph();
};

#endif /* NETWORKGRAPH_H_ */
