/**
 * @file Tarhan2013PFMBLProvisioning.cpp
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

#include "Tarhan2013PFMBLProvisioning.h"

#include "../SimulationMsgs.h"
#include "ProvisioningSchemeFactory.h"

/// by construction, this registers the class in the ProvisioningSchemeFactory factory.
static const ProvisioningSchemeFactory::Registrar<Tarhan2013PFMBLProvisioning> _reg("pfmbl");

Tarhan2013PFMBLProvisioning::Tarhan2013PFMBLProvisioning(const ProvisioningScheme::ParameterSet &p):
		k_pri(DEFAULT_K),
		k_bkp(DEFAULT_K),
		c1(880)
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
	it=p.find("c1");
	if(it!=p.end())
		c1=lrint(it->second*1000);

}

Tarhan2013PFMBLProvisioning::~Tarhan2013PFMBLProvisioning() {
}

Provisioning Tarhan2013PFMBLProvisioning::operator ()(const NetworkGraph& g,
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
	const std::vector<NetworkGraph::Path> &bkpPaths=y.getPaths(k_bkp);
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
		for(specIndex_t i=NUM_SLOTS-1; ; --i) {
			if(spec[i]) {
				count=0;
			} else if(++count>=neededSpec) {
				unsigned int cost=c1?(NUM_SLOTS-i)*c1+neededSpec*1000u:(NUM_SLOTS-i);
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

std::ostream& Tarhan2013PFMBLProvisioning::print(std::ostream& o) const {
	if(c1)
		return o<<"PF-MBL_1"<<'('<<k_pri<<','<<k_bkp<<", "<<c1*.001<<')';
	else
		return o<<"PF-MBL_0"<<'('<<k_pri<<','<<k_bkp<<')';
}

ProvisioningScheme* Tarhan2013PFMBLProvisioning::clone() {
	return new Tarhan2013PFMBLProvisioning(*this);
}
