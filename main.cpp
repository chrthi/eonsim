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

#include <boost/program_options.hpp>
#include <stddef.h>
#include <algorithm>
#include <condition_variable>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "globaldef.h"
#include "JobIterator.h"
#include "NetworkGraph.h"
#include "provisioning_schemes/ProvisioningScheme.h"
#include "Simulation.h"
#include "StatCounter.h"

namespace po = boost::program_options;

struct WorkPackage{
	size_t index;
	ProvisioningScheme* p;
	unsigned int load;
} nextWork;

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

int main(int argc, char **argv) {
	//handle program options
	po::options_description desc("Allowed options");
	desc.add_options()
	    ("help", "produce help message")
	    ("opts,p", po::value<std::string>()->default_value(""), "Global options")
	    ("algs,a", po::value<std::string>()->default_value(""), "Algorithms and their specific options")
	    ("input,i", po::value<std::string>()->default_value("-"), "Input file. Default: stdin")
	    ("output,o", po::value<std::string>()->default_value("-"), "Output file. Default: stdout")
	    ("threads,t", po::value<size_t>()->default_value(std::thread::hardware_concurrency()-1), "Output file. Default: stdout")
	;
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);
	JobIterator jobs(vm["opts"].as<std::string>(),vm["algs"].as<std::string>());

	//load the input data (network model) from file or stdin
	std::ifstream infile;
	std::istream *instream=&std::cin;
	if(vm["input"].as<std::string>()!="-") {
		infile.open(vm["input"].as<std::string>());
		instream=&infile;
	}
	NetworkGraph g=NetworkGraph::loadFromMatrix(*instream);
	if(infile.is_open()) infile.close();

	//send the output to a file or stdout
	std::ofstream outfile;
	std::ostream *outstream=&std::cout;
	if(vm["output"].as<std::string>()!="-") {
		outfile.open(vm["output"].as<std::string>(),std::ofstream::ate|std::ofstream::app);
		outstream=&outfile;
	}

	std::cerr<<std::thread::hardware_concurrency()
		<<" Threads supported; using "<<vm["threads"].as<size_t>()<<'.'
		<<std::endl;
	std::vector<std::thread> threadPool(vm["threads"].as<size_t>());
	for(auto &t:threadPool) t=std::thread(worker,std::ref(g));

	size_t resultIdx=0;
	*outstream<<"\"Algorithm\"" TABLE_COL_SEPARATOR "\"Load\""<<TABLE_COL_SEPARATOR
			<<StatCounter::tableHeader
			<<std::endl;
	while(!jobs.isEnd() || resultIdx<jobs.getCurrentIteration()) {
		bool printStat=false;
		{
			std::unique_lock<std::mutex> lck(mtx);
			cvMain.wait(lck,[]{return !newWork || newResult;});
			if(!newWork) {
				nextWork.index=jobs.getCurrentIteration();
				nextWork.load=jobs.getParam("load");
				nextWork.p=*jobs;
				newWork=true;
				++jobs;
			}
			if(newResult) {
				for(auto it=results.begin();
						it!=results.end() && it->first==resultIdx;
						++it, ++resultIdx, results.erase(std::prev(it)) ) {
					const ProvisioningScheme &prov=*(it->second.first.p);
					const unsigned int &load=it->second.first.load;
					const StatCounter &stat=it->second.second;
					*outstream
							//provisioning scheme and its parameters
							<<'"'<<prov<<'"'<<TABLE_COL_SEPARATOR
							//Load value
							<<load<<TABLE_COL_SEPARATOR
							//Statistics
							<<stat
							<<std::endl;
					delete it->second.first.p;
					printStat=true;
				}
				newResult=false;
			}
		}
		if(jobs.isEnd()) cvWorker.notify_all();
		else cvWorker.notify_one();
		if(printStat) {
			outstream->flush();
			std::cerr<<'['<<std::setw(3)<<resultIdx*100/jobs.getTotalIterations()<<std::setw(0)<<"%] "
					<<resultIdx<<" / "<<jobs.getTotalIterations()<<" done."<<std::endl;
		}
	}
	//wait for all threads to finish
	for(auto &t:threadPool) t.join();

	return 0;
}
