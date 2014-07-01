/**
 * @file NetworkState.h
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

#ifndef NETWORKSTATE_H_
#define NETWORKSTATE_H_

#include <bitset>
#include <cstdint>
#include <map>

#include "globaldef.h"
#include "modulation.h"
#include "NetworkGraph.h"
#include "SimulationMsgs.h"
#include "StatCounter.h"

/**
 * \brief Maintains the network's spectrum state during a simulation run.
 */
class NetworkState {
public:
	NetworkState(const NetworkGraph &topology);
	virtual ~NetworkState();
	void provision(const Provisioning &p);
	void terminate(const Provisioning &p);
	void reset();
	typedef std::bitset<NUM_SLOTS> spectrum_bits;
	spectrum_bits priAvailability(const NetworkGraph::Path &priPath) const;
	spectrum_bits bkpAvailability(
			const NetworkGraph::Path &priPath,
			const NetworkGraph::Graph::edge_descriptor bkpLink) const;
	spectrum_bits bkpAvailability(
			const NetworkGraph::Path &priPath,
			const NetworkGraph::Path &bkpPath) const;
	StatCounter::PerfMetrics getCurrentPerfMetrics() const;

	void sanityCheck(const std::multimap<unsigned long, Provisioning> &conns) const;

	//uint64_t getCurrentBkpBw() const;

	unsigned int calcCuts(const NetworkGraph& g, const NetworkGraph::Path &p,
			const specIndex_t begin, const specIndex_t end) const;
	double calcMisalignments(const NetworkGraph& g, const NetworkGraph::Path &p,
			const specIndex_t begin, const specIndex_t end) const;
	unsigned int countFreeBlocks(const NetworkGraph::Path& bkpPath,
			specIndex_t i) const;
	unsigned int countFreeBlocks(const NetworkGraph::Path &p,
			const specIndex_t begin, const specIndex_t end) const;

private:
	NetworkState(const NetworkState &n);
	linkIndex_t numLinks;
	nodeIndex_t numNodes;
	unsigned long numAmps;
	spectrum_bits *primaryUse;
	spectrum_bits *anyUse;
	/**
	 * This is a two-dimensional array of size numLinks^2.
	 * The bitset at sharing[i*numLinks+j] defines
	 * the backup spectrum in link i that protects primaries in j.
	 */
	spectrum_bits *sharing;
	uint64_t currentPriSlots;
	uint64_t currentBkpSlots;
	uint64_t currentBkpLpSlots;
	uint64_t currentTxSlots[MOD_NONE];
	typedef struct LinkFrag{
		specIndex_t priEnd, bkpBegin;
		double priFrag, bkpFrag, totalFrag;
		LinkFrag();
	} linkfrag_t;
	linkfrag_t *frag;
	unsigned short *linkAmps;
	void updateLinkFrag(const NetworkGraph::Path &p);
};

#endif /* NETWORKSTATE_H_ */
