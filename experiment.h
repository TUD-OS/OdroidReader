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

public:
	class Environment {
		public:
			std::string label, governor;
			uint32_t freq_min, freq_max, freq;
			bool big, little;
			std::vector<std::pair<double,double>> runs; //All datapoints for all repetitions
			QString description() const;
	};

	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int columnCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

	std::string title, prepare, cleanup, command;
	//QString governor;
	uint32_t tail_time, cooldown_time;
	//uint32_t freq_min, freq_max, freq;
	//bool big, little;
	//float start, end;

	void write(QJsonObject &jo) const;
	Experiment(QJsonObject &jo);
	Experiment();
	QVector<Environment> environments;
	std::string prepareMeasurement(float time);
	std::string cleanupMeasurement(float time);
	std::string startMeasurement(double time, int run);
	void finishedCleanup(float time);
};

#endif // EXPERIMENT_H
