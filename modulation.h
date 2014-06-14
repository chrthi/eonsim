/**
 * @file modulation.h
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

#ifndef MODULATION_H_
#define MODULATION_H_

#include "globaldef.h"

enum modulation_t {
	QAM64,
	QAM32,
	QAM16,
	QAM8,
	QPSK,
	BPSK,
	MOD_NONE
};

extern const char *const modulation_names[MOD_NONE+1];

extern const struct mod_properties_t {
	modulation_t m;
	distance_t reach;
	unsigned int bitPerSymbol;
} modulations[MOD_NONE];

modulation_t calcModulation(distance_t reach);
specIndex_t calcNumSlots(bandwidth_t bw, modulation_t mod);

#endif /* MODULATION_H_ */
