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
		vertices_size_type numverts, edges_size_type numedges, distance_t *dists):
		boost::compressed_sparse_row_graph<
		boost::directedS, //Graph type: Directed, Undirected, Bidirectional.
		boost::no_property, //Vertex properties
		boost::no_property, //Edge properties
		boost::no_property, //Graph properties
		nodeIndex_t, //vertex index type
		linkIndex_t  //edge index type
		>(edges_are_sorted,edge_begin,edge_end,numverts,numedges),
		dists(dists)
{
}

NetworkGraph::~NetworkGraph() {
}

NetworkGraph NetworkGraph::loadFromMatrix(std::istream &s) {
	vertices_size_type n;
	edges_size_type l;
	s>>n>>l;
	l*=2; //we treat the two directions as separate edges
	s.ignore(2); // skip the (cr)lf after the link count
	for(nodeIndex_t i=0; i<n; ++i) {
		s.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
	}
	std::vector<std::pair<nodeIndex_t, nodeIndex_t> > edges;
	edges.reserve(l);
	distance_t *dists=new distance_t[l];
	std::vector<nodeIndex_t> lengths;
	lengths.reserve(l);
	size_t li=0;
	for(nodeIndex_t i=0; i<n; ++i) {
		for(nodeIndex_t k=0; k<n; ++k) {
			double d;
			s>>d;
			if(d>0.0) {// if(std::isnormal(d)){
				edges.push_back(std::pair<nodeIndex_t, nodeIndex_t>(i,k));
				dists[li++]=lrint(d/DISTANCE_UNIT);
			}
		}
	}

	return NetworkGraph(edges.begin(),edges.end(),n,l,dists);
}

