#include "datasource.h"
#include <QDateTime>

double DataSource::localStart = 0;
QMap<const DataSource*,double> DataSource::offsets;

DataSource::DataSource(QString name, QObject* parent) :
    QObject(parent), _name(name) {}

void DataSource::execute(QString) {
	return;
}

void DataSource::setupEnvironment(const Experiment::Environment &) {
	return;
}

double DataSource::getGlobalTime(double time) {
    double curTime = QDateTime::currentMSecsSinceEpoch()/1000.0;
    if (localStart == 0) {
        localStart = curTime;
    }
    if (!offsets.contains(this))
        offsets.insert(this,curTime-time);
    return curTime-offsets[this]-time+(curTime-localStart);
}
