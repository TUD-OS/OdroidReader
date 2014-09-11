#ifndef ODROIDREADER_H
#define ODROIDREADER_H

#include <QMainWindow>
#include <devicemonitor.h>
#include "experiment.h"
#include <QVector>
#include <QMap>
#include <QModelIndex>
#include "qcustomplot.h"
#include <ui/dataexplorer.h>
#include <Data/datasource.h>
#include <Data/datadescriptor.h>
#include <Data/dataseries.h>
#include "environments.h"

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
  void updateCurve(int,int);
  void on_addExperiment_clicked();
  void on_exp_name_textChanged(const QString &arg1);
  void on_listWidget_itemSelectionChanged();
  void on_removeExperiment_clicked();
  void runSelectedOnSource(const QAction *act);
  void runAllOnSource(const QAction *act);

  void aboutToQuit();
  void runExperiments(DataSource &source, double time);

  void removeEnvironment(QModelIndex idx);
  void on_envAdd_clicked();

  void on_pushButton_clicked();

  void on_addConnection_clicked();

  void addDescriptors(QVector<const DataDescriptor *> descriptors);
  void addData(const DataDescriptor* desc, double data, double time);

  void on_addDevice_clicked();

  void on_loadData_clicked();
  void on_envSetAdd_clicked();

  void on_envSets_currentRowChanged(int currentRow);

  void on_envSetRemove_clicked();

  void on_envSetUpdate_clicked();

  void on_addEnvironment_clicked();

  void on_pushButton_2_clicked();
  void plotMousePress(QMouseEvent *event);
private:
  void updateSensors();
  void updateExperiments();
  void updateRunnables();
  int repetition;
  int lastEnv, lastSet;
  bool hasExecuted, isLoaded;
  ExperimentState es;
  Experiment const* curExp;
  std::vector<Experiment*> toRun;
  Ui::OdroidReader *ui;
  QTextStream ts;
  QVector<QPointF> samples;
  Experiment* currentExp;
  Environment* currentEnv;
  Experiment* selectedExp;
  QVector<DataSeries*> data;
  QVector<DataSource*> sources;
  QVector<Experiment*> experiments;
  Environments envs;
  QVector<QCPGraph*> graphs;
  QVector<int> rowMap;
  QVector<DeviceMonitor*> devMonitors;
};

#endif // ODROIDREADER_H
