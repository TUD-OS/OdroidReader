#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <QString>
#include <QPair>
#include <QObject>
#include "Data/statisticalset.h"
#include "Data/dataseries.h"

class Experiment;

class Environment : public QObject
{
public:
		Environment(QObject* parent = 0) : QObject(parent) {}
		Environment(const QJsonObject &o,QObject* parent = 0);
		QString label, governor;
		uint freq_min, freq_max, freq;
		bool big, little;
		QString description() const;
		StatisticalSet aggregate(int unit, const Experiment *e) const;
		StatisticalSet integral(int unit, const Experiment *e) const;
		DataSeries run(int unit, int run, const Experiment *e) const;
		bool operator<(const Environment& o) const;
		void write(QJsonObject& o) const;
};

#endif // ENVIRONMENT_H
