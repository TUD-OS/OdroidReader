//LD_LIBRARY_PATH=$LD_LIBRARY_PATH:. ./ffmpeg -i /sdcard/Movies/big_buck_bunny_1080p_h264.mov -f rawvideo /dev/null
//am start -S -a com.mxtech.intent.result.VIEW -n com.mxtech.videoplayer.ad/com.mxtech.videoplayer.ad.ActivityScreen -d file:////sdcard//Movies//big_buck_bunny_1080p_h264.mov
#include "odroidreader.h"
#include "ipvalidator.h"
#include "ui_odroidreader.h"
#include <QDebug>
#include <QtNetwork/QTcpSocket>
#include <QMessageBox>
#include <iostream>
#include <iomanip>
#include <QtEndian>
#include <cassert>
#include <QColor>
#include <QList>
#include <QFile>
#include <QDateTime>
#include <qwt6/qwt_plot.h>

QList<QColor> origcols({QColor(7,139,119),QColor(252,138,74),QColor(100,170,254),QColor(91,53,40),QColor(133,196,77),
					  QColor(104,115,15),QColor(133,3,43),QColor(188,186,111),QColor(168,115,19),QColor(63,184,67)});
QList<QColor> cols;

OdroidReader::OdroidReader(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::OdroidReader),
	sock(nullptr), execMode(false)
{
	ui->setupUi(this);
	ui->ip->setValidator(new IPValidator());
	ui->qwtPlot->setAutoReplot(true);
	connect(&tmr,SIGNAL(timeout()),this,SLOT(sendGet()));
	connect(ui->sensors,SIGNAL(cellClicked(int,int)),this,SLOT(updateCurve(int,int)));
}

OdroidReader::~OdroidReader()
{
	delete ui;
}

void OdroidReader::enableControls() {
	enableControls(true);
}

void OdroidReader::enableControls(bool status) {
	ui->tab->setEnabled(status);
	ui->tab_2->setEnabled(status);
}

void OdroidReader::updateCurve(int row, int col) {
	if (col != 0) return;
	if (ui->sensors->item(row,col)->checkState() == Qt::Checked) {
		if (cols.count() == 0) {
			ui->sensors->item(row,col)->setCheckState(Qt::Unchecked);
			return;
		}

		field_desc d = descs.at(row);
		QColor color = cols.takeFirst();
		d.pc->setPen(color,2.0);
		ui->sensors->item(row,col)->setBackgroundColor(color);
		d.pc->attach(ui->qwtPlot);
		d.pc->setSamples(d.samples);
		d.pc->setTitle(d.name);
		ui->qwtPlot->updateLegend();
	} else {
		ui->sensors->item(row,col)->setBackgroundColor(Qt::white);
		if (descs.at(row).pc->pen().color() == QColor(Qt::black))
			return;
		descs.at(row).pc->detach();
		cols.push_front(descs.at(row).pc->pen().color());
		ui->qwtPlot->replot();
	}
}

void OdroidReader::sendGet() {
	query = Query::GET;
	sock->write("GET\n");
}

void OdroidReader::updateSensors() {
	qDebug() << "Updating UI";
	ui->sensors->clear();
	ui->sensors->setColumnCount(6);
	ui->sensors->setRowCount(descs.length());
	ui->sensors->setSelectionBehavior(QTableView::SelectRows);
	ui->sensors->setHorizontalHeaderItem(0,new QTableWidgetItem("Show"));
	ui->sensors->setColumnWidth(0,30);
	ui->sensors->setHorizontalHeaderItem(1,new QTableWidgetItem("Name"));
	ui->sensors->setHorizontalHeaderItem(2,new QTableWidgetItem("current"));
	ui->sensors->setHorizontalHeaderItem(3,new QTableWidgetItem("Min"));
	ui->sensors->setHorizontalHeaderItem(4,new QTableWidgetItem("Max"));
	ui->sensors->setHorizontalHeaderItem(5,new QTableWidgetItem("Unit"));

	for (int i = 0; i < descs.length(); i++) {
		//qDebug() << "Adding item " << i;
		ui->sensors->setItem(i,0,new QTableWidgetItem(""));
		ui->sensors->item(i,0)->setCheckState(Qt::Unchecked);
		ui->sensors->setItem(i,1,new QTableWidgetItem(descs[i].name));
		ui->sensors->setItem(i,5,new QTableWidgetItem(descs[i].unit.trimmed()));
	}
}

void OdroidReader::connected() {
	cols = origcols;
	qDebug() << "Connected";
	ui->connect->setText("Disconnect");
	ui->connect->setEnabled(true);
	for (field_desc d : descs)
		delete(d.pc);
	ui->qwtPlot->replot();
	descs.clear();
	sock->write("DESC\n");
	query = Query::DESC;
}

void OdroidReader::readData() {
	quint16 got;
	quint32 val;
	double time = 0;
	switch (query) {
		case Query::GET: //Rework this!
			if (sock->bytesAvailable() < dsize+8) return;
			assert(sock->bytesAvailable() == dsize+8);
			time = qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(sock->read(4).constData()));
			time += qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(sock->read(4).constData()))/1000000000.0;

			for (int i = 0; i < descs.size(); i++) {
				field_desc d = descs.at(i);
				QTableWidgetItem* wi;
				switch (d.type) {
				  case FIELD_TYPE::BIGLITTLE:
				  case FIELD_TYPE::CHAR:
					d.ivalue = static_cast<unsigned char>(*(sock->read(1).constData()))*d.factor;
					wi = new QTableWidgetItem(QString::number(d.ivalue));
					break;
				  case FIELD_TYPE::FLOAT:
					val = qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(sock->read(4).constData()));
					d.fvalue = *reinterpret_cast<float*>(&val)*d.factor;
					wi = new QTableWidgetItem(QString::number(d.fvalue));
					break;
				  case FIELD_TYPE::UINT16T:
					d.ivalue = qFromBigEndian<quint16>(reinterpret_cast<const uchar*>(sock->read(2).constData()))*d.factor;
					wi = new QTableWidgetItem(QString::number(d.ivalue));
					break;
				  case FIELD_TYPE::UINT32T:
					d.ivalue = qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(sock->read(4).constData()))*d.factor;
					wi = new QTableWidgetItem(QString::number(d.ivalue));
					break;
				  default:
					qDebug() << "Error interpreting data " << i;
					wi = new QTableWidgetItem(QString("Error"));
				}
				if (!d.initialized) {
					d.max.ivalue = d.ivalue;
					d.min.ivalue = d.ivalue;
					d.initialized = true;
					wi->setBackgroundColor(Qt::yellow);
					QTableWidgetItem* nwi = new QTableWidgetItem(*wi);
					ui->sensors->setItem(i,3,nwi);
					nwi = new QTableWidgetItem(*nwi);
					ui->sensors->setItem(i,4,nwi);
					ui->sensors->item(i,5)->setBackgroundColor(Qt::yellow);
				}
				if (d.type != FIELD_TYPE::FLOAT && d.type != FIELD_TYPE::BIGLITTLE) {
					ts << d.ivalue << ";";
					if (d.ivalue > d.max.ivalue) {
						d.max.ivalue = d.ivalue;
						ui->sensors->setItem(i,4,new QTableWidgetItem(QString::number(d.max.ivalue)));
						ui->sensors->item(i,3)->setBackgroundColor(Qt::white);
						ui->sensors->item(i,5)->setBackgroundColor(Qt::white);
					}
					if (d.ivalue < d.min.ivalue) {
						d.min.ivalue = d.ivalue;
						ui->sensors->item(i,4)->setBackgroundColor(Qt::white);
						ui->sensors->item(i,5)->setBackgroundColor(Qt::white);
						ui->sensors->setItem(i,3,new QTableWidgetItem(QString::number(d.min.ivalue)));
					}
					d.samples.append(QPointF(time,d.ivalue));
				} else if (d.type == FIELD_TYPE::FLOAT) {
					ts << d.fvalue << ";";
					if (d.fvalue > d.max.fvalue) {
						d.max.fvalue = d.fvalue;
						ui->sensors->setItem(i,4,new QTableWidgetItem(QString::number(d.max.fvalue)));
						ui->sensors->item(i,3)->setBackgroundColor(Qt::white);
						ui->sensors->item(i,5)->setBackgroundColor(Qt::white);
					}
					if (d.fvalue < d.min.fvalue) {
						d.min.fvalue = d.fvalue;
						ui->sensors->setItem(i,3,new QTableWidgetItem(QString::number(d.min.fvalue)));
						ui->sensors->item(i,4)->setBackgroundColor(Qt::white);
						ui->sensors->item(i,5)->setBackgroundColor(Qt::white);
					}
					d.samples.append(QPointF(time,d.fvalue));
				}
				if (d.min.ivalue == d.max.ivalue) wi->setBackgroundColor(Qt::yellow);
				if (d.ivalue == d.max.ivalue && d.min.ivalue != d.max.ivalue) wi->setBackgroundColor(Qt::red);
				if (d.ivalue == d.min.ivalue && d.min.ivalue != d.max.ivalue) wi->setBackgroundColor(Qt::green);
				ui->sensors->setItem(i,2,wi);
				if (ui->sensors->item(i,0)->checkState() == Qt::Checked) {
					d.pc->setSamples(d.samples);
				}
				descs.replace(i,d);
			}
			ts << "\n";
			ts.flush();
			query = Query::NONE;
			if (sock->bytesAvailable() != 0)
				qDebug() << "Extra Data: " << sock->bytesAvailable();
			if (execMode && !descs[1].ivalue && !stop.isActive()) {
				connect(&stop,SIGNAL(timeout()),sock,SLOT(disconnectFromHostImplementation()));
				//connect(&stop,SIGNAL(timeout()),this,SLOT(on_connect_clicked()));
				stop.setInterval(ui->keepTime->value()*1000);
				stop.setSingleShot(true);
				stop.start();
				qDebug() << "Timing out in " << ui->keepTime->value()*1000;
			}
			break;
		case Query::DESC:
		  {
			got = static_cast<quint8>(sock->peek(1).at(0));
			if (got == 0) {
				assert(sock->bytesAvailable() == 1);
				sock->read(1);
				dsize = 0;
				if (ts.device() != nullptr) delete ts.device();
				QFile* f = new QFile("log_"+QDateTime::currentDateTime().toString()+".log");
				assert(f->open(QFile::ReadWrite | QFile::Truncate));
				ts.setDevice(f);
				for (field_desc d : descs) {
					switch (d.type) {
						case FIELD_TYPE::BIGLITTLE:
						case FIELD_TYPE::CHAR:    dsize++;  break;
						case FIELD_TYPE::UINT16T: dsize +=2; break;
						case FIELD_TYPE::UINT32T:
						case FIELD_TYPE::FLOAT:   dsize +=4;  break;
					}
					ts << d.name << ";";
				}
				ts << "\n";
				ts.flush();
				qDebug() << "Done reading" << descs.size() << "Descriptors" << "(Data Size:" << dsize << ")";
				updateSensors();
				if (execMode) {
					sock->write("EXEC\n");
					sock->write(ui->command->text().append("\n").toAscii());
					sendGet();
				}
				tmr.setInterval(ui->samplingInterval->value());
				tmr.start();
				return;
			}

			while (sock->bytesAvailable() >= 9+got) {
				descs.append({
					static_cast<quint8>(sock->read(1).at(0)),
					static_cast<FIELD_TYPE>(sock->read(1).at(0)),
					qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(sock->read(4).constData())),
					0,0,0,QVector<QPointF>(),new QwtPlotCurve(),
					QString(sock->read(got)),
					QString(sock->read(3)),
					false
				});

				if (sock->bytesAvailable() >= 1)
					got = static_cast<quint8>(sock->peek(1).at(0));
				else
					break;
				if (got == 0)
					readData();
			}
		  }
		  break;
		case Query::NONE:
			qDebug() << "ERROR: Got data without query!";
	}
}

void OdroidReader::connerror(QAbstractSocket::SocketError err) {
	tmr.stop();
	switch (err) {
		case  QAbstractSocket::RemoteHostClosedError:
			qDebug() << "Closed connection";
			break;
		case QAbstractSocket::HostNotFoundError:
		QMessageBox::information(this, tr("Odroid Reader"),tr("The host was not found. Please check the host name and port settings."));
			break;
		case QAbstractSocket::ConnectionRefusedError:
			QMessageBox::information(this, tr("Odroid Reader"),tr("The connection was refused by the peer. Make sure the metrics server is running, and check that the host name and port settings are correct."));
			break;
		default:
			QMessageBox::information(this, tr("Odroid Reader"),tr("The following error occurred: %1.").arg(sock->errorString()));
	}
	sock->close();
	ts.flush();
	ts.device()->close();
	delete ts.device();
	ts.setDevice(nullptr);
	ui->connect->setText("Connect");
	execMode = false;
	enableControls();
}

void OdroidReader::on_connect_clicked()
{
	qDebug() << "Connecting";
	if (sock && sock->isOpen()) {
		qDebug() << "Closing";
		tmr.stop();
		sock->close();
		sock->readAll();
		delete(sock);
		ui->connect->setText("Connect");
		enableControls();
		execMode = false;
		sock = nullptr;
		return;
	}
	enableControls(false);
	if (sock != nullptr) delete(sock);
	sock = new QTcpSocket();
	sock->connect(sock,SIGNAL(connected()),this,SLOT(connected()));
	sock->connect(sock,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(connerror(QAbstractSocket::SocketError)));
	sock->connect(sock,SIGNAL(readyRead()),this,SLOT(readData()));
	sock->connect(sock,SIGNAL(disconnected()),this,SLOT(enableControls()));

	sock->connectToHost(ui->ip->text(),1234);
}

void OdroidReader::on_execute_clicked()
{
	qDebug() << "Execute ...";
	execMode = true;
	on_connect_clicked();
}
