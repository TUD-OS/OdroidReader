#include "dataexplorer.h"
#include "ui_dataexplorer.h"
#include "environment.h"
#include <QToolTip>
static QList<QColor> origcols({QColor(7,139,119),QColor(252,138,74),QColor(100,170,254),QColor(91,53,40),QColor(133,196,77),
					  QColor(104,115,15),QColor(133,3,43),QColor(188,186,111),QColor(168,115,19),QColor(63,184,67)});


DataExplorer::DataExplorer(QWidget *parent) :
	QWidget(parent), exp(nullptr),
	ui(new Ui::DataExplorer)
{
	ui->setupUi(this);
	ui->selectMetric->setSelectionTolerance(5);
	ui->selectEnvironment->setSelectionTolerance(5);
	ui->runPlot->setSelectionTolerance(5);
	connect(ui->runPlot,&QCustomPlot::mouseMove,this,&DataExplorer::pointInfo);
}

DataExplorer::~DataExplorer()
{
	delete ui;
}

void DataExplorer::updateDetails() {
	//qDebug() << "SELECT!";
    ui->runPlot->clearPlottables();
	QVector<QString> labels;
	QVector<double> ticks;
	int colid = 0;
	switch (ui->detailType->currentIndex()) {
	  case 0:
		ui->runPlot->xAxis->setAutoTicks(true);
		ui->runPlot->xAxis->setAutoTickLabels(true);
		for (QCPAbstractPlottable *p : ui->selectEnvironment->selectedPlottables()) {
			int unitid = p->property("UID").toInt();
			int envid = p->property("EID").toInt();
			DataSeries v = exp->runs.keys().at(envid)->run(unitid,ui->runNo->value(),exp);
			QCPGraph *g = ui->runPlot->addGraph();
			g->setName(v.descriptor->name()+" @ "+exp->runs.keys().at(envid)->label);
			g->setProperty("Unit",v.descriptor->unit());
			g->setPen(origcols[colid++%origcols.size()]);
			g->setData(v.getTimestamps(),v.getValues());
		}
		break;
	  case 1:
		for (QCPAbstractPlottable *p : ui->selectEnvironment->selectedPlottables()) {
			int unitid = p->property("UID").toInt();
			int envid = p->property("EID").toInt();
			StatisticalSet vals = exp->runs.keys().at(envid)->integral(unitid,exp);
			QCPStatisticalBox* b = new QCPStatisticalBox(ui->runPlot->xAxis,ui->runPlot->yAxis);
			b->setData(colid,vals.min(),vals.quantile(0.25),vals.median(),vals.quantile(0.75),vals.max());
			b->setProperty("StdDev",vals.getStdDev());
			b->setProperty("avg",vals.avg());
			b->setProperty("avgTime",vals.avgTime());
			qWarning() << exp->data.at(unitid)->descriptor->name() <<  exp->runs.keys().at(envid)->label << vals.avg() << vals.avgTime() << vals.getStdDev();
			ui->runPlot->addPlottable(b);
			labels.append(QString("%1 @ %2").arg(exp->data.at(unitid)->descriptor->name(),exp->runs.keys().at(envid)->label));
			ticks.append(colid++);
			ui->runPlot->xAxis->setAutoTicks(false);
			ui->runPlot->xAxis->setAutoTickLabels(false);
			ui->runPlot->xAxis->setSubTickCount(0);
			ui->runPlot->xAxis->setTickLength(0, 4);
			ui->runPlot->xAxis->setTickLabelRotation(90);
			ui->runPlot->xAxis->setTickVector(ticks);
			ui->runPlot->xAxis->setTickVectorLabels(labels);
		}
		break;
	  case 2: break;
	}
    ui->runPlot->rescaleAxes();
    if (ui->axisFromZero->isChecked())
        ui->runPlot->yAxis->setRangeLower(0);
    ui->runPlot->replot();
}

void DataExplorer::updateEnvironment() {
	ui->selectEnvironment->clearPlottables();
	int idx = 0;
	QVector<double> ticks;
	QVector<QString> labels;
	for (QCPAbstractPlottable *p : ui->selectMetric->selectedPlottables()) {
		int unit = p->property("UID").toInt();
        int eid = 0;
		for (const Environment* e : exp->runs.keys()) {
			StatisticalSet vals = e->aggregate(unit,exp);
			QCPStatisticalBox* b = new QCPStatisticalBox(ui->selectEnvironment->xAxis,ui->selectEnvironment->yAxis);
			b->setData(idx,vals.min(),vals.quantile(0.25),vals.median(),vals.quantile(0.75),vals.max());
            b->setProperty("UID",unit);
            b->setProperty("EID",eid++);
			ui->selectEnvironment->addPlottable(b);
			b->setSelected(true);
			labels.append(QString("%1 @ %2").arg(exp->data.at(unit)->descriptor->name(),e->label));
			ticks.append(idx++);
		}
	}
	ui->selectEnvironment->xAxis->setAutoTicks(false);
	ui->selectEnvironment->xAxis->setAutoTickLabels(false);
	ui->selectEnvironment->xAxis->setSubTickCount(0);
	ui->selectEnvironment->xAxis->setTickLength(0, 4);
	ui->selectEnvironment->xAxis->setTickLabelRotation(90);
	ui->selectEnvironment->xAxis->setTickVector(ticks);
	ui->selectEnvironment->xAxis->setTickVectorLabels(labels);
	ui->selectEnvironment->setInteractions(QCP::iMultiSelect | QCP::iSelectPlottables);
	ui->selectEnvironment->setSelectionTolerance(10);
	ui->selectEnvironment->rescaleAxes();
	ui->selectEnvironment->xAxis->scaleRange(1.1, ui->selectEnvironment->xAxis->range().center());
	connect(ui->selectEnvironment,SIGNAL(selectionChangedByUser()), this, SLOT(updateDetails()));
	if (ui->axisFromZero->isChecked())
		ui->selectEnvironment->yAxis->setRangeLower(0);
	ui->selectEnvironment->replot();
	updateDetails();
}

void DataExplorer::on_runNo_valueChanged(int)
{
	std::vector<int> selectedM;
	QVector<int> selectedE;
	for (int i = 0; i < ui->selectMetric->plottableCount(); i++) {
		if (ui->selectMetric->plottable(i)->selected()) {
			selectedM.push_back(i);
		}
	}
	for (int i = 0; i < ui->selectEnvironment->plottableCount(); i++) {
		if (ui->selectEnvironment->plottable(i)->selected())
			selectedE.push_back(i);
	}
	on_dispUnit_currentIndexChanged(ui->dispUnit->currentIndex());
	for (int i : selectedM)
		ui->selectMetric->plottable(i)->setSelected(true);
	updateEnvironment();
	for (int i : selectedE)
		ui->selectEnvironment->plottable(i)->setSelected(true);
	updateDetails();
	ui->selectMetric->replot();
}

void DataExplorer::on_dispUnit_currentIndexChanged(int)
{
	QVector<double> ticks;
	ui->selectMetric->clearPlottables();
	QBrush boxBrush(QColor(60, 60, 255, 100),Qt::Dense6Pattern);

	QVector<QString> labels;
	int i = -1;
	for (DataSeries* p : exp->data) {
		if (p == nullptr) continue;
		if (p->descriptor->unit() != ui->dispUnit->currentText()) continue;
		++i;
		StatisticalSet v = exp->aggregate(p->descriptor->uid(),exp);
		QCPStatisticalBox* b = new QCPStatisticalBox(ui->selectMetric->xAxis,ui->selectMetric->yAxis);
		b->setBrush(boxBrush);
		b->setProperty("UID",p->descriptor->uid());
		b->setData(i,v.min(),v.quantile(0.25),v.median(),v.quantile(0.75),v.max());
		ui->selectMetric->addPlottable(b);
		ticks.append(i);
		labels.append(p->descriptor->name());
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

void DataExplorer::setExperiment(const Experiment *exp) {
	this->exp = exp;
	ui->runNo->setMaximum(exp->runs.first().size()-1);
	if (exp != nullptr) {
		on_runNo_valueChanged(0);

		for (DataSeries const *p : exp->data) {
			if (p == nullptr) continue;
			if (ui->dispUnit->findText(p->descriptor->unit()) == -1) {
				ui->dispUnit->addItem(p->descriptor->unit());
			}
		}
	}
}

void DataExplorer::on_axisFromZero_toggled(bool)
{
	on_runNo_valueChanged(ui->runNo->value());
}

void DataExplorer::on_detailType_currentIndexChanged(int)
{
	updateDetails();
}

void DataExplorer::on_pushButton_clicked()
{
	emit(removeMe(this));
}

void DataExplorer::pointInfo(QMouseEvent *event)
{
	QCPAbstractPlottable *plottable = ui->runPlot->plottableAt(event->localPos());
	if (!plottable) return;

	double x = ui->runPlot->xAxis->pixelToCoord(event->localPos().x());
	QToolTip::hideText();

	QCPGraph *graph = qobject_cast<QCPGraph*>(plottable);
	if (graph) {
		double key = 0;
		double value = 0;
		bool ok = false;
		double m = std::numeric_limits<double>::max();
		for (QCPData data : graph->data()->values()) {
			double d = qAbs(x - data.key);

			if(d < m) {
				key = data.key;
				value = data.value;

				ok = true;
				m = d;
			}
		}
		if (!ok) return;
		QToolTip::showText(event->globalPos(),
			tr("<center><b>%L1</b><br/>%L2 %L3@ %L4s</center>").
				arg(graph->name().isEmpty() ? "..." : graph->name()).
				arg(value).arg(graph->property("unit").toString()).
				arg(key),
			ui->runPlot, ui->runPlot->rect());
		return;
	}

	QCPStatisticalBox *graphBox = qobject_cast<QCPStatisticalBox*>(plottable);
	if (graphBox) {
		QToolTip::showText(event->globalPos(),
			tr("<center><b>%L1</b><br/></center>Max: %2<br/>Upper: %3<br/>Median: %4<br/>Lower: %5<br/>Min: %6<br/>StdDev: %7<br/>Avg: %8<br/>Avg Time: %9").
				arg(graphBox->name().isEmpty() ? "..." : graphBox->name()).
				arg(graphBox->maximum()).arg(graphBox->upperQuartile()).arg(graphBox->median()).
				arg(graphBox->lowerQuartile()).arg(graphBox->minimum()).
				arg(graphBox->property("StdDev").toDouble()).arg(graphBox->property("avg").toDouble()).
				arg(graphBox->property("avgTime").toDouble()),
			ui->runPlot, ui->runPlot->rect());
	}

}
