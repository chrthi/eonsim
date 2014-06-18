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

/**
 * \brief Interface of a provisioning heuristic.
 *
 * Classes that implement a heuristic should inherit from this class and define
 * a ProvisioningSchemeFactory::Registrar object to register with the
 * ProvisioningSchemeFactory. See one of the existing heuristics to see how this
 * is done.
 */
class ProvisioningScheme {
public:
	typedef std::map<std::string,double> ParameterSet;
	virtual ~ProvisioningScheme();
	virtual Provisioning operator()(const NetworkGraph &g, const NetworkState &s, const NetworkGraph::DijkstraData &data, const Request &r) =0;
	friend std::ostream& operator<<(std::ostream& o, ProvisioningScheme const& s);
protected:
	/**
	 * \brief Description of a parameter that a heuristic accepts; used for printing usage information.
	 */
	typedef struct{
		const char *name;
		const char *limits;
		const char *defval;
		const char *help;
	} paramDesc_t;
	virtual std::ostream& print(std::ostream &o) const =0;
	virtual std::ostream& printFormatted(std::ostream &o,
			const char *const helpstr, const paramDesc_t *const params) const;
};

#endif /* PROVISIONINGSCHEME_H_ */
