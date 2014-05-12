#ifndef ODROIDSMARTPOWERSOURCE_H
#define ODROIDSMARTPOWERSOURCE_H

#define _REQUEST_DATA      0x37
#define _REQUEST_STARTSTOP 0x80
#define _REQUEST_STATUS    0x81
#define _REQUEST_ONOFF     0x82
#define _REQUEST_VERSION   0x83

#include "qhidevice.h"
#include <Data/datasource.h>
#include <Data/datadescriptor.h>

class OdroidSmartPowerSource : public DataSource, public QHIDevice
{
    bool _running, restarted;
    QString _path;
    hid_device* device;
    QVector<const DataDescriptor*> descs;

    enum class Command {
		REQUEST_DATA      = 0x37,
		REQUEST_STARTSTOP = 0x80,
		REQUEST_STATUS    = 0x81,
		REQUEST_ONOFF     = 0x82,
		REQUEST_VERSION   = 0x83,
		NONE              = 0x00
	};
	Command lastCmd;
	int sendCommand(Command cmd, char param = 0x0);
public:
	OdroidSmartPowerSource(QString path);
	QString path() { return _path; }
	bool canExecute() const { return false; }
	bool isRunning() const { return false; }
	QString descriptor();

public slots:
	void start();
};

#endif // ODROIDSMARTPOWERSOURCE_H
