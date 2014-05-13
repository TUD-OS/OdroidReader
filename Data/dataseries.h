#ifndef DATASERIES_H
#define DATASERIES_H

#include <Data/datadescriptor.h>
#include <QObject>
#include <QVector>
#include <QPair>

class DataSeries : public QObject
{
	Q_OBJECT
public:
	const DataDescriptor *descriptor;
private:
	QVector<double> timestamps;
	QVector<double> values;
	double _min, _max, _avg;

public:
	explicit DataSeries(const DataDescriptor* desc, QObject *parent = nullptr);
	DataSeries(const DataSeries& src, double from, double to, bool timeAdjust = true);
	DataSeries(const DataSeries&);
	void addValue(double time, double value, bool scale = true);
	const QVector<double>& getTimestamps() { return timestamps; }
	const QVector<double>& getValues() { return values; }
	double getAvg() const { return _avg; }
	double getMin() const { return _min; }
	double getMax() const { return _max; }
    double getLast() const { return (values.size()==0)?NAN:values.last(); }
signals:
	void newMax(double max);
	void newMin(double min);
	void newAvg(double avg);
	void valuesUpdated(const QVector<double> &timestamps, const QVector<double>& values);
	void newValue(double time, double value);
};

#endif // DATASERIES_H
