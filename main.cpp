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

#include <stddef.h>
#include <algorithm>
#include <condition_variable>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include "globaldef.h"
#include "NetworkGraph.h"
#include "provisioning_schemes/ArasFFProvisioning.h"
#include "provisioning_schemes/ArasMFSBProvisioning.h"
#include "provisioning_schemes/ArasPFMBLProvisioning.h"
#include "provisioning_schemes/ProvisioningScheme.h"
#include "provisioning_schemes/ShortestFFLFProvisioning.h"
#include "Simulation.h"
#include "StatCounter.h"

struct WorkPackage{
	size_t index;
	std::unique_ptr<ProvisioningScheme> p;
	unsigned int load;
};

std::mutex mtx;
std::condition_variable cvWorker, cvMain;
bool newWork, newResult;
WorkPackage nextWork;
std::map<size_t,std::pair<WorkPackage,const StatCounter>> results;

void consume (const NetworkGraph &g) {
	Simulation sim(g);
	while(true) {
		WorkPackage mywork;
		{
			std::unique_lock<std::mutex> lck(mtx);
			cvWorker.wait(lck,[]{return newWork;});
			// consume:
			mywork=std::move(nextWork);
			newWork=false;
		}
		cvMain.notify_one();
		if(!mywork.p) return;
		//do the work here
		sim.reset();
		StatCounter cnt=sim.run(*mywork.p,DEFAULT_SIM_DISCARD,DEFAULT_SIM_ITERS,
				DEFAULT_AVG_INTARRIVAL,DEFAULT_AVG_INTARRIVAL*mywork.load);
		{
			std::unique_lock<std::mutex> lck(mtx);
			results.emplace(std::make_pair(mywork.index,std::make_pair(std::move(mywork),std::move(cnt))));
			newResult=true;
		}
		cvMain.notify_one();
	}
}

int main(int argc, char **argv) {
	std::ifstream in("input/input_att_d.txt");
	NetworkGraph g=NetworkGraph::loadFromMatrix(in);
	in.close();

	//const unsigned int nthreads=(std::thread::hardware_concurrency()+1)/2;
	const unsigned int nthreads=(std::thread::hardware_concurrency()+1)/2;
	std::cerr<<std::thread::hardware_concurrency()<<" Threads supported; using "<<nthreads<<'.'<<std::endl;
	std::vector<std::thread> threadPool(nthreads);
	for(auto &t:threadPool) t=std::thread(consume,std::ref(g));

	ShortestFFLFProvisioning p_fflf;
	ArasFFProvisioning p_ff(DEFAULT_K);
	ArasMFSBProvisioning p_mfsb(DEFAULT_K);
	ArasPFMBLProvisioning p_pfmbl0(DEFAULT_K,0), p_pfmbl1(DEFAULT_K,880);
	ProvisioningScheme *ps[]={&p_fflf,&p_ff,&p_mfsb,&p_pfmbl0,&p_pfmbl1};
	size_t resultIdx=0;
	const size_t totalWp=(sizeof(ps)/sizeof(*ps))
			*((DEFAULT_LOAD_MAX-DEFAULT_LOAD_MIN+DEFAULT_LOAD_STEP)/DEFAULT_LOAD_STEP);

	//stuff new work packages into the thread pool
	ProvisioningScheme **p=ps;
	nextWork.load=DEFAULT_LOAD_MIN-DEFAULT_LOAD_STEP;
	nextWork.index=-1;
	while(p || resultIdx<nextWork.index) {
		bool printStat=false;
		{
			std::unique_lock<std::mutex> lck(mtx);
			cvMain.wait(lck,[]{return !newWork || newResult;});
			if(!newWork) {
				if(p) {
					nextWork.load+=DEFAULT_LOAD_STEP;
					if(nextWork.load>DEFAULT_LOAD_MAX) {
						nextWork.load=DEFAULT_LOAD_MIN;
						++p;
					}
					if(p>=ps+sizeof(ps)/sizeof(*ps)) p=0;
				}
				if(!p){
					nextWork.p=std::unique_ptr<ProvisioningScheme>();
				} else {
					++nextWork.index;
					nextWork.p=std::move(std::unique_ptr<ProvisioningScheme>((*p)->clone()));
				}
				newWork=true;
			}
			if(newResult) {
				for(auto it=results.begin();
						it!=results.end() && it->first==resultIdx;
						++it, ++resultIdx, results.erase(std::prev(it)) ) {
					std::cout<<
							//provisioning scheme and its parameters
							*(it->second.first.p)<<SEPARATOR_CHAR
							//Load value
							<<it->second.first.load<<SEPARATOR_CHAR
							//Statistics
							<<it->second.second<<SEPARATOR_CHAR
							<<std::endl;
				}
				newResult=false;
				printStat=true;
			}
		}
		if(p) cvWorker.notify_one();
		else cvWorker.notify_all();
		if(printStat) {
			std::cout.flush();
			std::cerr<<'['<<std::setw(3)<<(resultIdx+1)*100/totalWp<<std::setw(0)<<"%] "
					<<(resultIdx+1)<<" / "<<totalWp<<" done."<<std::endl;
		}
	}
	//wait for all threads to finish
	for(auto &t:threadPool) t.join();
	return 0;
}
