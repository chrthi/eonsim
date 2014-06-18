/**
 * @file JobIterator.h
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

#ifndef JOBITERATOR_H_
#define JOBITERATOR_H_

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "provisioning_schemes/ProvisioningScheme.h"

/**
 * \brief Iterate over all specified parameter combinations for each specified algorithm.
 *
 * When dereferenced, the iterator returns a job_t packet that can be passed to the worker thread.
 */
class JobIterator {
public:
	JobIterator(const std::string &opts, const std::string &algs);
	virtual ~JobIterator();
	JobIterator &operator ++();
	bool isEnd() const;
	double getParam(const std::string &name) const;
	size_t getCurrentIteration() const;
	size_t getTotalIterations() const;
	/**
	 * \brief A parameter set for a simulation run that can be passed to a Simulation object.
	 */
	typedef struct{
		size_t index;
		std::string algname;
		ProvisioningScheme::ParameterSet params;
	} job_t;
	job_t operator*() const;
private:
	/**
	 * \brief A range specification for a single parameter.
	 */
	typedef struct{
		double min;
		double max;
		double step;
		int current;
	} paramRange_t;
	typedef std::map<std::string,paramRange_t> optmap_t;
	optmap_t globalopts;
	std::vector<std::pair<std::string, optmap_t> > algopts;
	std::vector<std::pair<std::string, optmap_t> >::const_iterator currentAlg;
	optmap_t currentParams;
	const char * parseOpts(const char *begin, const char *end, optmap_t &optmap) const;
	size_t totalIterations, currentIteration;

};

#endif /* JOBITERATOR_H_ */
