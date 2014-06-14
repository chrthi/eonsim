/**
 * @file KsqHybridCostProvisioning.cpp
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

#include "KsqHybridCostProvisioning.h"
#include "ProvisioningSchemeFactory.h"

#define DEFAULT_WEIGHT 1.0

/// by construction, this registers the class in the ProvisioningSchemeFactory factory.
static const ProvisioningSchemeFactory::Registrar<KsqHybridCostProvisioning> _reg("ksq");
const char *const KsqHybridCostProvisioning::helpstr=
		"The k-squared hybrid heuristic";
const ProvisioningScheme::paramDesc_t KsqHybridCostProvisioning::pdesc[]={
		{"k",      "0<k",      XSTR(DEFAULT_K),
				"Default value for k_pri and k_bkp"},
		{"k_pri",  "0<k_pri",  "k",
				"Number of primary paths to consider"},
		{"k_bkp",  "0<k_pri",  "k",
				"Number of backup paths to consider per primary"},
		{"c_cut",  "0<=c_cut", XSTR(DEFAULT_WEIGHT),
				"Weight of the \"Cut\" metric"},
		{"c_algn", "0<=c_algn", XSTR(DEFAULT_WEIGHT),
				"Weight of the \"Misalignment\" metric"},
		{"c_fsb",  "0<=c_fsb", XSTR(DEFAULT_WEIGHT),
				"Weight of the \"free spectrum block\" metric"},
		{"mode",   "{1,2,3}",    "3",
				"Consider the hybrid metric for (1) Primary, (2) Backup, (3) both"},
		{0,0,0,0}
};

KsqHybridCostProvisioning::KsqHybridCostProvisioning(
		const ProvisioningScheme::ParameterSet &p
):
		k_pri(DEFAULT_K),
		k_bkp(DEFAULT_K),
		c_cut(DEFAULT_WEIGHT),
		c_algn(DEFAULT_WEIGHT),
		c_fsb(DEFAULT_WEIGHT)
{
	auto it=p.find("k");
	if(it!=p.end())	k_pri=k_bkp=lrint(it->second);

	it=p.find("k_pri");
	if(it!=p.end())	k_pri=lrint(it->second);

	it=p.find("k_bkp");
	if(it!=p.end())	k_bkp=lrint(it->second);

	it=p.find("c_cut");
	if(it!=p.end())	c_cut=it->second;

	it=p.find("c_algn");
	if(it!=p.end())	c_algn=it->second;

	it=p.find("c_fsb");
	if(it!=p.end())	c_fsb=it->second;
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
	return printFormatted(o,helpstr,pdesc);
}

inline double KsqHybridCostProvisioning::costp(const NetworkGraph& g,
		const NetworkState& s, const NetworkGraph::Path& pp, specIndex_t beginp,
		specIndex_t endp) const {
	return	 c_fsb *(  pp.size()*(endp-beginp))
			+c_cut *(         s.calcCuts(g,pp,beginp,endp))
			+c_algn*(s.calcMisalignments(g,pp,beginp,endp))
			+       beginp*pp.size();
}

inline double KsqHybridCostProvisioning::costb(const NetworkGraph& g,
		const NetworkState& s, const NetworkGraph::Path& pb, specIndex_t beginb,
		specIndex_t endb) const {
	return	 c_fsb *(    s.countFreeBlocks(pb,beginb,endb))
			+c_cut *(         s.calcCuts(g,pb,beginb,endb))
			+c_algn*(s.calcMisalignments(g,pb,beginb,endb))
			+       (NUM_SLOTS-endb)*pb.size();
}
