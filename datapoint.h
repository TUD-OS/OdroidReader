#ifndef DATAPOINT_H
#define DATAPOINT_H

#include <QByteArray>
#include <QVector>
#include <QTableWidgetItem>
#include "qcustomplot.h"

typedef enum : quint8 {
	CHAR = 0, FLOAT = 1, UINT16T = 2, BIGLITTLE = 3, UINT32T =4
} FIELD_TYPE;

class DatapointBase
{
protected:
	DatapointBase(QByteArray ba);
	DatapointBase(const DatapointBase& src);
	uint32_t _factor;
public:
	virtual ~DatapointBase();
	const QString& name() const { return _name; }
	const QString& unit() const { return _unit; }
	quint32 factor() const { return _factor; }
	FIELD_TYPE type() const { return _type; }
	QTableWidgetItem *name_item, *unit_item, *min_item, *max_item, *avg_item, *last_item;

private:
	FIELD_TYPE _type;
	QString _name, _unit; //always 3 chars
	bool initialized;
};

#include "value.h"
template<class T> class Value;

template<class T>
class Datapoint: public DatapointBase
{
	Datapoint(const Datapoint& dp) = delete;
	Datapoint& operator=(const Datapoint& dp) = delete;
	Value<T> _value;
	QVector<double> keys;
	QVector<T> values;
public:
	QCPGraph* graph;
	Datapoint(QCustomPlot *p, QByteArray ba) : DatapointBase(ba), _value(*this) {
		graph = p->addGraph();
		graph->setVisible(false);
	}
	Datapoint(QCustomPlot *p, Datapoint<double>& orig,float from, float to)
		: DatapointBase(p,orig,from,to), _value(orig._value,from,to)
	{
		T min = INFINITY, max = -INFINITY, avg = 0, last = 0;
		size_t no = 0;
		for (size_t i = 0; i < orig.keys.size(); i++) {
			if (orig.keys.at(i) >= from and orig.keys.at(i) <= to) {
				if (min > orig.values.at(i)) min = orig.values.at(i);
				if (max < orig.values.at(i)) max = orig.values.at(i);
				avg += (orig.values.at(i)-avg)/++no;
				last = orig.values.at(i);
				values.push_back(orig.values.at(i));
				keys.push_back(orig.keys.at(i));
			}
		}
		last_item = new QTableWidgetItem(QString::number(last));
		min_item = new QTableWidgetItem(QString::number(min));
		max_item = new QTableWidgetItem(QString::number(max));
		avg_item = new QTableWidgetItem(QString::number(avg));
	}

	const Value<T>& value() const { return _value; }
	void addValue(T v,double time) {
		values.append(v);
		keys.append(time);
		graph->setData(keys,values);
		_value.add(v*_factor,time);
	}
};

#endif // DATAPOINT_H
