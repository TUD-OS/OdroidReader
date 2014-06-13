//LD_LIBRARY_PATH=$LD_LIBRARY_PATH:. ./ffmpeg -i /sdcard/Movies/big_buck_bunny_1080p_h264.mov -f rawvideo /dev/null
//am start -S -a com.mxtech.intent.result.VIEW -n com.mxtech.videoplayer.ad/com.mxtech.videoplayer.ad.ActivityScreen -d file:////sdcard//Movies//big_buck_bunny_1080p_h264.mov
#include "odroidreader.h"
#include "ipvalidator.h"
#include "ui_odroidreader.h"
#include <QtNetwork/QTcpSocket>
#include <QMessageBox>
#include <cassert>
#include <QColor>
#include <QFile>
#include "qcustomplot.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileDialog>
#include "environmentdelegate.h"
#include <QJsonArray>
#include <ui/tabstyle.h>
#include <smartpowermonitor.h>
#include <Sources/networksource.h>

Q_DECLARE_METATYPE(const Experiment*)
Q_DECLARE_METATYPE(DataSource*)
Q_DECLARE_METATYPE(QListWidgetItem*)

QList<QColor> origcols({QColor(7,139,119),QColor(252,138,74),QColor(100,170,254),QColor(91,53,40),QColor(133,196,77),
					  QColor(104,115,15),QColor(133,3,43),QColor(188,186,111),QColor(168,115,19),QColor(63,184,67)});

OdroidReader::OdroidReader(QWidget *parent) :
	QMainWindow(parent), hasExecuted(false), isLoaded(false), es(ExperimentState::Idle),
	ui(new Ui::OdroidReader), currentExp(nullptr), currentEnv(nullptr), selectedExp(nullptr)
{
	ui->setupUi(this);
	ui->runSelected->setMenu(new QMenu("On Client"));
	ui->totalProgress->hide();
	ui->experimentProgress->hide();
	ui->environmentProgress->hide();
	ui->runAll->setMenu(new QMenu("On Client"));
	ui->ip->setValidator(new IPValidator());
	ui->sourceType->tabBar()->setStyle(new TabStyle(Qt::Horizontal));
	ui->sensors->setColumnWidth(0,30);
	ui->globalPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
	DeviceMonitor *dm = new SmartPowerMonitor();
	devMonitors.append(dm);
	dm->monitor(1000);
	connect(dm,&DeviceMonitor::addSource,[this] (DataSource* src) {
		ui->foundDevices->addItem(src->descriptor(),QVariant::fromValue(src));
	});
	connect(dm,&DeviceMonitor::removeSource, [this] (DataSource* src) {
		if (sources.contains(src)) {
			qDebug() << "Removing Source!";
			if (src->isRunning()) src->start(); //Terminate if running
			sources.remove(sources.indexOf(src));
			delete src->property("Item").value<QListWidgetItem*>();
		}
		//TODO remove from sources if it has been added!
	   qDebug() << "Removed Source!";
	   ui->foundDevices->removeItem(ui->foundDevices->findData(QVariant::fromValue(src)));
	});
    ui->stopSampling->hide();
	ui->loadProgress->hide();

	connect(ui->runSelected->menu(),&QMenu::triggered,this,&OdroidReader::runSelectedOnSource);
	connect(ui->runAll->menu(),&QMenu::triggered,this,&OdroidReader::runAllOnSource);
	connect(ui->environment,SIGNAL(clicked(QModelIndex)),this,SLOT(removeEnvironment(QModelIndex)));
    connect(ui->stopSampling,&QPushButton::clicked,[this]() {
        ui->addDevice->setEnabled(true);
        ui->addConnection->setEnabled(true);
	});
    connect(ui->startSampling,&QPushButton::clicked,[this]() {
		hasExecuted = true;
        ui->addDevice->setEnabled(false);
        ui->addConnection->setEnabled(false);
	});
    connect(ui->startSampling,&QPushButton::clicked,[this] () {
        ui->startSampling->hide();
        ui->stopSampling->show();
		ui->repetitions->setEnabled(true);
	});
    connect(ui->stopSampling,&QPushButton::clicked,[this] () {
        for (DataSource* src : sources) src->stop();
        ui->startSampling->show();
        ui->stopSampling->hide();
		ui->totalProgress->hide();
		ui->experimentProgress->hide();
		ui->environmentProgress->hide();
		ui->repetitions->setDisabled(true);
	});
    connect(qApp,SIGNAL(aboutToQuit()),this,SLOT(aboutToQuit()));
	connect(&envs,&Environments::addedSet, [this] (const EnvironmentSet* e) {
		ui->availEnvs->addItem(e->name());
	});

	connect(&envs,&Environments::removingSet, [this] (const EnvironmentSet* e) {
		if (ui->availEnvs->findText(e->name()) == -1) return;
		ui->availEnvs->removeItem(ui->availEnvs->findText(e->name()));
		for (QListWidgetItem *i : ui->expEnvs->findItems(e->name(),Qt::MatchCaseSensitive))
			delete i;
	});
	connect(&envs,&Environments::renamingSet, [this] (QString oldName, QString newName) {
		int oldIdx = ui->availEnvs->findText(oldName);
		if (oldIdx == -1) return;
		ui->availEnvs->removeItem(oldIdx);
		ui->availEnvs->insertItem(oldIdx,newName);
		for (QListWidgetItem *i : ui->expEnvs->findItems(oldName,Qt::MatchCaseSensitive))
			i->setText(newName);
	});

	//Load state ...
	QFile f("experiments.json");
	if (!f.open(QFile::ReadOnly)) {
		qWarning() << "Could not read experiments!";
		return;
	};

	QJsonObject expData = QJsonDocument::fromJson(f.readAll()).object();
	for (const QJsonValue& v : expData["environments"].toArray())
		envs.addEnvironment(new Environment(v.toObject()));
	for (const QJsonValue& v : expData["sets"].toArray()) {
		QString name = v.toObject()["name"].toString();
		EnvironmentSet* s = envs.addSet(name);
		for (const QJsonValue& idx : v.toObject()["items"].toArray()) {
			s->addEnvironment(envs.environments()[idx.toInt()]);
		}
		ui->envSets->addItem(name);
	}
	for (const QJsonValue& v : expData["experiments"].toArray())
		experiments.append(new Experiment(v.toObject(), data, envs));
	updateExperiments();
	ui->environment->setModel(&envs);
	ui->environment->setItemDelegate(new EnvironmentDelegate(ui->environment));
	ui->environment->resizeColumnsToContents();
}

OdroidReader::~OdroidReader()
{
	delete ui;
	for (auto d : data) delete d;
	for (auto d : devMonitors) delete d;
}

void OdroidReader::updateCurve(int row, int col) {
	if (col != 0) return;
	bool enable = ui->sensors->item(row,col)->checkState() == Qt::Checked;
	if (enable && graphs.at(row) == nullptr) {
		if (origcols.count() == 0) {
			ui->sensors->item(row,col)->setCheckState(Qt::Unchecked);
			return;
		}
		QColor color = origcols.takeFirst();
		graphs[row] = ui->globalPlot->addGraph();
		graphs[row]->setPen(color);
		graphs.at(row)->setData(data.at(rowMap[row])->getTimestamps(), data.at(rowMap[row])->getValues());
		connect(data.at(rowMap[row]),&DataSeries::newValue,[row,this](double time, double value) {
			graphs.at(row)->addData(time,value);
			ui->globalPlot->rescaleAxes();
			ui->globalPlot->yAxis->scaleRange(1.2,ui->globalPlot->yAxis->range().center());
			ui->globalPlot->replot();
		});
		ui->sensors->item(row,col)->setBackgroundColor(color);
	} else if (!enable && graphs.at(row) != nullptr){
		disconnect(data.at(rowMap[row]),SIGNAL(newValue(double,double)),0,0);
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
	rowMap.clear();
	//TODO: Shound we remove old data?
	int i = 0;
	for (DataSeries* d : data) {
		if (d == nullptr) continue;
		rowMap.push_back(d->descriptor->uid());
		ui->sensors->setItem(i,0,new QTableWidgetItem(""));
		ui->sensors->item(i,0)->setCheckState(Qt::Unchecked);
		ui->sensors->setItem(i,1,new QTableWidgetItem(d->descriptor->name()));
        ui->sensors->setItem(i,2,new QTableWidgetItem(QString::number(d->getLast()))); //last
        ui->sensors->setItem(i,3,new QTableWidgetItem(QString::number(d->getMin()))); //min
        ui->sensors->setItem(i,4,new QTableWidgetItem(QString::number(d->getMax()))); //max
        ui->sensors->setItem(i,5,new QTableWidgetItem(QString::number(d->getAvg()))); //avg
		connect(d,&DataSeries::newValue,[this,i](double, double val) { ui->sensors->item(i,2)->setText(QString::number(val));});
		connect(d,&DataSeries::newMin,[this,i](double val) { ui->sensors->item(i,3)->setText(QString::number(val));});
		connect(d,&DataSeries::newMax,[this,i](double val) { ui->sensors->item(i,4)->setText(QString::number(val));});
		connect(d,&DataSeries::newAvg,[this,i](double val) { ui->sensors->item(i,5)->setText(QString::number(val));});
		ui->sensors->setItem(i++,6,new QTableWidgetItem(d->descriptor->unit()));
	}
	connect(ui->sensors,SIGNAL(cellChanged(int,int)),this,SLOT(updateCurve(int,int)));
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
	jo["command"] = ui->exp_command->text();
	jo["environments"] = QJsonArray();
	jo["cooldown_time"] = ui->cooldown->value();
	jo["tail_time"] = ui->tail->value();
	Experiment* exp = new Experiment(jo,data,envs);
	experiments.append(exp);
	updateExperiments();
}

void OdroidReader::on_exp_name_textChanged(const QString &arg1)
{
//	if (arg1.isEmpty()) { ui->addExperiment->setEnabled(false); }
	for (Experiment* e : experiments) {
		if (e->title == arg1) {
			ui->addExperiment->setEnabled(false);
			return;
		}
	}
//	ui->addExperiment->setEnabled(true);
}

void OdroidReader::on_listWidget_itemSelectionChanged()
{
	disconnect(ui->exp_name,SIGNAL(textChanged(QString)),0,0);
	disconnect(ui->exp_prepare,SIGNAL(textChanged(QString)),0,0);
	disconnect(ui->exp_cleanup,SIGNAL(textChanged(QString)),0,0);
	disconnect(ui->exp_command,SIGNAL(textChanged(QString)),0,0);
	disconnect(ui->cooldown,SIGNAL(valueChanged(int)),0,0);
	disconnect(ui->tail,SIGNAL(valueChanged(int)),0,0);
	bool isSelected = (ui->listWidget->selectedItems().size() != 0);
	selectedExp = nullptr;
	if (!isSelected || ui->listWidget->currentRow() == -1) return;
	ui->runSelected->setEnabled(toRun.empty() && ui->runSelected->menu()->actions().size() != 0);
	ui->removeExperiment->setEnabled(isSelected);
	selectedExp = experiments[ui->listWidget->currentRow()];
	ui->exp_name->setText(selectedExp->title);
	ui->exp_prepare->setText(selectedExp->prepare);
	ui->exp_cleanup->setText(selectedExp->cleanup);
	ui->exp_command->setText(selectedExp->command);
	ui->cooldown->setValue(selectedExp->cooldown_time);
	ui->tail->setValue(selectedExp->tail_time);
	ui->expEnvs->clear();
	for (const EnvironmentSet* e : selectedExp->envSets)
		ui->expEnvs->addItem(e->name());
	connect(ui->exp_name,SIGNAL(textChanged(QString)),selectedExp,SLOT(setTitle(QString)));
	connect(ui->exp_name,&QLineEdit::textChanged,[this](const QString &text) {ui->listWidget->selectedItems().first()->setText(text); });
	connect(ui->exp_prepare,SIGNAL(textChanged(QString)),selectedExp,SLOT(setPrepare(QString)));
	connect(ui->exp_cleanup,SIGNAL(textChanged(QString)),selectedExp,SLOT(setCleanup(QString)));
	connect(ui->exp_command,SIGNAL(textChanged(QString)),selectedExp,SLOT(setCommand(QString)));
	connect(ui->cooldown,SIGNAL(valueChanged(int)),selectedExp,SLOT(setCooldownTime(int)));
	connect(ui->tail,SIGNAL(valueChanged(int)),selectedExp,SLOT(setTailTime(int)));
}

void OdroidReader::on_removeExperiment_clicked()
{
	experiments.remove(ui->listWidget->currentRow());
	updateExperiments();
}

void OdroidReader::runExperiments(DataSource& source, double time = 0) {
	assert(source.canExecute());
	switch (es) {
		case ExperimentState::Idle:
			es = ExperimentState::Prepare;
			source.execute(toRun.back()->prepareMeasurement());
			break;
		case ExperimentState::Prepare:
			if (toRun.back()->envSets.size() <= lastSet) {
				qDebug() << "No sets selected!";
				disconnect(&source,SIGNAL(commandFinished(DataSource&,double)),0,0);
				return;
			}
			if (toRun.back()->envSets.at(lastSet)->environments().size() <= lastEnv) {
				qDebug() << "No envs in selected set!";
				disconnect(&source,SIGNAL(commandFinished(DataSource&,double)),0,0);
				return;
			}
			source.setupEnvironment(toRun.back()->envSets.at(lastSet)->environments().at(lastEnv));
			es = ExperimentState::Execute;
			qDebug() << "Start @" << time;
			source.execute(toRun.back()->startMeasurement(time,toRun.back()->envSets.at(lastSet)->environments().at(lastEnv)));
			break;
		case ExperimentState::Execute:
			es = ExperimentState::Cleanup;
			{
				QTimer* tmr = new QTimer();
				tmr->setSingleShot(true);
				connect(tmr,&QTimer::timeout,[&source,this,tmr,time] () {
					source.execute(toRun.back()->cleanupMeasurement(time+toRun.back()->cooldown_time));
					tmr->deleteLater();
				});
				tmr->start(toRun.back()->cooldown_time*1000);
			}
			if (toRun.back()->cooldown_time != 0)
				qDebug() << "Stop @" << time << "+" << (toRun.back()->cooldown_time);
			else
				qDebug() << "Stop @" << time;
			break;
		case ExperimentState::Cleanup:
			toRun.back()->finishedCleanup(time);
			ui->environmentProgress->setValue(ui->environmentProgress->value()+1);
			ui->experimentProgress->setValue(ui->experimentProgress->value()+1);
			ui->totalProgress->setValue(ui->totalProgress->value()+1);
			if (++repetition == ui->repetitions->value()) {
				if (++lastEnv == toRun.back()->envSets.at(lastSet)->environments().size()) {
					if (++lastSet == toRun.back()->envSets.size()) {
						qDebug() << "Poping last of " << toRun.size();
						toRun.pop_back();
						if (toRun.size() > 0) {
							ui->experimentProgress->setMaximum(toRun.back()->executions()*ui->repetitions->value());
							ui->experimentProgress->setValue(0);
							ui->experimentProgress->setFormat(QString("Experiment: %1 (Environment: %v/%m)").arg(toRun.back()->title));
						}
						lastEnv = 0;
						lastSet = 0;
					} else {
						lastEnv = 0;
					}
				}
				ui->environmentProgress->setValue(0);
				ui->environmentProgress->setMaximum(ui->repetitions->value());
				if (toRun.size() > 0) {
					ui->environmentProgress->setFormat(QString("Environment: %1 (Run: %v/%m)").arg(toRun.back()->envSets.at(lastSet)->environments().at(lastEnv)->label));
				}
				repetition = 0;
			}
			ui->environmentProgress->setValue(repetition);
			es = ExperimentState::Idle;
			if (!toRun.empty()) {
				qDebug() << "Tailtime: " << toRun.back()->tail_time;
				if (toRun.back()->tail_time != 0) {
					QTimer* tmr = new QTimer();
					tmr->setSingleShot(true);
					connect(tmr,&QTimer::timeout,[&source,this,tmr] () {
						tmr->deleteLater();
						runExperiments(source);
					});
					tmr->start(toRun.back()->tail_time*1000);
				} else {
					runExperiments(source,time);
				}
			} else {
				qDebug() << "DONE!";
				ui->expSelect->clear();
				for (const Experiment* e : experiments)
					if (e->hasData())
						ui->expSelect->addItem(e->title,QVariant::fromValue(e));
				ui->experiments->setEnabled(true);
				disconnect(&source,SIGNAL(commandFinished(DataSource&,double)),0,0);
			}
	}
}

void OdroidReader::runAllOnSource(const QAction *act)
{
	DataSource* ds = act->property("Source").value<DataSource*>();
	assert(ds->canExecute() && toRun.empty());
	int runs = 0;
	for (int i = 0; i < ui->listWidget->count(); i++) {
		if (ui->listWidget->item(i)->checkState() != Qt::Checked) continue;
		toRun.push_back(experiments[i]);
		runs += experiments[i]->executions();
	}
	runs *= ui->repetitions->value();
	repetition = 0;
	lastEnv = 0;
	lastSet = 0;
	if (toRun.size() != 0) {
		//ui->experimentProgress->setFormat(QString("Experiment: %1 (Environment: %v/%m)").arg(toRun.back()->title));
		ui->experimentProgress->setMaximum(toRun.back()->executions()*ui->repetitions->value());
		ui->experimentProgress->setValue(0);
		ui->environmentProgress->setFormat(QString("Environment: %1 (Run: %v/%m)").arg(toRun.back()->envSets.at(lastSet)->environments().at(lastEnv)->label));
		ui->environmentProgress->setMaximum(ui->repetitions->value());
		ui->environmentProgress->setValue(0);
//		ui->totalProgress->setFormat("Total: %p% (Experiment: %v/%m)");
		ui->totalProgress->setMaximum(runs);
		ui->totalProgress->setValue(0);
	}
	es = ExperimentState::Idle;
	ui->experiments->setEnabled(false);
	connect(ds,SIGNAL(commandFinished(DataSource&,double)),this,SLOT(runExperiments(DataSource&,double)));
	ui->totalProgress->show();
	ui->experimentProgress->show();
	ui->environmentProgress->show();
	runExperiments(*ds);
}

void OdroidReader::runSelectedOnSource(const QAction *act)
{
	DataSource* ds = act->property("Source").value<DataSource*>();
	assert(ds->canExecute() && toRun.empty());
	toRun.push_back(experiments[ui->listWidget->currentRow()]);
	unsigned runs = toRun.back()->executions()*ui->repetitions->value();
	ui->experimentProgress->setMaximum(runs);
	ui->experimentProgress->setValue(0);
	ui->environmentProgress->setMaximum(ui->repetitions->value());
	ui->environmentProgress->setValue(0);
	ui->totalProgress->setMaximum(runs);
	ui->totalProgress->setValue(0);
	es = ExperimentState::Idle;
	ui->experiments->setEnabled(false);
	repetition = 0;
	lastEnv = 0;
	lastSet = 0;
	ui->environmentProgress->setFormat(QString("Environment: %1 (Run: %v/%m)").arg(toRun.back()->envSets.at(lastSet)->environments().at(lastEnv)->label));
//	ui->experimentProgress->setFormat(QString("Experiment: %1 (Environment: %v/%m)").arg(toRun.back()->title));
//	ui->totalProgress->setFormat("Total: %p% (Experiment: %v/%m)");
	connect(ds,SIGNAL(commandFinished(DataSource&,double)),this,SLOT(runExperiments(DataSource&,double)));
	ui->totalProgress->show();
	ui->experimentProgress->show();
	ui->environmentProgress->show();
	runExperiments(*ds);
}

void OdroidReader::aboutToQuit()
{
	if (isLoaded) return; //Do not save if we just looked at data
	//Saving state...
	QFile f("experiments.json");
	if (!f.open(QFile::WriteOnly)) {
		qWarning() << "Could not save experiments!";
	};
	QJsonArray environmentArray;
	for (const Environment* env : envs.environments()) {
		QJsonObject e;
		env->write(e);
		environmentArray.append(e);
	}
	QJsonArray experimentArray, runArray, setArray;
	for (Experiment* e : experiments) {
		QJsonObject experimentData,experimentRun;
		e->write(experimentData);
		e->write(experimentRun,true);
		runArray.append(experimentRun);
		experimentArray.append(experimentData);
	}
	for (const EnvironmentSet* s : envs.sets()) {
		QJsonObject set;
		set["name"] = s->name();
		QJsonArray items;
		for (const Environment* e : s->environments()) {
			items.push_back(envs.index(e));
		}
		set["items"] = items;
		setArray.push_back(set);
	}
	if (hasExecuted) {
		QFile res("Results_"+QDateTime::currentDateTime().toString()+".json.z");
		QJsonObject testRun;
		if (!res.open(QFile::WriteOnly)) {
			qWarning() << "Could not save runs :(!";
		}
		QJsonArray datas;
		//TODO: need to rework to new data model!
		for (DataSeries *ds : data) {
			QJsonObject series;
			series["descriptor"] = ds->descriptor->json();
			series["data"] = ds->json();
			datas.push_back(series);
		}
		testRun["dataseries"] = datas;
		testRun["experimentRuns"] = runArray;
		res.write(qCompress(QJsonDocument(testRun).toJson()));
	}
	QJsonObject expData;
	expData["environments"] = environmentArray;
	expData["experiments"] = experimentArray;
	expData["sets"] = setArray;
	f.write(QJsonDocument(expData).toJson());
}

void OdroidReader::on_envAdd_clicked()
{
	QAbstractItemModel *em = ui->environment->model();
	qDebug() << em;
	if (em) em->insertRow(em->rowCount());
}

void OdroidReader::removeEnvironment(QModelIndex idx) {
	if (idx.column() != 7) return;
	QAbstractItemModel *em = ui->environment->model();
	if (em) em->removeRow(idx.row());
}

void OdroidReader::on_pushButton_clicked()
{
	if (ui->expSelect->currentData().value<const Experiment*>()) {
		DataExplorer* de = new DataExplorer(ui->scrollAreaWidgetContents);
		de->setExperiment(ui->expSelect->currentData().value<const Experiment*>());
		ui->scrollAreaWidgetContents->layout()->addWidget(de);
		connect(de,&DataExplorer::removeMe,[de]() { de->deleteLater(); });
	}
}

void OdroidReader::addData(const DataDescriptor *desc, double value, double time) {
	assert(data.size() > static_cast<signed int>(desc->uid()));
	data.at(desc->uid())->addValue(time,value);
}

void OdroidReader::addDescriptors(QVector<const DataDescriptor *> descs) {
    qDebug() << "Adding" << descs.size() << "descriptors.";
	for (const DataDescriptor* d : descs) {
		if (static_cast<signed int>(d->uid()) >= data.size()) //!TODO
			data.resize(d->uid()+1);
        if (data.at(d->uid()) == nullptr)
            data[d->uid()] = new DataSeries(d);
	}
	if (graphs.size() < data.size()) graphs.resize(data.size());
	updateSensors();
}

void OdroidReader::on_addConnection_clicked()
{
	NetworkSource *ns = new NetworkSource(ui->sourceName->text(),ui->ip->text(),ui->port->value(),ui->samplingInterval->value());
	connect(ui->startSampling,SIGNAL(clicked()),ns,SLOT(start()));
	connect(ns,SIGNAL(descriptorsAvailable(QVector<const DataDescriptor*>)),this,SLOT(addDescriptors(QVector<const DataDescriptor*>)));
	connect(ns,SIGNAL(dataAvailable(const DataDescriptor*,double,double)),this,SLOT(addData(const DataDescriptor*,double,double)));
	sources.append(ns);
	QListWidgetItem* lwi = new QListWidgetItem(QIcon::fromTheme("network-disconnect"),ns->descriptor());
	ns->setProperty("Item",QVariant::fromValue(lwi));
	ui->sourceList->addItem(lwi);
	connect(ns,&NetworkSource::connected, [lwi,this]() {
		lwi->setIcon(QIcon::fromTheme("network-connect"));
	});
	connect(ns,&NetworkSource::disconnected, [lwi,this]() {
		lwi->setIcon(QIcon::fromTheme("network-disconnect"));
	});
	updateRunnables();
}

void OdroidReader::on_addDevice_clicked()
{
    DataSource* src = ui->foundDevices->currentData().value<DataSource*>();
	((OdroidSmartPowerSource*)src)->setInterval(ui->deviceSI->value());
    for (DataSource *s: sources) {
        if (s->descriptor() == src->descriptor()) {
            qDebug() << "Can only be added once!";
            return;
        }
    }
    connect(ui->startSampling,SIGNAL(clicked()),src,SLOT(start()));
    connect(src,SIGNAL(descriptorsAvailable(QVector<const DataDescriptor*>)),this,SLOT(addDescriptors(QVector<const DataDescriptor*>)));
    connect(src,SIGNAL(dataAvailable(const DataDescriptor*,double,double)),this,SLOT(addData(const DataDescriptor*,double,double)));
    sources.append(src);
    QListWidgetItem* lwi = new QListWidgetItem(QIcon::fromTheme("network-disconnect"),src->descriptor());
    src->setProperty("Item",QVariant::fromValue(lwi));
    ui->sourceList->addItem(lwi);
    connect(src,&NetworkSource::connected, [lwi,this]() {
        lwi->setIcon(QIcon::fromTheme("network-connect"));
    });
    connect(src,&NetworkSource::disconnected, [lwi,this]() {
        lwi->setIcon(QIcon::fromTheme("network-disconnect"));
    });
    updateRunnables();
}

void OdroidReader::updateRunnables() {
    ui->runSelected->menu()->clear();
	ui->runAll->menu()->clear();
    for (DataSource* ds : sources) {
        if (ds->canExecute()) {
            QAction* a = ui->runSelected->menu()->addAction(ds->descriptor());
            a->setProperty("Source",QVariant::fromValue(ds));
			a = ui->runAll->menu()->addAction(ds->descriptor());
			a->setProperty("Source",QVariant::fromValue(ds));
        }
    }
}

void OdroidReader::on_loadData_clicked()
{
	if (hasExecuted) {
		int r = QMessageBox::question(this,"Discard experiment data?","Loading another experiment will discard all existing experiment data and experiment definitions! Do you really want to do this?");
		if (r == QMessageBox::No) return;
	}
	QString fileName = QFileDialog::getOpenFileName(this, "Open Experiment","","Compressed experiments (*.json.z)");
	if (fileName.isNull()) return;
	QFile f(fileName);
	if (!f.open(QFile::ReadOnly)) {
		QMessageBox::information(this,"Unable to load file","The experiment file "+fileName+" could not be opened for reading!");
		return;
	};
	hasExecuted = false;
	isLoaded = true;
	experiments.clear();
	data.clear();
	ui->sampleTab->setEnabled(false);

	QJsonObject experimentObject = QJsonDocument::fromJson(qUncompress(f.readAll())).object();
	ui->loadProgress->show();
	QJsonArray ea = experimentObject["experimentRuns"].toArray();
	for (int exp = 0; exp < ea.size(); ++exp) {
		QJsonObject experimentData = ea[exp].toObject();
		experiments.append(new Experiment(experimentData, data, envs));
	}
	QJsonArray da = experimentObject["dataseries"].toArray();
	ui->loadProgress->setValue(0);
	QVector<const DataDescriptor*> descs;
	for (int d = 0; d < da.size(); ++d) {
		QJsonObject datao = da.at(d).toObject();
		descs.push_back(new DataDescriptor(datao["descriptor"].toObject()));
	}
	addDescriptors(descs);
	for (int d = 0; d < da.size(); ++d) {
		QJsonObject datao = da.at(d).toObject();
		data[descs[d]->uid()]->fromJson(datao["data"].toObject());
		ui->loadProgress->setValue((d+1)*100.0/da.size());
		QCoreApplication::processEvents(); //TODO: Hack
	}
	ui->expSelect->clear();
	for (const Experiment* e : experiments)
		if (e->hasData())
			ui->expSelect->addItem(e->title,QVariant::fromValue(e));
	ui->loadProgress->hide();
	updateExperiments();
}

void OdroidReader::on_envSetAdd_clicked()
{
	QString title = ui->setTitle->text();
	if (envs.findSet(title)) {
		int i = 1;
		while (envs.findSet(title+QString(" (%1)").arg(i))) i++;
		title += QString(" (%1)").arg(i);
	}
	EnvironmentSet* s = envs.addSet(title);
	for (const QModelIndex& i : ui->environment->selectionModel()->selectedRows()) {
		s->addEnvironment(envs.at(i.row()));
	}
	ui->envSets->addItem(title);
}

void OdroidReader::on_envSets_currentRowChanged(int currentRow)
{
	disconnect(ui->setTitle,SIGNAL(textChanged(QString)),0,0);
	if (currentRow == -1) {
		ui->envSetRemove->setEnabled(false);
		ui->envSetUpdate->setEnabled(false);
		return;
	}
	ui->envSetRemove->setEnabled(true);
	ui->envSetUpdate->setEnabled(true);
	QString title = ui->envSets->item(currentRow)->text();
	ui->setTitle->setText(title);
	connect(ui->setTitle,&QLineEdit::textChanged,[this] (const QString& t) {
		QString old = ui->envSets->currentItem()->text();
		if (envs.findSet(t) && t != old ) {
			ui->setTitle->setText(old);
			return;
		}
		ui->envSets->currentItem()->setText(t);
		envs.findSet(old)->rename(t);
	});
	ui->environment->clearSelection();
	for (const Environment* e : envs.findSet(title)->environments()) {
		ui->environment->selectRow(envs.index(e));
	}
}

void OdroidReader::on_envSetRemove_clicked()
{
	assert(ui->envSets->currentRow() != -1);
	envs.removeSet(envs.findSet(ui->envSets->currentItem()->text()));
	delete ui->envSets->currentItem();
	ui->envSetRemove->setEnabled(false);
	ui->envSetUpdate->setEnabled(false);
}

void OdroidReader::on_envSetUpdate_clicked()
{
	assert(ui->envSets->currentRow() != -1);
	QString title = ui->envSets->currentItem()->text();
	envs.findSet(title)->clear();
	for (const QModelIndex& i : ui->environment->selectionModel()->selectedRows()) {
		envs.findSet(title)->addEnvironment(envs.at(i.row()));
	}
}

void OdroidReader::on_addEnvironment_clicked()
{
	QString set = ui->availEnvs->currentText();
	if (selectedExp == nullptr) {
		qDebug() << "No experiment selected!";
		return;
	}
	if (!ui->expEnvs->findItems(set,Qt::MatchCaseSensitive).empty()) {
		qDebug() << "Already added!";
		return;
	}
	ui->expEnvs->addItem(set);
	selectedExp->envSets.append(envs.findSet(set));
}

void OdroidReader::on_pushButton_2_clicked()
{
	if (ui->expEnvs->selectedItems().size() != 1) {
		qDebug() << "Nothing was selected!";
		return;
	}
	if (selectedExp == nullptr) {
		qDebug() << "No experiment selected!";
		return;
	}
	selectedExp->envSets.removeAll(envs.findSet(ui->expEnvs->selectedItems().first()->text()));
	delete ui->expEnvs->selectedItems().first();
}
