#include "Sources/odroidsmartpowersource.h"
#include <QDebug>
#include <cassert>
OdroidSmartPowerSource::OdroidSmartPowerSource(QString path) :
	DataSource("Odroid"), QHIDevice(path), lastCmd(Command::NONE), _running(false), restarted(false), _path(path)
{
	descs.append(new DataDescriptor("U_ext","V",1,DataDescriptor::Type::FLOAT));
	descs.append(new DataDescriptor("I_ext","A",1,DataDescriptor::Type::FLOAT));
	descs.append(new DataDescriptor("P_ext","W",1,DataDescriptor::Type::FLOAT));
	QHIDevice::connect(this,&QHIDevice::timeout,[this] () {
		qDebug() << "TIMEOUT!";
		if (_running) start();
	});
	QHIDevice::connect(this,&QHIDevice::receivedData,[this] (QByteArray data) {
		assert(lastCmd != Command::NONE);
		qDebug() << "Got: " << data;
		qDebug() << "Hex: " << data.toHex();
		switch (lastCmd) {
		  case Command::REQUEST_VERSION:
				sendCommand(Command::REQUEST_STATUS);
				qDebug() << "Requesting STATUS";
				read(64,1000);
				qDebug() << "Called";
				break;
		  case Command::REQUEST_DATA:
				qDebug() << "Received Data!";
				break;
		  case Command::REQUEST_ONOFF:
		  case Command::REQUEST_STARTSTOP:
		  case Command::REQUEST_STATUS:
				qDebug() << "Requesting DATA";
				sendCommand(Command::REQUEST_DATA);
				read(64,1000);
				break;
		}
		emit connected();
	});
}

int OdroidSmartPowerSource::sendCommand(Command cmd, char param) {
	lastCmd = cmd;
	QByteArray bw(65,'\0');
	bw[1] = static_cast<unsigned char>(cmd);
	bw[2] = param;
	int ret = write(bw);
	assert(ret != -1);
	return ret;
}

QString OdroidSmartPowerSource::descriptor() {
	return "[DEV] Odroid Smart Power " + _path;
}

void OdroidSmartPowerSource::start() {
	if (_running == false) {
		if (!good()) return;
		if (!restarted) {
			sendCommand(Command::REQUEST_VERSION);
			read(64,3000);
			emit descriptorsAvailable(descs);
		}
		_running = true;
	} else {
		emit disconnected();
		_running = false;
        //TODO: Implement stop
    }
}
