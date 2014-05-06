//LD_LIBRARY_PATH=$LD_LIBRARY_PATH:. ./ffmpeg -i /sdcard/Movies/big_buck_bunny_1080p_h264.mov -f rawvideo /dev/null
//am start -S -a com.mxtech.intent.result.VIEW -n com.mxtech.videoplayer.ad/com.mxtech.videoplayer.ad.ActivityScreen -d file:////sdcard//Movies//big_buck_bunny_1080p_h264.mov
#include "odroidreader.h"
#include "ipvalidator.h"
#include "ui_odroidreader.h"
#include <QDebug>
#include <QtNetwork/QTcpSocket>
#include <QMessageBox>
#include <cassert>
#include <QColor>
#include <limits>
#include <QFile>
#include "qcustomplot.h"
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include "environmentmodel.h"
#include "environmentdelegate.h"
#include <QJsonArray>
#include <Sources/networksource.h>

Q_DECLARE_METATYPE(Experiment*)

QList<QColor> origcols({QColor(7,139,119),QColor(252,138,74),QColor(100,170,254),QColor(91,53,40),QColor(133,196,77),
					  QColor(104,115,15),QColor(133,3,43),QColor(188,186,111),QColor(168,115,19),QColor(63,184,67)});
QList<QColor> colors;

QVector<Datapoint<double>*> OdroidReader::descs;

OdroidReader::OdroidReader(QWidget *parent) :
	QMainWindow(parent), executed(false), es(ExperimentState::Idle),
	ui(new Ui::OdroidReader),
	sock(nullptr), currentExp(nullptr), currentEnv(nullptr)
{
	ui->setupUi(this);
	ui->ip->setValidator(new IPValidator());
	ui->sensors->setColumnWidth(0,30);
	ui->globalPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    connect(&tmr,SIGNAL(timeout()),this,SLOT(sendGet()));
    connect(ui->sensors,SIGNAL(cellClicked(int,int)),this,SLOT(updateCurve(int,int)));
	connect(ui->environment,SIGNAL(clicked(QModelIndex)),this,SLOT(removeEnvironment(QModelIndex)));
	connect(qApp,SIGNAL(aboutToQuit()),this,SLOT(aboutToQuit()));

	//Load state ...
	QFile f("experiments.json");
	if (!f.open(QFile::ReadOnly)) {
		qWarning() << "Could not read experiments!";
		return;
	};

	QJsonArray experimentDoc = QJsonDocument::fromJson(f.readAll()).array();
	for (int exp = 0; exp < experimentDoc.size(); ++exp) {
		QJsonObject experimentData = experimentDoc[exp].toObject();
		experiments.append(Experiment(experimentData, &descs));
	}
	updateExperiments();
}

OdroidReader::~OdroidReader()
{
	delete ui;
	for (auto d : descs) delete d;
}

void OdroidReader::enableControls(bool status = true) {
	ui->sample->setEnabled(status);
}

void OdroidReader::updateCurve(int row, int col) {
	if (col != 0) return;
	Datapoint<double>* d = descs.at(row);
	bool enable = ui->sensors->item(row,col)->checkState() == Qt::Checked;
	if (enable) {
		if (colors.count() == 0) {
			ui->sensors->item(row,col)->setCheckState(Qt::Unchecked);
			return;
		}
		QColor color = colors.takeFirst();
		d->graph->setPen(color);
		ui->sensors->item(row,col)->setBackgroundColor(color);
	} else {
		ui->sensors->item(row,col)->setBackgroundColor(Qt::white);
		if (descs.at(row)->graph->pen().color() == QColor(Qt::black))
			return;
		colors.push_front(descs.at(row)->graph->pen().color());
	}
	d->graph->setVisible(enable);
	ui->globalPlot->rescaleAxes(true);
	ui->globalPlot->yAxis->scaleRange(1.2,ui->globalPlot->yAxis->range().center());
	ui->globalPlot->replot();
}

void OdroidReader::sendGet() {
	query = Query::GET;
	sock->write("GET\n");
}

void OdroidReader::updateSensors() {
	ui->sensors->setRowCount(descs.size());

	for (int i = 0; i < descs.size(); i++) {
		ui->sensors->setItem(i,0,new QTableWidgetItem(""));
		ui->sensors->item(i,0)->setCheckState(Qt::Unchecked);
		ui->sensors->setItem(i,1,descs[i]->name_item);
		ui->sensors->setItem(i,2,descs[i]->last_item);
		ui->sensors->setItem(i,3,descs[i]->min_item);
		ui->sensors->setItem(i,4,descs[i]->max_item);
		ui->sensors->setItem(i,5,descs[i]->unit_item);
	}
}

void OdroidReader::setupExperiment(Experiment::Environment const &run) {
	assert(sock);
	sock->write("SETUP\n");
	QString gov = run.governor;
	sock->write(gov.append("\n").toStdString().c_str());
    qDebug() << "Governor is:" << gov << run.freq << run.freq_min << "-" << run.freq_max;
	if (gov == "userspace\n")
		sock->write(std::to_string(run.freq).append("000\n").c_str());
	sock->write(std::to_string(run.freq_min).append("000\n").c_str());
	sock->write(std::to_string(run.freq_max).append("000\n").c_str());
}

void OdroidReader::runCommand(QString cmd) {
	assert(sock);
	qDebug() << "Executing" << cmd;
	sock->write("EXEC\n");
	sock->write(cmd.append("\n").toStdString().c_str());
	executed = true;
}

void OdroidReader::connected() {
	colors = origcols;
	qDebug() << "Connected";
	descs.clear();
	ui->sensors->clearContents();
	sock->write("DESC\n");
	query = Query::DESC;
}

void OdroidReader::readData() {
	quint32 val;
	switch (query) {
		case Query::GET: //Rework this!
			if (sock->bytesAvailable() < packetSize+8) return;
			assert(sock->bytesAvailable() == packetSize+8);
			lastTime = networkDecode<quint32>(sock->read(4));
			lastTime += networkDecode<quint32>(sock->read(4))/1000000000.0;

			for (int i = 0; i < descs.size(); i++) {
				Datapoint<double>* d = descs.at(i);

				switch (d->type()) {
				  case FIELD_TYPE::BIGLITTLE:
				  case FIELD_TYPE::CHAR:
					d->addValue(*reinterpret_cast<const uchar*>(sock->read(1).constData()),lastTime);
					break;
				  case FIELD_TYPE::FLOAT:
					val = networkDecode<quint32>(sock->read(4));
					d->addValue(*reinterpret_cast<float*>(&val),lastTime);
					break;
				  case FIELD_TYPE::UINT16T:
					d->addValue(networkDecode<quint16>(sock->read(2)),lastTime);
					break;
				  case FIELD_TYPE::UINT32T:
					d->addValue(networkDecode<quint32>(sock->read(4)),lastTime);
					break;
				  default:
					qDebug() << "Error interpreting data " << i;
				}
				if (d->type() != FIELD_TYPE::BIGLITTLE)
					ts << d->value().last() << ";";
			}
			ui->globalPlot->rescaleAxes(true);
			ui->globalPlot->yAxis->scaleRange(1.2,ui->globalPlot->yAxis->range().center());
			ui->globalPlot->replot();
			if (es != ExperimentState::Idle && descs.at(1)->value().last() == 0 && executed) {
				executed = false;
				qDebug() << "Stepping" << descs.at(1)->value().last() << descs.at(1)->value().changed();
				if (es == ExperimentState::Execute)
					QTimer::singleShot(toRun.back()->cooldown_time*1000,this,SLOT(runExperiments()));
				else
					runExperiments();
			}
			ts << "\n";
			ts.flush(); //TODO: Check if this is really a good idea (esp. for high frequency reading)
			query = Query::NONE;
			assert(sock->bytesAvailable() == 0 || "Extra Data");
			break;
		case Query::DESC:
		  uint16_t got;
		  do {
			got = static_cast<quint8>(sock->peek(1).at(0));
			if (got == 0) { //End of descriptors
				assert(sock->bytesAvailable() == 1);
				sock->read(1);
				packetSize = 0;
				if (ts.device() != nullptr) delete ts.device();
				QFile* f = new QFile("log_"+QDateTime::currentDateTime().toString()+".log");
				if (!f->open(QFile::ReadWrite | QFile::Truncate)) {
					qDebug() << "Creating logfile failed :(";
					QApplication::instance()->exit(-1);
				}
				ts.setDevice(f);
				for (DatapointBase* d : descs) {
					switch (d->type()) {
						case FIELD_TYPE::BIGLITTLE:
						case FIELD_TYPE::CHAR:    packetSize++;  break;
						case FIELD_TYPE::UINT16T: packetSize +=2; break;
						case FIELD_TYPE::UINT32T:
						case FIELD_TYPE::FLOAT:   packetSize +=4;  break;
					}
					ts << d->name() << ";";
				}
				ts << "\n";
				ts.flush();
				qDebug() << "Done reading" << descs.size() << "Descriptors" << "(Data Size:" << packetSize << ")";
				updateSensors();
				tmr.start(ui->samplingInterval->value());
				return;
			}

			while (sock->bytesAvailable() >= 9+got) {
				descs.push_back(new Datapoint<double>(ui->globalPlot,sock->read(9+got)));
				if (sock->bytesAvailable() < 1)
					break;
				got = static_cast<quint8>(sock->peek(1).at(0));
				if (got == 0) break;
			}
		  } while (got == 0);
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
	if (ts.device()) {
		ts.device()->close();
		delete ts.device();
		ts.setDevice(nullptr);
	}
	enableControls();
}

quint32 OdroidReader::freq2Int(QString s) {
	s.chop(4);
	return s.toInt();
}

void OdroidReader::updateExperiments() {
	ui->listWidget->clear();
	for (Experiment& e : experiments) {
		QListWidgetItem* lwi = new QListWidgetItem(e.title);
		lwi->setCheckState(Qt::Checked);
		ui->listWidget->addItem(lwi);
	}
}

void OdroidReader::on_addExperiment_clicked()
{
	Experiment experiment;
	Experiment::Environment run;
	experiment.title = ui->exp_name->text();
	experiment.cleanup = ui->exp_cleanup->text();
	experiment.command = ui->exp_command->text();
	experiment.prepare = ui->exp_prepare->text();
	experiment.cooldown_time = ui->cooldown->value();
	experiment.tail_time = ui->tail->value();
	experiment.environments.append(run);
	experiments.append(experiment);
	ui->addExperiment->setEnabled(false);
	updateExperiments();
}

void OdroidReader::on_exp_name_textChanged(const QString &arg1)
{
	if (arg1.isEmpty()) { ui->addExperiment->setEnabled(false); }
	for (Experiment e : experiments) {
		if (e.title == arg1) {
			ui->addExperiment->setEnabled(false);
			return;
		}
	}
	ui->addExperiment->setEnabled(true);
}

void OdroidReader::on_listWidget_itemSelectionChanged()
{
	bool canModify = (ui->listWidget->selectedItems().size() != 0);
	ui->removeExperiment->setEnabled(canModify);
	ui->updateExperiment->setEnabled(canModify);
	ui->runSelected->setEnabled(toRun.empty() && sock && sock->isWritable() && canModify);
	if (!canModify) return;
	currentExp = &experiments[ui->listWidget->currentRow()];
	ui->exp_name->setText(currentExp->title);
	ui->exp_cleanup->setText(currentExp->cleanup);
	ui->exp_command->setText(currentExp->command);
	ui->exp_prepare->setText(currentExp->prepare);
	ui->cooldown->setValue(currentExp->cooldown_time);
	ui->tail->setValue(currentExp->tail_time);
	ui->environment->setModel(new EnvironmentModel(currentExp));
	ui->environment->setItemDelegate(new EnvironmentDelegate(ui->environment));
	ui->environment->resizeColumnsToContents();
}

void OdroidReader::on_removeExperiment_clicked()
{
	experiments.remove(ui->listWidget->currentRow());
	updateExperiments();
}

void OdroidReader::on_updateExperiment_clicked()
{
	on_removeExperiment_clicked();
	on_addExperiment_clicked();
}

void OdroidReader::runExperiments() {
	QString cmd;
	switch (es) {
		case ExperimentState::Idle:
			cmd = toRun.back()->prepareMeasurement(lastTime);
			if (cmd.length() > 0) {
				qDebug() << "Prepare";
				runCommand(cmd);
				es = ExperimentState::Prepare;
				break;
			}
		case ExperimentState::Prepare:
			cmd = toRun.back()->startMeasurement(lastTime,lastEnv);
			if (cmd.length() > 0) {
				qDebug() << "Configure";
				setupExperiment(toRun.back()->environments.at(lastEnv));
				qDebug() << "Start";
				runCommand(cmd);
				es = ExperimentState::Execute;
				break;
			}
		case ExperimentState::Execute:
			cmd = toRun.back()->cleanupMeasurement(lastTime);
			if (cmd.length() > 0) {
				qDebug() << "Cleanup";
				runCommand(cmd);
				es = ExperimentState::Cleanup;
				break;
			}
		case ExperimentState::Cleanup:
			toRun.back()->finishedCleanup(lastTime);
			if (++repetition == ui->repetitions->value()) {
				if (++lastEnv == toRun.back()->environments.size()) {
					toRun.pop_back();
				}
				repetition = 0;
			}
			es = ExperimentState::Idle;
			if (!toRun.empty()) {
				qDebug() << "Tailtime: " << toRun.back()->tail_time;
				if (toRun.back()->tail_time != 0)
					QTimer::singleShot(toRun.back()->tail_time*1000,this,SLOT(runExperiments()));
				else
					runExperiments();
			} else {
				ui->expSelect->clear();
				for (Experiment& e : experiments) {
					if (e.hasData()) {
						ui->expSelect->addItem(e.title,QVariant::fromValue(&e));
					}
				}
				ui->experiments->setEnabled(true);
			}
	}
}

void OdroidReader::on_runSelected_clicked()
{
	if (!sock || !sock->isWritable()) return;
	toRun.clear();
	toRun.push_back(&experiments[ui->listWidget->currentRow()]);
	es = ExperimentState::Idle;
	ui->experiments->setEnabled(false);
	repetition = 0;
	lastEnv = 0;
	runExperiments();
}

void OdroidReader::aboutToQuit()
{
	//Saving state...
	QFile f("experiments.json");
	if (!f.open(QFile::WriteOnly)) {
		qWarning() << "Could not save experiments!";
	};
	QJsonArray experimentArray;
	for (Experiment e : experiments) {
		QJsonObject experimentData;
		e.write(experimentData);
		experimentArray.append(experimentData);
	}
	f.write(QJsonDocument(experimentArray).toJson());
}

void OdroidReader::on_envAdd_clicked()
{
	QAbstractItemModel *em = ui->environment->model();
	if (em) em->insertRow(em->rowCount());
}

void OdroidReader::removeEnvironment(QModelIndex idx) {
	if (idx.column() != 7) return;
	QAbstractItemModel *em = ui->environment->model();
	if (em) em->removeRow(idx.row());
}

void OdroidReader::on_pushButton_clicked()
{
	if (ui->expSelect->currentData().value<Experiment*>()) {
		DataExplorer* de = new DataExplorer(ui->scrollAreaWidgetContents);
		de->setExperiment(ui->expSelect->currentData().value<Experiment*>());
		ui->scrollAreaWidgetContents->layout()->addWidget(de);
		connect(de,SIGNAL(removeMe(DataExplorer*)),this,SLOT(removeDataExplorer(DataExplorer*)));
	}
}

void OdroidReader::removeDataExplorer(DataExplorer *de) {
	ui->scrollAreaWidgetContents->layout()->removeWidget(de);
}

void OdroidReader::on_addConnection_clicked()
{
	NetworkSource *ns = new NetworkSource(ui->sourceName->text(),ui->ip->text(),ui->port->value());
	sources.append(ns);
	connect(ui->startSampling,SIGNAL(clicked()),ns,SLOT(connect()));
	ui->sourceList->addItem(ns->descriptor());
}

void OdroidReader::on_startSampling_clicked()
{
#if 0 //Only for the backup mode where we do not have multi-source
    qDebug() << "Connecting";
	if (sock && sock->isOpen()) {
		qDebug() << "Closing";
		tmr.stop();
		sock->close();
		sock->readAll();
		delete(sock);
		enableControls();
		sock = nullptr;
		return;
	}
	enableControls(false);
	if (sock != nullptr) delete(sock);
	sock = new QTcpSocket();
	sock->connect(sock,SIGNAL(connected()),this,SLOT(connected()));
	sock->connect(sock,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(connerror(QAbstractSocket::SocketError)));
	sock->connect(sock,SIGNAL(readyRead()),this,SLOT(readData()));
	sock->connectToHost(ui->ip->text(),1234);
#endif
    for (DataSource *ds: sources) {
        ds->connect();
    }
}

void OdroidReader::addDescriptors(const QVector<const DataDescriptor*> &descs) {
    for (auto d : descs) descriptors.append(d);
}
