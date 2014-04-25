#include "experiment.h"
#include <iostream>
#include "odroidreader.h"

Experiment::Experiment()
{}

Experiment::Experiment(QTextStream& state) {
	qDebug() << "Constructor!";
	title = state.readLine().toStdString();
	runs.push_back(Experiment::Run()); //TODO! This needs to be read
	runs.front().big = state.readLine().toInt(); //TODO: This needs to be fixed for multi-run support
	runs.front().little = state.readLine().toInt();;
	prepare = state.readLine().toStdString();
	cleanup = state.readLine().toStdString();
	command = state.readLine().toStdString();
	runs.front().freq = state.readLine().toInt();;
	runs.front().freq_max = state.readLine().toInt();
	runs.front().freq_min = state.readLine().toInt();
	runs.front().governor = state.readLine().toStdString();
	cooldown_time = state.readLine().toInt();
	tail_time = state.readLine().toInt();
	std::cerr << "Read: " << title << " | " << runs.front().big << " | " << runs.front().little << " | " << prepare << " | " << cleanup << " | " << command << " | "
			 << runs.front().freq << " | " << runs.front().freq_max << " | " << runs.front().freq_min << " | " << runs.front().governor;
}

void Experiment::serialize(QTextStream& ts) {
	ts << QString::fromStdString(title) << "\n";
	ts << (int)runs.front().big << "\n"; //TODO!
	ts << (int)runs.front().little << "\n"; //TODO!
	ts << QString::fromStdString(prepare) << "\n" << QString::fromStdString(cleanup) << "\n" << QString::fromStdString(command) << "\n";
	ts << runs.front().freq << "\n" << runs.front().freq_max << "\n" << runs.front().freq_min << "\n"; //TODO
	ts << QString::fromStdString(runs.front().governor) << "\n";
	ts << cooldown_time << "\n";
	ts << tail_time << "\n";
	std::cerr << "Wrote: " << title << " | " << runs.front().big << " | " << runs.front().little << " | " << prepare << " | " << cleanup << " | " << command << " | "
			  << runs.front().freq << "|" << runs.front().freq_max << "|" << runs.front().freq_min << "|" << runs.front().governor;
}

std::string Experiment::prepareMeasurement(float) {
	return prepare;
}

std::string Experiment::startMeasurement(double time,int run) {
	currentRun = run;
	runs.at(run).repetitions.push_back(std::pair<double,double>(time,time));
	return command;
}

std::string Experiment::cleanupMeasurement(float time) {
	runs.at(currentRun).repetitions.back().second = time;
	return cleanup;
}

void Experiment::finishedCleanup(float) {}
