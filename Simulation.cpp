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

Simulation::~Simulation() {
}

Simulation::Simulation(const NetworkGraph& topology, ProvisioningScheme& p):
				topology(topology),
				state(topology),
				provision(p),
				count(0)
{}

const StatCounter& Simulation::run(unsigned long itersDiscard,
		unsigned long itersTotal,unsigned int avg_interarrival,unsigned int avg_holding) {
	boost::random::taus88 rng(0);
	boost::random::exponential_distribution<> requestTimeGen(1.0/avg_interarrival);
	boost::random::exponential_distribution<> holdingTimeGen(1.0/avg_holding);
	boost::random::uniform_int_distribution<size_t> sourceGen(0,num_vertices(topology)-1);
	boost::random::uniform_int_distribution<size_t> destGen(0,num_vertices(topology)-2);
	boost::random::uniform_int_distribution<unsigned int> bandwidthGen(4,320); //todo decide proper bandwidth limits
	unsigned long nextRequestTime=0;
	unsigned long currentTime=0;
	count.reset(itersDiscard);
	for(unsigned long i=0; i<itersTotal; ++i) {

		//terminate all connections that should have terminated by now
		for(std::map<unsigned long, Provisioning>::iterator nextTerm=activeConnections.begin();
				nextTerm!=activeConnections.end() && nextTerm->first<=nextRequestTime;
				nextTerm=activeConnections.begin()) {
			//a termination event is next.
			//update simulation time
			currentTime=nextTerm->first;

			//update the network state with the removed connection
			state.terminate(nextTerm->second);

			count.countTermination(nextTerm->second.bandwidth);

			//remove the connection from the list
			activeConnections.erase(nextTerm);
		}

		//update simulation time
		currentTime=nextRequestTime;

		//a request event is next.

		//Generate a random request
		Request r;
		size_t sourceIndex=sourceGen(rng);
		r.source=vertex(sourceIndex,topology);
		size_t destIndex=destGen(rng);
		if(destIndex>=sourceIndex) ++destIndex;
		r.dest=vertex(destIndex,topology);
		r.bandwidth=bandwidthGen(rng);

		//Run the provisioning algorithm
		Provisioning p=provision(topology,state,r);
		//todo how to handle blockings?

		count.countProvisioning(p.state,p.bandwidth);

		if(p.state==Provisioning::SUCCESS) {
			//update the network state with the new connection
			state.provision(p);

			//Add the connection to the active connection list with an expiry time
			activeConnections.insert(std::pair<const unsigned long, Provisioning>(
					currentTime+lrint(holdingTimeGen(rng)),p));
		}

		//decide the time for the next request
		nextRequestTime=currentTime+lrint(requestTimeGen(rng));
	}
	return count;
}
