/**
 * @file ArasFFProvisioning.h
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

#ifndef ARASFFPROVISIONING_H_
#define ARASFFPROVISIONING_H_

#include "../NetworkGraph.h"
#include "../NetworkState.h"
#include "../Simulation.h"

class ArasFFProvisioning: public ProvisioningScheme {
public:
	ArasFFProvisioning(unsigned int k);
	virtual ~ArasFFProvisioning();
	virtual Provisioning operator()(const NetworkGraph &g, const NetworkState &s, NetworkGraph::DijkstraData &data, const Request &r);
private:
	unsigned int k;
};

#endif /* ARASFFPROVISIONING_H_ */