#include "dataexplorer.h"
#include "ui_dataexplorer.h"
#include <datapoint.h>

Q_DECLARE_METATYPE(SimpleValue<double>*)
static QList<QColor> origcols({QColor(7,139,119),QColor(252,138,74),QColor(100,170,254),QColor(91,53,40),QColor(133,196,77),
					  QColor(104,115,15),QColor(133,3,43),QColor(188,186,111),QColor(168,115,19),QColor(63,184,67)});


DataExplorer::DataExplorer(QWidget *parent) :
	QWidget(parent), exp(nullptr),
	ui(new Ui::DataExplorer)
{
	ui->setupUi(this);
}

DataExplorer::~DataExplorer()
{
	delete ui;
}

void DataExplorer::updateDetails() {
    ui->runPlot->clearPlottables();
    qDebug() << "Details";
    int colid = 0;
    for (QCPAbstractPlottable *p : ui->selectEnvironment->selectedPlottables()) {
        int unitid = p->property("UID").toInt();
        int envid = p->property("EID").toInt();
        SimpleValue<double> v = exp->environments.at(envid).run(unitid,ui->runNo->value(),*exp);
        QVector<double> values;
        QVector<double> keys;
        for (QPair<double,double> pair : v.values()) {
            values.append(pair.second);
            keys.append(pair.first);
        }
        QCPGraph *g = ui->runPlot->addGraph();
        g->setPen(origcols[colid++%origcols.size()]);
        qDebug() << "Setting" << keys.size() << "Datapoints";
        g->setData(keys,values);
    }
    ui->runPlot->rescaleAxes();
    if (ui->axisFromZero->isChecked())
        ui->runPlot->yAxis->setRangeLower(0);
    ui->runPlot->replot();
}

void DataExplorer::updateEnvironment() {
	ui->selectEnvironment->clearPlottables();
	int idx = 0;
    qDebug() << "Starting";
	for (QCPAbstractPlottable *p : ui->selectMetric->selectedPlottables()) {
		int unit = p->property("UID").toInt();
		QVector<double> ticks;
        QVector<QString> labels;
        int eid = 0;
		for (const Experiment::Environment& e : exp->environments) {
			SimpleValue<double> vals = e.aggregate(unit,*exp);
			QCPStatisticalBox* b = new QCPStatisticalBox(ui->selectEnvironment->xAxis,ui->selectEnvironment->yAxis);
			b->setData(idx,vals.min(),vals.quantile(0.25),vals.median(),vals.quantile(0.75),vals.max());
            b->setProperty("UID",unit);
            b->setProperty("EID",eid++);
			ui->selectEnvironment->addPlottable(b);
			labels.append(QString("%1 @ %2").arg(exp->data->at(unit)->name(),e.label));
			qDebug() << "Added " << labels.back();
			ticks.append(idx++);
		}
		ui->selectEnvironment->xAxis->setAutoTicks(false);
		ui->selectEnvironment->xAxis->setAutoTickLabels(false);
		ui->selectEnvironment->xAxis->setSubTickCount(0);
		ui->selectEnvironment->xAxis->setTickLength(0, 4);
		ui->selectEnvironment->xAxis->setTickLabelRotation(40);
        ui->selectEnvironment->xAxis->setTickVector(ticks);
        ui->selectEnvironment->xAxis->setTickVectorLabels(labels);
        ui->selectEnvironment->setInteractions(QCP::iMultiSelect | QCP::iSelectPlottables);
	}
	ui->selectEnvironment->rescaleAxes();
	ui->selectEnvironment->xAxis->scaleRange(1.1, ui->selectEnvironment->xAxis->range().center());
    connect(ui->selectEnvironment,SIGNAL(selectionChangedByUser()), this, SLOT(updateDetails()));
    if (ui->axisFromZero->isChecked())
		ui->selectEnvironment->yAxis->setRangeLower(0);
	ui->selectEnvironment->replot();
}

void DataExplorer::on_runNo_valueChanged(int)
{
	std::vector<int> selected;
	for (int i = 0; i < ui->selectMetric->plottableCount(); i++) {
		if (ui->selectMetric->plottable(i)->selected()) {
			selected.push_back(i);
		}
	}
	on_dispUnit_currentIndexChanged(ui->dispUnit->currentIndex());
	for (int i : selected)
		ui->selectMetric->plottable(i)->setSelected(true);
	updateEnvironment();
	ui->selectMetric->replot();
}

void DataExplorer::on_dispUnit_currentIndexChanged(int)
{
	QVector<double> ticks;
	ui->selectMetric->clearPlottables();
	QBrush boxBrush(QColor(60, 60, 255, 100),Qt::Dense6Pattern);

	QVector<QString> labels;
	int i = -1;
	for (Datapoint<double>* p : *exp->data) {
		++i;
		if (p->unit() != ui->dispUnit->currentText()) continue;
		SimpleValue<double> v = exp->aggregate(i,*exp);
		QCPStatisticalBox* b = new QCPStatisticalBox(ui->selectMetric->xAxis,ui->selectMetric->yAxis);
		b->setBrush(boxBrush);
		b->setProperty("UID",i);
		b->setData(i,v.min(),v.quantile(0.25),v.median(),v.quantile(0.75),v.max());
		ui->selectMetric->addPlottable(b);
		ticks.append(i);
		labels.append(p->name());
	}
	ui->selectMetric->xAxis->setSubTickCount(0);
	ui->selectMetric->xAxis->setTickLength(0, 4);
	ui->selectMetric->xAxis->setTickLabelRotation(40);
	ui->selectMetric->xAxis->setAutoTicks(false);
	ui->selectMetric->xAxis->setAutoTickLabels(false);
	ui->selectMetric->xAxis->setTickVector(ticks);
	ui->selectMetric->xAxis->setTickVectorLabels(labels);
	ui->selectMetric->setInteractions(QCP::iMultiSelect | QCP::iSelectPlottables);

	ui->selectMetric->rescaleAxes();
	ui->selectMetric->xAxis->scaleRange(1.1, ui->selectMetric->xAxis->range().center());
	connect(ui->selectMetric,SIGNAL(selectionChangedByUser()), this, SLOT(updateEnvironment()));
	if (ui->axisFromZero->isChecked())
		ui->selectMetric->yAxis->setRangeLower(0);
	ui->selectMetric->replot();
}

void DataExplorer::on_aggregate_toggled(bool checked)
{
//	TODO: Do we still need this?
//	ui->runNo->setEnabled(!checked);
//	if (checked) {
//		ui->selectMetric->setInteraction(QCP::iSelectPlottables);
//	} else {
//		ui->selectMetric->setInteractions(QCP::iSelectPlottables | QCP::iMultiSelect);
//	}
//	on_dispUnit_currentIndexChanged(ui->dispUnit->currentIndex());
//	updateEnvironment();
}

void DataExplorer::setExperiment(const Experiment *exp) {
	this->exp = exp;
	if (exp != nullptr) {
		on_runNo_valueChanged(0);

		for (DatapointBase const *p : *exp->data) {
			if (ui->dispUnit->findText(p->unit()) == -1)
				ui->dispUnit->addItem(p->unit());
		}
	}
}

void DataExplorer::on_axisFromZero_toggled(bool)
{
	on_runNo_valueChanged(ui->runNo->value());
}
