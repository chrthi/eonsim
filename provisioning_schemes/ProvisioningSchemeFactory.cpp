/**
 * @file ProvisioningSchemeFactory.cpp
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

#include "ProvisioningSchemeFactory.h"

const ProvisioningSchemeFactory& ProvisioningSchemeFactory::getInstance() {
	return getMutableInstance();
}

std::unique_ptr<ProvisioningScheme> ProvisioningSchemeFactory::create(
		const std::string &name,
		const ProvisioningScheme::ParameterSet& params) const {
	auto it=factoryFunctionRegistry.find(name);

	if(it==factoryFunctionRegistry.end()) return 0;
	else return it->second(params);
}

ProvisioningSchemeFactory::ProvisioningSchemeFactory() {

}

ProvisioningSchemeFactory::~ProvisioningSchemeFactory() {
}

ProvisioningSchemeFactory& ProvisioningSchemeFactory::getMutableInstance() {
	static ProvisioningSchemeFactory f;
	return f;
}

std::ostream& ProvisioningSchemeFactory::printHelp(std::ostream& o) const {
	for(const auto &p:factoryFunctionRegistry) {
		auto ps=p.second(ProvisioningScheme::ParameterSet());
		o<<p.first<<": "<<*ps<<std::endl;
	}
	return o;
}
