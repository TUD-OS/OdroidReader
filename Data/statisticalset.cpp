#include "Data/statisticalset.h"

#include <algorithm>

StatisticalSet::StatisticalSet(const DataDescriptor* ds) : descriptor(ds)
{}

void StatisticalSet::addValue(double value) {
	auto low = std::lower_bound(_sorted.begin(),_sorted.end(),value);
	_sorted.insert(low,value);
}

void StatisticalSet::extend(const StatisticalSet& s) {
	for (double val : s._sorted) {
		addValue(val);
	}
}

double StatisticalSet::quantile(double q) const {
	if (std::ceil(q*_sorted.size()) == _sorted.size())
		return _max;
	if (std::ceil(_sorted.size()*q) == 1)
		return _min;
	if (std::floor(_sorted.size()*q) == _sorted.size()*q)
		return 0.5*(_sorted[_sorted.size()*q-1]+_sorted[_sorted.size()*q]);
	return _sorted[std::ceil(_sorted.size()*q)-1];
}
