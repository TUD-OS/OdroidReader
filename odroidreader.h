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
#include <qwt6/qwt_plot_curve.h>
#include <qwt6/qwt_color_map.h>
#include "datapoint.h"

typedef enum class {
	DESC = 0,
	GET = 1,
	NONE = 99
} Query;

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
  void on_connect_clicked();
  void connected();
  void connerror(QAbstractSocket::SocketError);
  void updateCurve(int,int);
  void readData();
  void sendGet();
  void on_execute_clicked();
  void enableControls();
  void on_useBig_toggled(bool checked);

  void on_useLittle_toggled(bool);

  void on_addExperiment_clicked();

  void on_exp_name_textChanged(const QString &arg1);

  void on_listWidget_itemSelectionChanged();

  void on_removeExperiment_clicked();

  void on_updateExperiment_clicked();

  void on_runSelected_clicked();

private:
  void enableControls(bool status);
  void updateSensors();
  void updateExperiments();
  quint32 freq2Int(QString);
  QTimer tmr,stop;
  Ui::OdroidReader *ui;
  int sock_descriptor;
  struct sockaddr_in serv_addr;
  QTcpSocket *sock;
  Query query;
  QTextStream ts;
  std::vector<DatapointBase*> descs;
  quint32 dsize;
  QVector<QPointF> samples;
  QwtLinearColorMap colormap;
  QwtLegendData legend;
  QVector<Experiment> experiments;
  bool execMode;
};

#endif // ODROIDREADER_H
