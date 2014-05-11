#include "smartpowermonitor.h"
#include <hidapi/hidapi.h>
#include <cassert>

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
    QList<OdroidSmartPowerSource*> toDelete = sources.toList();
	while (cur_dev) {
        bool newEntry = true;
        for (OdroidSmartPowerSource* s : sources) {
            if (s->path() == cur_dev->path) {
                assert(toDelete.contains(s));
                toDelete.removeAll(s);
                assert(!toDelete.contains(s));
                newEntry = false;
                break;
            }
        }
        if (newEntry) {
            sources.append(new OdroidSmartPowerSource(cur_dev->path));
            emit addSource(sources.last());
            qDebug() << "Serial      : " << QString::fromWCharArray(cur_dev->serial_number);
            qDebug() << "Manufacturer: " << QString::fromWCharArray(cur_dev->manufacturer_string);
            qDebug() << "Product     : " << QString::fromWCharArray(cur_dev->product_string);
            qDebug() << "Other:      : " << cur_dev->path << "\n";
        }
        cur_dev = cur_dev->next;
    }
    for (int i = 0; i < sources.size(); i++) {
        qDebug() << "Checking " << i;
        if (toDelete.contains(sources[i])) {
           qDebug() << "Removing " << i;
           emit removeSource(sources.at(i));
           delete sources.at(i);
           sources.remove(i);

        }
    }
    hid_free_enumeration(devs);
}
