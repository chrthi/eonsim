/**
 * @file Shao2012FFProvisioning.h
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

#ifndef SHAO2012FFPROVISIONING_H_
#define SHAO2012FFPROVISIONING_H_

#include <iostream>
#include "ProvisioningScheme.h"

class Shao2012FFProvisioning: public ProvisioningScheme {
public:
	Shao2012FFProvisioning(const ParameterSet &p);
	virtual ~Shao2012FFProvisioning();
	virtual ProvisioningScheme *clone();
	virtual Provisioning operator()(const NetworkGraph &g, const NetworkState &s, const NetworkGraph::DijkstraData &data, const Request &r);
protected:
	virtual std::ostream& print(std::ostream &o) const;
private:
	unsigned int k_pri, k_bkp;
};

#endif /* SHAO2012FFPROVISIONING_H_ */
