#ifndef VALUE_H
#define VALUE_H
#include <vector>
#include <algorithm>
#include <datapoint.h>
#include <QTableWidgetItem>

template<class T>
class Value
{
private:
	std::vector<std::pair<double,T>> _values;
	std::vector<T> _sorted;
	T _min,_max,_avg;
	DatapointBase& parent;
	bool _max_changed, _min_changed;
	void clearColors() {
		for ( auto i : {parent.name_item, parent.max_item, parent.min_item, parent.avg_item, parent.unit_item})
			i->setBackgroundColor(Qt::white);
	}

public:
	Value(DatapointBase& parent) : parent(parent){}
	Value(Value const& orig, double from, double to) : parent(orig.parent) {
		for (std::pair<double,T> p : orig._values)
			if (p.first >= from and p.first <= to)
				add(p.second,p.first);
		qDebug() << "Constructed value copy" << _values.size() << "of" << orig._values.size();
	}

	T min() const { return _min; }
	T max() const { return _max; }
	T avg() const { return _avg; }
	int elements() const { return _values.size(); }
	T median() const { return (_sorted.size() > 0)?_sorted[_sorted.size()/2]:0; }
	T quantile(double q) const {
		if (std::ceil(q*_sorted.size()) == _sorted.size())
			return _max;
		if (std::ceil(_sorted.size()*q) == 1)
			return _min;
		if (std::floor(_sorted.size()*q) == _sorted.size()*q)
			return 0.5*(_sorted[_sorted.size()*q-1]+_sorted[_sorted.size()*q]);
		return _sorted[std::ceil(_sorted.size()*q)-1];
	}

	T last() const { return _values.back().second; }

	bool changed() const { return _values.size() > 1 && _values.back().second != _values.at(_values.size()-2).second; }

	void add(T value, double time) {
		//qDebug() << "[" << parent.name() << "] Value:" << value << "Factor:" << parent.factor() << "time:" << time;
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
		_values.push_back(std::pair<double,T>(time,value));
		auto low = std::lower_bound(_sorted.begin(),_sorted.end(),value);
		_sorted.insert(low,value);
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
	std::vector<std::pair<double,T>> values() { return _values; }
};

#endif // VALUE_H
