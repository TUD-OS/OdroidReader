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
	const std::vector<Datapoint<double>*> *data;

public:
	class Environment {
		public:
			QString label, governor;
			uint32_t freq_min, freq_max, freq;
			bool big, little;
			std::vector<std::pair<double,double>> runs; //All datapoints for all repetitions
			QString description() const;
	};

	QString title, prepare, cleanup, command;
	uint32_t tail_time, cooldown_time;

	void write(QJsonObject &jo) const;
	Experiment(QJsonObject &jo, std::vector<Datapoint<double>*>* data);
	Experiment();
	bool hasData();
	QVector<Environment> environments;
	QString prepareMeasurement(float time);
	QString cleanupMeasurement(float time);
	QString startMeasurement(double time, int run);
	void finishedCleanup(float time);
};

#endif // EXPERIMENT_H
