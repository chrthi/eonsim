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

#include <boost/graph/detail/compressed_sparse_row_struct.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/named_function_params.hpp>
#include <boost/graph/properties.hpp>
#include <boost/property_map/property_map.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <iterator>
#include <limits>

//#include "globaldef.h"

using namespace boost;

NetworkGraph::NetworkGraph(edgeIterator edge_begin, edgeIterator edge_end,
		vertices_size_type numverts, edges_size_type numedges, distance_t *dists):
		boost::compressed_sparse_row_graph<
		boost::directedS, //Graph type: Directed, Undirected, Bidirectional.
		boost::no_property, //Vertex properties
		boost::no_property, //Edge properties
		boost::no_property, //Graph properties
		nodeIndex_t, //vertex index type
		linkIndex_t  //edge index type
		>(edges_are_sorted,edge_begin,edge_end,numverts,numedges),
		link_lengths(dists)
{
}

NetworkGraph::~NetworkGraph() {
	delete[] link_lengths;
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

NetworkGraph::DijkstraData::DijkstraData(const NetworkGraph &g):
		weights(new distance_t[num_edges(g)]),
		dists(new distance_t[num_vertices(g)]),
		preds(new vertex_descriptor[num_vertices(g)]),
		colors(new unsigned char[num_vertices(g)]),
		link_lengths(g.link_lengths),
		wSize(num_edges(g)*sizeof(distance_t))
{
	resetWeights();
}

NetworkGraph::DijkstraData::~DijkstraData() {
	delete[] colors;
	delete[] preds;
	delete[] dists;
	delete[] weights;
}

void NetworkGraph::DijkstraData::resetWeights() const {
	memcpy(weights,link_lengths,wSize);
}

NetworkGraph::Path NetworkGraph::dijkstra(
		vertex_descriptor s, vertex_descriptor d,
		const DijkstraData& data) const {
	boost::dijkstra_shortest_paths(
			*this,
			s,
			weight_map(make_iterator_property_map(data.weights,get(edge_index,*this)))
			.predecessor_map(make_iterator_property_map(data.preds,get(vertex_index,*this)))
			.distance_map(make_iterator_property_map(data.dists,get(vertex_index,*this)))
			.color_map(make_iterator_property_map(data.colors,get(vertex_index,*this)))
	);
	std::vector<edge_descriptor> r;
	if(d==data.preds[d]) return r;
	for(vertex_descriptor v=d; v!=s; ) {
		std::pair<edge_descriptor,bool> e=edge(data.preds[v],v,*this);
		if(!e.second || e.first.src==v) {
			r.clear();
			return r;
		}
		r.push_back(e.first);
		v=e.first.src;
	}
	std::reverse(r.begin(),r.end());

	/*for(std::vector<boost::graph_traits<NetworkGraph>::edge_descriptor>::iterator it=result.priPath.begin();
			it!=result.priPath.end(); ++it)
		std::cout << it->src<<"-";
	std::cout<<r.dest<<" ("<<dists[r.dest]<<')'<<std::endl;*/

	return r;
}

NetworkGraph::YenKShortestSearch::YenKShortestSearch(const NetworkGraph& g,
		vertex_descriptor s, vertex_descriptor d, const DijkstraData& data):
				g(g),
				s(s),
				d(d),
				data(data),
				A(),
				B()
{
}

std::vector<NetworkGraph::Path> &NetworkGraph::YenKShortestSearch::getPaths(unsigned int k) {
	if(k<=A.size()) return A;
	if(!A.size()) {
		const Path p=g.dijkstra(s,d,data);
		if(!p.empty()) A.push_back(p);
		if(k==1) return A;
	}
	while(A.size()<k) {
		const Path& prev=*(A.rbegin());
		distance_t rootD=0;
		for(nodeIndex_t i=0;i<prev.size(); ++i) {
			//for all previous paths
			for(std::vector<Path>::iterator itA=A.begin(); itA!=A.end(); ++itA) {
				//check if they are identical in the first i edges
				if(itA->size()<i) continue;
				bool sameRoot=true;
				for(nodeIndex_t k=0; k<i; ++k)
					if((*itA)[k].idx!=prev[k].idx) {sameRoot=false; break;}
				//if yes, remove the following edge from the graph.
				if(sameRoot)
					data.weights[(*itA)[i].idx]=std::numeric_limits<distance_t>::max();
			}

			//calculate shortest spur path
			boost::dijkstra_shortest_paths(
					g,
					prev[i].src,
					weight_map(make_iterator_property_map(data.weights,get(edge_index,g)))
					.predecessor_map(make_iterator_property_map(data.preds,get(vertex_index,g)))
					.distance_map(make_iterator_property_map(data.dists,get(vertex_index,g)))
					.color_map(make_iterator_property_map(data.colors,get(vertex_index,g)))
			);
			VertexPath newCandidate;
			for(vertex_descriptor v=d; v!=data.preds[v]; v=data.preds[v])
				newCandidate.push_back(data.preds[v]);

			if(newCandidate.size() && *newCandidate.rbegin()==prev[i].src) {
				for(int k=i-1; k>=0; --k)
					newCandidate.push_back(prev[k].src);
				bool isUnique=true;
				const std::pair<yen_path_buffer::iterator,yen_path_buffer::iterator>
					range=B.equal_range(rootD+data.dists[d]);
				for(yen_path_buffer::iterator it=range.first; it!=range.second; ++it)
					if(newCandidate==it->second) { isUnique=false; break; }
				if(isUnique)
					B.insert(yen_path_buffer::value_type(rootD+data.dists[d],newCandidate));
			}

			//restore edges
			for(std::vector<Path>::iterator itA=A.begin(); itA!=A.end(); ++itA)
				if(itA->size()>=i) data.weights[(*itA)[i].idx]=g.link_lengths[(*itA)[i].idx];

			rootD+=data.weights[prev[i].idx];
		}
		if(!B.size()) return A;
		const VertexPath &vp=B.begin()->second;
		Path p;
		p.reserve(vp.size());
		for(VertexPath::const_reverse_iterator it=vp.rbegin(); it!=vp.rend()-1; ++it) {
			p.push_back(edge(*it,it[1],g).first);
		}
		p.push_back(edge(*vp.begin(),d,g).first);
		A.push_back(p);
		B.erase(B.begin());
	}
	return A;
}
