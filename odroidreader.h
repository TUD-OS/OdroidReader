#ifndef ODROIDREADER_H
#define ODROIDREADER_H

#include <QMainWindow>
#include <QTimer>
#include "experiment.h"
#include <QVector>
#include <QPointF>
#include <vector>
#include <netinet/in.h>
#include <sys/socket.h>
#include <QtNetwork/QTcpSocket>
#include "datapoint.h"
#include "qcustomplot.h"
#include <QtEndian>
#include <QListWidgetItem>

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
  static std::vector<Datapoint<double>*> descs;
  ~OdroidReader();

private slots:
  void on_connect_clicked();
  void connected();
  void connerror(QAbstractSocket::SocketError);
  void updateCurve(int,int);
  void readData();
  void sendGet();
  void on_useBig_toggled(bool checked);
  void on_useLittle_toggled(bool);
  void on_addExperiment_clicked();
  void on_exp_name_textChanged(const QString &arg1);
  void on_listWidget_itemSelectionChanged();
  void on_removeExperiment_clicked();
  void on_updateExperiment_clicked();
  void on_runSelected_clicked();

  void aboutToQuit();
  void runExperiments();

  void on_listWidget_itemDoubleClicked(QListWidgetItem *item);
  void on_runNo_valueChanged(int arg1);
  void updateDetail();
  void on_dispUnit_currentIndexChanged(int);

  void on_aggregate_toggled(bool checked);
  void on_axisFromZero_toggled(bool checked);

private:
  void enableControls(bool status);
  void updateSensors();
  void updateExperiments();
  void setupExperiment(const Experiment::Run &run);
  void runCommand(std::string cmd);
  double lastTime;
  int repetition;
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
};

#endif // ODROIDREADER_H
