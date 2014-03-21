#ifndef DATAPOINT_H
#define DATAPOINT_H

#include <QByteArray>
#include <QTableWidgetItem>
#include <qwt_plot_curve.h>

typedef enum : quint8 {
	CHAR = 0, FLOAT = 1, UINT16T = 2, BIGLITTLE = 3, UINT32T =4
} FIELD_TYPE;

class DatapointBase
{
protected:
	DatapointBase(QByteArray ba);
public:
	virtual ~DatapointBase();
	QwtPlotCurve* pc;
	QVector<QPointF> samples;
	const QString& name() const { return _name; }
	const QString& unit() const { return _unit; }
	quint32 factor() const { return _factor; }
	FIELD_TYPE type() const { return _type; }
	QTableWidgetItem *name_item, *unit_item, *min_item, *max_item, *avg_item, *last_item;

private:
	FIELD_TYPE _type;
	quint32 _factor;
	QString _name;
	QString _unit; //always 3 chars
	bool initialized;
};

#include "value.h"
template<class T> class Value;

template<class T>
class Datapoint: public DatapointBase
{
	Value<T> _value;
public:
	Datapoint(QByteArray ba) : DatapointBase(ba), _value(*this) {  }
	const Value<T> value() { return _value; }
	void addValue(T v,float time) { _value.add(v,time); }
};

#endif // DATAPOINT_H
