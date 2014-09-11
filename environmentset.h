#ifndef ENVIRONMENTSET_H
#define ENVIRONMENTSET_H

#include <QObject>
#include "environment.h"

class EnvironmentSet : public QObject
{
	Q_OBJECT
	QString _name;
	QList<Environment*> _envs;
public:
	explicit EnvironmentSet(QString name, QObject *parent = 0);
	QString name() const { return _name; }
	void rename(QString newName);
	const QList<Environment*> environments() const { return _envs; }
signals:
	void changed();
	void renamingSet(QString oldName, QString newName);
public slots:
	void addEnvironment(Environment* env);
	void removeEnvironment(Environment* env);
	void clear();
};

#endif // ENVIRONMENTSET_H
