#ifndef NETWORKSOURCE_H
#define NETWORKSOURCE_H
#include <Data/datasource.h>
#include <Data/datadescriptor.h>
#include <QTcpSocket>
#include <QThread>
#include <QTimer>

class NetworkSource : public DataSource
{
	Q_OBJECT
private:
	typedef enum class {
		DESC = 0,
		GET = 1,
		NONE = 99
	} Query;

	QTcpSocket socket;
	QString _address;
	quint16 _port;
	QThread *networkThread;
	QTimer getTimer;
	Query query;
	size_t packetSize;
	double lastTime;
public:
	NetworkSource(QString name, QString address, quint16 port, int interval, QObject *parent = nullptr);
	virtual ~NetworkSource();
	inline virtual bool canExecute() { return true; }
	virtual void execute(QString exec);
	inline const QString& address() { return _address; }
	inline quint16 port() { return _port; }
    inline virtual QString descriptor() { return QString("[Net] %1 @ %2:%3").arg(_name,_address,QString::number(_port)); }
signals:
	void descriptorsAvailable(QVector<const DataDescriptor*> descriptors);
	void dataAvailable(const DataDescriptor* desc, double data, double time); //TODO

public slots:
	void conerror(QAbstractSocket::SocketError error);
	void readData();
	void connected();
protected:
	virtual void start();
};

#endif // NETWORKSOURCE_H
