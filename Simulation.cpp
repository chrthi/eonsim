/**
 * @file Simulation.cpp
 *
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

#include "Simulation.h"

#include <boost/graph/compressed_sparse_row_graph.hpp>
#include <boost/random/exponential_distribution.hpp>
#include <boost/random/taus88.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <stddef.h>
#include <cmath>
#include <utility>

#include "globaldef.h"
#include "provisioning_schemes/ProvisioningScheme.h"
#include "StatCounter.h"

Simulation::~Simulation() {
}

Simulation::Simulation(const NetworkGraph& topology):
				topology(topology),
				scratchpad(topology),
				state(topology)
{}

const StatCounter Simulation::run(ProvisioningScheme &provision,
		unsigned long itersDiscard, unsigned long itersTotal,
		unsigned int avg_interarrival, unsigned int avg_holding) {
	boost::random::taus88 rng(0);
	boost::random::exponential_distribution<> requestTimeGen(1.0/avg_interarrival);
	boost::random::exponential_distribution<> holdingTimeGen(1.0/avg_holding);
	boost::random::uniform_int_distribution<size_t> sourceGen(0,num_vertices(topology.g)-1);
	boost::random::uniform_int_distribution<size_t> destGen(0,num_vertices(topology.g)-2);
	boost::random::uniform_int_distribution<unsigned int>bandwidthGen(
			ceil(DEFAULT_BW_MIN/SLOT_WIDTH),
			ceil(DEFAULT_BW_MAX/SLOT_WIDTH));
	unsigned long nextRequestTime=0;
	unsigned long currentTime=0;
	reset();
	StatCounter count(itersDiscard);
	for(unsigned long numProvisionings=0; numProvisionings<itersTotal; ++numProvisionings) {
		//terminate all connections that should have terminated by now
		for(std::map<unsigned long, Provisioning>::iterator nextTerm=activeConnections.begin();
				nextTerm!=activeConnections.end() && nextTerm->first<=nextRequestTime;
				nextTerm=activeConnections.begin()) {
			//a termination event is next.

			//advance simulation time to the next instant
			if(nextTerm->first!=currentTime) {
				currentTime=nextTerm->first;
				count.countNetworkState(state,currentTime);
			}

			//update the network state with the removed connection
			state.terminate(nextTerm->second);

			count.countTermination(nextTerm->second);

			//remove the connection from the list
			activeConnections.erase(nextTerm);

		}

		//advance simulation time to the next instant
		if(nextRequestTime!=currentTime) {
			currentTime=nextRequestTime;
			count.countNetworkState(state,currentTime);
		}

		//a request event is next.

		//Generate a random request
		Request r;
		nodeIndex_t sourceIndex=sourceGen(rng);
		r.source=vertex(sourceIndex,topology.g);
		nodeIndex_t destIndex=destGen(rng);
		if(destIndex>=sourceIndex) ++destIndex;
		r.dest=vertex(destIndex,topology.g);
		r.bandwidth=bandwidthGen(rng);

		//Run the provisioning algorithm
		Provisioning p=provision(topology,state,scratchpad,r);

		count.countProvisioning(p);

		if(p.state==Provisioning::SUCCESS) {
			//update the network state with the new connection
			state.provision(p);

			//Add the connection to the active connection list with an expiry time
			activeConnections.insert(std::pair<const unsigned long, Provisioning>(
					currentTime+lrint(holdingTimeGen(rng)),p));

#ifdef DEBUG
			if(count.getProvisioned()%100==1) state.sanityCheck(activeConnections);
#endif
		}

		//decide the time for the next request
		nextRequestTime=currentTime+lrint(requestTimeGen(rng));
	}
	return count;
}

void Simulation::reset() {
	scratchpad.resetWeights();
	state.reset();
	activeConnections.clear();
}
