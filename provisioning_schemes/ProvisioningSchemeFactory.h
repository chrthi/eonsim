/**
 * @file ProvisioningSchemeFactory.h
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

#ifndef PROVISIONINGSCHEMEFACTORY_H_
#define PROVISIONINGSCHEMEFACTORY_H_

#include <functional>
#include <map>
#include <string>
#include <iostream>

#include "ProvisioningScheme.h"

/**
 * \brief Creates instances of the heuristics based on the names supplied on the command line.
 */
class ProvisioningSchemeFactory {
public:
	~ProvisioningSchemeFactory();
	static const ProvisioningSchemeFactory &getInstance();
	std::unique_ptr<ProvisioningScheme> create(const std::string &name, const ProvisioningScheme::ParameterSet &params) const;
	std::ostream &printHelp(std::ostream &o) const;
private:
	ProvisioningSchemeFactory();
	ProvisioningSchemeFactory(const ProvisioningSchemeFactory &);
	static ProvisioningSchemeFactory &getMutableInstance();
	std::map<std::string, std::function<std::unique_ptr<ProvisioningScheme>(const ProvisioningScheme::ParameterSet &)>> factoryFunctionRegistry;
public:
	/**
	 * \brief A helper object that, when constructed, registers the template parameter class with the ProvisioningSchemeFactory.
	 */
	template<class T> class Registrar {
	public:
		Registrar(const std::string &name) {
			ProvisioningSchemeFactory::getMutableInstance().factoryFunctionRegistry[name]=
					[](const ProvisioningScheme::ParameterSet &p) -> std::unique_ptr<ProvisioningScheme>{
				return std::unique_ptr<ProvisioningScheme>(new T(p));
			};
		}
	};
};


#endif /* PROVISIONINGSCHEMEFACTORY_H_ */
