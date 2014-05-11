#ifndef NETWORKSOURCE_H
#define NETWORKSOURCE_H
#include <Data/datasource.h>
#include <Data/datadescriptor.h>
#include <QTcpSocket>
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
	qint64 packetSize;
	qint64 recon_ctr;
	bool started, _running, reconnect;
	double lastTime;
public:
	NetworkSource(QString name, QString address, quint16 port, int interval, QObject *parent = nullptr);
	virtual ~NetworkSource();
    virtual bool isRunning() const { return _running; }
	inline virtual bool canExecute() const { return true; }
	virtual void execute(QString exec);
	virtual void setupEnvironment(const Experiment::Environment &env);
	inline const QString& address() { return _address; }
	inline quint16 port() { return _port; }
    inline virtual QString descriptor() { return QString("[Net] %1 @ %2:%3").arg(_name,_address,QString::number(_port)); }
signals:
	void descriptorsAvailable(QVector<const DataDescriptor*> descriptors);
	void dataAvailable(const DataDescriptor* desc, double data, double time); //TODO
	void commandStarted(DataSource& ds, double time);
	void commandFinished(DataSource& ds, double time);


public slots:
	void conerror(QAbstractSocket::SocketError error);
	void readData();
protected:
	virtual void start();
};

#endif // NETWORKSOURCE_H
