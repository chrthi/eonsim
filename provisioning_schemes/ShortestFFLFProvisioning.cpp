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
#include <bitset>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <string>
#include <vector>

#include "../globaldef.h"
#include "../modulation.h"
#include "../SimulationMsgs.h"

//#include "../NetworkState.h"

using namespace boost;

ShortestFFLFProvisioning::ShortestFFLFProvisioning() {
}

ShortestFFLFProvisioning::~ShortestFFLFProvisioning() {
}

Provisioning ShortestFFLFProvisioning::operator ()(const NetworkGraph& g,
		const NetworkState& s, const NetworkGraph::DijkstraData &data, const Request& r) {
	Provisioning result;
	result.bandwidth=r.bandwidth;

	//reset link weights to distances
	data.resetWeights();

	//compute primary path
	result.priPath=g.dijkstra(r.source,r.dest,data);
	result.priMod=calcModulation(data.dists[r.dest]);
	if(result.priPath.empty() || result.priMod==MOD_NONE) {
		result.state=Provisioning::BLOCK_PRI_NOPATH;
		return result;
	}

	//calculate needed spectrum
	specIndex_t neededSpec=calcNumSlots(r.bandwidth,result.priMod);

	//get path spectrum
	NetworkState::spectrum_bits spec=s.priAvailability(result.priPath);

	//first-fit
	specIndex_t count=0;
	result.priSpecEnd=0;
	for(specIndex_t i=0; i<NUM_SLOTS; ++i) {
		if(spec[i]) count=0;
		else if(++count==neededSpec) {
			result.priSpecBegin=i-count+1;
			result.priSpecEnd=i+1;
			break;
		}
	}
	if(!result.priSpecEnd) {
		result.state=Provisioning::BLOCK_PRI_NOSPEC;
		return result;
	}

	//prune primary graph by setting weights to infinite
	for(std::vector<NetworkGraph::Graph::edge_descriptor>::iterator it=result.priPath.begin();
			it!=result.priPath.end(); ++it)
		data.weights[it->idx]=std::numeric_limits<distance_t>::max();

	//compute backup path
	result.bkpPath=g.dijkstra(r.source,r.dest,data);
	result.bkpMod=calcModulation(data.dists[r.dest]);
	if(result.bkpPath.empty() || result.bkpMod==MOD_NONE) {
		result.state=Provisioning::BLOCK_SEC_NOPATH;
		return result;
	}

	//calculate needed spectrum
	neededSpec=calcNumSlots(r.bandwidth,result.bkpMod);

	//get path spectrum
	spec=s.bkpAvailability(result.priPath,result.bkpPath);


	//last-fit
	count=0;
	//result.bkpSpecBegin=0;
	result.bkpSpecEnd=0;
	for(specIndex_t i=NUM_SLOTS-1; ; --i) {
		if(spec[i]) count=0;
		else if(++count==neededSpec) {
			result.bkpSpecBegin=i;
			result.bkpSpecEnd=i+count;
			break;
		}
		if(!i) break;
	}

	if(!result.bkpSpecEnd) {
		result.state=Provisioning::BLOCK_SEC_NOSPEC;
		return result;
	}

	result.state=Provisioning::SUCCESS;

	return result;
}

std::ostream& ShortestFFLFProvisioning::print(std::ostream& o) const {
	return o<<"ShFFLF";
}

ProvisioningScheme* ShortestFFLFProvisioning::clone() {
	return new ShortestFFLFProvisioning(*this);
}
