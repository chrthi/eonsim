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

//use this when testing new provisioning methods to check if they generate illegal provisionings.
//#undef NDEBUG

#include "NetworkState.h"

#include <assert.h>
#include <boost/graph/compressed_sparse_row_graph.hpp>
#include <boost/graph/graph_traits.hpp>
#include <iterator>
#include <vector>

#include "NetworkGraph.h"
#include "Simulation.h"

NetworkState::NetworkState(const NetworkGraph& topology) :
numLinks(boost::num_edges(topology.g)),
numNodes(boost::num_vertices(topology.g)),
numAmps(),
primaryUse(new spectrum_bits[numLinks]),
anyUse(new spectrum_bits[numLinks]),
sharing(new spectrum_bits[numLinks*numLinks]),
currentPriSlots(0),
currentBkpSlots(0),
currentBkpLpSlots(0),
currentTxSlots(),
frag(new linkfrag_t[numLinks]),
linkAmps(new unsigned short[numLinks])
{
	for(linkIndex_t i=0; i<numLinks; ++i) {
		linkAmps[i]=lrint(ceil(topology.link_lengths[i]*DISTANCE_UNIT/AMP_DIST)+1);
		numAmps+=linkAmps[i];
	}
}

NetworkState::~NetworkState() {
	delete[] sharing;
	delete[] anyUse;
	delete[] primaryUse;
}

void NetworkState::provision(const Provisioning &p) {
	for(const auto &e:p.priPath) {
		for(specIndex_t i=p.priSpecBegin;i<p.priSpecEnd;++i) {
#ifndef NDEBUG
			assert(!primaryUse[e.idx][i]);
			assert(!anyUse[e.idx][i]);
#endif
			primaryUse[e.idx][i]=true;
			anyUse[e.idx][i]=true;
		}
		if(frag[e.idx].priEnd<p.priSpecEnd)
			frag[e.idx].priEnd=p.priSpecEnd;
	}
	for(const auto &eb:p.bkpPath) {
		for(specIndex_t i=p.bkpSpecBegin;i<p.bkpSpecEnd;++i)
			if(!anyUse[eb.idx][i]) {
				++currentBkpSlots;
				anyUse[eb.idx][i]=true;
			}
		if(frag[eb.idx].bkpBegin>p.bkpSpecBegin)
			frag[eb.idx].bkpBegin=p.bkpSpecBegin;
		for(const auto &ep:p.priPath) {
#ifndef NDEBUG
			assert(eb.idx!=ep.idx);
#endif
			auto &s=sharing[eb.idx*numLinks+ep.idx];
			for(specIndex_t i=p.bkpSpecBegin;i<p.bkpSpecEnd;++i) {
#ifndef NDEBUG
				if(s[i]) {
					std::cerr<<i<<'\n'<<s.to_string('_','X')<<'\n';
					std::cerr<<bkpAvailability(p.priPath,p.bkpPath).to_string('_','X')<<'\n';
					assert(false);
				}
#endif
				s[i]=true;
			}
		}
	}

	updateLinkFrag(p.priPath);
	updateLinkFrag(p.bkpPath);
	currentBkpLpSlots+=(p.bkpSpecEnd-p.bkpSpecBegin)*p.bkpPath.size();
	currentPriSlots+=(p.priSpecEnd-p.priSpecBegin)*p.priPath.size();
	currentTxSlots[p.priMod]+=p.priSpecEnd-p.priSpecBegin;
}

void NetworkState::terminate(const Provisioning &p) {
	for(const auto &e:p.priPath) {
		for(specIndex_t i=p.priSpecBegin;i<p.priSpecEnd;++i) {
			primaryUse[e.idx][i]=false;
			anyUse[e.idx][i]=false;
		}
		if(frag[e.idx].priEnd==p.priSpecEnd) {
			frag[e.idx].priEnd=0;
			for(specIndex_t i=0; i<p.priSpecEnd; ++i)
				if(primaryUse[e.idx][i]) frag[e.idx].priEnd=i;
		}
	}
	//the primary links now do not share any backup here any more
	for(const auto &eb:p.bkpPath)
		for(const auto &ep:p.priPath)
			for(specIndex_t i=p.bkpSpecBegin;i<p.bkpSpecEnd;++i)
				sharing[eb.idx*numLinks+ep.idx][i]=false;

	/* On each previous backup link, construct the new backup spectrum
	 * by checking all other links' sharing entries.
	 * Takes a huge O(bkpLen*numLinks*numSlots).
	 * This is a possible downside of the sharing matrix implementation.
	 */
	for(const auto &eb:p.bkpPath) {
		anyUse[eb.idx]=primaryUse[eb.idx];
		spectrum_bits * const shb=sharing+eb.idx*numLinks;
		spectrum_bits bkpUse=shb[0];
		for(linkIndex_t i=1; i<numLinks; ++i)
			bkpUse|=shb[i];
		anyUse[eb.idx]|=bkpUse;

		//account for the freed slots
		for(specIndex_t i=p.bkpSpecBegin; i<p.bkpSpecEnd; ++i)
			if(!bkpUse[i]) --currentBkpSlots;

		//if the beginning of the backup spectrum has moved, find the new
		//position.
		if(frag[eb.idx].bkpBegin==p.bkpSpecBegin) {
			frag[eb.idx].bkpBegin=NUM_SLOTS;
			for(specIndex_t i=NUM_SLOTS; i>=p.bkpSpecBegin && i<=NUM_SLOTS; --i)
				if(bkpUse[i]) frag[eb.idx].bkpBegin=i;
		}
	}

	updateLinkFrag(p.priPath);
	updateLinkFrag(p.bkpPath);
	currentBkpLpSlots-=(p.bkpSpecEnd-p.bkpSpecBegin)*p.bkpPath.size();
	currentPriSlots-=(p.priSpecEnd-p.priSpecBegin)*p.priPath.size();
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
	currentPriSlots=0;
	currentBkpSlots=0;
	currentBkpLpSlots=0;
	for(size_t i=0; i<sizeof(currentTxSlots)/sizeof(currentTxSlots[0]); ++i) currentTxSlots[i]=0;
	for(size_t i=0; i<numLinks; ++i) frag[i]=LinkFrag();
}

#ifndef NDEBUG
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

unsigned int NetworkState::countFreeBlocks(const NetworkGraph::Path& bkpPath,
		specIndex_t i) const {
	specIndex_t result=0;
	for(auto const &e:bkpPath)
		if(!anyUse[e.idx][i]) ++result;
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

StatCounter::PerfMetrics NetworkState::getCurrentPerfMetrics() const {
	StatCounter::PerfMetrics p;
	p.utilization=(double)(currentPriSlots+currentBkpSlots);///(double)(numLinks*NUM_SLOTS);
	p.sharability=(double)currentBkpLpSlots/(double)currentBkpSlots;
	p.numLinks=numLinks;
	unsigned long idleAmps=0;
	for(linkIndex_t i=0; i<numLinks; ++i) {
		p.addLink(frag[i].bkpBegin,frag[i].bkpFrag,
				frag[i].priEnd,frag[i].priFrag,
				frag[i].totalFrag);
		if(frag[i].priEnd==0 && frag[i].bkpBegin==NUM_SLOTS)
			idleAmps+=linkAmps[i];
	}
	p.e_stat=(numLinks/2)*85.0 + numNodes*150.0	+(numAmps/2)*140.0;
	p.e_dyn+=(numAmps-idleAmps)*30.0;
	p.e_dyn=fma((double)currentTxSlots[BPSK] , 47.13,p.e_dyn);
	p.e_dyn=fma((double)currentTxSlots[QPSK] , 62.75,p.e_dyn);
	p.e_dyn=fma((double)currentTxSlots[QAM8] , 78.38,p.e_dyn);
	p.e_dyn=fma((double)currentTxSlots[QAM16], 94.00,p.e_dyn);
	p.e_dyn=fma((double)currentTxSlots[QAM32],109.63,p.e_dyn);
	p.e_dyn=fma((double)currentTxSlots[QAM64],125.23,p.e_dyn);
	return p;
}

NetworkState::LinkFrag::LinkFrag():
	priEnd(0),
	bkpBegin(NUM_SLOTS),
	priFrag(0.0),
	bkpFrag(0.0),
	totalFrag(0.0)
{}

void NetworkState::updateLinkFrag(const NetworkGraph::Path& p) {
	for(const auto &e:p) {
		specIndex_t longestFree=0, totalLongestFree=0;
		specIndex_t sectionTotalFree=0, totalFree=0;
		specIndex_t c=0;
		for(specIndex_t i=0; i<frag[e.idx].priEnd; ++i) {
			if(!anyUse[e.idx][i]) {
				++c;
			}else if(c) {
				if(c>longestFree) {
					longestFree=c;
				}
				sectionTotalFree+=c;
				c=0;
			}
		}
		frag[e.idx].priFrag=sectionTotalFree?
				1.0-(double)longestFree/(double)sectionTotalFree : 0.0;
		totalFree=sectionTotalFree;
		totalLongestFree=longestFree;

		specIndex_t mid=0;
		if(frag[e.idx].bkpBegin>frag[e.idx].priEnd) {
			mid=frag[e.idx].bkpBegin-frag[e.idx].priEnd;
			totalFree+=mid;
			if(mid>totalLongestFree) totalLongestFree=mid;
		}
		c=0;
		longestFree=0;
		sectionTotalFree=0;
		for(specIndex_t i=frag[e.idx].bkpBegin; i<NUM_SLOTS; ++i) {
			if(!anyUse[e.idx][i]) {
				++c;
			}else if(c) {
				if(c>longestFree) {
					if(c>totalLongestFree) totalLongestFree=c;
					longestFree=c;
				}
				sectionTotalFree+=c;
				if(i>frag[e.idx].priEnd) totalFree+=c;
				c=0;
			}
		}
		if(longestFree>totalLongestFree) totalLongestFree=longestFree;
		frag[e.idx].bkpFrag=sectionTotalFree?
				1.0-(double)longestFree/(double)sectionTotalFree : 0.0;
		frag[e.idx].totalFrag=sectionTotalFree?
				1.0-(double)totalLongestFree/(double)totalFree : 0.0;
	}
}
