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

void DataExplorer::updateDetail() {
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

void DataExplorer::on_runNo_valueChanged(int)
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

void DataExplorer::on_dispUnit_currentIndexChanged(int)
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
		std::pair<double,double> pair = exp->environments.front().runs.at(ui->runNo->value());
		for (Datapoint<double>* p : *exp->data) {
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

void DataExplorer::on_aggregate_toggled(bool checked)
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
