/**
 * @file KsqHybridCostProvisioning.cpp
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

#include "KsqHybridCostProvisioning.h"

KsqHybridCostProvisioning::KsqHybridCostProvisioning(unsigned int k_pri,
		unsigned int k_bkp):
		k_pri(k_pri),
		k_bkp(k_bkp)
{
}

KsqHybridCostProvisioning::~KsqHybridCostProvisioning() {
}

Provisioning KsqHybridCostProvisioning::operator ()(
		const NetworkGraph& g, const NetworkState& s, const NetworkGraph::DijkstraData &data,
		const Request& r) {
	Provisioning result;
	result.bandwidth=r.bandwidth;
	///@todo implement k-squared provisioning

	return result;
}

std::ostream& KsqHybridCostProvisioning::print(std::ostream& o) const {
	return o<<"PFMBL,"<<k_pri<<','<<k_bkp;
}

ProvisioningScheme* KsqHybridCostProvisioning::clone() {
	return new KsqHybridCostProvisioning(*this);
}
