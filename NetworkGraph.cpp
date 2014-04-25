/**
 * @file NetworkGraph.cpp
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

#include "NetworkGraph.h"

#include <boost/graph/detail/adjacency_list.hpp>
#include <cmath>
#include <iostream>
#include <limits>
#include <utility>                   // for std::pair

#include "globaldef.h"

using namespace boost;

NetworkGraph::NetworkGraph() {
}

NetworkGraph::~NetworkGraph() {
}

void NetworkGraph::loadFromMatrix(std::istream &s) {
	unsigned int n, l;
	s>>n>>l;
	s.ignore(2); // skip the (cr)lf after the link count
	for(unsigned int i=0; i<n; ++i) {
		s.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
		add_vertex(*this);
	}
	for(unsigned int i=0; i<n; ++i) {
		for(unsigned int k=0; k<n; ++k) {
			double d;
			s>>d;
			if(d>0.0) // if(std::isnormal(d))
				add_edge(i,k,lrint(d/DISTANCE_UNIT),*this);
		}
	}
}
