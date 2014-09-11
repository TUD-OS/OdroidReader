#ifndef STATISTICALSET_H
#define STATISTICALSET_H

#include <QList>
#include <Data/datadescriptor.h>

class StatisticalSet
{
private:
	const DataDescriptor* descriptor;
	QList<double> _sorted;
	double _min, _max, _avg, _stddev;
	bool haveStdDev;
	double _timeAvg;
	int timeCnt;
public:
	StatisticalSet(const DataDescriptor* ds);
	void addValue(double val);
	void addTime(double t);
	double getStdDev();
	void extend(const StatisticalSet& s);
	double quantile(double q) const;
	double min() const { return quantile(0); }
	double max() const { return quantile(1); }
	double avg() const { return _avg; }
	double avgTime() const { return _timeAvg; }
	double median() const { return quantile(0.5); } //return (_sorted.size() > 0)?_sorted[_sorted.size()/2]:0; }
};

#endif // STATISTICALSET_H
