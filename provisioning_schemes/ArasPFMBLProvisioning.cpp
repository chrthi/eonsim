/**
 * @file ArasPFMBLProvisioning.cpp
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

#include "ArasPFMBLProvisioning.h"

#include "../SimulationMsgs.h"

ArasPFMBLProvisioning::ArasPFMBLProvisioning(const NetworkGraph& g,
		unsigned int k, unsigned int c1):
		data(g),
		k(k),
		c1(c1)
{
}

ArasPFMBLProvisioning::~ArasPFMBLProvisioning() {
	// TODO Auto-generated destructor stub
}

Provisioning ArasPFMBLProvisioning::operator ()(const NetworkGraph& g,
		const NetworkState& s, const Request& r) {
	Provisioning result;
	result.bandwidth=r.bandwidth;
	result.priSpecEnd=0;
	result.bkpSpecEnd=0;

	NetworkGraph::YenKShortestSearch y(g,r.source,r.dest,data);
	{
		const std::vector<NetworkGraph::Path> &priPaths=y.getPaths(k);
		if(priPaths.empty()) {
			result.state=Provisioning::BLOCK_PRI_NOPATH;
			return result;
		}
		for(auto const &p:priPaths) {
			distance_t len=0;
			for(auto const &e:p) len+=data.weights[e.idx];

			result.priMod=calcModulation(len);
			if(result.priMod==MOD_NONE) {
				result.state=Provisioning::BLOCK_PRI_NOPATH;
				return result;
			}
			specIndex_t neededSpec=calcNumSlots(r.bandwidth,result.priMod);

			const NetworkState::spectrum_bits spec=s.priAvailability(p);

			specIndex_t count=0;
			for(specIndex_t i=0; i<NUM_SLOTS; ++i) {
				if(spec[i]) count=0;
				else if(++count==neededSpec) {
					result.priSpecBegin=i-count+1;
					result.priSpecEnd=i+1;
					break;
				}
			}
			if(result.priSpecEnd) {
				result.priPath=p;
				break;
			}
		}
	}
	if(!result.priSpecEnd) {
		result.state=Provisioning::BLOCK_PRI_NOSPEC;
		return result;
	}

	y.reset();

	unsigned int bestCost=std::numeric_limits<unsigned int>::max();
	const NetworkGraph::Path *bestPath=0;
	for(auto const &e:result.priPath) data.weights[e.idx]=std::numeric_limits<distance_t>::max();
	const std::vector<NetworkGraph::Path> &bkpPaths=y.getPaths(k);
	for(auto const &e:result.priPath) data.weights[e.idx]=g.link_lengths[e.idx];
	if(bkpPaths.empty()) {
		result.state=Provisioning::BLOCK_SEC_NOPATH;
		return result;
	}
	for(auto const &p:bkpPaths) {
		distance_t len=0;
		for(auto const &e:p) len+=data.weights[e.idx];

		modulation_t mod=calcModulation(len);
		if(mod==MOD_NONE) break;
		specIndex_t neededSpec=calcNumSlots(r.bandwidth,mod);

		const NetworkState::spectrum_bits spec=s.bkpAvailability(result.priPath,p);

		specIndex_t count=0;
		for(specIndex_t i=NUM_SLOTS; ; --i) {
			if(spec[i]) {
				count=0;
			} else if(++count>=neededSpec) {
				unsigned int cost=(NUM_SLOTS-i)*c1+neededSpec*1000u;
				if(cost<bestCost) {
					bestCost=cost;
					bestPath=&p;
					result.bkpSpecBegin=i;
					result.bkpSpecEnd=i+neededSpec;
					result.bkpMod=mod;
				}
				break;
			}
			if(!i) break;
		}
	}
	if(bestPath) {
		result.bkpPath=*bestPath;
		result.state=Provisioning::SUCCESS;
	} else {
		result.state=Provisioning::BLOCK_SEC_NOSPEC;
	}
	return result;
}
