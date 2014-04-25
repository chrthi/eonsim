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
numLinks(boost::num_edges(topology)),
primaryUse(new std::bitset<NUM_SLOTS>[numLinks]),
anyUse(new std::bitset<NUM_SLOTS>[numLinks]),
sharing(new std::bitset<NUM_SLOTS>[numLinks*numLinks])
{
}

NetworkState::~NetworkState() {
	delete[] sharing;
	delete[] anyUse;
	delete[] primaryUse;
}

void NetworkState::provision(const Provisioning &p) {
	typedef std::vector<boost::graph_traits<NetworkGraph>::edge_descriptor>::const_iterator edgeIt;
	for(edgeIt it=p.priPath.begin(); it!=p.priPath.end(); ++it) {
		for(specIndex_t i=p.priSpecBegin;i<p.priSpecEnd;++i) {
			primaryUse[it->idx].set(i,true);
			anyUse[it->idx].set(i,true);
		}
	}
	for(edgeIt it=p.bkpPath.begin(); it!=p.bkpPath.end(); ++it)
		for(specIndex_t i=p.priSpecBegin;i<p.priSpecEnd;++i)
			anyUse[it->idx].set(i,true);
	for(edgeIt itp=p.priPath.begin(); itp!=p.priPath.end(); ++itp)
		for(edgeIt itb=p.bkpPath.begin(); itb!=p.bkpPath.end(); ++itb)
			for(specIndex_t i=p.priSpecBegin;i<p.priSpecEnd;++i)
				sharing[itp->idx*numLinks+itb->idx].set(i, true);
}

void NetworkState::terminate(const Provisioning &p) {
	typedef std::vector<boost::graph_traits<NetworkGraph>::edge_descriptor>::const_iterator edgeIt;
	for(edgeIt it=p.priPath.begin(); it!=p.priPath.end(); ++it) {
		for(specIndex_t i=p.priSpecBegin;i<p.priSpecEnd;++i) {
			primaryUse[it->idx].set(i,false);
			anyUse[it->idx].set(i,false);
		}
	}
	//the primary links now do not share any backup here any more
	for(edgeIt itb=p.bkpPath.begin(); itb!=p.bkpPath.end(); ++itb)
		for(edgeIt itp=p.priPath.begin(); itp!=p.priPath.end(); ++itp)
			for(specIndex_t i=p.priSpecBegin;i<p.priSpecEnd;++i)
				sharing[itb->idx*numLinks+itp->idx].set(i, false);

	/* On each previous backup link, construct the new backup spectrum
	 * by checking all other links' sharing entries.
	 * Takes a huge O(bkpLen*numLinks*numSlots).
	 * This is a possible downside of the sharing matrix implementation.
	 */
	for(edgeIt itb=p.bkpPath.begin(); itb!=p.bkpPath.end(); ++itb) {
		anyUse[itb->idx]=primaryUse[itb->idx];
		std::bitset<NUM_SLOTS> *shb=sharing+itb->idx*numLinks;
		for(linkIndex_t i=0; i<numLinks; ++i)
			anyUse[itb->idx]|=shb[i];
	}
}

std::bitset<NUM_SLOTS> NetworkState::bkpAvailability(
		const std::vector<boost::graph_traits<NetworkGraph>::edge_descriptor> &priPath,
		const boost::graph_traits<NetworkGraph>::edge_descriptor bkpLink) const {
	typedef std::vector<boost::graph_traits<NetworkGraph>::edge_descriptor>::const_iterator edgeIt;
	std::bitset<NUM_SLOTS> result;
	for(edgeIt it=priPath.begin(); it!=priPath.end(); ++it)
		result|=sharing[bkpLink.idx*numLinks+it->idx];
	return result;
}
