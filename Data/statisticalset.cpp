#include "Data/statisticalset.h"

#include <algorithm>
#include <QDebug>
StatisticalSet::StatisticalSet(const DataDescriptor* ds) : descriptor(ds), haveStdDev(false), _avg(0), _timeAvg(0), timeCnt(0)
{}

void StatisticalSet::addValue(double value) {
	haveStdDev = false;
	auto low = std::lower_bound(_sorted.begin(),_sorted.end(),value);
	double _oldavg = _avg;
	if (_sorted.size() > 0) {
		double avgval = value;
		_avg += (avgval-_avg)/(_sorted.size()+1);
	} else {
		_avg = value;
	}
	_sorted.insert(low,value);
}

void StatisticalSet::addTime(double t) {
	_timeAvg += (t-_timeAvg)/(++timeCnt);
}

#define square(i) ((i)*(i))

double StatisticalSet::getStdDev() {
	if (haveStdDev) return _stddev;
	double i = 0;
	for (double v : _sorted) {
		_stddev = (square(i)/square(i+1))*_stddev+(square(_avg-v)/square(i+1));
		i++;
	}
	_stddev = sqrt(_stddev);
	haveStdDev = true;
	return _stddev;
}


void StatisticalSet::extend(const StatisticalSet& s) {
	haveStdDev = false;
	for (double val : s._sorted) {
		addValue(val);
	}
}

double StatisticalSet::quantile(double q) const {
	if (_sorted.size() == 0) return 0;
	if (q == 1) return _sorted.last();
	if (q == 0) return _sorted.first();
	if (std::floor(_sorted.size()*q) == _sorted.size()*q)
		return 0.5*(_sorted[_sorted.size()*q-1]+_sorted[_sorted.size()*q]);
	return _sorted[std::ceil(_sorted.size()*q)-1];
}
