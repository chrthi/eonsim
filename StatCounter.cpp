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

StatCounter::StatCounter(const unsigned long discard) :
	discard(discard),
	nBlockings(),
	nProvisioned(),
	nTerminated(),
	bwBlocked(),
	bwProvisioned(),
	bwTerminated()
{}

StatCounter::~StatCounter() {
}

/**
 * Reset the statistics counter.
 * This sets all counters to zero and makes the counter object discard
 * a number of future events before counting starts again.
 * @param discard Number of events (provisioning and blocking; terminations do not count) to discard
 */
void StatCounter::reset(const unsigned long discard) {
	this->discard=discard;
	std::fill(nBlockings,nBlockings+EVENT_MAX+1,0UL);
	nProvisioned=0;
	nTerminated=0;
	std::fill(bwBlocked,bwBlocked+EVENT_MAX+1,0.0);
	bwProvisioned=0;
	bwTerminated=0;
}

/**
 * Count a provisioning/blocking/termination event.
 * This method is called to inform the counter object that a
 * connection has been blocked for some reason, provisioned or terminated.
 *
 * @param reason The type of event that shall be counted
 * @param bandwidth The requested amount of bandwidth of this connection
 */
void StatCounter::countBlocking(const blockreason_t reason, unsigned long bandwidth) {
	if(reason>EVENT_MAX) return;
	if(discard) {
		--discard;
	} else {
		++nBlockings[reason];
		this->bwBlocked[reason]+=bandwidth;
	}
}

void StatCounter::countProvisioning(unsigned long bandwidth) {
	if(discard) {
		--discard;
	} else {
		++nProvisioned;
		bwProvisioned+=bandwidth;
	}
}

void StatCounter::countTermination(unsigned long bandwidth) {
	if(!discard) {
		++nTerminated;
		bwTerminated+=bandwidth;
	}
}

/**
 * Output an event type as human-readable text.
 * This provides an iostream output operator so that event types can be
 * conveniently streamed into files or console output.
 */
std::ostream& operator<<(std::ostream &o, const StatCounter::blockreason_t &e) {
	switch(e) {
	case StatCounter::BLOCK_PRI_NOPATH: return o<<"Blocked (no primary path)";
	case StatCounter::BLOCK_PRI_NOSPEC: return o<<"Blocked (no primary spec)";
	case StatCounter::BLOCK_SEC_NOPATH: return o<<"Blocked (no backup path) ";
	case StatCounter::BLOCK_SEC_NOSPEC: return o<<"Blocked (no backup spec) ";
	}
	return o;
}

/**
 * Output the statistics to a stream.
 * This writes all counters (and percentage values calculated from them) to a stream.
 * This can be used
 */
std::ostream& operator<<(std::ostream &o, const StatCounter &s) {
	unsigned long events_sum=s.nProvisioned;
	unsigned long bandwidth_sum=s.bwProvisioned;
	for(int e=StatCounter::EVENT_MIN; e<StatCounter::EVENT_MAX; ++e) {
		events_sum+=s.nBlockings[e];
		bandwidth_sum+=s.bwBlocked[e];
	}
	for(int e=StatCounter::EVENT_MIN; e<=StatCounter::EVENT_MAX; ++e)
		o << static_cast<StatCounter::blockreason_t>(e)
		  << boost::format(": %4lu conns (%6.4f%%); %8.1f Gbps (%6.4f%%)") %
		     s.nBlockings[e] % ((double)s.nBlockings[e]/events_sum) %
		     (s.bwBlocked[e]*BANDWIDTH_UNIT) % ((double)s.bwBlocked[e]/bandwidth_sum)
		  << std::endl;
	o << "Provisioned successfully "
	  << boost::format(": %4lu conns (%6.4f%%); %8.1f Gbps (%6.4f%%)") %
		 s.nProvisioned % ((double)s.nProvisioned/events_sum) %
		 (s.bwProvisioned*BANDWIDTH_UNIT) % ((double)s.bwProvisioned/bandwidth_sum)
	  << std::endl;
	o << "Terminated               "
	  << boost::format(": %4lu conns (%6.4f%%); %8.1f Gbps (%6.4f%%)") %
		 s.nTerminated % ((double)s.nTerminated/events_sum) %
		 (s.bwTerminated*BANDWIDTH_UNIT) % ((double)s.bwTerminated/bandwidth_sum)
	  << std::endl;
	return o;
}
