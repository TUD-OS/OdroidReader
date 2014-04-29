#include "experiment.h"
#include "odroidreader.h"
#include <QJsonArray>

Experiment::Experiment()
{}

Experiment::Experiment(QJsonObject& jo)
{
	title = jo["title"].toString().toStdString();
	prepare = jo["prepare"].toString().toStdString();
	cleanup = jo["cleanup"].toString().toStdString();
	command = jo["command"].toString().toStdString();
	QJsonArray ja = jo["environments"].toArray();
	for (int i = 0; i < ja.size(); i++) {
		const QJsonObject& o = ja.at(i).toObject();
		Experiment::Environment e;
		e.label = o["label"].toString().toStdString();
		e.big = o["big"].toBool();
		e.little = o["little"].toBool();
		e.freq =  o["frequency"].toInt();
		e.freq_max = o["frequency_max"].toInt();
		e.freq_min = o["frequency_min"].toInt();
		e.governor = o["governor"].toString().toStdString();
		environments.push_back(e);
	}
	cooldown_time = jo["cooldown_time"].toInt();
	tail_time = jo["tail_time"].toInt();
}

void Experiment::write(QJsonObject& jo) const {
	jo["title"] = QString::fromStdString(title);
	jo["prepare"] = QString::fromStdString(prepare);
	jo["cleanup"] = QString::fromStdString(cleanup);
	jo["command"] = QString::fromStdString(command);
	QJsonArray envs;
	for (const Experiment::Environment &e : environments) {
		QJsonObject o;
		o["big"] = environments.front().big;
		o["little"] = environments.front().big;
		o["label"] = QString::fromStdString(e.label);
		o["frequency"] = static_cast<qint64>(e.freq);
		o["frequency_max"] = static_cast<qint64>(e.freq_max);
		o["frequency_min"] = static_cast<qint64>(e.freq_min);
		o["governor"] = QString::fromStdString(e.governor);
		envs.append(o);
	}
	jo["environments"] = envs;
	jo["cooldown_time"] = static_cast<qint64>(cooldown_time);
	jo["tail_time"] = static_cast<qint64>(tail_time);
}

std::string Experiment::prepareMeasurement(float) {
	return prepare;
}

std::string Experiment::startMeasurement(double time,int run) {
	currentRun = run;
	environments[run].runs.push_back(std::pair<double,double>(time,time));
	return command;
}

std::string Experiment::cleanupMeasurement(float time) {
	environments[currentRun].runs.back().second = time;
	return cleanup;
}

QString Experiment::Environment::description() const {
	QString desc("%1 MHz[%2-%3 MHz]@%4 -> %5");
	QString bl = "?";
	if (big && little) {
		bl = "big.LITTLE";
	} else {
		bl = big?"big":"LITTLE";
	}
	return desc.arg(QString::number(freq),QString::number(freq_min),QString::number(freq_max),QString::fromStdString(governor),bl);
}

void Experiment::finishedCleanup(float) {}

int Experiment::rowCount(const QModelIndex &) const {
	return environments.size();
}

int Experiment::columnCount(const QModelIndex &) const {
	return 6;
}

QVariant Experiment::data(const QModelIndex &index, int role) const {
	if (role == Qt::DisplayRole) {
		switch (index.column()) {
			case 0: return environments.at(index.row()).freq;
		}

		return QString("Row %1, Col %2").arg(index.row()+1).arg(index.column()+1);
	}
	return QVariant();
}
