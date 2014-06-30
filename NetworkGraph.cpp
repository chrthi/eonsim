/**
 * @file NetworkGraph.cpp
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

#include "NetworkGraph.h"

#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/named_function_params.hpp>
#include <boost/graph/properties.hpp>
#include <boost/property_map/property_map.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iterator>
#include <limits>
#include <map>

using namespace boost;

NetworkGraph::NetworkGraph(edgeIterator edge_begin, edgeIterator edge_end,
		Graph::vertices_size_type numverts, Graph::edges_size_type numedges, const std::vector<distance_t> &dists):
		link_lengths(new distance_t[dists.size()]),
		g(edges_are_sorted,edge_begin,edge_end,numverts,numedges)
{
	distance_t *pd=const_cast<distance_t*>(link_lengths);
	for(auto const &d:dists) {
		*pd++=d;
	}
}

NetworkGraph::~NetworkGraph() {
	delete[] link_lengths;
}

NetworkGraph NetworkGraph::loadFromMatrix(std::istream &s) {
	Graph::vertices_size_type n;
	s>>n;
	while(isspace(s.peek())) s.ignore(); // skip the (cr)lf after the node count
	std::vector<std::pair<nodeIndex_t, nodeIndex_t> > edges;
	if(s.peek()>='1' && s.peek()<='9') {
		linkIndex_t l;
		s>>l;
		while(isspace(s.peek())) s.ignore(); // skip the (cr)lf after the link count
		edges.reserve(l*2);
		for(int i=0; i<n; ++i)
			s.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
		while(isspace(s.peek())) s.ignore(); // skip the (cr)lf after the last discarded line
	}
	std::vector<distance_t> dists;
	for(nodeIndex_t i=0; i<n; ++i) {
		for(nodeIndex_t k=0; k<n; ++k) {
			double d;
			s>>d;
			if(d>0.0) {// if(std::isnormal(d)){
				edges.push_back(std::pair<nodeIndex_t, nodeIndex_t>(i,k));
				dists.push_back(lrint(d/DISTANCE_UNIT));
			}
		}
	}
	return NetworkGraph(edges.begin(),edges.end(),n,edges.size(),dists);
}

void NetworkGraph::printAsDot(std::ostream& s) const {
	s<<'#'<<num_vertices(g)<<" Nodes, "<<num_edges(g)<<" Links\n";
	s<<"digraph {\ngraph[overlap=scale, normalize=90];\n";
	auto es = boost::edges(g);
	for (auto eit = es.first; eit != es.second; ++eit) {
		s<<eit->src<<" -> "<<target(*eit,g)
				<<" [len="<<link_lengths[eit->idx]*0.1
				<<", label=\"("<<eit->idx<<") "<<link_lengths[eit->idx]<<"\"];\n";
	}
	s<<"}\n";
}

NetworkGraph::DijkstraData::DijkstraData(const NetworkGraph &g):
		weights(new distance_t[num_edges(g.g)]),
		tmpWeights(new distance_t[num_edges(g.g)]),
		dists(new distance_t[num_vertices(g.g)]),
		preds(new Graph::vertex_descriptor[num_vertices(g.g)]),
		colors(new unsigned char[num_vertices(g.g)]),
		link_lengths(g.link_lengths),
		wSize(num_edges(g.g)*sizeof(distance_t))
{
	resetWeights();
}

NetworkGraph::DijkstraData::~DijkstraData() {
	delete[] colors;
	delete[] preds;
	delete[] dists;
	delete[] tmpWeights;
	delete[] weights;
}

void NetworkGraph::DijkstraData::resetWeights() const {
	memcpy(weights,link_lengths,wSize);
	memcpy(tmpWeights,link_lengths,wSize);
}

NetworkGraph::Path NetworkGraph::dijkstra(
		Graph::vertex_descriptor s, Graph::vertex_descriptor d,
		const DijkstraData& data) const {
	boost::dijkstra_shortest_paths(
			g,
			s,
			weight_map(make_iterator_property_map(data.weights,get(edge_index,g)))
			.predecessor_map(make_iterator_property_map(data.preds,get(vertex_index,g)))
			.distance_map(make_iterator_property_map(data.dists,get(vertex_index,g)))
			.color_map(make_iterator_property_map(data.colors,get(vertex_index,g)))
	);
	std::vector<Graph::edge_descriptor> r;
	if(d==data.preds[d]) return r;
	for(Graph::vertex_descriptor v=d; v!=s; ) {
		std::pair<Graph::edge_descriptor,bool> e=edge(data.preds[v],v,g);
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
		Graph::vertex_descriptor s, Graph::vertex_descriptor d, const DijkstraData& data):
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
		else return A;
		if(k==1) return A;
	}
	memcpy(data.tmpWeights,data.weights,num_edges(g.g)*sizeof(distance_t));
	while(A.size()<k) {
		const Path& prev=*(A.rbegin());
		distance_t rootD=0;
		for(nodeIndex_t i=0;i<prev.size(); ++i) {
			//for all previous paths
			for(std::vector<Path>::iterator itA=A.begin(); itA!=A.end(); ++itA) {
				//need at least i+1 elements: i to compare, 1 to remove.
				if(itA->size()<=i) continue;
				//check if the paths are identical in the first i edges
				bool sameRoot=true;
				for(nodeIndex_t k=0; k<i; ++k)
					if((*itA)[k].idx!=prev[k].idx) {sameRoot=false; break;}
				//if yes, remove the following edge from the graph.
				if(sameRoot)
					data.tmpWeights[(*itA)[i].idx]=std::numeric_limits<distance_t>::max();
			}

			//do not allow nodes of the root path to be visited again.
			//This is easiest accomplished by removing their outward edges.
			for(auto eit=prev.cbegin(); eit!=prev.cbegin()+i; ++eit)
				BGL_FORALL_OUTEDGES_T(eit->src,e,g.g,const Graph)
					data.tmpWeights[e.idx]=std::numeric_limits<distance_t>::max();

			//calculate shortest spur path
			boost::dijkstra_shortest_paths(
					g.g,
					prev[i].src,
					weight_map(make_iterator_property_map(data.tmpWeights,get(edge_index,g.g)))
					.predecessor_map(make_iterator_property_map(data.preds,get(vertex_index,g.g)))
					.distance_map(make_iterator_property_map(data.dists,get(vertex_index,g.g)))
					.color_map(make_iterator_property_map(data.colors,get(vertex_index,g.g)))
			);
			VertexPath newCandidate;
			for(Graph::vertex_descriptor v=d; v!=data.preds[v]; v=data.preds[v])
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
				if(itA->size()>i) data.tmpWeights[(*itA)[i].idx]=data.weights[(*itA)[i].idx];
			for(auto eit=prev.cbegin(); eit!=prev.cbegin()+i; ++eit)
				BGL_FORALL_OUTEDGES_T(eit->src,e,g.g,const Graph)
					data.tmpWeights[e.idx]=data.weights[e.idx];

			rootD+=data.weights[prev[i].idx];
		}
		if(!B.size()) return A;
		const VertexPath &vp=B.begin()->second;
		Path p;
		p.reserve(vp.size());
		for(VertexPath::const_reverse_iterator it=vp.rbegin(); it!=vp.rend()-1; ++it) {
			p.push_back(edge(*it,it[1],g.g).first);
		}
		p.push_back(edge(*vp.begin(),d,g.g).first);
		A.push_back(p);
		B.erase(B.begin());
	}
	return A;
}

void NetworkGraph::YenKShortestSearch::reset() {
	A.clear();
	B.clear();
}

void NetworkGraph::YenKShortestSearch::reset(Graph::vertex_descriptor s, Graph::vertex_descriptor d) {
	reset();
	this->s=s;
	this->d=d;
}

std::ostream & operator<<(std::ostream &os, const NetworkGraph::Path& p) {
	for(auto const &e:p) os<<e.src<<'-';
	return os;
}
