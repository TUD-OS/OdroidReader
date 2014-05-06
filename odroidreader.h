#ifndef ODROIDREADER_H
#define ODROIDREADER_H

#include <QMainWindow>
#include <QTimer>
#include "experiment.h"
#include <QVector>
#include <QPointF>
#include <netinet/in.h>
#include <sys/socket.h>
#include <QtNetwork/QTcpSocket>
#include "Data/datapoint.h"
#include "qcustomplot.h"
#include <QtEndian>
#include <QListWidgetItem>
#include <ui/dataexplorer.h>
#include <Data/datasource.h>
#include <Data/datadescriptor.h>

typedef enum class {
	DESC = 0,
	GET = 1,
	NONE = 99
} Query;

typedef enum class {
	Prepare, Execute, Cleanup, Idle
} ExperimentState;

namespace Ui {
  class OdroidReader;
}

template<typename T>
T networkDecode(QByteArray const &ba) {
	return qFromBigEndian<T>(reinterpret_cast<const uchar*>(ba.constData()));
}

class OdroidReader : public QMainWindow
{
  Q_OBJECT

public:
  explicit OdroidReader(QWidget *parent = 0);
  static QVector<Datapoint<double>*> descs;
  ~OdroidReader();

private slots:
  void removeDataExplorer(DataExplorer* de);
  void connected();
  void connerror(QAbstractSocket::SocketError);
  void updateCurve(int,int);
  void readData();
  void sendGet();
  void on_addExperiment_clicked();
  void on_exp_name_textChanged(const QString &arg1);
  void on_listWidget_itemSelectionChanged();
  void on_removeExperiment_clicked();
  void on_updateExperiment_clicked();
  void on_runSelected_clicked();

  void aboutToQuit();
  void runExperiments();

  void removeEnvironment(QModelIndex idx);
  void on_envAdd_clicked();

  void on_pushButton_clicked();

  void on_addConnection_clicked();

  void on_startSampling_clicked();

  void addDescriptors(const QVector<const DataDescriptor *> &descriptors);

private:
  void enableControls(bool status);
  void updateSensors();
  void updateExperiments();
  void setupExperiment(const Experiment::Environment &run);
  void runCommand(QString cmd);
  double lastTime;
  int repetition;
  int lastEnv;
  bool executed;
  ExperimentState es;
  Experiment const* curExp;
  std::vector<Experiment*> toRun;
  quint32 freq2Int(QString);
  QTimer tmr,stop;
  Ui::OdroidReader *ui;
  int sock_descriptor;
  struct sockaddr_in serv_addr;
  QTcpSocket *sock;
  Query query;
  QTextStream ts;
  quint32 packetSize;
  QVector<QPointF> samples;
  QVector<Experiment> experiments;
  Experiment* currentExp;
  Experiment::Environment* currentEnv;
  QVector<const DataDescriptor*> descriptors;
  QVector<DataSource*> sources;
};

#endif // ODROIDREADER_H
