#ifndef DATADESCRIPTOR_H
#define DATADESCRIPTOR_H

#include <QString>

class DataDescriptor
{
public:
	typedef enum class {
		CHAR = 0, FLOAT = 1, UINT16T = 2, BIGLITTLE = 3, UINT32T =4
	} Type;

private:
	QString _name, _unit;
	double _factor;
	unsigned int _uuid;
	Type _type;

	static unsigned int _uid_ctr;
	static unsigned int getUUID();
public:
	inline const QString& name() const { return _name; }
	inline const QString& unit() const { return _unit; }
	inline double factor() const { return _factor; }
	inline unsigned int uid() const { return _uuid; }
	inline Type type() const { return _type; }
	QString str() const;
	static Type typeFromId(quint8 id);
	DataDescriptor(QString name, QString unit, double factor, Type t);
	//No copies!
	DataDescriptor& operator=(const DataDescriptor& dd) = delete;
	DataDescriptor(const DataDescriptor& dd) = delete;
};

#endif // DATADESCRIPTOR_H
