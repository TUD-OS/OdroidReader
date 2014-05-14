#include "experiment.h"
#include "odroidreader.h"
#include <QJsonArray>

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
		if (o.contains("runs")) {
			QJsonArray ra = o["runs"].toArray();
			for (int j = 0; j < ra.size(); j++) {
				QJsonObject ro = ra.at(j).toObject();
				QPair<double,double> v;
				v.first = ro["from"].toDouble();
				v.second = ro["to"].toDouble();
				e.runs.push_back(v);
				wasRun = true;
			}
		}
		environments.push_back(e);
	}
	cooldown_time = jo["cooldown_time"].toInt();
	tail_time = jo["tail_time"].toInt();
}

void Experiment::write(QJsonObject& jo, bool withRuns) const {
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
		if (withRuns) {
			QJsonArray runArray;
			for (QPair<double,double> run : e.runs) {
				QJsonObject jr;
				jr["from"] = run.first;
				jr["to"] = run.second;
				runArray.push_back(jr);
			}
			o["runs"] = runArray;
		}
		envs.append(o);
	}
	jo["environments"] = envs;
	jo["cooldown_time"] = static_cast<qint64>(cooldown_time);
	jo["tail_time"] = static_cast<qint64>(tail_time);
}

QString Experiment::prepareMeasurement() {
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

bool Experiment::hasData() const { return wasRun; }

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

StatisticalSet Experiment::Environment::integral(int unit, const Experiment &e) const {
	StatisticalSet allRuns(e.data.at(unit)->descriptor);
	for (int i = 0; i < runs.size(); i++) {
		double value = -1;
		QPair<double,double> last;
		DataSeries s = run(unit,i,e);
		for (int i = 0; i < s.getTimestamps().size(); i++) {
			double ts =  s.getTimestamps().at(i);
			double val = s.getValues().at(i);
			if (value == -1) {
				value = 0;
			} else {
				double t = ts-last.first;
				value += t*last.second+t*(val-last.second)/2;
			}
			last = QPair<double,double>(ts,val);
		}
		allRuns.addValue(value);
	}
	return allRuns;
}

StatisticalSet Experiment::Environment::aggregate(int unit, const Experiment &e) const {
	StatisticalSet s(e.data.at(unit)->descriptor);
	qDebug() << "Aggregating environment over" << runs.size() << "runs";
	for (int i = 0; i < runs.size(); i++) {
		s.addValue(run(unit,i,e).getAvg());
	}
	return s;
}

StatisticalSet Experiment::aggregate(int unit, const Experiment &e) const {
	qDebug() << "Aggregating Experiment " << e.title << "Unit: " << unit;
	StatisticalSet s(e.data.at(unit)->descriptor);
	for (const Environment &env : environments) {
		qDebug() << "Extending Set" << env.description();
		s.extend(env.aggregate(unit,e));
	}
	return s;
}

DataSeries Experiment::Environment::run(int unit, int run, const Experiment &e) const {
	qDebug() << "Extracting Dataseries ...";
	return DataSeries(*e.data.at(unit),runs.at(run).first,runs.at(run).second);
}
