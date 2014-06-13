#include "smartpowermonitor.h"
#include <qhidmanager.h>
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
	QVector<QHIDInfo> devs = QHIDManager::get().enumerate(0x04d8,0x003f);
    QList<OdroidSmartPowerSource*> toDelete = sources.toList();
	//Delete obsoletes
	for (QHIDInfo info : devs) {
        bool newEntry = true;
        for (OdroidSmartPowerSource* s : sources) {
			if (s->path() == info.path) {
                assert(toDelete.contains(s));
                toDelete.removeAll(s);
				assert(!toDelete.contains(s));
                newEntry = false;
                break;
            }
        }
        if (newEntry) {
			OdroidSmartPowerSource* sps = new OdroidSmartPowerSource(info.path.toStdString().c_str());
			if (!sps->good()) continue;
			sources.append(sps);
            emit addSource(sources.last());
			qDebug() << "Serial      : " << info.serial_number;
			qDebug() << "Manufacturer: " << info.manufacturer_string;
			qDebug() << "Product     : " << info.product_string;
			qDebug() << "Other:      : " << info.path << "\n";
        }
    }
    for (int i = 0; i < sources.size(); i++) {
        if (toDelete.contains(sources[i])) {
           emit removeSource(sources.at(i));
           delete sources.at(i);
           sources.remove(i);
        }
    }
}
