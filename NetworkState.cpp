/**
 * @file NetworkState.cpp
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

#include "NetworkState.h"

#include <boost/graph/compressed_sparse_row_graph.hpp>
#include <boost/graph/graph_traits.hpp>
#include <iterator>
#include <vector>

#include "NetworkGraph.h"
#include "Simulation.h"

//use this when testing new provisioning methods to check if they generate illegal provisionings.
//Alternatively, compile with make -DPROVDEBUG
//#define PROVDEBUG

NetworkState::NetworkState(const NetworkGraph& topology) :
numLinks(boost::num_edges(topology.g)),
numNodes(boost::num_vertices(topology.g)),
primaryUse(new spectrum_bits[numLinks]),
anyUse(new spectrum_bits[numLinks]),
sharing(new spectrum_bits[numLinks*numLinks]),
currentBkpBw(0),
currentSwitchings(0),
currentTxSlots()
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
#ifdef PROVDEBUG
			assert(!primaryUse[it->idx][i]);
			assert(!anyUse[it->idx][i]);
#endif
			primaryUse[it->idx][i]=true;
			anyUse[it->idx][i]=true;
		}
	}
	for(edgeIt it=p.bkpPath.begin(); it!=p.bkpPath.end(); ++it)
		for(specIndex_t i=p.bkpSpecBegin;i<p.bkpSpecEnd;++i)
			anyUse[it->idx][i]=true;
	for(edgeIt itb=p.bkpPath.begin(); itb!=p.bkpPath.end(); ++itb)
		for(edgeIt itp=p.priPath.begin(); itp!=p.priPath.end(); ++itp) {
#ifdef PROVDEBUG
			assert(itb->idx!=itp->idx);
#endif
			for(specIndex_t i=p.bkpSpecBegin;i<p.bkpSpecEnd;++i) {
#ifdef PROVDEBUG
				if(sharing[itb->idx*numLinks+itp->idx][i]) {
					std::cerr<<i<<'\n'<<
							(sharing[itb->idx*numLinks+itp->idx]).to_string('_','X')<<'\n';
					std::cerr<<bkpAvailability(p.priPath,p.bkpPath).to_string('_','X')<<'\n';
					assert(false);
				}
#endif
				sharing[itb->idx*numLinks+itp->idx][i]=true;
			}
		}
	currentBkpBw+=(p.bkpSpecEnd-p.bkpSpecBegin)*p.bkpPath.size();
	currentSwitchings+=p.priPath.size();
	currentTxSlots[p.priMod]+=p.priSpecEnd-p.priSpecBegin;
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
	currentBkpBw-=(p.bkpSpecEnd-p.bkpSpecBegin)*p.bkpPath.size();
	currentSwitchings-=p.priPath.size();
	currentTxSlots[p.priMod]-=p.priSpecEnd-p.priSpecBegin;
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

void NetworkState::reset() {
	for(size_t i=0; i<numLinks; ++i) primaryUse[i].reset();
	for(size_t i=0; i<numLinks; ++i) anyUse[i].reset();
	for(size_t i=0; i<static_cast<size_t>(numLinks*numLinks); ++i) sharing[i].reset();
	currentBkpBw=0;
}

specIndex_t NetworkState::countFreeBlocks(const NetworkGraph::Path& bkpPath,
		specIndex_t i) const {
	specIndex_t result=0;
	for(auto const &e:bkpPath)
		if(!anyUse[e.idx][i]) ++result;
	return result;
}

unsigned long NetworkState::getTotalPri() const {
	unsigned long result=0;
	for(size_t i=0; i<numLinks; ++i) result+=primaryUse[i].count();
	return result;
}

specIndex_t NetworkState::getUsedSpectrum(linkIndex_t l) const {
	return anyUse[l].count();
}

#ifdef PROVDEBUG
void NetworkState::sanityCheck(
		const std::multimap<unsigned long, Provisioning>& conns) const {
	unsigned int totalHops=0;
	for(auto const &c:conns) {
		totalHops+=c.second.priPath.size();
		for(auto const &ep:c.second.priPath) {
			for(specIndex_t i=c.second.priSpecBegin; i<c.second.priSpecEnd; ++i) {
				assert(primaryUse[ep.idx][i]);
				assert(anyUse[ep.idx][i]);
			}
		}
		for(auto const &eb:c.second.bkpPath) {
			for(specIndex_t i=c.second.bkpSpecBegin; i<c.second.bkpSpecEnd; ++i) {
				assert(!primaryUse[eb.idx][i]);
				assert(anyUse[eb.idx][i]);
			}
			for(auto const &ep:c.second.priPath) {
				for(specIndex_t i=c.second.bkpSpecBegin; i<c.second.bkpSpecEnd; ++i) {
					assert(sharing[eb.idx*numLinks+ep.idx][i]);
				}
			}
		}
	}
	for(linkIndex_t b=0; b<numLinks; ++b) {
		spectrum_bits anyUseTest=primaryUse[b];
		spectrum_bits * const shb=sharing+b*numLinks;
		for(linkIndex_t p=0; p<numLinks; ++p)
			anyUseTest|=shb[p];
		assert(anyUse[b]==anyUseTest);
	}
}
#endif

uint64_t NetworkState::getCurrentBkpBw() const {
	return currentBkpBw;
}

specIndex_t NetworkState::getLargestSegment(linkIndex_t l) const {
	specIndex_t result=0, count=0;
	for(int i=0; i<NUM_SLOTS; ++i) {
		if(anyUse[l][i])
			count=0;
		else if(++count>result)
			result=count;
	}
	return result;
}

unsigned int NetworkState::calcCuts(const NetworkGraph& g,
		const NetworkGraph::Path& p,
		const specIndex_t begin, const specIndex_t end) const {
	if(begin==0 || end==NUM_SLOTS) return 0;
	unsigned int result=0;
	for(auto const &e:p)
		if(!anyUse[e.idx][begin-1] && !anyUse[e.idx][end])
			++result;
	return result;
}

double NetworkState::calcMisalignments(const NetworkGraph& g,
		const NetworkGraph::Path& p,
		const specIndex_t begin, const specIndex_t end) const {
	double result=0.0;
	typedef boost::graph_traits <NetworkGraph::Graph>::out_edge_iterator out_edge_iterator;
	for(auto const &e:p) {
		std::pair<out_edge_iterator, out_edge_iterator> outEdges =
				boost::out_edges(e.src, g.g);
		unsigned int numFreeSlots=0, numAdjLinks=boost::out_degree(e.src,g.g);
		for(; outEdges.first != outEdges.second; ++outEdges.first)
			if(outEdges.first.dereference().idx!=e.idx)
				for(specIndex_t i=begin; i<end; ++i)
					if(!anyUse[outEdges.first.dereference().idx][i])
						++numFreeSlots;
		result+=(double)numFreeSlots/(double)numAdjLinks;
	}

	return result;
}

unsigned int NetworkState::countFreeBlocks(const NetworkGraph::Path& p,
		const specIndex_t begin, const specIndex_t end) const {
	unsigned int result=0;
	for(auto const &e:p)
		for(specIndex_t i=begin; i<end; ++i)
			if(!anyUse[e.idx][i]) ++result;
	return result;
}

uint64_t NetworkState::getCurrentSwitchings() const {
	return currentSwitchings;
}

linkIndex_t NetworkState::getNumLinks() const {
	return numLinks;
}

nodeIndex_t NetworkState::getNumNodes() const {
	return numNodes;
}

const uint64_t* NetworkState::getCurrentTxSlots() const {
	return currentTxSlots;
}
/*
 * Energy
 * Cicek's code:
#define En_idle 150 //node idle power? / diff active-idle for a node
#define E_transceiver 5.9 // 0.01 is receiver, 5.89 is transmitter
#define E_switch 1.757 //switching energy, per connection and link
#define E_Amp 9 // E_Amp*((length/80)+2)

 */
