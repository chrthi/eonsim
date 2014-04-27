/**
 * @file main.cpp
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

#include <sys/time.h>
#include <ctime>
#include <iomanip>
#include <iostream>

#include "NetworkGraph.h"
#include "NetworkState.h"
#include "provisioning_schemes/KsqHybridCostProvisioning.h"
#include "provisioning_schemes/ShortestFFLFProvisioning.h"
#include "Simulation.h"
#include "StatCounter.h"

static inline unsigned long timediff(const timespec &start, const timespec &end)
{
	return (end.tv_sec-start.tv_sec)*1000000000ul+end.tv_nsec-start.tv_nsec;
}

int main(int argc, char **argv) {
	timespec tp1, tp2;

	std::ifstream in("input/input_att_d.txt");
	NetworkGraph g=NetworkGraph::loadFromMatrix(in);
	in.close();

	//for parameters...
	{
		ShortestFFLFProvisioning p(g.getNumNodes(),g.getNumLinks());
		//KsqHybridCostProvisioning p(5,5);
		Simulation s(g,p);
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&tp1);
		const StatCounter &stats=s.run(1000,100000,1000,30000);
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&tp2);
		std::cout<<stats;
	}
	std::cout<<std::setw(11)<<timediff(tp1,tp2)<<" ns"<<std::endl;
}


