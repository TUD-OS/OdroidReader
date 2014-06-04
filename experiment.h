#ifndef EXPERIMENT_H
#define EXPERIMENT_H

#include <string>
#include <vector>
#include <QVector>
#include <QPair>
#include <QMap>
#include <QJsonObject>
#include "Data/dataseries.h"
#include "Data/statisticalset.h"
#include "environment.h"
#include "environments.h"
#include "environmentset.h"

class Experiment : public QObject
{
Q_OBJECT
private:
	int currentRun;
	bool wasRun;
public:
	const QVector<DataSeries*> &data;

public:
	QString title, prepare, cleanup, command;
	quint32 tail_time, cooldown_time;
	Experiment(const QJsonObject &jo, const QVector<DataSeries*>& data, const Environments &envs);
	Experiment() = delete;
	void write(QJsonObject &jo, bool withRuns = false) const;
	StatisticalSet aggregate(int unit, const Experiment *e) const;
	bool hasData() const;
	QList<EnvironmentSet*> envSets;
	QMap<const Environment*,QVector<QPair<double,double>>> runs;
	QString prepareMeasurement();
	QString cleanupMeasurement(float time, const Environment* env);
	QString startMeasurement(double time, const Environment* env);
	void finishedCleanup(float time);
	StatisticalSet aggregate(int unit, const Environment* e) const;
	StatisticalSet integral(int unit, const Environment* e) const;
	DataSeries run(int unit, int run, const Environment* e) const;

public slots:
	inline void setTitle(const QString& v) { title = v; }
	inline void setPrepare(const QString& v) { prepare = v; }
	inline void setCleanup(const QString& v) { cleanup = v; }
	inline void setCommand(const QString& v) { command = v; }
	inline void setTailTime(int v) { tail_time = v; }
	inline void setCooldownTime(int v) { cooldown_time = v; }
};

#endif // EXPERIMENT_H
