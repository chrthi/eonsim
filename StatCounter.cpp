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
	nBlocked(),
	nProvisioned(),
	nTerminated(),
	bwBlocked(),
	bwProvisioned(),
	bwTerminated(),
	perf(),
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
	nBlocked=0;
	nProvisioned=0;
	nTerminated=0;
	bwBlocked=0;
	bwProvisioned=0;
	bwTerminated=0;
	perf.reset();
	simTime=0;
	discardedTime=0;
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
		++nBlocked;
		bwBlocked+=p.bandwidth;
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
	uint64_t deltaT=timestamp-simTime;
	PerfMetrics p=s.getCurrentPerfMetrics();
	perf+=p*deltaT;
	/*
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
	fragmentation=fma(deltaT,currentFrag,fragmentation);
	specUtil+=deltaT*anyUse;
	if(anyUse-primary>0)
		sharability=fma(deltaT,(double)s.getCurrentBkpBw()/(anyUse-primary),sharability);

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
	*/
	simTime=timestamp;
}

/**
 * Output the statistics to a stream.
 */
std::ostream& operator<<(std::ostream &o, const StatCounter &s) {

	/*double p_static=
			(s.numLinks/2)*85.0 + s.numNodes*150.0
			+(s.numAmps/2)*140.0;
	*/

	uint64_t t=s.simTime-s.discardedTime;
	StatCounter::PerfMetrics p=s.perf/t;

	//When changing this, remember to change printTableHeader accordingly!
	o		//Blocking probability
			<< (double)(s.nBlocked)/(s.nProvisioned+s.nBlocked)<<TABLE_COL_SEPARATOR
			//Bandwidth blocking probability
			<< (double)(s.bwBlocked)/(s.bwProvisioned+s.bwBlocked)<<TABLE_COL_SEPARATOR
			//Sharability
			<< p.sharability <<TABLE_COL_SEPARATOR
			//Fragmentation
			<< p.priEnd   /(double)p.numLinks <<TABLE_COL_SEPARATOR
			<< (double)NUM_SLOTS-(p.bkpBegin /(double)p.numLinks) <<TABLE_COL_SEPARATOR
			<< p.priFrag  /(double)p.numLinks <<TABLE_COL_SEPARATOR
			<< p.bkpFrag  /(double)p.numLinks <<TABLE_COL_SEPARATOR
			<< p.totalFrag/(double)p.numLinks <<TABLE_COL_SEPARATOR
			//Primary/backup spectrum collisions
			<< p.collisions/(double)p.numLinks <<TABLE_COL_SEPARATOR
			//Spectrum Utilization
			<< p.utilization/(double)(p.numLinks*NUM_SLOTS) <<TABLE_COL_SEPARATOR
			//Static energy
			<< p.e_stat <<TABLE_COL_SEPARATOR
			//Dynamic energy
			<< p.e_dyn;
	return o;
}

const char* const StatCounter::tableHeader=
			"\"BP\"" TABLE_COL_SEPARATOR
			"\"BBP\"" TABLE_COL_SEPARATOR
			"\"Shar\"" TABLE_COL_SEPARATOR
			"\"Pri width\"" TABLE_COL_SEPARATOR
			"\"Bkp width\"" TABLE_COL_SEPARATOR
			"\"Pri frag\"" TABLE_COL_SEPARATOR
			"\"Bkp frag\"" TABLE_COL_SEPARATOR
			"\"Total frag\"" TABLE_COL_SEPARATOR
			"\"Pri/Bkp collisions\"" TABLE_COL_SEPARATOR
			"\"Spec Util\"" TABLE_COL_SEPARATOR
			"\"Static energy\"" TABLE_COL_SEPARATOR
			"\"Dynamic energy\""
			;

StatCounter::PerfMetrics::PerfMetrics():
	sharability(),
	priFrag(),
	bkpFrag(),
	totalFrag(),
	priEnd(),
	bkpBegin(),
	collisions(),
	utilization(),
	e_stat(),
	e_dyn(),
	numLinks()
{}

void StatCounter::PerfMetrics::addLink(specIndex_t bkpBegin, double bkpFrag,
		specIndex_t priEnd, double priFrag, double totalFrag) {
	this->bkpBegin+=bkpBegin;
	this->bkpFrag+=bkpFrag;
	this->priEnd+=priEnd;
	this->priFrag+=priFrag;
	this->totalFrag+=totalFrag;
	if(bkpBegin<=priEnd) collisions+=1.0;
}

void StatCounter::PerfMetrics::reset() {
	sharability=0.0;
	priFrag=0.0;
	bkpFrag=0.0;
	totalFrag=0.0;
	priEnd=0.0;
	bkpBegin=0.0;
	collisions=0.0;
	utilization=0.0;
	e_stat=0.0;
	e_dyn=0.0;
}

StatCounter::PerfMetrics StatCounter::PerfMetrics::operator *(double b) const {
	PerfMetrics p;
	p.sharability=sharability*b;
	p.priFrag=priFrag*b;
	p.bkpFrag=bkpFrag*b;
	p.totalFrag=totalFrag*b;
	p.priEnd=priEnd*b;
	p.bkpBegin=bkpBegin*b;
	p.collisions=collisions*b;
	p.utilization=utilization*b;
	p.e_stat=e_stat;
	p.e_dyn=e_dyn*b;
	p.numLinks=numLinks;
	return p;
}

StatCounter::PerfMetrics StatCounter::PerfMetrics::operator /(double b) const {
	PerfMetrics p;
	p.sharability=sharability/b;
	p.priFrag=priFrag/b;
	p.bkpFrag=bkpFrag/b;
	p.totalFrag=totalFrag/b;
	p.priEnd=priEnd/b;
	p.bkpBegin=bkpBegin/b;
	p.collisions=collisions/b;
	p.utilization=utilization/b;
	p.e_stat=e_stat;
	p.e_dyn=e_dyn/b;
	p.numLinks=numLinks;
	return p;
}

StatCounter::PerfMetrics& StatCounter::PerfMetrics::operator +=(const PerfMetrics& b) {
	sharability+=b.sharability;
	priFrag+=b.priFrag;
	bkpFrag+=b.bkpFrag;
	totalFrag+=b.totalFrag;
	priEnd+=b.priEnd;
	bkpBegin+=b.bkpBegin;
	collisions+=b.collisions;
	utilization+=b.utilization;
	e_stat=b.e_stat;
	e_dyn+=b.e_dyn;
	numLinks=b.numLinks;
	return *this;
}
