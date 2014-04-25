/**
 * @file Simulation.h
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

#ifndef SIMULATION_H_
#define SIMULATION_H_

#include <boost/graph/graph_traits.hpp>
#include <map>
#include <vector>

class Simulation;
struct Request;
struct Provisioning;

#include "NetworkGraph.h"
#include "NetworkState.h"
#include "StatCounter.h"

struct Request{
	boost::graph_traits<NetworkGraph>::vertex_descriptor source, dest;
	unsigned int bandwidth;
};
struct Provisioning{
	std::vector<boost::graph_traits<NetworkGraph>::edge_descriptor> priPath;
	std::vector<boost::graph_traits<NetworkGraph>::edge_descriptor> bkpPath;
	unsigned int priSpecBegin, priSpecEnd;
	unsigned int bkpSpecBegin, bkpSpecEnd;
	unsigned int bandwidth;
};
class ProvisioningScheme{
public:
	virtual Provisioning operator()(const NetworkGraph &g, const NetworkState &s, const Request &r) =0;
	virtual ~ProvisioningScheme() {};
};

class Simulation {
public:
	Simulation(const NetworkGraph &topology, ProvisioningScheme &p);
	const StatCounter &run(unsigned long itersDiscard, unsigned long itersTotal,
			unsigned int avg_interarrival,unsigned int avg_holding);
	~Simulation();
private:
	const NetworkGraph& topology;
	NetworkState state;
	ProvisioningScheme &provision;
	StatCounter count;
	std::multimap<unsigned long, Provisioning> activeConnections;
};

/*
In the old simulator, the average holding time is 1s, the arrival rate is given as a parameter.
When a connection request is handled, a new request is generated.
*/

#endif /* SIMULATION_H_ */
