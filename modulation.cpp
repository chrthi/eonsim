/**
 * @file modulation.cpp
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

#include "modulation.h"
#include "globaldef.h"

const struct mod_properties_t modulations[MOD_NONE]={
	{QAM64, static_cast<distance_t>( 125/DISTANCE_UNIT), 6},
	{QAM32, static_cast<distance_t>( 250/DISTANCE_UNIT), 5},
	{QAM16, static_cast<distance_t>( 500/DISTANCE_UNIT), 4},
	{QAM8,  static_cast<distance_t>(1000/DISTANCE_UNIT), 3},
	{QPSK,  static_cast<distance_t>(2000/DISTANCE_UNIT), 2},
	{BPSK,  static_cast<distance_t>(4000/DISTANCE_UNIT), 1}
};

const char *const modulation_names[MOD_NONE+1]={
	"QAM64",
	"QAM32",
	"QAM16",
	"QAM8",
	"QPSK",
	"BPSK",
	"NONE"
};

modulation_t calcModulation(distance_t reach) {
	for(size_t i=0;i<sizeof(modulations)/sizeof(*modulations); ++i)
		if(modulations[i].reach>=reach) return modulations[i].m;
	return MOD_NONE;
}

specIndex_t calcNumSlots(bandwidth_t bw, modulation_t mod) {
	return  DEFAULT_GUARDBAND + (
			bw % modulations[mod].bitPerSymbol ?
			bw / modulations[mod].bitPerSymbol + 1 :
			bw / modulations[mod].bitPerSymbol);
}
