#include "experiment.h"

Experiment::Experiment()
{}

Experiment::Experiment(QTextStream& state) {
	title = state.readLine();
	int tmp;
	state >> tmp; big = tmp;
	state >> tmp; little = tmp;
	prepare = state.readLine();
	cleanup = state.readLine();
	command = state.readLine();
	state >> freq >> freq_max >> freq_min >> governor;
}

void Experiment::serialize(QTextStream& ts) {
	ts << title << "\n";
	ts << (int)big << (int)little;
	ts << prepare << "\n" << cleanup << "\n" << command << "\n";
	ts << freq << freq_max << freq_min << governor;
}
