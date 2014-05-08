#include "experiment.h"
#include "odroidreader.h"
#include <QJsonArray>

//Experiment::Experiment()
//{}

Experiment::Experiment(QJsonObject& jo, const QVector<DataSeries *> &descs)
	: wasRun(false), data(descs)
{
	title = jo["title"].toString();
	prepare = jo["prepare"].toString();
	cleanup = jo["cleanup"].toString();
	command = jo["command"].toString();
	QJsonArray ja = jo["environments"].toArray();
	for (int i = 0; i < ja.size(); i++) {
		const QJsonObject& o = ja.at(i).toObject();
		Experiment::Environment e;
		e.label = o["label"].toString();
		e.big = o["big"].toBool();
		e.little = o["little"].toBool();
		e.freq =  o["frequency"].toInt();
		e.freq_max = o["frequency_max"].toInt();
		e.freq_min = o["frequency_min"].toInt();
		e.governor = o["governor"].toString();
		environments.push_back(e);
	}
	cooldown_time = jo["cooldown_time"].toInt();
	tail_time = jo["tail_time"].toInt();
}

void Experiment::write(QJsonObject& jo) const {
	jo["title"] = title;
	jo["prepare"] = prepare;
	jo["cleanup"] = cleanup;
	jo["command"] = command;
	QJsonArray envs;
	for (const Experiment::Environment &e : environments) {
		QJsonObject o;
		o["big"] = environments.front().big;
		o["little"] = environments.front().big;
		o["label"] = e.label;
		o["frequency"] = static_cast<qint64>(e.freq);
		o["frequency_max"] = static_cast<qint64>(e.freq_max);
		o["frequency_min"] = static_cast<qint64>(e.freq_min);
		o["governor"] = e.governor;
		envs.append(o);
	}
	jo["environments"] = envs;
	jo["cooldown_time"] = static_cast<qint64>(cooldown_time);
	jo["tail_time"] = static_cast<qint64>(tail_time);
}

QString Experiment::prepareMeasurement(float) {
	return prepare;
}

QString Experiment::startMeasurement(double time,int run) {
	currentRun = run;
	environments[run].runs.append(QPair<double,double>(time,time));
	return command;
}

QString Experiment::cleanupMeasurement(float time) {
	environments[currentRun].runs.back().second = time;
	wasRun = true;
	return cleanup;
}

bool Experiment::hasData() { return wasRun; }

QString Experiment::Environment::description() const {
	QString desc("%1 MHz[%2-%3 MHz]@%4 -> %5");
	QString bl = "?";
	if (big && little) {
		bl = "big.LITTLE";
	} else {
		bl = big?"big":"LITTLE";
	}
	return desc.arg(QString::number(freq),QString::number(freq_min),QString::number(freq_max),governor,bl);
}

void Experiment::finishedCleanup(float) {}

SimpleValue<double> Experiment::Environment::integral(int unit, const Experiment &e) const {
	SimpleValue<double> allRuns;
	for (int i = 0; i < runs.size(); i++) {
		double value = -1;
		QPair<double,double> last;
		for (const QPair<double,double> &p : run(unit,i,e).values()) {
			if (value == -1) {
				last = p;
				value = 0;
				continue;
			}
			double t = p.first-last.first;
			value += t*last.second+t*(p.second-last.second)/2;
			last = p;
		}
		allRuns.add(value,0);
	}
	return allRuns;
}

SimpleValue<double> Experiment::Environment::aggregate(int unit, const Experiment &e) const {
	SimpleValue<double> allRuns;
	for (int i = 0; i < runs.size(); i++) {
		allRuns.extend(run(unit,i,e));
	}
	return allRuns;
}

SimpleValue<double> Experiment::aggregate(int unit, const Experiment &e) const {
	SimpleValue<double> allEnvironments;
	for (const Environment &env : environments) {
		allEnvironments.extend(env.aggregate(unit,e));
	}
	return allEnvironments;
}

SimpleValue<double> Experiment::Environment::run(int unit, int run, const Experiment &e, bool normalize) const {
	return SimpleValue<double>(); //e.data->at(unit)->value(),runs.at(run).first,runs.at(run).second,normalize);
}
