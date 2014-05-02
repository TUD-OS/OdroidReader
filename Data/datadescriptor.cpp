#include "datadescriptor.h"

unsigned int DataDescriptor::_uid_ctr = 0;

DataDescriptor::DataDescriptor(QString name, QString unit, double factor) :
	_name(name), _unit(unit), _factor(factor)
{
	_uuid = getUUID();
}

unsigned int DataDescriptor::getUUID() {
	return _uid_ctr;
}
