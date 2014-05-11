#include "smartpowermonitor.h"
#include <hidapi/hidapi.h>

#include <QDebug>
SmartPowerMonitor::SmartPowerMonitor()
{}

QString SmartPowerMonitor::getName() {
	return "Odroid";
}

void SmartPowerMonitor::monitor(quint32 interval_ms) {
	connect(&tmr,SIGNAL(timeout()),this,SLOT(checkDevices()));
	tmr.start(interval_ms);
}

void SmartPowerMonitor::checkDevices() {
	struct hid_device_info *devs, *cur_dev;
	devs = hid_enumerate(0x04d8, 0x003f);
	cur_dev = devs;
	qDebug() << "Devices:";
	while (cur_dev) {
		qDebug() << "Serial      : " << QString::fromWCharArray(cur_dev->serial_number);
		qDebug() << "Manufacturer: " << QString::fromWCharArray(cur_dev->manufacturer_string);
		qDebug() << "Product     : " << QString::fromWCharArray(cur_dev->product_string);
		qDebug() << "Other:      : " << cur_dev->path << "\n";
		cur_dev = cur_dev->next;
	}
	hid_free_enumeration(devs);
}
