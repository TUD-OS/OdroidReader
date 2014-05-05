#ifndef NETWORKSOURCE_H
#define NETWORKSOURCE_H
#include <Data/datasource.h>
#include <QTcpSocket>
#include <QThread>

class NetworkSource : public DataSource
{
	Q_OBJECT
private:
	QTcpSocket socket;
	QString _address;
	quint16 _port;
	QThread *networkThread;
public:
	NetworkSource(QString name, QString address, quint16 port, QObject *parent = 0);
	inline const QString& address() { return _address; }
	inline quint16 port() { return _port; }
	inline virtual QString descriptor() { return QString("[Net] %1 @ %2:%3").arg(_name,_address,QString(_port)); }
signals:

public slots:
	virtual void connect();
};

#endif // NETWORKSOURCE_H
