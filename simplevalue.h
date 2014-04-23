#ifndef SIMPLEVALUE_H
#define SIMPLEVALUE_H

#include <vector>
#include <algorithm>
#include <datapoint.h>
#include <QTableWidgetItem>

template<typename T>
class SimpleValue
{
protected:
	std::vector<std::pair<double,T>> _values;
	std::vector<T> _sorted;
	T _min,_max,_avg;

public:
	SimpleValue() { }
	SimpleValue(const SimpleValue& dp) = delete;
	SimpleValue& operator=(const SimpleValue& dp) = delete;
	SimpleValue(SimpleValue const& orig, double from, double to) {
		for (std::pair<double,T> p : orig._values)
			if (p.first >= from and p.first <= to) {
				add(p.second,p.first);
			}
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
		_values.push_back(std::pair<double,T>(time,value));
		auto low = std::lower_bound(_sorted.begin(),_sorted.end(),value);
		_sorted.insert(low,value);
		if (value > _max) _max = value;
		if (value < _min) _min = value;
		//Knuth/Wellford Algorithm
		_avg += (value-_avg)/_values.size();
	}
		//Adds the value and returns the new average
	std::vector<std::pair<double,T>> values() { return _values; }
};
#endif // SIMPLEVALUE_H
