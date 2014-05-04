/**
 * @file StatCounter.cpp
 * Network statistics counter class.
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

#include "StatCounter.h"

#include <boost/format.hpp>
#include <algorithm>

#include "globaldef.h"
#include "Simulation.h"

StatCounter::StatCounter(const uint64_t discard) :
	discard(discard),
	nBlockings(),
	nProvisioned(),
	nTerminated(),
	bwBlocked(),
	bwProvisioned(),
	bwTerminated(),
	sharability(),
	fragmentation(),
	specUtil(),
	numLinks(),
	simTime()
{}

StatCounter::~StatCounter() {
}

/**
 * Reset the statistics counter.
 * This sets all counters to zero and makes the counter object discard
 * a number of future events before counting starts again.
 * @param discard Number of events (provisioning and blocking; terminations do not count) to discard
 */
void StatCounter::reset(const uint64_t discard) {
	this->discard=discard;
	std::fill(nBlockings,nBlockings+Provisioning::SUCCESS+1,0ul);
	nProvisioned=0;
	nTerminated=0;
	std::fill(bwBlocked,bwBlocked+Provisioning::SUCCESS+1,0ul);
	bwProvisioned=0;
	bwTerminated=0;
	sharability=0.0;
	fragmentation=0.0;
	specUtil=0;
	simTime=0;
}

/**
 * Count a provisioning/blocking event.
 * This method is called to inform the counter object that a
 * connection has been provisioned or blocked for some reason.
 *
 * @param p The object describing the connection that shall be counted
 */
void StatCounter::countProvisioning(const Provisioning&p) {
	if(discard) {
		--discard;
		return;
	}
	if(p.state==Provisioning::SUCCESS) {
		++nProvisioned;
		bwProvisioned+=p.bandwidth;
	} else {
		++nBlockings[p.state];
		bwBlocked[p.state]+=p.bandwidth;
	}
}

/**
 * Count a termination event.
 * This method is called to inform the counter object that a
 * connection has been terminated.
 *
 * @param p The object describing the connection that shall be counted
 */
void StatCounter::countTermination(const Provisioning&p) {
	if(!discard) {
		++nTerminated;
		bwTerminated+=p.bandwidth;
	}
}

void StatCounter::countNetworkState(const NetworkState& s, uint64_t timestamp) {
	if(discard) return;
	unsigned long int primary=s.getTotalPri();
	unsigned long int anyUse=0;
	numLinks=s.getNumLinks();
	double currentFrag=0.0;
	for(linkIndex_t i=0; i<numLinks; ++i) {
		specIndex_t used=s.getUsedSpectrum(i);
		anyUse+=used;
		if(NUM_SLOTS-used>0)
			currentFrag+=1.0-(double)s.getLargestSegment(i)/(NUM_SLOTS-used);
	}
	uint64_t deltaT=timestamp-simTime;
	fragmentation+=deltaT*currentFrag;
	specUtil+=deltaT*anyUse;
	if(anyUse-primary>0)
		sharability+=deltaT*(double)s.getCurrentBkpBw()/(anyUse-primary);
	simTime=timestamp;
}

/**
 * Output an event type as human-readable text.
 * This provides an iostream output operator so that event types can be
 * conveniently streamed into files or console output.
 */
std::ostream& operator<<(std::ostream &o, const Provisioning::state_t &e) {
	switch(e) {
	case Provisioning::BLOCK_PRI_NOPATH: return o<<"Blocked (no primary path)";
	case Provisioning::BLOCK_PRI_NOSPEC: return o<<"Blocked (no primary spec)";
	case Provisioning::BLOCK_SEC_NOPATH: return o<<"Blocked (no backup path) ";
	case Provisioning::BLOCK_SEC_NOSPEC: return o<<"Blocked (no backup spec) ";
	default: return o;
	}
	return o;
}

/**
 * Output the statistics to a stream.
 */
std::ostream& operator<<(std::ostream &o, const StatCounter &s) {
	unsigned long events_sum=s.nProvisioned;
	unsigned long bandwidth_sum=s.bwProvisioned;
	for(int e=0; e<Provisioning::SUCCESS; ++e) {
		events_sum+=s.nBlockings[e];
		bandwidth_sum+=s.bwBlocked[e];
	}

	//When changing this, remember to change printTableHeader accordingly!
	o		//Blocking probability
			<< (double)(events_sum-s.nProvisioned)/events_sum<<TABLE_COL_SEPARATOR
			//Bandwidth blocking probability
			<< (double)(bandwidth_sum-s.bwProvisioned)/bandwidth_sum<<TABLE_COL_SEPARATOR
			//Sharability
			<< s.sharability / (s.simTime)<<TABLE_COL_SEPARATOR
			//Fragmentation
			<< s.fragmentation/(s.simTime*s.numLinks)<<TABLE_COL_SEPARATOR
			//Spectrum Utilization
			<< (double) s.specUtil/(NUM_SLOTS*s.simTime*s.numLinks)<<TABLE_COL_SEPARATOR
			//Primary-to-total blocking reason ratio
			<< (double)(s.nBlockings[Provisioning::BLOCK_PRI_NOPATH]+s.nBlockings[Provisioning::BLOCK_PRI_NOSPEC])
				/(events_sum-s.nProvisioned);
	/*
	for(int e=0; e<Provisioning::SUCCESS; ++e)
		o <<'#'<<TABLE_COL_SEPARATOR<<'\t' << static_cast<Provisioning::state_t>(e)
		  << boost::format(": %6lu conns (%6.3f%%); %8.1f Gbps (%6.3f%%)") %
		     s.nBlockings[e] % (100.0*(double)s.nBlockings[e]/events_sum) %
		     (s.bwBlocked[e]*SLOT_WIDTH) % (100.0*(double)s.bwBlocked[e]/bandwidth_sum)
		  << std::endl;
	o <<'#'<<TABLE_COL_SEPARATOR<<'\t' << "Provisioned successfully "
	  << boost::format(": %6lu conns (%6.3f%%); %8.1f Tbps (%6.3f%%)") %
		 s.nProvisioned % (100.0*(double)s.nProvisioned/events_sum) %
		 (s.bwProvisioned*SLOT_WIDTH*.001) % (100.0*(double)s.bwProvisioned/bandwidth_sum)
	  << std::endl;
	o <<'#'<<TABLE_COL_SEPARATOR<<'\t' << "Terminated               "
	  << boost::format(": %6lu conns (%6.3f%%); %8.1f Tbps (%6.3f%%)") %
		 s.nTerminated % (100.0*(double)s.nTerminated/events_sum) %
		 (s.bwTerminated*SLOT_WIDTH*.001) % (100.0*(double)s.bwTerminated/bandwidth_sum)
	  << std::endl;
	  */
	return o;
}

const char* const StatCounter::tableHeader=
			"\"Blocking probability\"" TABLE_COL_SEPARATOR
			"\"Bandwidth blocking probability\"" TABLE_COL_SEPARATOR
			"\"Sharability\"" TABLE_COL_SEPARATOR
			"\"Fragmentation\"" TABLE_COL_SEPARATOR
			"\"Spectrum Utilization\"" TABLE_COL_SEPARATOR
			"\"Primary as blocking reason\""
			;

uint64_t StatCounter::getProvisioned() const {
	return nProvisioned;
}
