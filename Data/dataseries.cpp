#include "Data/dataseries.h"
#include <limits>
#include <cassert>
#include <QDebug>
#include <QJsonArray>

DataSeries::DataSeries(const DataDescriptor *desc, QObject *parent)
	: QObject(parent), descriptor(desc),
	  _min(std::numeric_limits<double>::max()),
	  _max(std::numeric_limits<double>::min()),
	  _avg(0)
{}

DataSeries::DataSeries(const DataSeries &src) : QObject(src.parent()), descriptor(src.descriptor) {
	timestamps = src.timestamps;
	values = src.values;
	_min = src._min;
	_max = src._max;
	_avg = src._avg;
}

DataSeries::DataSeries(const DataSeries &src, double from, double to, bool timeAdjust) :
	descriptor(src.descriptor)
{
	int ctr = 0;
	for (int i = 0; i < src.timestamps.size(); i++) {
		double time = src.timestamps.at(i);
		if (time >= from && time <= to) {
			addValue((timeAdjust)?time-from:time,src.values.at(i),false);
			ctr++;
		}
	}
	//qDebug() << "Copied " << ctr << "values [" << from << "-" << to << "] of" << src.timestamps.size();
}

//TODO: requires values to be added in time order
void DataSeries::addValue(double time, double value, bool scale) {
	if (timestamps.size() > 0 && time < timestamps.last())  {//qDebug() << time << "vs." << timestamps.last();
		qWarning() << time << timestamps.last();
	}
	if (scale) value *= descriptor->factor();
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

QJsonObject DataSeries::json() const {
	QJsonArray vals, times;
	for (double value : values)
		vals.push_back(value);
	for (double time : timestamps)
		times.push_back(time);
	QJsonObject jo;
	jo["values"] = vals;
	jo["timestamps"] = times;
	return jo;
}

void DataSeries::fromJson(const QJsonObject &jo) {
	QJsonArray times = jo["timestamps"].toArray();
	QJsonArray vals = jo["values"].toArray();
	for (int i = 0; i < times.size(); i++)
		addValue(times.at(i).toDouble(),vals.at(i).toDouble(),false);
}
