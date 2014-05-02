#include "datasource.h"

DataSource::DataSource(QString name, QObject* parent) :
	QObject(parent), _name(name) {}
