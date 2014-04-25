#ifndef EXPERIMENT_H
#define EXPERIMENT_H

#include <string>
#include <vector>
#include "datapoint.h"

class Experiment
{
private:
	int currentRun;

public:
	class Run {
		public:
			std::string label, governor;
			uint32_t freq_min, freq_max, freq;
			bool big, little;
			std::vector<std::pair<double,double>> repetitions; //All datapoints for all repetitions
	};

	std::string title, prepare, cleanup, command;
	//QString governor;
	uint32_t tail_time, cooldown_time;
	//uint32_t freq_min, freq_max, freq;
	//bool big, little;
	//float start, end;

	void serialize(QTextStream& ts);
	Experiment(QTextStream& state);
	Experiment();
	std::vector<Run> runs;
	std::string prepareMeasurement(float time);
	std::string cleanupMeasurement(float time);
	std::string startMeasurement(double time, int run);
	void finishedCleanup(float time);
};

#endif // EXPERIMENT_H
