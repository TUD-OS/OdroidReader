#include "environment.h"
#include "experiment.h"
#include <QDebug>

Environment::Environment(const QJsonObject& o, QObject *parent) : QObject(parent) {
	label = o["label"].toString();
	governor = o["governor"].toString();
	freq_min = o["freq_min"].toInt();
	freq_max = o["freq_max"].toInt();
	freq = o["freq"].toInt();
	big = o["big"].toBool();
	little = o["little"].toBool();
}

QString Environment::description() const {
	QString desc("%1 MHz[%2-%3 MHz]@%4 -> %5");
	QString bl = "?";
	if (big && little) {
		bl = "big.LITTLE";
	} else {
		bl = big?"big":"LITTLE";
	}
	return desc.arg(QString::number(freq),QString::number(freq_min),QString::number(freq_max),governor,bl);
}

StatisticalSet Environment::integral(int unit, const Experiment* e) const {
	StatisticalSet allRuns(e->data.at(unit)->descriptor);
	for (int i = 0; i < e->runs[this].size(); i++) {
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

StatisticalSet Environment::aggregate(int unit, const Experiment* e) const {
	StatisticalSet s(e->data.at(unit)->descriptor);
	//qDebug() << "Aggregating environment over" << e->runs[this].size() << "runs";
	for (int i = 0; i < e->runs[this].size(); i++) {
		s.addValue(run(unit,i,e).getAvg());
	}
	return s;
}

DataSeries Environment::run(int unit, int run, const Experiment* e) const {
	//qDebug() << "Extracting Dataseries ...";
	const QPair<double,double> &r = e->runs[this].at(run);
	return DataSeries(*e->data.at(unit),r.first,r.second);
}

bool Environment::operator<(const Environment& o) const {
	if (label < o.label) return true;
	if (governor < o.governor) return true;
	if (freq_min < o.freq) return true;
	if (freq_max < o.freq_max) return true;
	if (freq < o.freq) return true;
	if (big < o.big) return true;
	if (little < o.little) return true;
	return false;
}

void Environment::write(QJsonObject &o) const {
	o["label"] = label;
	o["governor"] = governor;
	o["freq_min"] = (int)freq_min;
	o["freq_max"] = (int)freq_max;
	o["freq"] = (int)freq;
	o["big"] = big;
	o["little"] = little;
}
