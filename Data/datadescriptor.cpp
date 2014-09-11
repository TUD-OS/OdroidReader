#include "Data/datadescriptor.h"
#include <type_traits>
#include <QDebug>

unsigned int DataDescriptor::_uid_ctr = 0;

DataDescriptor::DataDescriptor(QString name, QString unit, double factor, Type t) :
	_name(name), _unit(unit), _factor(factor), _type(t)
{
	_uuid = getUUID();
}

DataDescriptor::DataDescriptor(const QJsonObject &jo) {
	_uuid = getUUID();
	_name = jo["name"].toString();
	_unit = jo["unit"].toString();
	_factor = jo["factor"].toDouble();
	_type = static_cast<Type>(jo["type"].toInt());
}

DataDescriptor::Type DataDescriptor::typeFromId(quint8 id) {
	switch (id) {
		case 0: return Type::CHAR;
		case 1: return Type::FLOAT;
		case 2: return Type::UINT16T;
		case 3: return Type::BIGLITTLE;
		default: return Type::UINT32T;
	}
}

QString DataDescriptor::str() const {
	QString test("%1 [%2] * %3");
	return test.arg(_name,_unit,QString::number(_factor));
}

unsigned int DataDescriptor::getUUID() {
	return _uid_ctr++;
}

QJsonObject DataDescriptor::json() const {
	QJsonObject json;
	json["name"] = _name;
	json["unit"] = _unit;
	json["factor"] = _factor;
	json["type"] = static_cast<std::underlying_type<Type>::type>(_type);
	return json;
}
