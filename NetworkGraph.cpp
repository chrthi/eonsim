/**
 * @file NetworkGraph.cpp
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

#include "NetworkGraph.h"

#include <cmath>
#include <iostream>
#include <iterator>
#include <limits>

#include "globaldef.h"

using namespace boost;

NetworkGraph::NetworkGraph(edge_iterator edge_begin, edge_iterator edge_end,
		link_distance_iterator ep_iter, vertices_size_type numverts,
		edges_size_type numedges):
		boost::compressed_sparse_row_graph<
		boost::directedS, //Graph type: Directed, Undirected, Bidirectional.
		boost::no_property, //Vertex properties
		boost::property<boost::edge_weight_t,unsigned int>, //Edge properties
		boost::no_property, //Graph properties
		unsigned int, //vertex index type
		unsigned int  //edge index type
		>(edges_are_sorted,edge_begin,edge_end,ep_iter,numverts,numedges)
{
}

NetworkGraph::~NetworkGraph() {
}

NetworkGraph NetworkGraph::loadFromMatrix(std::istream &s) {
	unsigned int n, l;
	s>>n>>l;
	s.ignore(2); // skip the (cr)lf after the link count
	for(unsigned int i=0; i<n; ++i) {
		s.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
	}
	std::vector<std::pair<unsigned int, unsigned int> > edges;
	edges.reserve(l);
	std::vector<unsigned int> lengths;
	lengths.reserve(l);
	for(unsigned int i=0; i<n; ++i) {
		for(unsigned int k=0; k<n; ++k) {
			double d;
			s>>d;
			if(d>0.0) {// if(std::isnormal(d)){
				edges.push_back(std::pair<unsigned int, unsigned int>(i,k));
				lengths.push_back(lrint(d/DISTANCE_UNIT));
			}
		}
	}
	return NetworkGraph(edges.begin(),edges.end(),lengths.begin(),n,l);
}

