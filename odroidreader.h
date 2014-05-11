#ifndef ODROIDREADER_H
#define ODROIDREADER_H

#include <QMainWindow>
#include <devicemonitor.h>
#include "experiment.h"
#include <QVector>
#include <QModelIndex>
#include "qcustomplot.h"
#include <ui/dataexplorer.h>
#include <Data/datasource.h>
#include <Data/datadescriptor.h>
#include <Data/dataseries.h>

typedef enum class {
	Prepare, Execute, Cleanup, Idle
} ExperimentState;

namespace Ui {
  class OdroidReader;
}

class OdroidReader : public QMainWindow
{
  Q_OBJECT

public:
  explicit OdroidReader(QWidget *parent = 0);
  ~OdroidReader();

private slots:
  void removeDataExplorer(DataExplorer* de);
  void updateCurve(int,int);
  void on_addExperiment_clicked();
  void on_exp_name_textChanged(const QString &arg1);
  void on_listWidget_itemSelectionChanged();
  void on_removeExperiment_clicked();
  void on_updateExperiment_clicked();
  void runSelectedOnSource(const QAction *act);

  void aboutToQuit();
  void runExperiments(DataSource &source, double time);

  void removeEnvironment(QModelIndex idx);
  void on_envAdd_clicked();

  void on_pushButton_clicked();

  void on_addConnection_clicked();

  void addDescriptors(QVector<const DataDescriptor *> descriptors);
  void addData(const DataDescriptor* desc, double data, double time);

  void on_pushButton_2_clicked();

private:
  void updateSensors();
  void updateExperiments();
  void updateRunnables();
  int repetition;
  int lastEnv;
  ExperimentState es;
  Experiment const* curExp;
  std::vector<Experiment*> toRun;
  quint32 freq2Int(QString);
  Ui::OdroidReader *ui;
  //Query query;
  QTextStream ts;
  //quint32 packetSize;
  QVector<QPointF> samples;
  Experiment* currentExp;
  Experiment::Environment* currentEnv;
  QVector<DataSeries*> data;
  QVector<DataSource*> sources;
  QVector<Experiment*> experiments;
  QVector<QCPGraph*> graphs;
  QVector<DeviceMonitor*> devMonitors;
};

#endif // ODROIDREADER_H
