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

QList<QColor> origcols({QColor(7,139,119),QColor(252,138,74),QColor(100,170,254),QColor(91,53,40),QColor(133,196,77),
					  QColor(104,115,15),QColor(133,3,43),QColor(188,186,111),QColor(168,115,19),QColor(63,184,67)});
QList<QColor> colors;

std::vector<Datapoint<double>*> OdroidReader::descs;

OdroidReader::OdroidReader(QWidget *parent) :
	QMainWindow(parent), executed(false), es(ExperimentState::Idle),
	ui(new Ui::OdroidReader),
	sock(nullptr)
{
	ui->setupUi(this);
	ui->ip->setValidator(new IPValidator());
	ui->sensors->setColumnWidth(0,30);
	ui->globalPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

	connect(&tmr,SIGNAL(timeout()),this,SLOT(sendGet()));
	connect(ui->sensors,SIGNAL(cellClicked(int,int)),this,SLOT(updateCurve(int,int)));
	connect(ui->samplingInterval,SIGNAL(valueChanged(int)),this,SLOT(updateInterval(int)));
	connect(qApp,SIGNAL(aboutToQuit()),this,SLOT(aboutToQuit()));

	//Load state ...
	QFile f("experiments.exp");
	if (!f.exists()) return;
	f.open(QFile::ReadOnly);
	QTextStream ts(&f);
	while (!ts.atEnd()) {
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

void OdroidReader::enableControls(bool status = true) {
	ui->connect->setEnabled(status);
	ui->repetitions->setEnabled(status); //TODO: Must this really be disabled?
	ui->ip->setEnabled(status);
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

void OdroidReader::setupExperiment(Experiment::Run const &run) {
	assert(sock);
	sock->write("SETUP\n");
	std::string gov = run.governor;
	sock->write(gov.append("\n").c_str());
	std::cerr << "Governor is:" << gov;
	if (gov == "ondemand\n")
		sock->write(std::to_string(run.freq).append("000\n").c_str());
	sock->write(std::to_string(run.freq_min).append("000\n").c_str());
	sock->write(std::to_string(run.freq_max).append("000\n").c_str());
}

void OdroidReader::runCommand(std::string cmd) {
	assert(sock);
	std::cerr << "Executing" << cmd;
	sock->write("EXEC\n");
	sock->write(cmd.append("\n").c_str());
	executed = true;
}

void OdroidReader::connected() {
	colors = origcols;
	qDebug() << "Connected";
	ui->connect->setText("Disconnect");
	ui->connect->setEnabled(true);
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

			for (size_t i = 0; i < descs.size(); i++) {
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
					QTimer::singleShot(experiments.back().cooldown_time*1000,this,SLOT(runExperiments()));
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
					std::cerr << "Creating logfile failed :(" << std::endl;
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
			std::cerr << "ERROR: Got data without query!" << std::endl;
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
		QListWidgetItem* lwi = new QListWidgetItem(QString::fromStdString(e.title));
		lwi->setCheckState(Qt::Checked);
		ui->listWidget->addItem(lwi);
	}
}

void OdroidReader::on_addExperiment_clicked()
{
	Experiment experiment;
	Experiment::Run run;
	experiment.title = ui->exp_name->text().toStdString();
	experiment.cleanup = ui->exp_cleanup->text().toStdString();
	experiment.command = ui->exp_command->text().toStdString();
	experiment.prepare = ui->exp_prepare->text().toStdString();
	experiment.cooldown_time = ui->cooldown->value();
	experiment.tail_time = ui->tail->value();
	run.big = ui->useBig->isChecked();
	run.little = ui->useLittle->isChecked();
	run.label = "Run 0";
	run.freq = freq2Int(ui->freq->currentText());
	run.freq_max = freq2Int(ui->freq_max->currentText());
	run.freq_min = freq2Int(ui->freq_min->currentText());
	run.governor = ui->freq_gov->currentText().toStdString();
	experiment.runs.push_back(run);
	experiments.append(experiment);
	ui->addExperiment->setEnabled(false);
	updateExperiments();
}

void OdroidReader::on_exp_name_textChanged(const QString &arg1)
{
	if (arg1.isEmpty()) { ui->addExperiment->setEnabled(false); }
	for (Experiment e : experiments) {
		if (e.title == arg1.toStdString()) {
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
	const Experiment::Run& run = e.runs.back(); //TODO: This should be fixed for run configuration
	ui->useLittle->setCheckState(Qt::Checked); //We need this! Else we run into the neither nor situation
	ui->exp_name->setText(QString::fromStdString(e.title));
	ui->exp_cleanup->setText(QString::fromStdString(e.cleanup));
	ui->exp_command->setText(QString::fromStdString(e.command));
	ui->exp_prepare->setText(QString::fromStdString(e.prepare));
	ui->cooldown->setValue(e.cooldown_time);
	ui->tail->setValue(e.tail_time);
	ui->useBig->setCheckState(run.big?Qt::Checked:Qt::Unchecked);
	ui->useLittle->setCheckState(run.little?Qt::Checked:Qt::Unchecked);
	for (int i = 0; i < ui->freq->count(); i++)
		if (ui->freq->itemText(i) == QString::number(run.freq)+" MHz")
			ui->freq->setCurrentIndex(i);
	for (int i = 0; i < ui->freq_max->count(); i++)
		if (ui->freq_max->itemText(i) == QString::number(run.freq_max)+" MHz")
			ui->freq_max->setCurrentIndex(i);
	for (int i = 0; i < ui->freq_min->count(); i++)
		if (ui->freq_min->itemText(i) == QString::number(run.freq_min)+" MHz")
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
	std::string cmd;
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
			cmd = toRun.back()->startMeasurement(lastTime,0); //TODO
			if (cmd.length() > 0) {
				qDebug() << "Configure";
				setupExperiment(toRun.back()->runs.back());
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
	for (DatapointBase const *p : descs)
		if (ui->dispUnit->findText(p->unit()) == -1)
			ui->dispUnit->addItem(p->unit());
	on_runNo_valueChanged(0);
	ui->expResult->setCurrentIndex(2);
}

Q_DECLARE_METATYPE(SimpleValue<double>*)

void OdroidReader::on_dispUnit_currentIndexChanged(int)
{
	QVector<double> ticks;
	ui->selectPower->clearPlottables();
	QBrush boxBrush(QColor(60, 60, 255, 100));
	boxBrush.setStyle(Qt::Dense6Pattern); // make it look oldschool
	std::vector<SimpleValue<double>*> vals;
	std::vector<int> dpId;
	QVector<QString> labels;
	if (ui->aggregate->isChecked()) {
//		bool init = true;
		//TODO: Aggregate is disabled for now!
//		for (std::vector<Datapoint<double>*> v : curExp->runs.front().repetitions.data) {
//			int i = 0;
//			int idx = 0;
//			for (Datapoint<double>* p : v) {
//				idx++;
//				if (p->unit() != ui->dispUnit->currentText()) continue;
//				if (init) {
//					vals.push_back(new SimpleValue<double>());
//					labels.push_back(p->name());
//					dpId.push_back(idx-1);
//				}
//				vals.at(i++)->add(p->value().avg(),0);
//			}
//			init = false;
//		}
	} else {
		std::pair<double,double> pair = curExp->runs.front().repetitions.at(ui->runNo->value());
		for (Datapoint<double>* p : descs) {
			if (p->unit() != ui->dispUnit->currentText()) continue;
			vals.push_back(new SimpleValue<double>(p->value(),pair.first,pair.second));
			labels.push_back(p->name());
		}
	}
	int i = 0;
	for (SimpleValue<double> *p : vals) {
		QCPStatisticalBox* b = new QCPStatisticalBox(ui->selectPower->xAxis,ui->selectPower->yAxis);
		b->setProperty("Datapoint",QVariant::fromValue(p));
		if (ui->aggregate->isChecked())
			b->setProperty("DP_ID",QVariant(dpId.at(i)));
		b->setBrush(boxBrush);
		b->setData(i,p->min(),p->quantile(0.25),p->median(),p->quantile(0.75),p->max());
		ui->selectPower->addPlottable(b);
		ticks.append(i++);
	};
	ui->selectPower->xAxis->setSubTickCount(0);
	ui->selectPower->xAxis->setTickLength(0, 4);
	ui->selectPower->xAxis->setTickLabelRotation(40);
	ui->selectPower->xAxis->setAutoTicks(false);
	ui->selectPower->xAxis->setAutoTickLabels(false);
	ui->selectPower->xAxis->setTickVector(ticks);
	ui->selectPower->xAxis->setTickVectorLabels(labels);
	ui->selectPower->setInteractions(QCP::iMultiSelect | QCP::iSelectPlottables);

	ui->selectPower->rescaleAxes();
	ui->selectPower->xAxis->scaleRange(1.1, ui->selectPower->xAxis->range().center());
	connect(ui->selectPower,SIGNAL(selectionChangedByUser()), this, SLOT(updateDetail()));
	if (ui->axisFromZero->isChecked())
		ui->selectPower->yAxis->setRangeLower(0);
	ui->selectPower->replot();
}

void OdroidReader::updateDetail() {
	ui->runPlot->clearPlottables();
	int i = 0;
	for (auto p : ui->selectPower->selectedPlottables()) {
		if (ui->aggregate->isChecked()) {
//			QVector<double> ticks;
//			QVector<QString> labels;
//			int x = p->property("DP_ID").value<int>();
//			int i = 0;
//			for (std::vector<Datapoint<double>*> v : curExp->runs.front().data) {
//				const Value<double> &p = v.at(x)->value();
//				QCPStatisticalBox* b = new QCPStatisticalBox(ui->runPlot->xAxis,ui->runPlot->yAxis);
//				b->setData(i,p.min(),p.quantile(0.25),p.median(),p.quantile(0.75),p.max());
//				ui->runPlot->addPlottable(b);
//				labels.append(QString("Run %1").arg(i+1));
//				ticks.append(i++);
//			}
//			ui->runPlot->xAxis->setTickVector(ticks);
//			ui->runPlot->xAxis->setTickVectorLabels(labels);
//			ui->runPlot->xAxis->setAutoTicks(false);
//			ui->runPlot->xAxis->setAutoTickLabels(false);
//			ui->runPlot->setInteractions(0);
			//Disabled for now! TODO
		} else {
			ui->runPlot->xAxis->setAutoTicks(true);
			ui->runPlot->xAxis->setAutoTickLabels(true);
			ui->runPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
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
	}
	ui->runPlot->rescaleAxes();
	if (ui->aggregate->isChecked())
		ui->runPlot->xAxis->scaleRange(1.1, ui->selectPower->xAxis->range().center());
	if (ui->axisFromZero->isChecked())
		ui->runPlot->yAxis->setRangeLower(0);
	ui->runPlot->replot();
}

void OdroidReader::on_runNo_valueChanged(int)
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
	if (checked) {
		ui->selectPower->setInteraction(QCP::iSelectPlottables);
	} else {
		ui->selectPower->setInteractions(QCP::iSelectPlottables | QCP::iMultiSelect);
	}
	on_dispUnit_currentIndexChanged(ui->dispUnit->currentIndex());
	updateDetail();
}

void OdroidReader::on_axisFromZero_toggled(bool)
{
	on_runNo_valueChanged(ui->runNo->value());
}
