#include "experiment.h"
#include "odroidreader.h"

Experiment::Experiment()
{}

Experiment::Experiment(QTextStream& state) {
	title = state.readLine();
	big = state.readLine().toInt();
	little = state.readLine().toInt();;
	prepare = state.readLine();
	cleanup = state.readLine();
	command = state.readLine();
	freq = state.readLine().toInt();;
	freq_max = state.readLine().toInt();
	freq_min = state.readLine().toInt();
	governor = state.readLine();
	cooldown_time = state.readLine().toInt();
	tail_time = state.readLine().toInt();
	qDebug() << "Read:" << title << "|" << big << "|" << little << "|" << prepare << "|" << cleanup << "|" << command << "|" << freq << "|" << freq_max << "|" << freq_min << "|" << governor;
}

void Experiment::serialize(QTextStream& ts) {
	ts << title << "\n";
	ts << (int)big << "\n";
	ts << (int)little << "\n";
	ts << prepare << "\n" << cleanup << "\n" << command << "\n";
	ts << freq << "\n" << freq_max << "\n" << freq_min << "\n";
	ts << governor << "\n";
	ts << cooldown_time << "\n";
	ts << tail_time << "\n";
	qDebug() << "Wrote:" << title << "|" << big << "|" << little << "|" << prepare << "|" << cleanup << "|" << command << "|" << freq << "|" << freq_max << "|" << freq_min << "|" << governor;
}

QString Experiment::prepareMeasurement(float) {
	return prepare;
}

QString Experiment::startMeasurement(float time) {
	start = time;
	runs.push_back(std::vector<Datapoint<double>*>());
	return command;
}

QString Experiment::cleanupMeasurement(float time) {
	end = time;
	for (Datapoint<double>* d : OdroidReader::descs) {
		runs.back().push_back(new Datapoint<double>(*d,start,end));
	}
	return cleanup;
}

void Experiment::finishedCleanup(float) {}
