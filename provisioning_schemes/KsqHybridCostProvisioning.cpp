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
#include "ProvisioningSchemeFactory.h"

/// by construction, this registers the class in the ProvisioningSchemeFactory factory.
static const ProvisioningSchemeFactory::Registrar<KsqHybridCostProvisioning> _reg("ksq");

KsqHybridCostProvisioning::KsqHybridCostProvisioning(
		const ProvisioningScheme::ParameterSet &p
):
		k_pri(DEFAULT_K),
		k_bkp(DEFAULT_K),
		c_cut(1.0),
		c_algn(1.0),
		c_sep(1.0)
{
	auto it=p.find("k");
	if(it!=p.end())	k_pri=k_bkp=lrint(it->second);

	it=p.find("k_pri");
	if(it!=p.end())	k_pri=lrint(it->second);

	it=p.find("k_bkp");
	if(it!=p.end())	k_bkp=lrint(it->second);

	it=p.find("c_cut");
	if(it!=p.end())	c_cut=it->second<-2.0?0:pow(2.0,it->second);

	it=p.find("c_algn");
	if(it!=p.end())	c_algn=it->second<-2.0?0:pow(2.0,it->second);

	it=p.find("c_sep");
	if(it!=p.end())	c_sep=it->second<-3.0?0:pow(2.0,it->second);
}

KsqHybridCostProvisioning::~KsqHybridCostProvisioning() {
}

Provisioning KsqHybridCostProvisioning::operator ()(
		const NetworkGraph& g, const NetworkState& s, const NetworkGraph::DijkstraData &data,
		const Request& r) {
	Provisioning result;
	result.bandwidth=r.bandwidth;
	result.state=Provisioning::BLOCK_PRI_NOPATH;
	double copt=std::numeric_limits<double>::infinity();

	NetworkGraph::YenKShortestSearch y(g,r.source,r.dest,data);
	const std::vector<NetworkGraph::Path> priPaths=y.getPaths(k_pri);
	for(auto const &pp:priPaths) {
		distance_t lenp=0;
		for(auto const &e:pp) lenp+=data.weights[e.idx];
		const modulation_t modp=calcModulation(lenp);
		const specIndex_t widthp=calcNumSlots(r.bandwidth,modp);
		const NetworkState::spectrum_bits specp=s.priAvailability(pp);

		double coptp=std::numeric_limits<double>::infinity();
		specIndex_t usedp=0;
		specIndex_t ip=NUM_SLOTS;
		for(specIndex_t i=0; i<widthp-1; ++i)
			if(specp[i]) ++usedp;
		for(specIndex_t i=0; i<=NUM_SLOTS-widthp; ++i) {
			if(specp[i+widthp-1]) ++usedp;
			if(!usedp) {
				double c=costp(g,s,pp,i,i+widthp);
				if(c<coptp) {
					ip=i;
					coptp=c;
				}
			}
			if(specp[i]) --usedp;
		}
		if(ip==NUM_SLOTS) continue;

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

			specIndex_t usedb=0;
			for(specIndex_t ib=0; ib<widthb-1; ++ib)
				if(specb[ib]) ++usedb;
			for(specIndex_t ib=0; ib<=NUM_SLOTS-widthb; ++ib) {
				if(specb[ib+widthb-1]) ++usedb;
				if(!usedb) {
					double c=coptp+costb(g,s,pb,ib,ib+widthb);
					if(c<copt) {
						result.state=Provisioning::SUCCESS;
						result.priPath=pp;
						result.priSpecBegin=ip;
						result.priSpecEnd=ip+widthp;
						result.priMod=modp;
						result.bkpPath=pb;
						result.bkpSpecBegin=ib;
						result.bkpSpecEnd=ib+widthb;
						result.bkpMod=modb;
						copt=c;
						//return result;
					}
				}
				if(specb[ib]) --usedb;
			}
		}
	}
	return result;
}

std::ostream& KsqHybridCostProvisioning::print(std::ostream& o) const {
	return o<<"K-SQ("<<k_pri<<", "<<k_bkp<<')'
			<<TABLE_COL_SEPARATOR<<c_cut
			<<TABLE_COL_SEPARATOR<<c_algn
			<<TABLE_COL_SEPARATOR<<c_sep;
}

ProvisioningScheme* KsqHybridCostProvisioning::clone() {
	return new KsqHybridCostProvisioning(*this);
}

inline double KsqHybridCostProvisioning::costp(const NetworkGraph& g,
		const NetworkState& s, const NetworkGraph::Path& pp, specIndex_t beginp,
		specIndex_t endp) const {
	/* Ideas:
	 *  1 * Number of free blocks that will be used. [O(hops*slots)]
	 * a1 * Number of spectrum "cuts" (count only if free slots (anyUse=0) on left and right) [O(hops); 0,0.2,0.5,1,5,10]
	 * a2 * Number of misalignments: free slots (anyUse) on adjacent links/nodedeg [O(hops*slots); 0.2,0.4,0.8,1.6,3.2,6.4]
	 * a3 * Distance from left/right * hops [0:0.1:0.5]
	 * 0  * hopcount-slots product [O(hops*slots)]
	 *
	 * a1, a2 in {0.00, 0.25, 0.50, 1.00, 2.00, 4.00};
	 * a3 in {0.0625, 0.125, 0.250, 0.500, 1.000};
	 */
	return	(double)(  pp.size()*(endp-beginp))
			+c_cut *(         s.calcCuts(g,pp,beginp,endp))
			+c_algn*(s.calcMisalignments(g,pp,beginp,endp))
			+c_sep *beginp*pp.size();
}

inline double KsqHybridCostProvisioning::costb(const NetworkGraph& g,
		const NetworkState& s, const NetworkGraph::Path& pb, specIndex_t beginb,
		specIndex_t endb) const {
	return	(double)(    s.countFreeBlocks(pb,beginb,endb))
			+c_cut *(         s.calcCuts(g,pb,beginb,endb))
			+c_algn*(s.calcMisalignments(g,pb,beginb,endb))
			+c_sep *(NUM_SLOTS-endb)*pb.size();
}
