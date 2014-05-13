#ifndef DATASOURCE_H
#define DATASOURCE_H

#include <QString>
#include <QObject>
#include <QMap>
#include <QVector>
#include <experiment.h>
#include <Data/datadescriptor.h>

Q_DECLARE_METATYPE(QVector<const DataDescriptor*>)

class DataSource : public QObject
{
    Q_OBJECT
    static double localStart;
protected:
    static QMap<const DataSource*,double> offsets;
    QString _name;
    double getGlobalTime(double time);

public:
	DataSource(QString names, QObject* parent = nullptr);
	DataSource(const DataSource&) = delete;
	DataSource& operator=(const DataSource&) = delete;
	inline const QString& name() const { return _name; }
	virtual QString descriptor() = 0;
	virtual bool canExecute() const = 0;
	virtual void execute(QString exec);
    virtual bool isRunning() const = 0;
	virtual void setupEnvironment(const Experiment::Environment& env);
public slots:
	virtual void start() = 0;
    virtual void stop() = 0;

signals:
	void dataReceived(QString source);
	void commandStarted(DataSource& ds, double time);
	void commandFinished(DataSource& ds, double time);
	void descriptorsAvailable(QVector<const DataDescriptor*> descriptors);
	void dataAvailable(const DataDescriptor* desc,double data, double time); //TODO
	void connected();
	void disconnected();
};

#endif // DATASOURCE_H
