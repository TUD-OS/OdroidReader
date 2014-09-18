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

	for (QJsonValueRef o : jo["environment_sets"].toArray()) {
		EnvironmentSet* s = envs.findSet(o.toString());
		if (s == nullptr) {
			qWarning() << "Unknown environment set for Experiment " << title << "Broken file?";
		} else {
			envSets.append(envs.findSet(o.toString()));
		}
	}

	for (QJsonValueRef v : jo["environments"].toArray()) {
		const QJsonObject& o = v.toObject();
		Environment* e = new Environment(o);
		if (o.contains("runs")) {
			for (QJsonValueRef vr : o["runs"].toArray()) {
				QJsonObject ro = vr.toObject();
				QPair<double,double> v(ro["from"].toDouble(),ro["to"].toDouble());
				runs[e].push_back(v);
			}
			wasRun = true;
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
	lastEnvironment = env;
	runs[lastEnvironment].push_back(QPair<double,double>(time,time));
	return command;
}

QString Experiment::cleanupMeasurement(float time) {
	runs[lastEnvironment].back().second = time;
	wasRun = true;
	return cleanup;
}

bool Experiment::hasData() const { return wasRun; }
void Experiment::finishedCleanup(float) {}

StatisticalSet Experiment::aggregate(int unit, const Experiment* e) const {
	StatisticalSet s(e->data.at(unit)->descriptor);
	for (const Environment* env : runs.keys())
		s.extend(env->aggregate(unit,e));

	return s;
}

unsigned Experiment::executions() const {
	unsigned i = 0;
	for (EnvironmentSet* s : envSets)
		i += s->environments().size();

	return i;
}
