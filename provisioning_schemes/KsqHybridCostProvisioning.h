/**
 * @file KsqHybridCostProvisioning.h
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

#ifndef KSQHYBRIDCOSTPROVISIONING_H_
#define KSQHYBRIDCOSTPROVISIONING_H_

#include <iostream>

#include "../globaldef.h"
#include "../NetworkGraph.h"
#include "../NetworkState.h"
#include "ProvisioningScheme.h"

class KsqHybridCostProvisioning: public ProvisioningScheme {
public:
	KsqHybridCostProvisioning(const ParameterSet &p);
	virtual ~KsqHybridCostProvisioning();
	virtual Provisioning operator()(const NetworkGraph &g, const NetworkState &s, const NetworkGraph::DijkstraData &data, const Request &r);
protected:
	virtual std::ostream& print(std::ostream &o) const;
private:
	static const char *const helpstr;
	static const paramDesc_t pdesc[];
	unsigned int k_pri; ///< Number of paths to consider for the primary
	unsigned int k_bkp; ///< Number of paths to consider for backup, per primary
	double costp(const NetworkGraph &g, const NetworkState &s,
			const NetworkGraph::Path &pp, specIndex_t beginp, specIndex_t endp) const;
	double costb(const NetworkGraph &g, const NetworkState &s,
			const NetworkGraph::Path &pb, specIndex_t beginb, specIndex_t endb) const;
	double c_cut, c_algn, c_sep;
};

#endif /* KSQHYBRIDCOSTPROVISIONING_H_ */
