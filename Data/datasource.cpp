#include "datasource.h"

DataSource::DataSource(QString name, QObject* parent) :
	QObject(parent), _name(name) {}

void DataSource::execute(QString) {
	return;
}

void DataSource::setupEnvironment(const Experiment::Environment &) {
	return;
}
