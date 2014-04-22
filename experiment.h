#ifndef EXPERIMENT_H
#define EXPERIMENT_H

#include <QString>
#include <QTextStream>
#include <datapoint.h>

class Experiment
{
public:
	QString title, prepare, cleanup, command, governor;
	quint32 tail_time, cooldown_time, freq_min, freq_max, freq;
	bool big, little;
	float start, end;

	void serialize(QTextStream& ts);
	Experiment(QTextStream& state);
	Experiment();
	std::vector<std::vector<Datapoint<double>*>> runs;
	QString prepareMeasurement(float time);
	QString cleanupMeasurement(float time);
	QString startMeasurement(float time);
	void finishedCleanup(float time);
};

#endif // EXPERIMENT_H
