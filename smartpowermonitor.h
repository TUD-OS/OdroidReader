#ifndef SMARTPOWERMONITOR_H
#define SMARTPOWERMONITOR_H

#include "devicemonitor.h"
#include <QTimer>

class SmartPowerMonitor : public DeviceMonitor
{
Q_OBJECT
private:
	QTimer tmr;
public:
	SmartPowerMonitor();
	virtual QString getName();
public slots:
	void monitor(quint32 interval_ms);
private slots:
	void checkDevices();
};

#endif // SMARTPOWERMONITOR_H
