#ifndef EXPERIMENT_H
#define EXPERIMENT_H

#include <QString>
#include <QTextStream>

class Experiment
{
public:
	QString title, prepare, cleanup, command, governor;
	quint32 tail_time, cooldown_time, freq_min, freq_max, freq;
	bool big, little;

	void serialize(QTextStream& ts);
	Experiment(QTextStream& state);
	Experiment();
};

#endif // EXPERIMENT_H
