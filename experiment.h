#ifndef EXPERIMENT_H
#define EXPERIMENT_H

#include <string>
#include <vector>
#include <QJsonObject>
#include "Data/dataseries.h"
#include "Data/statisticalset.h"
#include <Data/o_datapoint.h>

class Experiment
{

private:
	int currentRun;
	bool wasRun;
public:
	const QVector<DataSeries*> &data;

public:
	class Environment {
		public:
			QString label, governor;
			uint32_t freq_min, freq_max, freq;
			bool big, little;
			QVector<QPair<double,double>> runs; //All datapoints for all repetitions
			QString description() const;
			StatisticalSet aggregate(int unit, const Experiment& e) const;
			StatisticalSet integral(int unit, const Experiment& e) const;
			DataSeries run(int unit, int run, const Experiment &e) const;
	};

	QString title, prepare, cleanup, command;
	uint32_t tail_time, cooldown_time;

	Experiment(QJsonObject &jo, const QVector<DataSeries*>& data);
	Experiment() = delete;
	void write(QJsonObject &jo) const;
	StatisticalSet aggregate(int unit, const Experiment& e) const;
	bool hasData() const;
	QVector<Environment> environments;
	QString prepareMeasurement();
	QString cleanupMeasurement(float time);
	QString startMeasurement(double time, int run);
	void finishedCleanup(float time);
};

#endif // EXPERIMENT_H
