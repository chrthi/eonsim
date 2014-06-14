/**
 * @file ProvisioningScheme.h
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

#ifndef PROVISIONINGSCHEME_H_
#define PROVISIONINGSCHEME_H_

#include <iostream>

#include "../NetworkGraph.h"
#include "../NetworkState.h"
#include "../SimulationMsgs.h"

class ProvisioningScheme {
public:
	typedef std::map<std::string,double> ParameterSet;
	virtual ~ProvisioningScheme();
	virtual ProvisioningScheme *clone() =0;
	virtual Provisioning operator()(const NetworkGraph &g, const NetworkState &s, const NetworkGraph::DijkstraData &data, const Request &r) =0;
	friend std::ostream& operator<<(std::ostream& o, ProvisioningScheme const& s);
protected:
	virtual std::ostream& print(std::ostream &o) const =0;
};

#endif /* PROVISIONINGSCHEME_H_ */
