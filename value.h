#ifndef VALUE_H
#define VALUE_H
#include <vector>
#include <datapoint.h>
#include <QTableWidgetItem>

template<class T>
class Value
{
private:
	std::vector<T> _values;
	T _min,_max,_avg;
	DatapointBase& parent;
	bool _max_changed, _min_changed;
	void clearColors() {
		for ( auto i : {parent.name_item, parent.max_item, parent.min_item, parent.avg_item, parent.unit_item})
			i->setBackgroundColor(Qt::white);
	}

public:
	Value(DatapointBase& parent) : parent(parent){}

	T min() { return _min; }

	T max() { return _max; }

	T avg() { return _avg; }

	T last() const { return _values.back(); }

	void add(T value, float time) {
		qDebug() << "[" << parent.name() << "] Value:" << value << "Factor:" << parent.factor() << "time:" << time;
		value *= parent.factor();
		parent.samples.append(QPointF(time,value));
		parent.pc->setSamples(parent.samples);
		if (_values.empty()) {
			_max = _min = _avg = value;
			parent.min_item->setText(QString::number(value));
			parent.max_item->setText(QString::number(value));
			parent.avg_item->setText(QString::number(value));
			parent.last_item->setText(QString::number(value));
		}
		_values.push_back(value);
		if (value > _max) {
			if (_max == _min) clearColors();
			parent.last_item->setBackgroundColor(Qt::red);
			_max = value;
			parent.max_item->setText(QString::number(value));
			_max_changed = true;
		}
		if (value < _min) {
			if (_max == _min) clearColors();
			parent.last_item->setBackgroundColor(Qt::green);
			_min = value;
			parent.min_item->setText(QString::number(value));
			_min_changed = true;
		}
		//Knuth/Wellford Algorithm
		_avg += (value-_avg)/_values.size();
		parent.avg_item->setText(QString::number(_avg));
		parent.last_item->setText(QString::number(value));
		if (_min != _max && _max != value && _min != value)
			parent.last_item->setBackgroundColor(Qt::white);
}

	//Adds the value and returns the new average
	std::vector<T> values() { return _values; }
};

#endif // VALUE_H
