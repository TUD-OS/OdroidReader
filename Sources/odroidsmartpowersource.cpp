#include "Sources/odroidsmartpowersource.h"
#include <QDebug>
#include <cassert>
#include <QDateTime>

OdroidSmartPowerSource::OdroidSmartPowerSource(QString path) :
	DataSource("Odroid"), QHIDevice(path), lastCmd(Command::NONE), _running(false), restarted(false), _path(path)
{
	static_cast<QHIDevice*>(this)->connect(&getDataTmr,&QTimer::timeout,[this] () {
		sendCommand(Command::REQUEST_DATA);
		read(64,1000);
	});
	descs.append(new DataDescriptor("U_ext","V",1,DataDescriptor::Type::FLOAT));
	descs.append(new DataDescriptor("I_ext","A",1,DataDescriptor::Type::FLOAT));
	descs.append(new DataDescriptor("P_ext","W",1,DataDescriptor::Type::FLOAT));
	descs.append(new DataDescriptor("E_ext","Wh",1,DataDescriptor::Type::FLOAT));
	descs.append(new DataDescriptor("Ext_running","",1,DataDescriptor::Type::CHAR));
	QHIDevice::connect(this,&QHIDevice::timeout,[this] () {
		qDebug() << "TIMEOUT!";
		if (_running) start();
	});
	QHIDevice::connect(this,&QHIDevice::receivedData,[this] (QByteArray data) {
		assert(data.size() == 64);
		assert(lastCmd != Command::NONE);
		assert(data[0] = static_cast<char>(lastCmd));
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
				qDebug() << "Voltage: " << QString(data.mid(2,5)).toFloat();
				qDebug() << "Current: " << QString(data.mid(10,5)).toFloat();
				qDebug() << "Power: " << QString(data.mid(18,5)).toFloat();
				qDebug() << "Energy: " << QString(data.mid(24,7)).toFloat();
				emit dataAvailable(descs.at(0),QString(data.mid(2,5)).toFloat(),QDateTime::currentMSecsSinceEpoch()/1000.0);
				if (data.at(10) != '-') {
					emit dataAvailable(descs.at(1),QString(data.mid(10,5)).toFloat(),QDateTime::currentMSecsSinceEpoch()/1000.0);
					emit dataAvailable(descs.at(2),QString(data.mid(18,5)).toFloat(),QDateTime::currentMSecsSinceEpoch()/1000.0);
					emit dataAvailable(descs.at(3),QString(data.mid(24,5)).toFloat(),QDateTime::currentMSecsSinceEpoch()/1000.0);
				} else {
					emit dataAvailable(descs.at(1),0,QDateTime::currentMSecsSinceEpoch()/1000.0);
					emit dataAvailable(descs.at(2),0,QDateTime::currentMSecsSinceEpoch()/1000.0);
					emit dataAvailable(descs.at(3),8,QDateTime::currentMSecsSinceEpoch()/1000.0);
				}
				emit dataAvailable(descs.at(4),data.at(9) == '*',QDateTime::currentMSecsSinceEpoch()/1000.0);
				break;
		  case Command::REQUEST_ONOFF:
		  case Command::REQUEST_STARTSTOP:
		  case Command::REQUEST_STATUS:
				getDataTmr.start(1000);
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
