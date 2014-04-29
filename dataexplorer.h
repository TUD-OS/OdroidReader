#ifndef DATAEXPLORER_H
#define DATAEXPLORER_H

#include <QWidget>
#include <experiment.h>

namespace Ui {
class DataExplorer;
}

class DataExplorer : public QWidget
{
	Q_OBJECT

public:
	explicit DataExplorer(QWidget *parent = 0);
	~DataExplorer();
	const Experiment* exp;
	void setExperiment(const Experiment* exp);
public:
	Ui::DataExplorer *ui;

public slots:
	void on_runNo_valueChanged(int);
	void on_aggregate_toggled(bool checked);
	void on_axisFromZero_toggled(bool);
	void on_dispUnit_currentIndexChanged(int);
	void updateDetail();
};

#endif // DATAEXPLORER_H
