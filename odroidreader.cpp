//LD_LIBRARY_PATH=$LD_LIBRARY_PATH:. ./ffmpeg -i /sdcard/Movies/big_buck_bunny_1080p_h264.mov -f rawvideo /dev/null
//am start -S -a com.mxtech.intent.result.VIEW -n com.mxtech.videoplayer.ad/com.mxtech.videoplayer.ad.ActivityScreen -d file:////sdcard//Movies//big_buck_bunny_1080p_h264.mov
#include "odroidreader.h"
#include "ipvalidator.h"
#include "ui_odroidreader.h"
#include <QDebug>
#include <QtNetwork/QTcpSocket>
#include <QMessageBox>
#include <QtEndian>
#include <QTextStream>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <QColor>
#include <limits>
#include <QList>
#include <QFile>
#include "qcustomplot.h"
#include <QDateTime>
#include <qwt6/qwt_plot.h>

QList<QColor> origcols({QColor(7,139,119),QColor(252,138,74),QColor(100,170,254),QColor(91,53,40),QColor(133,196,77),
					  QColor(104,115,15),QColor(133,3,43),QColor(188,186,111),QColor(168,115,19),QColor(63,184,67)});
QList<QColor> cols;

std::vector<Datapoint<double>*> OdroidReader::descs;

OdroidReader::OdroidReader(QWidget *parent) :
	QMainWindow(parent), executed(false), es(ExperimentState::Idle),
	ui(new Ui::OdroidReader),
	sock(nullptr)
{
	ui->setupUi(this);
	ui->ip->setValidator(new IPValidator());
	ui->qwtPlot->setAutoReplot(true);
	ui->sensors->setColumnWidth(0,30);
//	ui->selectPower->setMu

	connect(&tmr,SIGNAL(timeout()),this,SLOT(sendGet()));
	connect(ui->sensors,SIGNAL(cellClicked(int,int)),this,SLOT(updateCurve(int,int)));
	connect(ui->samplingInterval,SIGNAL(valueChanged(int)),this,SLOT(updateInterval(int)));
	connect(qApp,SIGNAL(aboutToQuit()),this,SLOT(aboutToQuit()));

	//Load state ...
	QFile f("experiments.exp");
	if (!f.exists()) return;
	f.open(QFile::ReadOnly);
	QTextStream ts(&f);
	qDebug() << "Reading experiments";
	while (!ts.atEnd()) {
		qDebug() << "Reading experiment";
		experiments.append(Experiment(ts));
	}
	updateExperiments();
}

void OdroidReader::updateInterval(int msec) {
	tmr.setInterval(msec);
}

OdroidReader::~OdroidReader()
{
	delete ui;
	for (auto d : descs) delete d;
}

void OdroidReader::enableControls() {
	enableControls(true);
}

void OdroidReader::enableControls(bool status) {
	ui->connect->setEnabled(status);
	ui->repetitions->setEnabled(status); //TODO: Must this really be disabled?
	ui->ip->setEnabled(status);
}

void OdroidReader::updateCurve(int row, int col) {
	if (col != 0) return;
	if (ui->sensors->item(row,col)->checkState() == Qt::Checked) {
		if (cols.count() == 0) {
			ui->sensors->item(row,col)->setCheckState(Qt::Unchecked);
			return;
		}

		const Datapoint<double>* d = descs.at(row);
		QColor color = cols.takeFirst();
		d->pc->setPen(color,2.0);
		ui->sensors->item(row,col)->setBackgroundColor(color);
		d->pc->attach(ui->qwtPlot);
		ui->qwtPlot->updateLegend();
	} else {
		ui->sensors->item(row,col)->setBackgroundColor(Qt::white);
		if (descs.at(row)->pc->pen().color() == QColor(Qt::black))
			return;
		descs.at(row)->pc->detach();
		cols.push_front(descs.at(row)->pc->pen().color());
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
	ui->sensors->setRowCount(descs.size());

	for (size_t i = 0; i < descs.size(); i++) {
		ui->sensors->setItem(i,0,new QTableWidgetItem(""));
		ui->sensors->item(i,0)->setCheckState(Qt::Unchecked);
		ui->sensors->setItem(i,1,descs[i]->name_item);
		ui->sensors->setItem(i,2,descs[i]->last_item);
		ui->sensors->setItem(i,3,descs[i]->min_item);
		ui->sensors->setItem(i,4,descs[i]->max_item);
		ui->sensors->setItem(i,5,descs[i]->unit_item);
	}
}

void OdroidReader::runCommand(QString cmd) {
	assert(sock);
	qDebug() << "Executing" << cmd;
	sock->write("EXEC\n");
	cmd.append("\n");
	sock->write(cmd.toStdString().c_str());
	executed = true;
}

void OdroidReader::connected() {
	cols = origcols;
	qDebug() << "Connected";
	ui->connect->setText("Disconnect");
	ui->connect->setEnabled(true);
	descs.clear();
	ui->sensors->clear();
	sock->write("DESC\n");
	query = Query::DESC;
}

void OdroidReader::readData() {
	quint16 got;
	quint32 val;
	switch (query) {
		case Query::GET: //Rework this!
			if (sock->bytesAvailable() < dsize+8) return;
			assert(sock->bytesAvailable() == dsize+8);
			lastTime = qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(sock->read(4).constData()));
			lastTime += qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(sock->read(4).constData()))/1000000000.0;

			for (size_t i = 0; i < descs.size(); i++) {
				Datapoint<double>* d = const_cast<Datapoint<double>*>(descs.at(i));

				switch (d->type()) {
				  case FIELD_TYPE::BIGLITTLE:
				  case FIELD_TYPE::CHAR:
					d->addValue(*reinterpret_cast<const uchar*>(sock->read(1).constData()),lastTime);
					break;
				  case FIELD_TYPE::FLOAT:
					val = qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(sock->read(4).constData()));
					d->addValue(*reinterpret_cast<float*>(&val),lastTime);
					break;
				  case FIELD_TYPE::UINT16T:
					d->addValue(qFromBigEndian<quint16>(reinterpret_cast<const uchar*>(sock->read(2).constData())),lastTime);
					break;
				  case FIELD_TYPE::UINT32T:
					d->addValue(qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(sock->read(4).constData())),lastTime);
					break;
				  default:
					qDebug() << "Error interpreting data " << i;
				}
				if (d->type() != FIELD_TYPE::BIGLITTLE)
					ts << d->value().last() << ";";
				if (ui->sensors->item(i,0)->checkState() == Qt::Checked)
					d->pc->setSamples(d->samples);
			}
			if (es != ExperimentState::Idle && descs.at(1)->value().last() == 0 && executed) {
				executed = false;
				qDebug() << "Stepping" << descs.at(1)->value().last() << descs.at(1)->value().changed();
				if (es == ExperimentState::Execute) {
					qDebug() << "Cooldown" << experiments.back().cooldown_time*1000;
					QTimer::singleShot(experiments.back().cooldown_time*1000,this,SLOT(runExperiments())); }
				else
					runExperiments();
			}
			ts << "\n";
			ts.flush();
			query = Query::NONE;
			if (sock->bytesAvailable() != 0)
				qDebug() << "Extra Data: " << sock->bytesAvailable();
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
				for (DatapointBase* d : descs) {
					switch (d->type()) {
						case FIELD_TYPE::BIGLITTLE:
						case FIELD_TYPE::CHAR:    dsize++;  break;
						case FIELD_TYPE::UINT16T: dsize +=2; break;
						case FIELD_TYPE::UINT32T:
						case FIELD_TYPE::FLOAT:   dsize +=4;  break;
					}
					ts << d->name() << ";";
				}
				ts << "\n";
				ts.flush();
				qDebug() << "Done reading" << descs.size() << "Descriptors" << "(Data Size:" << dsize << ")";
				updateSensors();
				tmr.setInterval(ui->samplingInterval->value());
				tmr.start();
				return;
			}

			while (sock->bytesAvailable() >= 9+got) {
				descs.push_back(new Datapoint<double>(sock->read(9+got)));
				if (sock->bytesAvailable() < 1)
					break;
				got = static_cast<quint8>(sock->peek(1).at(0));
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
	if (ts.device()) {
		ts.device()->close();
		delete ts.device();
		ts.setDevice(nullptr);
	}
	ui->connect->setText("Connect");
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

void OdroidReader::on_useBig_toggled(bool)
{
	if (!ui->useBig->isChecked() && !ui->useLittle->isChecked()) ui->useBig->setChecked(true);
}

void OdroidReader::on_useLittle_toggled(bool)
{
	if (!ui->useBig->isChecked() && !ui->useLittle->isChecked()) ui->useLittle->setChecked(true);
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
	Experiment e;
	e.big = ui->useBig->isChecked();
	e.little = ui->useLittle->isChecked();
	e.title = ui->exp_name->text();
	e.cleanup = ui->exp_cleanup->text();
	e.command = ui->exp_command->text();
	e.prepare = ui->exp_prepare->text();
	e.cooldown_time = ui->cooldown->value();
	e.tail_time = ui->tail->value();
	e.freq = freq2Int(ui->freq->currentText());
	e.freq_max = freq2Int(ui->freq_max->currentText());
	e.freq_min = freq2Int(ui->freq_min->currentText());
	e.governor = ui->freq_gov->currentText();
	experiments.append(e);
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
	const Experiment& e = experiments.at(ui->listWidget->currentRow());
	ui->useLittle->setCheckState(Qt::Checked); //We need this! Else we run into the neither nor situation
	ui->useBig->setCheckState(e.big?Qt::Checked:Qt::Unchecked);
	ui->useLittle->setCheckState(e.little?Qt::Checked:Qt::Unchecked);
	ui->exp_name->setText(e.title);
	ui->exp_cleanup->setText(e.cleanup);
	ui->exp_command->setText(e.command);
	ui->exp_prepare->setText(e.prepare);
	ui->cooldown->setValue(e.cooldown_time);
	ui->tail->setValue(e.tail_time);
	for (int i = 0; i < ui->freq->count(); i++)
		if (ui->freq->itemText(i) == QString::number(e.freq)+" MHz")
			ui->freq->setCurrentIndex(i);
	for (int i = 0; i < ui->freq_max->count(); i++)
		if (ui->freq_max->itemText(i) == QString::number(e.freq_max)+" MHz")
			ui->freq_max->setCurrentIndex(i);
	for (int i = 0; i < ui->freq_min->count(); i++)
		if (ui->freq_min->itemText(i) == QString::number(e.freq_min)+" MHz")
			ui->freq_min->setCurrentIndex(i);
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
			if (!cmd.isEmpty()) {
				qDebug() << "Prepare";
				runCommand(cmd);
				es = ExperimentState::Prepare;
				break;
			}
		case ExperimentState::Prepare:
			cmd = toRun.back()->startMeasurement(lastTime);
			if (!cmd.isEmpty()) {
				qDebug() << "Start";
				runCommand(cmd);
				es = ExperimentState::Execute;
				break;
			}
		case ExperimentState::Execute:
			cmd = toRun.back()->cleanupMeasurement(lastTime);
			if (!cmd.isEmpty()) {
				qDebug() << "Cleanup";
				runCommand(cmd);
				es = ExperimentState::Cleanup;
				break;
			}
		case ExperimentState::Cleanup:
			toRun.back()->finishedCleanup(lastTime);
			if (++repetition == ui->repetitions->value()) {
				toRun.pop_back();
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
	runExperiments();
}

void OdroidReader::aboutToQuit()
{
	//Saving state...
	QFile f("experiments.exp");
	f.open(QFile::WriteOnly);
	QTextStream ts(&f);
	for (Experiment e : experiments)
		e.serialize(ts);
	ts.flush();
}


void OdroidReader::on_listWidget_itemDoubleClicked(QListWidgetItem *item)
{
	if (experiments.at(ui->listWidget->row(item)).runs.empty()) {
		qDebug() << "This experiment has not yet been run!";
		return;
	}
	qDebug() << "We have data from " << experiments.at(ui->listWidget->row(item)).runs.size() << "Runs. Using first";
	curExp = &experiments.at(ui->listWidget->row(item));
	ui->runNo->setMaximum(curExp->runs.size()-1);
	ui->runNo->setValue(0);
	ui->dispUnit->clear();
	for (DatapointBase const *p : curExp->runs.front())
		if (ui->dispUnit->findText(p->unit()) == -1)
			ui->dispUnit->addItem(p->unit());
	on_runNo_valueChanged(0);
	ui->expResult->setCurrentIndex(2);
}

Q_DECLARE_METATYPE(SimpleValue<double>*)

void OdroidReader::on_dispUnit_currentIndexChanged(int)
{
	int i = 0;
	QVector<double> ticks;
	QVector<QString> labels;
	ui->selectPower->clearPlottables();
	QBrush boxBrush(QColor(60, 60, 255, 100));
	boxBrush.setStyle(Qt::Dense6Pattern); // make it look oldschool
	std::vector<SimpleValue<double>*> vals;
	if (ui->aggregate->isChecked()) {
		bool init = true;
		for (std::vector<Datapoint<double>*> v : curExp->runs) {
			int i = 0;
			for (Datapoint<double>* p : v) {
				if (p->unit() != ui->dispUnit->currentText()) continue;
				if (init) {
					vals.push_back(new SimpleValue<double>());
					labels.push_back(p->name());
				}
				vals.at(i++)->add(p->value().avg(),0);
			}
			init = false;
		}
	} else {
		for (Datapoint<double> *p : curExp->runs.at(ui->runNo->value())) {
			if (p->unit() != ui->dispUnit->currentText()) continue;
			vals.push_back(new SimpleValue<double>(p->value(),0,std::numeric_limits<double>::max()));
			labels.push_back(p->name());
		}
	}
	for (SimpleValue<double> *p : vals) {
		QCPStatisticalBox* b = new QCPStatisticalBox(ui->selectPower->xAxis,ui->selectPower->yAxis);
		b->setProperty("Datapoint",QVariant::fromValue(p));
		b->setBrush(boxBrush);
		b->setData(i,p->min(),p->quantile(0.25),p->median(),p->quantile(0.75),p->max());
		ui->selectPower->addPlottable(b);
		ticks.append(i);
		i++;
	};
	ui->selectPower->xAxis->setSubTickCount(0);
	ui->selectPower->xAxis->setTickLength(0, 4);
	ui->selectPower->xAxis->setTickLabelRotation(20);
	ui->selectPower->xAxis->setAutoTicks(false);
	ui->selectPower->xAxis->setAutoTickLabels(false);
	ui->selectPower->xAxis->setTickVector(ticks);
	ui->selectPower->xAxis->setTickVectorLabels(labels);
	ui->selectPower->setInteractions(QCP::iMultiSelect | QCP::iSelectPlottables);

	ui->selectPower->xAxis->scaleRange(1.7, ui->selectPower->xAxis->range().center());
	ui->selectPower->rescaleAxes();
	connect(ui->selectPower,SIGNAL(selectionChangedByUser()), this, SLOT(updateDetail()));
	ui->selectPower->replot();
}

void OdroidReader::updateDetail() {
	ui->runPlot->clearGraphs();
	int i = 0;
	for (auto p : ui->selectPower->selectedPlottables()) {
		SimpleValue<double>* dp = p->property("Datapoint").value<SimpleValue<double>*>();
		QCPGraph *g = ui->runPlot->addGraph();
		g->setPen(origcols[i%origcols.size()]);
		QVector<double> x,y;
		double start = dp->values().front().first;
		for (std::pair<double,double> v : dp->values()) {
			x.append(v.first-start);
			y.append(v.second);
		}
		g->setData(x,y);
		p->setSelectedBrush(origcols[i++%origcols.size()]);
	}
	ui->runPlot->rescaleAxes();
	ui->runPlot->replot();
}

void OdroidReader::on_runNo_valueChanged(int index)
{
	std::vector<int> selected;
	for (int i = 0; i < ui->selectPower->plottableCount(); i++) {
		if (ui->selectPower->plottable(i)->selected()) {
			selected.push_back(i);
		}
	}
	on_dispUnit_currentIndexChanged(ui->dispUnit->currentIndex());
	for (int i : selected)
		ui->selectPower->plottable(i)->setSelected(true);
	updateDetail();
	ui->selectPower->replot();
}

void OdroidReader::on_aggregate_toggled(bool checked)
{
	ui->runNo->setEnabled(!checked);
	on_dispUnit_currentIndexChanged(ui->dispUnit->currentIndex());
	updateDetail();
}
