/**
 * @file globaldef.h
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

#ifndef GLOBALDEF_H_
#define GLOBALDEF_H_

#include <stddef.h>

#define SLOT_WIDTH 12.5
#define DISTANCE_UNIT 12.5
#define NUM_SLOTS 320

typedef unsigned short specIndex_t;
typedef unsigned short bandwidth_t;
typedef unsigned short nodeIndex_t;
typedef unsigned short linkIndex_t;
typedef unsigned long simtime_t;
typedef unsigned short distance_t;

#endif /* GLOBALDEF_H_ */
