/**
 * @file Simulation.h
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

#ifndef SIMULATION_H_
#define SIMULATION_H_

#include <map>

#include "JobIterator.h"
#include "NetworkGraph.h"
#include "NetworkState.h"
#include "SimulationMsgs.h"

class ProvisioningScheme;
class StatCounter;

/**
 * \brief Implementation of the event-driven simulation main loop.
 *
 * The run member function is called by the worker thread.
 * The Simulation class maintains the random number generator and active
 * connection list. To measure performance metrics, the simulation uses a
 * StatCounter object.
 */
class Simulation {
public:
	Simulation(const NetworkGraph &topology);
	const StatCounter run(const JobIterator::job_t &job);
	~Simulation();
	void reset();
private:
	const NetworkGraph& topology;
	const NetworkGraph::DijkstraData scratchpad;
	NetworkState state;
	std::multimap<unsigned long, Provisioning> activeConnections;
};

#endif /* SIMULATION_H_ */
