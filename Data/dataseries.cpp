#include "Data/dataseries.h"
#include <limits>
#include <cassert>
#include <QDebug>

DataSeries::DataSeries(const DataDescriptor *desc, QObject *parent)
	: QObject(parent), descriptor(desc),
	  _min(std::numeric_limits<double>::max()),
	  _max(std::numeric_limits<double>::min()),
	  _avg(0)
{}

//TODO: requires values to be added in time order
void DataSeries::addValue(double time, double value) {
	assert(timestamps.size() == 0 || time >= timestamps.last()); //If this is not true we have to recalculate avg :(
	value *= descriptor->factor();
	//qDebug() << descriptor->str() << ": Adding " << value << "at time" << time;
	if (value < _min) {
		_min = value;
		emit newMin(value);
	}
	if (value > _max) {
		_max = value;
		emit newMax(value);
	}
	double _oldavg = _avg;
	if (values.size() > 0) {
		double t = time - timestamps.last();
		double v1 = values.last();
		double avgval = v1*t+(value-v1)*t/2;
		_avg += (avgval-_avg)/values.size();
	}
	values.append(value);
	timestamps.append(time);
	if (_oldavg != _avg) {
		emit newAvg(_avg);
	}
	emit newValue(time,value);
	emit valuesUpdated(timestamps,values);
}
