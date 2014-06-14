/**
 * @file ProvisioningScheme.cpp
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

#include "ProvisioningScheme.h"

#include <iomanip>

ProvisioningScheme::~ProvisioningScheme() {
}

std::ostream& operator<<(std::ostream& o, ProvisioningScheme const& s) {
	return s.print(o);
}

std::ostream& ProvisioningScheme::printFormatted(std::ostream& o,
		const char* const helpstr, const paramDesc_t* const params) const {
	o<<helpstr<<". Supported parameters:"<<std::endl;
	o.fill(' ');
	o<<std::left;
	for(size_t i=0; params[i].name; ++i) {
		o<<'\t' << std::setw(8) << params[i].name << std::setw(0)
		 << std::setw(12) << params[i].limits << std::setw(0)
		 <<" default=" << std::setw(5) << params[i].defval << std::setw(0)
		 <<' ' << params[i].help <<'.'<< std::endl;
	}
	return o;
}
