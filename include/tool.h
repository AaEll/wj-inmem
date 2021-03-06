#ifndef _H_TOOL_
#define _H_TOOL_
#include <iostream>
#include <ctime>
#include <fstream>
#include <cstdint>
#include <vector>
#include <list>
#include <cstring>
#include <cstdlib>
#include <boost/math/special_functions/erf.hpp>
#include <cmath>
#include <algorithm>
#include <bitset>
#include "base.h"
using namespace std;
namespace tool
{
std::vector<std::string> split(std::string &str, const char *c);
void start_report();
void report(const std::list<double> &est, const size_t &, const size_t &, const size_t &, const double &prob);
void report(const size_t &time, const size_t &n, const size_t &n_rejected_join, const size_t &n_rejected_cond, const double &freq, const double &, const double &, const double &);
double get_variance(const std::list<double> &, const double &mean);
inline double xql_erf(const double &z){
	return boost::math::erf(z);
}
inline double xql_erf_inv(const double &p){
	return boost::math::erf_inv(p);
}
double get_zp(const double &);
std::vector<std::vector<size_t> > full_permutation(const size_t num);
double get_mean(const std::list<double> &);
double loose_ci(const double&, const double&, const double&, const double&);
double calc_ci(const double&, const double&, const double &, const double&, const double&);
double calc_ci(const double &var, const double &round, const double &prob);
double calc_var(const double&, const double&, const double &, const double&);
template<class RawType>
double get_selectivity(const std::vector<RawType> &t, std::function<bool(const RawType &)> cond_func){
	size_t count = 0;
	for(const RawType &x : t){
		if(cond_func(x) > 0)
			++count;
	}
	return 1.0 * count / t.size();
}
std::vector<std::vector<size_t>> gen_order(const uint32_t &);
std::vector<std::vector<size_t> > back_trace(const std::vector<std::vector<size_t> >&, const std::vector<std::vector<size_t> > &);
void read_date_t(date_t &, date_t &);
date_t read_config(std::string& name, std::string& tag);
};
#endif
