/**
 * @file KsqHybridCost2Provisioning.cpp
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

#include "KsqHybridCost2Provisioning.h"
#include "ProvisioningSchemeFactory.h"

#define DEFAULT_WEIGHT 1.0

/// by construction, this registers the class in the ProvisioningSchemeFactory factory.
static const ProvisioningSchemeFactory::Registrar<KsqHybridCost2Provisioning> _reg("ksq");
const char *const KsqHybridCost2Provisioning::helpstr=
		"The k-squared hybrid heuristic (separate cost metrics)";
const ProvisioningScheme::paramDesc_t KsqHybridCost2Provisioning::pdesc[]={
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
		{0,0,0,0}
};

KsqHybridCost2Provisioning::KsqHybridCost2Provisioning(
		const ProvisioningScheme::ParameterSet &p
):
		k_pri(DEFAULT_K),
		k_bkp(DEFAULT_K),
#ifdef TEST_METRICS
		n(0),
		mpsum({0}),
		mbsum({0}),
#endif
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

#ifdef TEST_METRICS
	it=p.find("discard");
	if(it!=p.end())	n=0-lrint(it->second);
#endif
}

KsqHybridCost2Provisioning::~KsqHybridCost2Provisioning() {
#ifdef TEST_METRICS
	std::cerr<<"Primary: "
			<< mpsum.m_sep/n <<"; "<< mpsum.m_algn/n <<"; "
			<< mpsum.m_cut/n <<"; "<< mpsum.m_fsb/n <<std::endl;
	std::cerr<<"Backup:  "
			<< mbsum.m_sep/n <<"; "<< mbsum.m_algn/n <<"; "
			<< mbsum.m_cut/n <<"; "<< mbsum.m_fsb/n <<std::endl;
#endif
}

Provisioning KsqHybridCost2Provisioning::operator ()(
		const NetworkGraph& g, const NetworkState& s, const NetworkGraph::DijkstraData &data,
		const Request& r) {
#ifdef TEST_METRICS
	metricvals_t mp={0}, mb={0}, mpopt={0}, mbopt={0};
#endif
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
				double c;
#ifdef TEST_METRICS
				c=costp(g,s,pp,i,i+widthp,mp);
#else
				c=costp(g,s,pp,i,i+widthp);
#endif
				if(c<coptp) {
					ip=i;
					coptp=c;
				}
			}
			if(specp[i]) --usedp;
		}
		if(ip==NUM_SLOTS || coptp>copt) continue;

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
					double c;
#ifdef TEST_METRICS
					c=coptp+costb(g,s,pb,ib,ib+widthb,mb);
#else
					c=coptp+costb(g,s,pb,ib,ib+widthb);
#endif
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
#ifdef TEST_METRICS
						mpopt=mp;
						mbopt=mb;
#endif
					}
				}
				if(specb[ib]) --usedb;
			}
		}
	}
#ifdef TEST_METRICS
	++n;
	if(n>0) {
		mpsum.m_sep += mpopt.m_sep;
		mpsum.m_fsb += mpopt.m_fsb;
		mpsum.m_cut += mpopt.m_cut;
		mpsum.m_algn+= mpopt.m_algn;
		mbsum.m_sep += mbopt.m_sep;
		mbsum.m_fsb += mbopt.m_fsb;
		mbsum.m_cut += mbopt.m_cut;
		mbsum.m_algn+= mbopt.m_algn;
	}
#endif
	return result;
}

std::ostream& KsqHybridCost2Provisioning::print(std::ostream& o) const {
	return printFormatted(o,helpstr,pdesc);
}

#ifdef TEST_METRICS

inline double KsqHybridCost2Provisioning::costp(const NetworkGraph& g,
		const NetworkState& s, const NetworkGraph::Path& pp, specIndex_t beginp,
		specIndex_t endp, metricvals_t &m) const {
	m.m_fsb=pp.size()*(endp-beginp);
	m.m_cut=s.calcCuts(g,pp,beginp,endp);
	m.m_algn=s.calcMisalignments(g,pp,beginp,endp);
	m.m_sep=beginp*pp.size();
	return    c_cut  * m.m_cut
			+ c_algn * m.m_algn
			+          m.m_sep;
	/*
	return	 c_cut *(         s.calcCuts(g,pp,beginp,endp))
			+c_algn*(s.calcMisalignments(g,pp,beginp,endp))
			+       beginp*pp.size();
	*/
}

inline double KsqHybridCost2Provisioning::costb(const NetworkGraph& g,
		const NetworkState& s, const NetworkGraph::Path& pb, specIndex_t beginb,
		specIndex_t endb, metricvals_t &m) const {
	m.m_fsb=s.countFreeBlocks(pb,beginb,endb);
	m.m_cut=s.calcCuts(g,pb,beginb,endb);
	m.m_algn=s.calcMisalignments(g,pb,beginb,endb);
	m.m_sep=(NUM_SLOTS-endb)*pb.size();
	return    c_fsb  * m.m_fsb
			+          m.m_sep;
	/*
	return	 c_fsb *(    s.countFreeBlocks(pb,beginb,endb))
			+       (NUM_SLOTS-endb)*pb.size();
	*/
}

#else

inline double KsqHybridCost2Provisioning::costp(const NetworkGraph& g,
		const NetworkState& s, const NetworkGraph::Path& pp, specIndex_t beginp,
		specIndex_t endp) const {
	return	 c_cut *(         s.calcCuts(g,pp,beginp,endp))
			+c_algn*(s.calcMisalignments(g,pp,beginp,endp))
			+       beginp*pp.size();
}

inline double KsqHybridCost2Provisioning::costb(const NetworkGraph& g,
		const NetworkState& s, const NetworkGraph::Path& pb, specIndex_t beginb,
		specIndex_t endb) const {
	return	 c_fsb *(    s.countFreeBlocks(pb,beginb,endb))
			+       (NUM_SLOTS-endb)*pb.size();
}

#endif
