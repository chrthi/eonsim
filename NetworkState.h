/**
 * @file NetworkState.h
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

#ifndef NETWORKSTATE_H_
#define NETWORKSTATE_H_

#include <boost/dynamic_bitset/dynamic_bitset.hpp>
#include <vector>

//#include "Simulation.h"

struct Provisioning;

class NetworkGraph;

class NetworkState {
public:
	NetworkState(const NetworkGraph &topology, unsigned int nSlots);
	virtual ~NetworkState();
	void provision(const Provisioning &p);
	void terminate(const Provisioning &p);
private:
	std::vector<boost::dynamic_bitset<> > primaryUse;
	std::vector<boost::dynamic_bitset<> > anyUse;
	std::vector<std::vector<boost::dynamic_bitset<> > > sharing;
};

#endif /* NETWORKSTATE_H_ */
