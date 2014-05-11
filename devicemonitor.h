#ifndef DEVICEMONITOR_H
#define DEVICEMONITOR_H

#include <Data/datasource.h>
#include <QObject>

class DeviceMonitor : public QObject
{
	Q_OBJECT
public:
	DeviceMonitor(QObject *parent = nullptr);
	virtual QString getName() = 0;
public slots:
	virtual void monitor(quint32 interval_ms) = 0;
signals:
	void addSource(DataSource* ds);
	void removeSource(DataSource* ds);
};

#endif // DEVICEMONITOR_H
