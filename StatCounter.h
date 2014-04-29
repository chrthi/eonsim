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

#ifndef STATCOUNTER_H_
#define STATCOUNTER_H_

#include <cstdint>
#include <iostream>

#include "globaldef.h"
#include "NetworkState.h"
#include "SimulationMsgs.h"

class StatCounter {
public:
	StatCounter(const unsigned long discard);
	virtual ~StatCounter();
	void reset(const unsigned long discard);
	void countProvisioning(const Provisioning::state_t state, bandwidth_t bandwidth);
	void countTermination(bandwidth_t bandwidth);
	void countNetworkState(const NetworkState &s);
	friend std::ostream& operator<<(std::ostream &o, const StatCounter &s);
private:
	/**
	 * The number of blocking or provisioning events to discard before counting starts.
	 * This is set by the constructor or the reset() method and counted down when countBwEvent()
	 * is called. The statistics counting only starts when discard is zero.
	 */
	unsigned long discard;

	/// Counters for each supported type of event.
	unsigned long nBlockings[Provisioning::SUCCESS];
	unsigned long nProvisioned; ///< Number of connections that were successfully provisioned.
	unsigned long nTerminated; ///< Number of connections that were terminated.
	/**
	 * Bandwidth counters for each supported type of event.
	 * At each event, one of these is increased by the amount of requested bandwidth.
	 */
	unsigned long bwBlocked[Provisioning::SUCCESS];
	unsigned long bwProvisioned; ///< Total bandwidth of connections that were successfully provisioned.
	unsigned long bwTerminated; ///< Total bandwidth of connections that were terminated.

	uint64_t sharability;
	uint64_t fragmentation;
	uint64_t specUtil;
	linkIndex_t numLinks;
	uint64_t stateCounts;
};

#endif /* STATCOUNTER_H_ */
