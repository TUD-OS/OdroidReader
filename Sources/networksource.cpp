#include "Sources/networksource.h"
#include <QMessageBox>
#include <cassert>
#include <QtEndian>
#include <Data/o_datapoint.h>

template<typename T>
T networkDecode(QByteArray const &ba) {
	return qFromBigEndian<T>(reinterpret_cast<const uchar*>(ba.constData()));
}

NetworkSource::NetworkSource(QString name, QString address, quint16 port, int interval, QObject *parent) :
	DataSource(name,parent), _address(address), _port(port), started(false)
{
	getTimer.setInterval(interval);
	connect(&getTimer,&QTimer::timeout, [&]() { socket.write("GET\n"); });
	connect(&socket,&QTcpSocket::connected, [this]() {
		query = Query::DESC;
		emit connected();
		socket.write("DESC\n");
	});
}

NetworkSource::~NetworkSource() {
	for (auto d : descs) delete d;
}

void NetworkSource::conerror(QAbstractSocket::SocketError err) {
	getTimer.stop();
	switch (err) {
		case  QAbstractSocket::RemoteHostClosedError:
			qDebug() << "Closed connection";
			break;
		case QAbstractSocket::HostNotFoundError:
			QMessageBox::information(nullptr, tr("Odroid Reader")+descriptor(),tr("The host was not found. Please check the host name and port settings."));
			break;
		case QAbstractSocket::ConnectionRefusedError:
			QMessageBox::information(nullptr, tr("Odroid Reader"),tr("The connection was refused by the peer. Make sure the metrics server is running, and check that the host name and port settings are correct."));
			break;
		default:
			QMessageBox::information(nullptr, tr("Odroid Reader"),tr("The following error occurred: %1.").arg(socket.errorString()));
	}
	socket.close();
}

void NetworkSource::readData() {
	quint32 val;
	switch (query) {
		case Query::GET: //Rework this!
			//assert(false || "Not yet :(");
			if (socket.bytesAvailable() < packetSize+8) return;
			assert(socket.bytesAvailable() == packetSize+8);
			lastTime = networkDecode<quint32>(socket.read(4));
			lastTime += networkDecode<quint32>(socket.read(4))/1000000000.0;

			for (int i = 0; i < descs.size(); i++) {
				const DataDescriptor* d = descs.at(i);
				switch (d->type()) {
				  case DataDescriptor::Type::BIGLITTLE:
				  case DataDescriptor::Type::CHAR:
					{
						unsigned char data = *reinterpret_cast<const uchar*>(socket.read(1).constData());
						if (d->name() == "running") {
							if (started == false && data == 1) {
								started = true;
								emit commandStarted(*this,lastTime);
							} else if (started == true && data == 0) {
								started = false;
								emit commandFinished(*this,lastTime);
							}
						}
						emit dataAvailable(d,data,lastTime);
						break;
					}
				  case DataDescriptor::Type::FLOAT:
					val = networkDecode<quint32>(socket.read(4));
					emit dataAvailable(d,*reinterpret_cast<float*>(&val),lastTime);
					break;
				  case DataDescriptor::Type::UINT16T:
					emit dataAvailable(d,networkDecode<quint16>(socket.read(2)),lastTime);
					break;
				  case DataDescriptor::Type::UINT32T:
					emit dataAvailable(d,networkDecode<quint32>(socket.read(4)),lastTime);
					break;
				  default:
					qDebug() << "Error interpreting data " << i;
				}
			}
			break;
		case Query::DESC:
		  uint16_t got;
		  do {
			got = static_cast<quint8>(socket.peek(1).at(0));
			if (got == 0) { //End of descriptors
				assert(socket.bytesAvailable() == 1);
				socket.read(1);
				packetSize = 0;
				for (const DataDescriptor* d : descs) {
					switch (d->type()) {
						case DataDescriptor::Type::BIGLITTLE:
						case DataDescriptor::Type::CHAR:    packetSize++;  break;
						case DataDescriptor::Type::UINT16T: packetSize +=2; break;
						case DataDescriptor::Type::UINT32T:
						case DataDescriptor::Type::FLOAT:   packetSize +=4;  break;
					}
				}
				qDebug() << "Done reading" << descs.size() << "Descriptors" << "(Data Size:" << packetSize << ")";
				qRegisterMetaType<QVector<const DataDescriptor*>>("QVector<const DataDescriptor*>");
				emit descriptorsAvailable(descs);
				query = Query::GET;
				getTimer.start();
				return;
			}

			while (socket.bytesAvailable() >= 9+got) {
				QByteArray ba = socket.read(9+got);
				quint8 name_len = static_cast<quint8>(ba.left(1).constData()[0]);
				ba.remove(0,1);
				DataDescriptor::Type _type = DataDescriptor::typeFromId(static_cast<quint8>(ba.left(1).constData()[0]));
				ba.remove(0,1);
				quint32 _factor = qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(ba.left(4).constData()));
				ba.remove(0,4);
				QString _name = ba.left(name_len).constData();
				ba.remove(0,name_len);
				QString _unit = ba.constData();
				descs.push_back(new DataDescriptor(_name,_unit,_factor,_type));
				if (socket.bytesAvailable() < 1)
					break;
				got = static_cast<quint8>(socket.peek(1).at(0));
				if (got == 0) break;
			}
		  } while (got == 0);
		  break;
		case Query::NONE:
			qDebug() << "ERROR: Got data without query!";
	}
}

void NetworkSource::execute(QString exec) {
	if (exec.length() == 0) {
		emit commandFinished(*this,lastTime);
		return;
	}
	assert(started == false); //TODO! This implies single instances
	socket.write("EXEC\n");
	socket.write(exec.append("\n").toStdString().c_str());
}

void NetworkSource::setupEnvironment(const Experiment::Environment &env) {
	socket.write("SETUP\n");
	socket.write(QString("%1\n").arg(env.governor).toStdString().c_str());
	qDebug() << "Governor is:" << env.governor << env.freq << env.freq_min << "-" << env.freq_max;
	if (env.governor == "userspace")
		socket.write(std::to_string(env.freq).append("000\n").c_str());
	socket.write(std::to_string(env.freq_min).append("000\n").c_str());
	socket.write(std::to_string(env.freq_max).append("000\n").c_str());
}

void NetworkSource::start() {
	qDebug() << "Connecting to " << descriptor();
	socket.connect(&socket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(conerror(QAbstractSocket::SocketError)));
	socket.connect(&socket,SIGNAL(readyRead()),this,SLOT(readData()));
	socket.connectToHost(_address,_port);
}
