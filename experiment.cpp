#include "experiment.h"
#include "odroidreader.h"

Experiment::Experiment()
{}

Experiment::Experiment(QJsonObject& jo) {
	title = jo["title"].toString().toStdString();
	runs.push_back(Experiment::Run());
	runs.front().big = jo["big"].toBool();
	runs.front().little = jo["little"].toBool();
	prepare = jo["prepare"].toString().toStdString();
	cleanup = jo["cleanup"].toString().toStdString();
	command = jo["command"].toString().toStdString();
	runs.front().freq = jo["frequency"].toInt();
	runs.front().freq_max = jo["frequency_max"].toInt();
	runs.front().freq_min = jo["frequency_min"].toInt();
	runs.front().governor = jo["governor"].toString().toStdString();
	cooldown_time = jo["cooldown_time"].toInt();
	tail_time = jo["tail_time"].toInt();
}

void Experiment::write(QJsonObject& jo) {
	jo["title"] = QString::fromStdString(title);
	jo["big"] = runs.front().big;
	jo["little"] = runs.front().big;
	jo["prepare"] = QString::fromStdString(prepare);
	jo["cleanup"] = QString::fromStdString(cleanup);
	jo["command"] = QString::fromStdString(command);
	jo["frequency"] = static_cast<qint64>(runs.front().freq);
	jo["frequency_max"] = static_cast<qint64>(runs.front().freq_max);
	jo["frequency_min"] = static_cast<qint64>(runs.front().freq_min);
	jo["governor"] = QString::fromStdString(runs.front().governor);
	jo["cooldown_time"] = static_cast<qint64>(cooldown_time);
	jo["tail_time"] = static_cast<qint64>(tail_time);
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
