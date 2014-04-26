/**
 * @file ShortestFFLFProvisioning.cpp
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

#include "ShortestFFLFProvisioning.h"

#include <boost/graph/compressed_sparse_row_graph.hpp>
#include <boost/graph/detail/compressed_sparse_row_struct.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/named_function_params.hpp>
#include <boost/graph/properties.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/smart_ptr/shared_array.hpp>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <iterator>
#include <limits>
#include <utility>
#include <vector>

#include "../globaldef.h"
#include "../NetworkGraph.h"

//#include "../NetworkState.h"

using namespace boost;

ShortestFFLFProvisioning::ShortestFFLFProvisioning() {
}

ShortestFFLFProvisioning::~ShortestFFLFProvisioning() {
}

Provisioning ShortestFFLFProvisioning::operator ()(const NetworkGraph& g,
		const NetworkState& s, const Request& r) {
	Provisioning result;

	typedef boost::graph_traits<NetworkGraph>::vertex_descriptor vertd;
	typedef boost::graph_traits<NetworkGraph>::edge_descriptor edged;
	distance_t *dists=new distance_t[boost::num_vertices(g)];
	vertd *preds=new vertd[boost::num_vertices(g)];

	distance_t *weights=new distance_t[boost::num_edges(g)];
	memcpy(weights,g.dists.get(),boost::num_edges(g)*sizeof(distance_t));

	boost::dijkstra_shortest_paths(
			g,
			r.source,
			weight_map(make_iterator_property_map(weights,get(edge_index,g)))
			.predecessor_map(make_iterator_property_map(preds,get(vertex_index,g)))
			.distance_map(make_iterator_property_map(dists,get(vertex_index,g)))
	);

	edged ed;
	for(vertd v=r.dest; v!=r.source; v=ed.src) {
		ed=edge(preds[v],v,g).first;
		result.priPath.push_back(ed);
		weights[ed.idx]=std::numeric_limits<distance_t>::max();
	}
	std::reverse(result.priPath.begin(),result.priPath.end());

	for(std::vector<boost::graph_traits<NetworkGraph>::edge_descriptor>::iterator it=result.priPath.begin();
			it!=result.priPath.end(); ++it)
		std::cout << it->src<<"-";
	std::cout<<r.dest<<" ("<<dists[r.dest]<<')'<<std::endl;

	boost::dijkstra_shortest_paths(
			g,
			r.source,
			predecessor_map(make_iterator_property_map(preds,get(vertex_index,g)))
			.distance_map(make_iterator_property_map(dists,get(vertex_index,g)))
			.weight_map(make_iterator_property_map(weights,get(edge_index,g)))
	);
	for(vertd v=r.dest; v!=r.source; v=ed.src) {
		ed=edge(preds[v],v,g).first;
		result.bkpPath.push_back(ed);
	}
	std::reverse(result.bkpPath.begin(),result.bkpPath.end());
	for(std::vector<boost::graph_traits<NetworkGraph>::edge_descriptor>::iterator it=result.bkpPath.begin();
			it!=result.bkpPath.end(); ++it)
		std::cout << it->src<<"-";
	std::cout<<r.dest<<" ("<<dists[r.dest]<<')'<<std::endl;

	delete[] dists;
	delete[] weights;
	delete[] preds;

	return result;
}
