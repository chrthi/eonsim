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
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#ifndef NOTHREAD
#include <mutex>
#include <condition_variable>
#include <thread>
#endif
#include <utility>
#include <vector>

#include "globaldef.h"
#include "NetworkGraph.h"
#include "provisioning_schemes/Chao2012FFProvisioning.h"
#include "provisioning_schemes/Chen2013MFSBProvisioning.h"
#include "provisioning_schemes/Tarhan2013PFMBLProvisioning.h"
#include "provisioning_schemes/ProvisioningScheme.h"
#include "provisioning_schemes/ShortestFFLFProvisioning.h"
#include "Simulation.h"
#include "StatCounter.h"

struct WorkPackage{
	size_t index;
	std::unique_ptr<ProvisioningScheme> p;
	unsigned int load;
} nextWork;

#ifndef NOTHREAD
std::mutex mtx;
std::condition_variable cvWorker, cvMain;
bool newWork, newResult;
std::map<size_t,std::pair<WorkPackage,const StatCounter>> results;

static void worker(const NetworkGraph &g) {
	Simulation sim(g);
	while(true) {
		WorkPackage mywork;
		{
			std::unique_lock<std::mutex> lck(mtx);
			cvWorker.wait(lck,[]{return newWork;});
			// consume:
			mywork=std::move(nextWork);
			if(mywork.p) newWork=false;
		}
		cvMain.notify_one();
		if(!mywork.p) return;
		//do the work here
		StatCounter cnt=sim.run(*mywork.p,DEFAULT_SIM_DISCARD,DEFAULT_SIM_ITERS,
				DEFAULT_AVG_INTARRIVAL,DEFAULT_AVG_INTARRIVAL*mywork.load);
		{
			std::unique_lock<std::mutex> lck(mtx);
			results.emplace(std::make_pair(mywork.index,std::make_pair(std::move(mywork),cnt)));
			newResult=true;
		}
		cvMain.notify_one();
	}
}
#endif

int main(int argc, char **argv) {
	std::ifstream in("input/input_att_d.txt");
	NetworkGraph g=NetworkGraph::loadFromMatrix(in);
	in.close();

#ifndef NOTHREAD
	const unsigned int nthreads=std::thread::hardware_concurrency()<4?
			std::thread::hardware_concurrency(): // for the lab pc (Core2)
			std::thread::hardware_concurrency()>4?
					std::thread::hardware_concurrency()/2: // for the ICT server (8*4core Xeon)
					std::thread::hardware_concurrency()-1; // for my laptop (4-core i7)

	std::cerr<<std::thread::hardware_concurrency()<<" Threads supported; using "<<nthreads<<'.'<<std::endl;
	std::vector<std::thread> threadPool(nthreads);
	for(auto &t:threadPool) t=std::thread(worker,std::ref(g));
#else
	Simulation sim(g);
#endif
	ShortestFFLFProvisioning p_fflf;
	Chao2012FFProvisioning p_ff(DEFAULT_K);
	Chen2013MFSBProvisioning p_mfsb(DEFAULT_K);
	Tarhan2013PFMBLProvisioning p_pfmbl0(DEFAULT_K,0), p_pfmbl1(DEFAULT_K,880);
	ProvisioningScheme *ps[]={&p_fflf,&p_ff,&p_mfsb,&p_pfmbl0,&p_pfmbl1};
	const size_t totalWp=(sizeof(ps)/sizeof(*ps))
			*((DEFAULT_LOAD_MAX-DEFAULT_LOAD_MIN+DEFAULT_LOAD_STEP)/DEFAULT_LOAD_STEP);

	//stuff new work packages into the thread pool
	ProvisioningScheme **p=ps;
	nextWork.load=DEFAULT_LOAD_MIN-DEFAULT_LOAD_STEP;
	size_t resultIdx=0;
	nextWork.index=-1;
	std::cout<<"\"Algorithm\"" TABLE_COL_SEPARATOR "\"Load\""<<TABLE_COL_SEPARATOR
			<<StatCounter::tableHeader
			<<std::endl;
	while(p || resultIdx<=nextWork.index) {
#ifndef NOTHREAD
		bool printStat=false;
		{
			std::unique_lock<std::mutex> lck(mtx);
			cvMain.wait(lck,[]{return !newWork || newResult;});
			if(!newWork) {
				if(p) {
#endif
					nextWork.load+=DEFAULT_LOAD_STEP;
					if(nextWork.load>DEFAULT_LOAD_MAX) {
						nextWork.load=DEFAULT_LOAD_MIN;
						++p;
					}
					if(p>=ps+sizeof(ps)/sizeof(*ps)) p=0;
#ifdef NOTHREAD
					if(p) {
					const ProvisioningScheme &prov=**p;
					const unsigned int &load=nextWork.load;
					StatCounter stat=sim.run(**p,DEFAULT_SIM_DISCARD,DEFAULT_SIM_ITERS,
							DEFAULT_AVG_INTARRIVAL,DEFAULT_AVG_INTARRIVAL*nextWork.load);
					++nextWork.index;
					resultIdx=nextWork.index+1;
#else
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
					const ProvisioningScheme &prov=*(it->second.first.p);
					const unsigned int &load=it->second.first.load;
					const StatCounter &stat=it->second.second;
#endif
					std::cout
							//provisioning scheme and its parameters
							<<'"'<<prov<<'"'<<TABLE_COL_SEPARATOR
							//Load value
							<<load<<TABLE_COL_SEPARATOR
							//Statistics
							<<stat
							<<std::endl;
#ifndef NOTHREAD
					printStat=true;
				}
				newResult=false;
			}
		}
		if(p) cvWorker.notify_one();
		else cvWorker.notify_all();
		if(printStat) {
#endif
			std::cout.flush();
			std::cerr<<'['<<std::setw(3)<<resultIdx*100/totalWp<<std::setw(0)<<"%] "
					<<resultIdx<<" / "<<totalWp<<" done."<<std::endl;
		}
	}
#ifndef NOTHREAD
	//wait for all threads to finish
	for(auto &t:threadPool) t.join();
#endif
	return 0;
}
