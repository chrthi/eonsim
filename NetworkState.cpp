/**
 * @file NetworkState.cpp
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

#include "NetworkState.h"
#include "NetworkGraph.h"

NetworkState::NetworkState(const NetworkGraph& topology, unsigned int nSlots) :
primaryUse(boost::num_edges(topology),boost::dynamic_bitset<>(nSlots,0)),
anyUse(boost::num_edges(topology),boost::dynamic_bitset<>(nSlots,0)),
sharing(boost::num_edges(topology),std::vector<boost::dynamic_bitset<> >(boost::num_edges(topology),boost::dynamic_bitset<>(nSlots,0)))
{
}

NetworkState::~NetworkState() {
}

void NetworkState::provision(const Provisioning &p) {
	//todo implement provisioning update
}

void NetworkState::terminate(const Provisioning &p) {
	//todo implement termination update
}
