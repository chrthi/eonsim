/**
 * @file ShortestFFLFProvisioning.h
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

#ifndef SHORTESTFFLFPROVISIONING_H_
#define SHORTESTFFLFPROVISIONING_H_

#include <iostream>
#include "ProvisioningScheme.h"

struct Request;

/**
 * \brief Implementation of a simple heuristic that uses only the shortest paths and assigns spectrum by first-fit/last-fit.
 */
class ShortestFFLFProvisioning: public ProvisioningScheme {
public:
	ShortestFFLFProvisioning(const ParameterSet &p);
	virtual ~ShortestFFLFProvisioning();
	virtual ProvisioningScheme *clone();
	virtual Provisioning operator()(const NetworkGraph &g, const NetworkState &s, const NetworkGraph::DijkstraData &data, const Request &r);
protected:
	static const char *const helpstr;
	virtual std::ostream& print(std::ostream &o) const;
};

#endif /* SHORTESTFFLFPROVISIONING_H_ */
