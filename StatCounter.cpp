/**
 * @file StatCounter.cpp
 * Network statistics counter class.
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
	energy(),
	specUtil(),
	numLinks(),
	numNodes(),
	numAmps(),
	simTime(),
	discardedTime()
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
	energy=0.0;
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

void StatCounter::countNetworkState(const NetworkGraph &g, const NetworkState& s, uint64_t timestamp) {
	if(discard) {
		simTime=timestamp;
		return;
	}
	if(!discardedTime)
		discardedTime=timestamp;
	unsigned long int primary=s.getTotalPri();
	unsigned long int anyUse=0;
	if(!numLinks) {
		numLinks=boost::num_edges(g.g);
		numNodes=boost::num_vertices(g.g);
		numAmps=g.getNumAmps();
	}
	double currentFrag=0.0;
	unsigned long idleAmps=0;
	for(linkIndex_t i=0; i<numLinks; ++i) {
		specIndex_t used=s.getUsedSpectrum(i);
		anyUse+=used;
		if(!used) idleAmps+=lrint(ceil(g.link_lengths[i]*DISTANCE_UNIT/AMP_DIST))+1;
		if(NUM_SLOTS-used>0)
			currentFrag+=1.0-(double)s.getLargestSegment(i)/(NUM_SLOTS-used);
	}
	uint64_t deltaT=timestamp-simTime;
	fragmentation=fma(deltaT,currentFrag,fragmentation);
	specUtil+=deltaT*anyUse;
	if(anyUse-primary>0)
		sharability=fma(deltaT,(double)s.getCurrentBkpBw()/(anyUse-primary),sharability);
	simTime=timestamp;
	const uint64_t *txslots=s.getCurrentTxSlots();
	double eTx=     txslots[BPSK] * 47.13;
	eTx=fma((double)txslots[QPSK] , 62.75,eTx);
	eTx=fma((double)txslots[QAM8] , 78.38,eTx);
	eTx=fma((double)txslots[QAM16], 94.00,eTx);
	eTx=fma((double)txslots[QAM32],109.63,eTx);
	eTx=fma((double)txslots[QAM64],125.23,eTx);
	energy=fma(
			deltaT,
			eTx+(g.getNumAmps()-idleAmps)*30.0,
			energy);
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

	double p_static=
			(s.numLinks/2)*85.0 + s.numNodes*150.0
			+(s.numAmps/2)*140.0;

	uint64_t t=s.simTime-s.discardedTime;

	//When changing this, remember to change printTableHeader accordingly!
	o		//Blocking probability
			<< (double)(events_sum-s.nProvisioned)/events_sum<<TABLE_COL_SEPARATOR
			//Bandwidth blocking probability
			<< (double)(bandwidth_sum-s.bwProvisioned)/bandwidth_sum<<TABLE_COL_SEPARATOR
			//Sharability
			<< s.sharability/t <<TABLE_COL_SEPARATOR
			//Fragmentation
			<< s.fragmentation/(t*s.numLinks)<<TABLE_COL_SEPARATOR
			//Spectrum Utilization
			<< (double) s.specUtil/(NUM_SLOTS*t*s.numLinks)<<TABLE_COL_SEPARATOR
			//Primary-to-total blocking reason ratio
			<< (double)(s.nBlockings[Provisioning::BLOCK_PRI_NOPATH]+s.nBlockings[Provisioning::BLOCK_PRI_NOSPEC])
				/(events_sum-s.nProvisioned)<<TABLE_COL_SEPARATOR
			<< p_static <<TABLE_COL_SEPARATOR
			<< s.energy/t;
	return o;
}

const char* const StatCounter::tableHeader=
			"\"Blocking probability\"" TABLE_COL_SEPARATOR
			"\"Bandwidth blocking probability\"" TABLE_COL_SEPARATOR
			"\"Sharability\"" TABLE_COL_SEPARATOR
			"\"Fragmentation\"" TABLE_COL_SEPARATOR
			"\"Spectrum Utilization\"" TABLE_COL_SEPARATOR
			"\"Primary as blocking reason\"" TABLE_COL_SEPARATOR
			"\"Static energy\"" TABLE_COL_SEPARATOR
			"\"Dynamic energy\""
			;

uint64_t StatCounter::getProvisioned() const {
	return nProvisioned;
}
