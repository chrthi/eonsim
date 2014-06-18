/**
 * @file JobIterator.cpp
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

#include "JobIterator.h"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <stdexcept>

JobIterator::JobIterator(const std::string &opts, const std::string &algs):
	globalopts(),
	algopts(),
	currentAlg(),
	currentParams(),
	totalIterations(0),
	currentIteration(0)
{
	const char *p1=algs.c_str(), *p2, *end;
	end=algs.end().operator ->();
	globalopts.insert(std::make_pair("iters",
			paramRange_t({DEFAULT_SIM_ITERS,DEFAULT_SIM_ITERS,1.0,0})
			));
	globalopts.insert(std::make_pair("discard",
			paramRange_t({DEFAULT_SIM_DISCARD,DEFAULT_SIM_DISCARD,1.0,0})
			));
	globalopts.insert(std::make_pair("k",
			paramRange_t({DEFAULT_K,DEFAULT_K,1.0,0})
			));
	globalopts.insert(std::make_pair("load",
			paramRange_t({DEFAULT_LOAD_MIN,DEFAULT_LOAD_MAX,DEFAULT_LOAD_STEP,0})
			));
	globalopts.insert(std::make_pair("bwmin",
			paramRange_t({DEFAULT_BW_MIN,DEFAULT_BW_MIN,1.0,0})
			));
	globalopts.insert(std::make_pair("bwmax",
			paramRange_t({DEFAULT_BW_MAX,DEFAULT_BW_MAX,1.0,0})
			));
	p2=parseOpts(opts.c_str(),opts.end().operator ->(),globalopts);
	if(p2!=opts.end().operator ->())
		throw std::runtime_error(
			std::string("Error parsing the global options:\n")
			+opts+'\n'
			+std::string(p2-opts.c_str(),'-')+'^'
			);

	while(p1<end){
		//algorithm name
		while(p1<end && isspace(*p1)) ++p1;
		p2=p1;
		while(p2<end && (isalnum(*p2)||*p2=='_')) ++p2;
		if(p2<end && !isspace(*p2) && *p2!='(') break;
		algopts.push_back(std::make_pair(std::string(p1,p2-p1),optmap_t()));
		while(p2<end && isspace(*p2)) ++p2;
		if(p2>=end && *p2!='(') break;
		p1=parseOpts(p2+1,end,std::prev(algopts.end())->second);
		if(p1>=end) break;
		else if(*p1!=')')
			throw std::runtime_error(
				std::string("Error parsing the algorithm options:\n")
				+algs+'\n'
				+std::string(p1-algs.c_str(),'-')+'^'
				);
		++p1;
		while(p1<end && isspace(*p1)) ++p1;
		if(p1>=end || *p1!=',') break;
		++p1;
	}

	for(auto a=algopts.rbegin(); a!=algopts.rend(); ++a) {
		currentParams=a->second;
		currentParams.insert(globalopts.begin(),globalopts.end());
		size_t algIterations=1;
		for(const auto &p: currentParams) {
			algIterations*=1+lrint(floor((p.second.max-p.second.min)/p.second.step));
		}
		totalIterations+=algIterations;
	}
	currentAlg=algopts.begin();
}

JobIterator::~JobIterator() {
}

JobIterator& JobIterator::operator ++() {
	if(isEnd()) return *this;
	++currentIteration;
	bool algCompleted=true;
	for(auto &p:currentParams) {
		++p.second.current;
		if(p.second.min+p.second.current*p.second.step>p.second.max) {
			p.second.current=0;
		} else {
			algCompleted=false;
			break;
		}
	}
	if(!algCompleted) return *this;
	++currentAlg;
	if(currentAlg==algopts.end()) {
		currentParams.clear();
		return *this;
	}
	currentParams=currentAlg->second;
	currentParams.insert(globalopts.begin(),globalopts.end());
	return *this;
}

JobIterator::job_t JobIterator::operator *() const {
	job_t result={0,""};
	if(isEnd()) return result;
	result.index=currentIteration;
	result.algname=currentAlg->first;
	for(const auto &p:currentParams)
		result.params.insert(std::make_pair(
				p.first,
				p.second.min+p.second.current*p.second.step)
		);
	return result;
}

bool JobIterator::isEnd() const {
	return (currentAlg==algopts.end());
}

double JobIterator::getParam(const std::string& name) const {
	optmap_t::const_iterator it=currentParams.find(name);
	if(it==currentParams.end()) return nan("");
	return it->second.min+it->second.current*it->second.step;
}

size_t JobIterator::getCurrentIteration() const {
	return currentIteration;
}

size_t JobIterator::getTotalIterations() const {
	return totalIterations;
}

const char * JobIterator::parseOpts(const char *begin, const char *end, optmap_t &optmap) const {
	const char *p2;
	while(1) {
		//param name
		while(isspace(*begin) && begin<end) ++begin;
		if(*begin==')') return begin;
		p2=begin;
		while(p2<end && (isalnum(*p2)||*p2=='_')) ++p2;
		if(p2<end && !isspace(*p2) && *p2!='=') return p2;
		auto &par=optmap[std::string(begin,p2-begin)];
		while(p2<end && isspace(*p2)) ++p2;
		if(p2>=end || *p2!='=') return p2;
		par.current=0;

		//first value
		begin=p2+1;
		double d1=strtod(begin,const_cast<char **>(&p2));
		while(p2<end && isspace(*p2)) ++p2;
		if(p2<end && *p2==':') {
			//second value
			begin=p2+1;
			double d2=strtod(begin,const_cast<char **>(&p2));
			while(p2<end && isspace(*p2)) ++p2;
			if(p2>=end || *p2!=':') return p2;
			par.step=d2;
			//third value
			begin=p2+1;
			par.max=strtod(begin,const_cast<char **>(&p2));
		} else {
			par.step=1.0;
			par.max=d1;
		}
		par.min=d1;
		while(p2<end && isspace(*p2)) ++p2;
		if(p2>=end || *p2!=',') return p2;
		begin=p2+1;
	}
	return 0; //unreachable, but makes eclipse's static analyzer happy.
}
