#ifndef DATASOURCE_H
#define DATASOURCE_H

#include <QString>
#include <datapoint.h>

class DataSource
{
private:
	QString name;
public:
	DataSource(QString names);
signals:
	void dataReceived(QString source);
};

#endif // DATASOURCE_H
