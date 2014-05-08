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

OdroidReader::OdroidReader(QWidget *parent) :
	QMainWindow(parent), executed(false), es(ExperimentState::Idle),
	ui(new Ui::OdroidReader),
	sock(nullptr), currentExp(nullptr), currentEnv(nullptr)
{
	ui->setupUi(this);
	ui->ip->setValidator(new IPValidator());
	ui->sensors->setColumnWidth(0,30);
	ui->globalPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

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
		experiments.append(new Experiment(experimentData, data));
	}
	updateExperiments();
}

OdroidReader::~OdroidReader()
{
	delete ui;
	for (auto d : data) delete d;
}

void OdroidReader::enableControls(bool status = true) {
	ui->sample->setEnabled(status);
}

void OdroidReader::updateCurve(int row, int col) {
	if (col != 0) return;
	//Datapoint<double>* d = data.at(row);
	bool enable = ui->sensors->item(row,col)->checkState() == Qt::Checked;
	if (enable && graphs.at(row) == nullptr) {
		if (origcols.count() == 0) {
			ui->sensors->item(row,col)->setCheckState(Qt::Unchecked);
			return;
		}
		QColor color = origcols.takeFirst();
		graphs[row] = ui->globalPlot->addGraph();
		graphs[row]->setPen(color);
		graphs.at(row)->setData(data.at(row)->getTimestamps(), data.at(row)->getValues());
		connect(data.at(row),&DataSeries::newValue,[row,this](double time, double value) {
			graphs.at(row)->addData(time,value);
			ui->globalPlot->rescaleAxes();
			ui->globalPlot->yAxis->scaleRange(1.2,ui->globalPlot->yAxis->range().center());
			ui->globalPlot->replot();
		});
		ui->sensors->item(row,col)->setBackgroundColor(color);
	} else if (!enable && graphs.at(row) != nullptr){
		disconnect(data.at(row),SIGNAL(newValue(double,double)),0,0);
		origcols.push_front(graphs.at(row)->pen().color());
		ui->globalPlot->removeGraph(graphs.at(row));
		graphs[row] = nullptr;
		ui->sensors->item(row,col)->setBackgroundColor(Qt::white);
	}
	ui->globalPlot->rescaleAxes();
	ui->globalPlot->yAxis->scaleRange(1.2,ui->globalPlot->yAxis->range().center());
	ui->globalPlot->replot();
}

void OdroidReader::updateSensors() {
	ui->sensors->setRowCount(data.size());
	//TODO: Shound we remove old data?
	int i = 0;
	for (DataSeries* d : data) {
		if (d == nullptr) continue;
		ui->sensors->setItem(i,0,new QTableWidgetItem(""));
		ui->sensors->item(i,0)->setCheckState(Qt::Unchecked);
		ui->sensors->setItem(i,1,new QTableWidgetItem(d->descriptor->name()));
		ui->sensors->setItem(i,2,new QTableWidgetItem("0")); //last
		ui->sensors->setItem(i,3,new QTableWidgetItem("0")); //min
		ui->sensors->setItem(i,4,new QTableWidgetItem("0")); //max
		ui->sensors->setItem(i,5,new QTableWidgetItem("0")); //max
		connect(d,&DataSeries::newValue,[this,i](double, double val) { ui->sensors->item(i,2)->setText(QString::number(val));});
		connect(d,&DataSeries::newMin,[this,i](double val) { ui->sensors->item(i,3)->setText(QString::number(val));});
		connect(d,&DataSeries::newMax,[this,i](double val) { ui->sensors->item(i,4)->setText(QString::number(val));});
		connect(d,&DataSeries::newAvg,[this,i](double val) { ui->sensors->item(i,5)->setText(QString::number(val));});
		ui->sensors->setItem(i++,6,new QTableWidgetItem(d->descriptor->unit()));
	}
	connect(ui->sensors,SIGNAL(cellChanged(int,int)),this,SLOT(updateCurve(int,int)));
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

quint32 OdroidReader::freq2Int(QString s) {
	s.chop(4);
	return s.toInt();
}

void OdroidReader::updateExperiments() {
	ui->listWidget->clear();
	for (Experiment* e : experiments) {
		QListWidgetItem* lwi = new QListWidgetItem(e->title);
		lwi->setCheckState(Qt::Checked);
		ui->listWidget->addItem(lwi);
	}
}

void OdroidReader::on_addExperiment_clicked()
{
	QJsonObject jo;
	jo["title"] = ui->exp_name->text();
	jo["prepare"] = ui->exp_prepare->text();
	jo["cleanup"] = ui->exp_cleanup->text();
	jo["command"] = ui->exp_cleanup->text();
	jo["environments"] = QJsonArray();
	jo["cooldown_time"] = ui->cooldown->value();
	jo["tail_time"] = ui->tail->value();
	Experiment* exp = new Experiment(jo,data);
	experiments.append(exp);
	ui->addExperiment->setEnabled(false);
	updateExperiments();
}

void OdroidReader::on_exp_name_textChanged(const QString &arg1)
{
	if (arg1.isEmpty()) { ui->addExperiment->setEnabled(false); }
	for (Experiment* e : experiments) {
		if (e->title == arg1) {
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
	currentExp = experiments[ui->listWidget->currentRow()];
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

//TODO
/*
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
*/
void OdroidReader::on_runSelected_clicked()
{
	if (!sock || !sock->isWritable()) return;
	toRun.clear();
	toRun.push_back(experiments[ui->listWidget->currentRow()]);
	es = ExperimentState::Idle;
	ui->experiments->setEnabled(false);
	repetition = 0;
	lastEnv = 0;
//TODO	runExperiments();
}

void OdroidReader::aboutToQuit()
{
	//Saving state...
	QFile f("experiments.json");
	if (!f.open(QFile::WriteOnly)) {
		qWarning() << "Could not save experiments!";
	};
	QJsonArray experimentArray;
	for (Experiment* e : experiments) {
		QJsonObject experimentData;
		e->write(experimentData);
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
	NetworkSource *ns = new NetworkSource(ui->sourceName->text(),ui->ip->text(),ui->port->value(),ui->samplingInterval->value());
	sources.append(ns);
	connect(ui->startSampling,SIGNAL(clicked()),ns,SLOT(start()));
	connect(ns,SIGNAL(descriptorsAvailable(QVector<const DataDescriptor*>)),this,SLOT(addDescriptors(QVector<const DataDescriptor*>)));
	connect(ns,SIGNAL(dataAvailable(const DataDescriptor*,double,double)),this,SLOT(addData(const DataDescriptor*,double,double)));
	ui->sourceList->addItem(ns->descriptor());
}

void OdroidReader::addData(const DataDescriptor *desc, double value, double time) {
	assert(data.size() > desc->uid());
	data.at(desc->uid())->addValue(time,value);
}

void OdroidReader::addDescriptors(QVector<const DataDescriptor *> descs) {
	for (const DataDescriptor* d : descs) {
		if (d->uid() >= data.size())
			data.resize(d->uid()+1);
		data[d->uid()] = new DataSeries(d);
		qDebug() << d->str();
	}
	if (graphs.size() < data.size()) graphs.resize(data.size());
	updateSensors();
}
