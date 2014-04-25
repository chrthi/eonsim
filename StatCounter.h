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

#include <iostream>

class StatCounter {
public:
	/**
	 * All different reasons for blocking a connection request that the statistics counter can count.
	 */
	enum blockreason_t{
		/// No feasible primary path could be found.
		BLOCK_PRI_NOPATH,
		/// No spectrum was available on any primary path candidate.
		BLOCK_PRI_NOSPEC,
		/// No feasible backup path could be found.
		BLOCK_SEC_NOPATH,
		/// No spectrum was available on any backup path candidate.
		BLOCK_SEC_NOSPEC,
		/// This is a dummy value to ease iterating over all possible types.
		EVENT_MIN=BLOCK_PRI_NOPATH,
		/// This is a dummy value to ease iterating over all possible types and declaring arrays.
		EVENT_MAX=BLOCK_SEC_NOSPEC
	};
	StatCounter(const unsigned long discard);
	virtual ~StatCounter();
	void reset(const unsigned long discard);
	void countBlocking(const blockreason_t reason, unsigned long bandwidth);
	void countProvisioning(unsigned long bandwidth);
	void countTermination(unsigned long bandwidth);
	friend std::ostream& operator<<(std::ostream &o, const StatCounter &s);
private:
	/**
	 * The number of blocking or provisioning events to discard before counting starts.
	 * This is set by the constructor or the reset() method and counted down when countBwEvent()
	 * is called. The statistics counting only starts when discard is zero.
	 */
	unsigned long discard;

	/// Counters for each supported type of event.
	unsigned long nBlockings[EVENT_MAX+1];
	unsigned long nProvisioned; ///< Number of connections that were successfully provisioned.
	unsigned long nTerminated; ///< Number of connections that were terminated.
	/**
	 * Bandwidth counters for each supported type of event.
	 * At each event, one of these is increased by the amount of requested bandwidth.
	 */
	unsigned long bwBlocked[EVENT_MAX+1];
	unsigned long bwProvisioned; ///< Total bandwidth of connections that were successfully provisioned.
	unsigned long bwTerminated; ///< Total bandwidth of connections that were terminated.
};

#endif /* STATCOUNTER_H_ */
