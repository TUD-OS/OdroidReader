#ifndef EXPERIMENT_H
#define EXPERIMENT_H

#include <string>
#include <vector>
#include <QJsonObject>
#include "datapoint.h"

class Experiment
{

private:
	int currentRun;
	bool wasRun;
public:
	const QVector<Datapoint<double>*> *data;

public:
	class Environment {
		public:
			QString label, governor;
			uint32_t freq_min, freq_max, freq;
			bool big, little;
			QVector<QPair<double,double>> runs; //All datapoints for all repetitions
			QString description() const;
			SimpleValue<double> aggregate(int unit, const Experiment& e) const;
			SimpleValue<double> run(int unit, int run, const Experiment &e) const;
	};

	QString title, prepare, cleanup, command;
	uint32_t tail_time, cooldown_time;

	void write(QJsonObject &jo) const;
	Experiment(QJsonObject &jo, QVector<Datapoint<double> *> *data);
	Experiment();
	SimpleValue<double> aggregate(int unit, const Experiment& e) const;
	bool hasData();
	QVector<Environment> environments;
	QString prepareMeasurement(float time);
	QString cleanupMeasurement(float time);
	QString startMeasurement(double time, int run);
	void finishedCleanup(float time);
};

#endif // EXPERIMENT_H
