#ifndef SIMPLEVALUE_H
#define SIMPLEVALUE_H

#include <algorithm>
#include <Data/datapoint.h>
#include <QTableWidgetItem>

template<typename T>
class SimpleValue
{
protected:
	QVector<QPair<double,T>> _values;
	QVector<T> _sorted;
	T _min,_max,_avg;

public:
	SimpleValue() { }
	SimpleValue(const SimpleValue& orig) {
		_values = orig._values;
		_min = orig._min;
		_max = orig._max;
		_avg = orig._avg;
		_sorted = orig._sorted;
	}

	SimpleValue& operator=(const SimpleValue& dp) {
		_values = dp._values;
		_min = dp._min;
		_max = dp._max;
		_avg = dp._avg;
		_sorted = dp._sorted;
		return *this;
	}
	SimpleValue(SimpleValue const& orig, double from, double to, bool normalize = false) {
		for (QPair<double,T> p : orig._values)
			if (p.first >= from and p.first <= to) {
				add(p.second,(normalize)?p.first-from:p.first);
			}
	}
	void extend(const SimpleValue &by) {
		for (QPair<double,T> p : by._values)
				add(p.second,p.first);
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
		if (_values.empty())
			_max = _min = _avg = value;
		_values.append(QPair<double,T>(time,value));
		auto low = std::lower_bound(_sorted.begin(),_sorted.end(),value);
		_sorted.insert(low,value);
		if (value > _max) _max = value;
		if (value < _min) _min = value;
		//Knuth/Wellford Algorithm
		_avg += (value-_avg)/_values.size();
	}
		//Adds the value and returns the new average
	const QVector<QPair<double,T>> values() const { return _values; }
};
#endif // SIMPLEVALUE_H
