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

#ifndef STATCOUNTER_H_
#define STATCOUNTER_H_

#include <cstdint>
#include <iostream>

#include "globaldef.h"
#include "NetworkGraph.h"
#include "SimulationMsgs.h"

class NetworkState;

/**
 * \brief Keeps track of blocking statistics and performance metrics during a simulation run.
 */
class StatCounter {
public:
	StatCounter(const uint64_t discard);
	virtual ~StatCounter();
	void reset(const uint64_t discard);
	void countProvisioning(const Provisioning&p);
	void countTermination(const Provisioning&p);
	void countNetworkState(const NetworkGraph &g, const NetworkState &s, uint64_t timestamp);
	friend std::ostream& operator<<(std::ostream &o, const StatCounter &s);
	static const char* const tableHeader;
	struct PerfMetrics{
		double sharability;
		double priFrag, bkpFrag, totalFrag;
		double priEnd, bkpBegin;
		double collisions;
		double utilization;
		double e_stat;
		double e_dyn;
		linkIndex_t numLinks;
		PerfMetrics();
		void addLink(specIndex_t bkpBegin, double bkpFrag, specIndex_t priEnd,
				double priFrag, double totalFrag);
		void reset();
		PerfMetrics operator *(double b) const;
		PerfMetrics operator /(double b) const;
		PerfMetrics &operator +=(const PerfMetrics &b);
	};
private:
	/**
	 * The number of blocking or provisioning events to discard before counting starts.
	 * This is set by the constructor or the reset() method and counted down when countBwEvent()
	 * is called. The statistics counting only starts when discard is zero.
	 */
	uint64_t discard;

	/// Counters for each supported type of event.
	uint64_t nBlocked;
	uint64_t nProvisioned; ///< Number of connections that were successfully provisioned.
	uint64_t nTerminated; ///< Number of connections that were terminated.
	/**
	 * Bandwidth counters for each supported type of event.
	 * At each event, one of these is increased by the amount of requested bandwidth.
	 */
	uint64_t bwBlocked;
	uint64_t bwProvisioned; ///< Total bandwidth of connections that were successfully provisioned.
	uint64_t bwTerminated; ///< Total bandwidth of connections that were terminated.

	PerfMetrics perf;
	uint64_t simTime, discardedTime;
};

#endif /* STATCOUNTER_H_ */
