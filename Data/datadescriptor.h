#ifndef DATADESCRIPTOR_H
#define DATADESCRIPTOR_H
#include <QString>
#include <QVector>
#include <QJsonObject>

class DataDescriptor
{
public:
	enum class Type {
		CHAR, FLOAT, UINT16T, BIGLITTLE, UINT32T
	};

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
	QJsonObject json() const;
	static Type typeFromId(quint8 id);
	DataDescriptor(QString name, QString unit, double factor, Type t);
	explicit DataDescriptor(const QJsonObject& jo);
	//No copies!
	DataDescriptor& operator=(const DataDescriptor& dd) = delete;
	DataDescriptor(const DataDescriptor& dd) = delete;
};

#endif // DATADESCRIPTOR_H
