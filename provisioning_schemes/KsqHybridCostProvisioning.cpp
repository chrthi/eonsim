/**
 * @file KsqHybridCostProvisioning.cpp
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

#include "KsqHybridCostProvisioning.h"

KsqHybridCostProvisioning::KsqHybridCostProvisioning(unsigned int k_pri,
		unsigned int k_bkp):
		k_pri(k_pri),
		k_bkp(k_bkp)
{
}

KsqHybridCostProvisioning::~KsqHybridCostProvisioning() {
}

Provisioning KsqHybridCostProvisioning::operator ()(
		const NetworkGraph& g, const NetworkState& s, const NetworkGraph::DijkstraData &data,
		const Request& r) {
	Provisioning result;
	result.bandwidth=r.bandwidth;
	double copt=INFINITY; ///todo: ?

	NetworkGraph::YenKShortestSearch y(g,r.source,r.dest,data);
	const std::vector<NetworkGraph::Path> priPaths=y.getPaths(k_pri);
	for(auto const &pp:priPaths) {
		distance_t lenp=0;
		for(auto const &e:pp) lenp+=data.weights[e.idx];
		const modulation_t modp=calcModulation(lenp);
		const specIndex_t widthp=calcNumSlots(r.bandwidth,modp);
		const NetworkState::spectrum_bits specp=s.priAvailability(pp);

		y.reset();
		for(auto const &e:pp) data.weights[e.idx]=std::numeric_limits<distance_t>::max();
		const std::vector<NetworkGraph::Path> &bkpPaths=y.getPaths(k_bkp);
		for(auto const &e:pp) data.weights[e.idx]=g.link_lengths[e.idx];
		for(auto const &pb:bkpPaths) {
			distance_t lenb=0;
			for(auto const &e:pb) lenb+=data.weights[e.idx];
			const modulation_t modb=calcModulation(lenb);
			const specIndex_t widthb=calcNumSlots(r.bandwidth,modb);
			const NetworkState::spectrum_bits specb=s.bkpAvailability(pp,pb);

			specIndex_t usedp=0;
			for(specIndex_t ip=0; ip<widthp-1; ++ip)
				if(specp[ip]) ++usedp;
			for(specIndex_t ip=0; ip<=NUM_SLOTS-widthp; ++ip) {
				if(specp[ip+widthp-1]) ++usedp;
				if(!usedp) {
					specIndex_t usedb=0;
					for(specIndex_t ib=0; ib<widthb-1; ++ib)
						if(specb[ib]) ++usedb;
					for(specIndex_t ib=0; ib<=NUM_SLOTS-widthb; ++ib) {
						if(specb[ib+widthb-1]) ++usedb;
						if(!usedb) {
							double c=cost(g,s,pp,ip,ip+widthp,pb,ib,ib+widthb);
							if(c<copt) {
								result.priPath=pp;
								result.priSpecBegin=ip;
								result.priSpecEnd=ip+widthp;
								result.priMod=modp;
								result.bkpPath=pb;
								result.bkpSpecBegin=ib;
								result.bkpSpecEnd=ib+widthb;
								result.bkpMod=modb;
							}
						}
						if(specb[ib]) --usedb;
					}
				}
				if(specp[ip]) --usedp;
			}
		}
	}
	return result;
}

std::ostream& KsqHybridCostProvisioning::print(std::ostream& o) const {
	return o<<"K-SQ("<<k_pri<<", "<<k_bkp<<')';
}

ProvisioningScheme* KsqHybridCostProvisioning::clone() {
	return new KsqHybridCostProvisioning(*this);
}

inline double KsqHybridCostProvisioning::cost(const NetworkGraph& g,
		const NetworkState& s, const NetworkGraph::Path& pp, specIndex_t beginp,
		specIndex_t endp, const NetworkGraph::Path& pb, specIndex_t beginb,
		specIndex_t endb) const {
	return 0.0;
}
