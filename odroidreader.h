#ifndef ODROIDREADER_H
#define ODROIDREADER_H

#include <QMainWindow>
#include <QTimer>
#include <QTextStream>
#include <QVector>
#include <QPointF>
#include <netinet/in.h>
#include <sys/socket.h>
#include <QtNetwork/QTcpSocket>
#include <qwt6/qwt_plot_curve.h>
#include <qwt6/qwt_color_map.h>

typedef enum : quint8 {
	CHAR = 0, FLOAT = 1, UINT16T = 2, BIGLITTLE = 3, UINT32T =4
} FIELD_TYPE;

typedef enum class {
	DESC = 0,
	GET = 1,
	NONE = 99
} Query;

typedef struct {
	quint8 name_len;
	FIELD_TYPE type;
	quint32 factor;
	union {
		quint32 ivalue;
		float fvalue;
	};
	union {
		quint32 ivalue;
		float fvalue;
	} min;
	union {
		quint32 ivalue;
		float fvalue;
	} max;
	QVector<QPointF> samples;
	QwtPlotCurve* pc;
	QString name;
	QString unit; //always 3 chars
	bool initialized;
} field_desc;

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
private:
  void enableControls(bool status);
  void updateSensors();
  QTimer tmr,stop;
  Ui::OdroidReader *ui;
  int sock_descriptor;
  struct sockaddr_in serv_addr;
  QTcpSocket *sock;
  Query query;
  QTextStream ts;
  QList<field_desc> descs;
  quint32 dsize;
  QVector<QPointF> samples;
  QwtLinearColorMap colormap;
  QwtLegendData legend;
  bool execMode;
};

#endif // ODROIDREADER_H
