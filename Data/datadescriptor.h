#ifndef DATADESCRIPTOR_H
#define DATADESCRIPTOR_H

#include <QString>

class DataDescriptor
{
private:
	QString _name, _unit;
	double _factor;
	unsigned int _uuid;

	static unsigned int _uid_ctr;
	static unsigned int getUUID();
public:
	inline const QString& name() { return _name; }
	inline const QString& unit() { return _unit; }
	inline double factor() { return _factor; }
	inline double uid() { return _uuid; }
	DataDescriptor(QString name, QString unit, double factor);
	//No copies!
	DataDescriptor& operator=(const DataDescriptor& dd) = delete;
	DataDescriptor(const DataDescriptor& dd) = delete;
};

#endif // DATADESCRIPTOR_H
