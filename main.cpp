/**
 * @file main.cpp
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
#include "provisioning_schemes/ProvisioningSchemeFactory.h"
#include "Simulation.h"
#include "StatCounter.h"

namespace po = boost::program_options;

std::mutex mtx;
std::condition_variable cvWorker, cvMain;
JobIterator::job_t nextWork;
std::map<size_t,std::pair<JobIterator::job_t,const StatCounter>> results;
bool newWork, newResult;

static void worker(const NetworkGraph &g) {
	Simulation sim(g);
	while(true) {
		JobIterator::job_t mywork;
		{
			std::unique_lock<std::mutex> lck(mtx);
			cvWorker.wait(lck,[]{return newWork;});
			// consume:
			mywork=std::move(nextWork);
			if(mywork.algname.size()) newWork=false;
		}
		cvMain.notify_one();
		if(!mywork.algname.size()) return;
		//do the work here
		StatCounter cnt=sim.run(mywork);
		{
			std::unique_lock<std::mutex> lck(mtx);
			results.emplace(std::make_pair(mywork.index,std::make_pair(std::move(mywork),cnt)));
			newResult=true;
		}
		cvMain.notify_one();
	}
}

static void printUsage(po::options_description &desc) {
	std::cerr<<desc<<"Supported Algorithms:"<<std::endl;
	ProvisioningSchemeFactory::getInstance().printHelp(std::cerr);
}

int main(int argc, char **argv) {
	//handle program options
	po::options_description desc("Usage");
	desc.add_options()
	    ("help,h", "produce help message")
	    ("opts,p", po::value<std::string>()->default_value(""),
	    		"Global options")
	    ("algs,a", po::value<std::string>()->default_value(""),
	    		"Algorithms and their specific options")
	    ("input,i", po::value<std::string>()->default_value("-"),
	    		"Input file. Default: stdin")
	    ("output,o", po::value<std::string>()->default_value("-"),
	    		"Output file. Default: stdout")
	    ("threads,t", po::value<size_t>()->default_value(
	    			std::thread::hardware_concurrency() ),
	    		"Number of threads."
	    		" The default is determined from the hardware.")
	    ("skip,s", po::value<size_t>()->default_value(0),
	    		"Skip the first n iterations."
	    		" Useful to continue after an interruption.")
	;
	po::variables_map vm;
	try{
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);
	} catch( boost::program_options::unknown_option & e )
	{
		std::cerr<<e.what()<<std::endl;
		printUsage(desc);
		return -1;
	}
	//set up the job iterator (parameter combinations)
	JobIterator jobs(vm["opts"].as<std::string>(),vm["algs"].as<std::string>());

	if(vm.count("help") || jobs.getTotalIterations()==0) {
		printUsage(desc);
		return 0;
	}

	std::cerr<<"Configured for "<<jobs.getTotalIterations()<<" iterations";
	if(vm.count("skip") && vm["skip"].as<size_t>()>0) {
		size_t s=vm["skip"].as<size_t>();
		std::cerr<<", skipping the first "<<s;
		if(jobs.getTotalIterations()<=s) {
			std::cerr<<" - that doesn't make sense."<<std::endl;
			return -1;
		}
		for(size_t i=0; i<s; ++i) ++jobs;
	}
	std::cerr<<'.'<<std::endl;

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
	std::string lastAlg("");
	while(!jobs.isEnd() || resultIdx<jobs.getCurrentIteration()) {
		bool printProgress=false;
		{
			std::unique_lock<std::mutex> lck(mtx);
			cvMain.wait(lck,[]{return !newWork || newResult;});
			if(!newWork) {
				nextWork=*jobs;
				++jobs;
				newWork=true;
			}
			if(newResult) {
				for(auto it=results.begin();
						it!=results.end() && it->first==resultIdx;
						++it, ++resultIdx, results.erase(std::prev(it)) ) {
					if(lastAlg!=it->second.first.algname) {
						*outstream << '#' << it->second.first.algname << ':';
						for(const auto &pn:it->second.first.params)
							*outstream << pn.first << TABLE_COL_SEPARATOR;
						*outstream<<it->second.second.tableHeader<<std::endl;
						lastAlg=it->second.first.algname;
					}
					for(const auto &pn:it->second.first.params)
						*outstream<<pn.second << TABLE_COL_SEPARATOR;
					*outstream << it->second.second <<std::endl;
					printProgress=true;
				}
				newResult=false;
			}
		}
		if(jobs.isEnd()) cvWorker.notify_all();
		else cvWorker.notify_one();
		if(printProgress) {
			outstream->flush();
			std::cerr<<'['<<std::setw(3)<<resultIdx*100/jobs.getTotalIterations()<<std::setw(0)<<"%] "
					<<resultIdx<<" / "<<jobs.getTotalIterations()<<" done."<<std::endl;
		}
	}
	//wait for all threads to finish
	for(auto &t:threadPool) t.join();

	return 0;
}
