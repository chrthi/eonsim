/**
 * @file Shao2012FFProvisioning.cpp
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

#include "Shao2012FFProvisioning.h"

#include <boost/graph/detail/compressed_sparse_row_struct.hpp>
#include <vector>

#include "../globaldef.h"
#include "../modulation.h"
#include "../SimulationMsgs.h"
#include "ProvisioningSchemeFactory.h"

/// by construction, this registers the class in the ProvisioningSchemeFactory factory.
static const ProvisioningSchemeFactory::Registrar<Shao2012FFProvisioning> _reg("ff");

Shao2012FFProvisioning::Shao2012FFProvisioning(const ProvisioningScheme::ParameterSet &p):
	k_pri(DEFAULT_K),
	k_bkp(DEFAULT_K)
{
	auto it=p.find("k");
	if(it!=p.end())
		k_pri=k_bkp=lrint(it->second);
	it=p.find("k_pri");
	if(it!=p.end())
		k_pri=lrint(it->second);
	it=p.find("k_bkp");
	if(it!=p.end())
		k_bkp=lrint(it->second);
}

Shao2012FFProvisioning::~Shao2012FFProvisioning() {
}

Provisioning Shao2012FFProvisioning::operator ()(const NetworkGraph& g,
		const NetworkState& s, const NetworkGraph::DijkstraData &data, const Request& r) {
	Provisioning result;
	result.bandwidth=r.bandwidth;
	result.priSpecEnd=0;
	result.bkpSpecEnd=0;

	NetworkGraph::YenKShortestSearch y(g,r.source,r.dest,data);
	{
		const std::vector<NetworkGraph::Path> &priPaths=y.getPaths(k_pri);
		if(priPaths.empty()) {
			result.state=Provisioning::BLOCK_PRI_NOPATH;
			return result;
		}
		for(auto const &p:priPaths) {
			distance_t len=0;
			for(auto const &e:p) len+=data.weights[e.idx];

			result.priMod=calcModulation(len);
			if(result.priMod==MOD_NONE) { //path is too long - and thus all following nth-shortest paths as well.
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

	{
		for(auto const &e:result.priPath) data.weights[e.idx]=std::numeric_limits<distance_t>::max();
		const std::vector<NetworkGraph::Path> &bkpPaths=y.getPaths(k_bkp);
		for(auto const &e:result.priPath) data.weights[e.idx]=g.link_lengths[e.idx];
		if(bkpPaths.empty()) {
			result.state=Provisioning::BLOCK_SEC_NOPATH;
			return result;
		}
		for(auto const &p:bkpPaths) {
			distance_t len=0;
			for(auto const &e:p) len+=data.weights[e.idx];

			result.bkpMod=calcModulation(len);
			if(result.bkpMod==MOD_NONE) { //path is too long - and thus all following nth-shortest paths as well.
				result.state=Provisioning::BLOCK_SEC_NOPATH;
				return result;
			}
			specIndex_t neededSpec=calcNumSlots(r.bandwidth,result.bkpMod);

			const NetworkState::spectrum_bits spec=s.bkpAvailability(result.priPath,p);

			specIndex_t count=0;
			for(specIndex_t i=0; i<NUM_SLOTS; ++i) {
				if(spec[i]) count=0;
				else if(++count==neededSpec) {
					result.bkpSpecBegin=i-count+1;
					result.bkpSpecEnd=i+1;
					break;
				}
			}
			if(result.bkpSpecEnd) {
				result.bkpPath=p;
				break;
			}
		}
	}
	if(!result.bkpSpecEnd) {
		result.state=Provisioning::BLOCK_SEC_NOSPEC;
		return result;
	}

	result.state=Provisioning::SUCCESS;
	return result;
}

std::ostream& Shao2012FFProvisioning::print(std::ostream& o) const {
	return o<<"FF("<<k_pri<<','<<k_bkp<<')';
}

ProvisioningScheme* Shao2012FFProvisioning::clone() {
	return new Shao2012FFProvisioning(*this);
}
