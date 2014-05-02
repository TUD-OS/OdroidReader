#ifndef DATASOURCE_H
#define DATASOURCE_H

#include <QString>
#include <QObject>
#include <QVector>
#include <Data/datadescriptor.h>

class DataSource : public QObject
{
	Q_OBJECT
private:
	QString _name;
	QVector<DataDescriptor> descs;
public:
	DataSource(QString names, QObject* parent);
	DataSource(const DataSource&) = delete;
	DataSource& operator=(const DataSource&) = delete;
	inline const QString& name() { return _name; }

public slots:
	virtual void connect() = 0;

signals:
	void dataReceived(QString source);
	void descriptorsAvailable(const QVector<DataDescriptor>& descriptors);
	void dataAvailable(); //TODO
	void connected();
};

#endif // DATASOURCE_H
