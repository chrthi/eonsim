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
#include <array>
#include <condition_variable>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <thread>

#include "NetworkGraph.h"
#include "provisioning_schemes/ArasFFProvisioning.h"
#include "provisioning_schemes/ArasMFSBProvisioning.h"
#include "provisioning_schemes/ArasPFMBLProvisioning.h"
#include "provisioning_schemes/ProvisioningScheme.h"
#include "provisioning_schemes/ShortestFFLFProvisioning.h"
#include "Simulation.h"
#include "StatCounter.h"

/*
static inline unsigned long timediff(const timespec &start, const timespec &end)
{
	return (end.tv_sec-start.tv_sec)*1000000000ul+end.tv_nsec-start.tv_nsec;
}
*/

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
		std::cerr<<mywork.load<<'-'<<std::this_thread::get_id()<<std::endl;
		StatCounter cnt=sim.run(*mywork.p,1000,10000,1000,1000*mywork.load);
		{
			std::unique_lock<std::mutex> lck(mtx);
			results.emplace(std::make_pair(mywork.index,std::make_pair(std::move(mywork),std::move(cnt))));
			newResult=true;
		}
		cvMain.notify_one();
	}
}

int main(int argc, char **argv) {
	//timespec tp1, tp2;

	std::ifstream in("input/input_att_d.txt");
	NetworkGraph g=NetworkGraph::loadFromMatrix(in);
	in.close();

	std::vector<std::thread> threadPool(3);
	for(auto &t:threadPool) t=std::thread(consume,std::ref(g));

	ShortestFFLFProvisioning p_fflf;
	ArasFFProvisioning p_ff(4);
	ArasMFSBProvisioning p_mfsb(4);
	ArasPFMBLProvisioning p_pfmbl(4,880);
	ProvisioningScheme *ps[]={&p_fflf,&p_ff,&p_mfsb,&p_pfmbl};
	size_t resultIdx=0;

	//stuff new work packages into the thread pool
	ProvisioningScheme **p=ps;
	nextWork.load=145;
	nextWork.index=-1;
	while(p || resultIdx<nextWork.index) {
		{
			std::unique_lock<std::mutex> lck(mtx);
			cvMain.wait(lck,[]{return !newWork || newResult;});
			if(!newWork) {
				if(p) {
					nextWork.load+=5;
					if(nextWork.load>200) {
						nextWork.load=150;
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
					std::cerr<<resultIdx<<": "<<*(it->second.first.p)<<','
							<<it->second.first.load<<std::endl;
				}
				newResult=false;
			}
		}
		if(p) cvWorker.notify_one();
		else cvWorker.notify_all();
	}
	//wait for all threads to finish
	for(auto &t:threadPool) t.join();
	return 0;
/*
	//for parameters...
	{
		ArasPFMBLProvisioning p(4,880);
		//KsqHybridCostProvisioning p(5,5);
		Simulation s(g,p);
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&tp1);
		const StatCounter &stats=s.run(10000,100000,1000,200000);
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&tp2);
		std::cout<<stats;
	}
	std::cout<<std::setw(11)<<timediff(tp1,tp2)<<" ns"<<std::endl;
	*/
}
