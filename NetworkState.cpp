/**
 * @file NetworkState.cpp
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

#include "NetworkState.h"

#include <boost/graph/compressed_sparse_row_graph.hpp>
#include <boost/graph/graph_traits.hpp>
#include <iterator>
#include <vector>

#include "NetworkGraph.h"
#include "Simulation.h"

NetworkState::NetworkState(const NetworkGraph& topology) :
numLinks(boost::num_edges(topology.g)),
primaryUse(new spectrum_bits[numLinks]),
anyUse(new spectrum_bits[numLinks]),
sharing(new spectrum_bits[numLinks*numLinks])
{
}

NetworkState::~NetworkState() {
	delete[] sharing;
	delete[] anyUse;
	delete[] primaryUse;
}

void NetworkState::provision(const Provisioning &p) {
	typedef NetworkGraph::Path::const_iterator edgeIt;
	for(edgeIt it=p.priPath.begin(); it!=p.priPath.end(); ++it) {
		for(specIndex_t i=p.priSpecBegin;i<p.priSpecEnd;++i) {
			primaryUse[it->idx][i]=true;
			anyUse[it->idx][i]=true;
		}
	}
	for(edgeIt it=p.bkpPath.begin(); it!=p.bkpPath.end(); ++it)
		for(specIndex_t i=p.bkpSpecBegin;i<p.bkpSpecEnd;++i)
			anyUse[it->idx][i]=true;
	for(edgeIt itb=p.bkpPath.begin(); itb!=p.bkpPath.end(); ++itb)
		for(edgeIt itp=p.priPath.begin(); itp!=p.priPath.end(); ++itp)
			for(specIndex_t i=p.bkpSpecBegin;i<p.bkpSpecEnd;++i)
				sharing[itb->idx*numLinks+itp->idx][i]=true;
}

void NetworkState::terminate(const Provisioning &p) {
	typedef NetworkGraph::Path::const_iterator edgeIt;
	for(edgeIt it=p.priPath.begin(); it!=p.priPath.end(); ++it) {
		for(specIndex_t i=p.priSpecBegin;i<p.priSpecEnd;++i) {
			primaryUse[it->idx][i]=false;
			anyUse[it->idx][i]=false;
		}
	}
	//the primary links now do not share any backup here any more
	for(edgeIt itb=p.bkpPath.begin(); itb!=p.bkpPath.end(); ++itb)
		for(edgeIt itp=p.priPath.begin(); itp!=p.priPath.end(); ++itp)
			for(specIndex_t i=p.bkpSpecBegin;i<p.bkpSpecEnd;++i)
				sharing[itb->idx*numLinks+itp->idx][i]=false;

	/* On each previous backup link, construct the new backup spectrum
	 * by checking all other links' sharing entries.
	 * Takes a huge O(bkpLen*numLinks*numSlots).
	 * This is a possible downside of the sharing matrix implementation.
	 */
	for(edgeIt itb=p.bkpPath.begin(); itb!=p.bkpPath.end(); ++itb) {
		anyUse[itb->idx]=primaryUse[itb->idx];
		spectrum_bits * const shb=sharing+itb->idx*numLinks;
		for(linkIndex_t i=0; i<numLinks; ++i)
			anyUse[itb->idx]|=shb[i];
	}
}

NetworkState::spectrum_bits NetworkState::priAvailability(
		const NetworkGraph::Path& priPath) const {
	typedef NetworkGraph::Path::const_iterator edgeIt;
	spectrum_bits result;
	for(edgeIt it=priPath.begin(); it!=priPath.end(); ++it)
		result|=anyUse[it->idx];
	return result;
}

NetworkState::spectrum_bits NetworkState::bkpAvailability(
		const NetworkGraph::Path &priPath,
		const NetworkGraph::Graph::edge_descriptor bkpLink) const {
	typedef NetworkGraph::Path::const_iterator edgeIt;
	spectrum_bits result=primaryUse[bkpLink.idx];
	for(edgeIt it=priPath.begin(); it!=priPath.end(); ++it)
		result|=sharing[bkpLink.idx*numLinks+it->idx];
	return result;
}

NetworkState::spectrum_bits NetworkState::bkpAvailability(
		const NetworkGraph::Path& priPath,
		const NetworkGraph::Path& bkpPath) const {
	typedef NetworkGraph::Path::const_iterator edgeIt;
	spectrum_bits result;
	for(edgeIt itb=bkpPath.begin(); itb!=bkpPath.end(); ++itb) {
		result|=primaryUse[itb->idx];
		for(edgeIt itp=priPath.begin(); itp!=priPath.end(); ++itp)
			result|=sharing[itb->idx*numLinks+itp->idx];
	}
	return result;
}

specIndex_t NetworkState::countFreeBlocks(const NetworkGraph::Path& bkpPath,
		specIndex_t i) const {
	specIndex_t result=0;
	for(auto const &e:bkpPath)
		if(!anyUse[e.idx][i]) ++result;
	return result;
}
