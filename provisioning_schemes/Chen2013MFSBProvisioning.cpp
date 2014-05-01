/**
 * @file Chen2013MFSBProvisioning.cpp
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

#include "Chen2013MFSBProvisioning.h"

#include "../SimulationMsgs.h"

Chen2013MFSBProvisioning::Chen2013MFSBProvisioning(unsigned int k):
		k(k)
{
}

Chen2013MFSBProvisioning::~Chen2013MFSBProvisioning() {
}

Provisioning Chen2013MFSBProvisioning::operator ()(const NetworkGraph& g,
		const NetworkState& s, const NetworkGraph::DijkstraData &data, const Request& r) {
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

	specIndex_t bestFSB=std::numeric_limits<specIndex_t>::max();
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
		specIndex_t fsb=0;
		for(specIndex_t i=0; i<NUM_SLOTS; ++i) {
			if(spec[i]) {
				count=0;
				fsb=0;
			} else {
				fsb+=s.countFreeBlocks(p,i);
				++count;
				if(count> neededSpec) fsb-=s.countFreeBlocks(p,i-neededSpec);
				if(count>=neededSpec) {
					if(fsb<bestFSB) {
						bestFSB=fsb;
						bestPath=&p;
						result.bkpSpecBegin=i-neededSpec+1;
						result.bkpSpecEnd=i+1;
						result.bkpMod=mod;
					}
					break;
				}
			}
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

std::ostream& Chen2013MFSBProvisioning::print(std::ostream& o) const {
	return o<<"MFSB("<<k<<')';
}

ProvisioningScheme* Chen2013MFSBProvisioning::clone() {
	return new Chen2013MFSBProvisioning(*this);
}
