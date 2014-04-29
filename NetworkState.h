/**
 * @file NetworkState.h
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

#ifndef NETWORKSTATE_H_
#define NETWORKSTATE_H_

#include <boost/graph/graph_traits.hpp>
#include <bitset>
#include <vector>

#include "globaldef.h"
#include "NetworkGraph.h"

struct Provisioning;

class NetworkGraph;

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
	specIndex_t countFreeBlocks(const NetworkGraph::Path &bkpPath, specIndex_t i) const;
private:
	NetworkState(const NetworkState &n);
	linkIndex_t numLinks;
	spectrum_bits *primaryUse;
	spectrum_bits *anyUse;
	/**
	 * This is a two-dimensional array of size numLinks^2.
	 * The bitset at sharing[i*numLinks+j] defines
	 * the backup spectrum in link i that protects primaries in j.
	 */
	spectrum_bits *sharing;
};

#endif /* NETWORKSTATE_H_ */
