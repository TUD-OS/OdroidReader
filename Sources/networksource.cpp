#include "Sources/networksource.h"
#include <QMessageBox>
#include <cassert>
#include <QtEndian>
#include <Data/o_datapoint.h>

template<typename T>
T networkDecode(QByteArray const &ba) {
	return qFromBigEndian<T>(reinterpret_cast<const uchar*>(ba.constData()));
}

NetworkSource::NetworkSource(QString name, QString address, quint16 port, QObject *parent) :
	DataSource(name,parent), _address(address), _port(port)
{
	networkThread = new QThread();
	socket.moveToThread(networkThread);
	this->moveToThread(networkThread); //TODO: this should not work if we have a parent. Enforce?
	networkThread->start();
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

void NetworkSource::connected() {
	qDebug() << "Connected to " << descriptor();
	query = Query::DESC;
	socket.write("DESC\n");
}

void NetworkSource::readData() {
	quint32 val;
	switch (query) {
		case Query::GET: //Rework this!
			assert(false || "Not yet :(");
/*TODO			if (socket.bytesAvailable() < packetSize+8) return;
			assert(socket.bytesAvailable() == packetSize+8);
			lastTime = networkDecode<quint32>(socket.read(4));
			lastTime += networkDecode<quint32>(socket.read(4))/1000000000.0;

			for (int i = 0; i < descs.size(); i++) {
				Datapoint<double>* d = descs.at(i);

				switch (d->type()) {
				  case FIELD_TYPE::BIGLITTLE:
				  case FIELD_TYPE::CHAR:
					d->addValue(*reinterpret_cast<const uchar*>(socket.read(1).constData()),lastTime);
					break;
				  case FIELD_TYPE::FLOAT:
					val = networkDecode<quint32>(socket.read(4));
					d->addValue(*reinterpret_cast<float*>(&val),lastTime);
					break;
				  case FIELD_TYPE::UINT16T:
					d->addValue(networkDecode<quint16>(socket.read(2)),lastTime);
					break;
				  case FIELD_TYPE::UINT32T:
					d->addValue(networkDecode<quint32>(socket.read(4)),lastTime);
					break;
				  default:
					qDebug() << "Error interpreting data " << i;
				}
				if (d->type() != FIELD_TYPE::BIGLITTLE)
					ts << d->value().last() << ";";
			}
			*/
//			ui->globalPlot->rescaleAxes(true);
//			ui->globalPlot->yAxis->scaleRange(1.2,ui->globalPlot->yAxis->range().center());
//			ui->globalPlot->replot();
//			if (es != ExperimentState::Idle && descs.at(1)->value().last() == 0 && executed) {
//				executed = false;
//				qDebug() << "Stepping" << descs.at(1)->value().last() << descs.at(1)->value().changed();
//				if (es == ExperimentState::Execute)
//					QTimer::singleShot(toRun.back()->cooldown_time*1000,this,SLOT(runExperiments()));
//				else
//					runExperiments();
//			}
//			ts << "\n";
//			ts.flush(); //TODO: Check if this is really a good idea (esp. for high frequency reading)
//			query = Query::NONE;
//			assert(socket.bytesAvailable() == 0 || "Extra Data");
//			break;
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
				qDebug() << "Emitted!";
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
	qDebug() << "Executing" << exec;
	socket.write("EXEC\n");
	socket.write(exec.append("\n").toStdString().c_str());
}

void NetworkSource::start() {
	qDebug() << "Connecting to " << descriptor();
	socket.connect(&socket,SIGNAL(connected()),this,SLOT(connected()));
	socket.connect(&socket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(conerror(QAbstractSocket::SocketError)));
	socket.connect(&socket,SIGNAL(readyRead()),this,SLOT(readData()));
	socket.connectToHost(_address,_port);
}
