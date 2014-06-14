/**
 * @file SimulationMsgs.h
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

#ifndef SIMULATIONMSGS_H_
#define SIMULATIONMSGS_H_

#include <boost/graph/graph_traits.hpp>
#include <vector>

#include "globaldef.h"
#include "modulation.h"
#include "NetworkGraph.h"

struct Request{
	NetworkGraph::Graph::vertex_descriptor source, dest;
	unsigned int bandwidth;
};

struct Provisioning{
	NetworkGraph::Path priPath;
	specIndex_t priSpecBegin, priSpecEnd;
	modulation_t priMod;

	NetworkGraph::Path bkpPath;
	specIndex_t bkpSpecBegin, bkpSpecEnd;
	modulation_t bkpMod;

	bandwidth_t bandwidth;

	/**
	 * All different reasons for blocking a connection request that the statistics counter can count.
	 */
	enum state_t{
		/// No feasible primary path could be found.
		BLOCK_PRI_NOPATH,
		/// No spectrum was available on any primary path candidate.
		BLOCK_PRI_NOSPEC,
		/// No feasible backup path could be found.
		BLOCK_SEC_NOPATH,
		/// No spectrum was available on any backup path candidate.
		BLOCK_SEC_NOSPEC,
		SUCCESS
	} state;
};

#endif /* SIMULATIONMSGS_H_ */
