#ifndef VALUE_H
#define VALUE_H

#include "simplevalue.h"
#include <QTableWidgetItem>

template<typename T>
class Value : public SimpleValue<T>
{
private:
	DatapointBase& parent;
	void clearColors() {
		for ( auto i : {parent.name_item, parent.max_item, parent.min_item, parent.avg_item, parent.unit_item})
			i->setBackgroundColor(Qt::white);
	}
	using SimpleValue<T>::_values;
	using SimpleValue<T>::_sorted;
	using SimpleValue<T>::_min;
	using SimpleValue<T>::_max;
	using SimpleValue<T>::_avg;

public:
	Value(DatapointBase& parent) : parent(parent){}
	Value(Value const& orig, double from, double to) : SimpleValue<T>(orig,from,to), parent(orig.parent) {}
	Value(const Value& orig) = delete;
	Value& operator=(const Value& orig) = delete;

	void add(T value, double time) {
		if (_values.empty()) {
			parent.min_item->setText(QString::number(value));
			parent.max_item->setText(QString::number(value));
			parent.avg_item->setText(QString::number(value));
			parent.last_item->setText(QString::number(value));
		}
		SimpleValue<T>::add(value,time);
		_values.append(QPair<double,T>(time,value));
		auto low = std::lower_bound(_sorted.begin(),_sorted.end(),value);
		_sorted.insert(low,value);
		if (value == _max) {
			if (_max == _min) clearColors();
			parent.last_item->setBackgroundColor(Qt::red);
			parent.max_item->setText(QString::number(value));
		}
		if (value == _min) {
			if (_max == _min) clearColors();
			parent.last_item->setBackgroundColor(Qt::green);
			parent.min_item->setText(QString::number(value));
		}
		//Knuth/Wellford Algorithm
		_avg += (value-_avg)/_values.size();
		parent.avg_item->setText(QString::number(_avg));
		parent.last_item->setText(QString::number(value));
		if (_min != _max && _max != value && _min != value)
			parent.last_item->setBackgroundColor(Qt::white);
	}

	//Adds the value and returns the new average
	QVector<QPair<double,T>> values() { return _values; }
};

#endif // VALUE_H
