#include "experiment.h"
#include "odroidreader.h"
#include "environments.h"
#include <QJsonArray>

Experiment::Experiment(const QJsonObject &jo, const QVector<DataSeries *> &descs, const Environments& envs)
	: wasRun(false), data(descs)
{
	title = jo["title"].toString();
	prepare = jo["prepare"].toString();
	cleanup = jo["cleanup"].toString();
	command = jo["command"].toString();
	QJsonArray esets = jo["environment_sets"].toArray();
	for (int i = 0; i < esets.size(); i++) {
		envSets.append(envs.findSet(esets.at(i).toString()));
	}

	QJsonArray ja = jo["environments"].toArray();
	for (int i = 0; i < ja.size(); i++) {
		const QJsonObject& o = ja.at(i).toObject();
		Environment* e = new Environment();
		e->label = o["label"].toString();
		e->big = o["big"].toBool();
		e->little = o["little"].toBool();
		e->freq =  o["frequency"].toInt();
		e->freq_max = o["frequency_max"].toInt();
		e->freq_min = o["frequency_min"].toInt();
		e->governor = o["governor"].toString();
		if (o.contains("runs")) {
			QJsonArray ra = o["runs"].toArray();
			for (int j = 0; j < ra.size(); j++) {
				QJsonObject ro = ra.at(j).toObject();
				QPair<double,double> v;
				v.first = ro["from"].toDouble();
				v.second = ro["to"].toDouble();
				runs[e].push_back(v);
				wasRun = true;
			}
		}
	}
	cooldown_time = jo["cooldown_time"].toInt();
	tail_time = jo["tail_time"].toInt();
}

void Experiment::write(QJsonObject& jo, bool withRuns) const {
	jo["title"] = title;
	jo["prepare"] = prepare;
	jo["cleanup"] = cleanup;
	jo["command"] = command;
	QJsonArray sets;
	for (const EnvironmentSet* s : envSets) {
		sets.push_back(s->name());
	}
	QJsonArray envs;
	for (const Environment* e : runs.keys()) {
		QJsonObject o;
		o["big"] = e->big;
		o["little"] = e->big;
		o["label"] = e->label;
		o["frequency"] = static_cast<qint64>(e->freq);
		o["frequency_max"] = static_cast<qint64>(e->freq_max);
		o["frequency_min"] = static_cast<qint64>(e->freq_min);
		o["governor"] = e->governor;
		if (withRuns) {
			QJsonArray runArray;
			for (QPair<double,double> run : runs[e]) {
				QJsonObject jr;
				jr["from"] = run.first;
				jr["to"] = run.second;
				runArray.push_back(jr);
			}
			o["runs"] = runArray;
		}
		envs.append(o);
	}
	jo["environment_sets"] = sets;
	jo["environments"] = envs;
	jo["cooldown_time"] = static_cast<qint64>(cooldown_time);
	jo["tail_time"] = static_cast<qint64>(tail_time);
}

QString Experiment::prepareMeasurement() {
	return prepare;
}

QString Experiment::startMeasurement(double time,const Environment* env) {
	runs[env].push_back(QPair<double,double>(time,time));
	qDebug() << "Returning command " << command;
	return command;
}

QString Experiment::cleanupMeasurement(float time, const Environment *env) {
	runs[env].back().second = time;
	wasRun = true;
	return cleanup;
}

bool Experiment::hasData() const { return wasRun; }
void Experiment::finishedCleanup(float) {}

StatisticalSet Experiment::aggregate(int unit, const Experiment* e) const {
	qDebug() << "Aggregating Experiment " << e->title << "Unit: " << unit;
	StatisticalSet s(e->data.at(unit)->descriptor);
	for (const Environment* env : runs.keys()) {
		qDebug() << "Extending Set" << env->description();
		s.extend(env->aggregate(unit,e));
	}
	return s;
}
