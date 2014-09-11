#ifndef ENVIRONMENTS_H
#define ENVIRONMENTS_H

#include <QAbstractTableModel>
#include "environmentset.h"

class Environments : public QAbstractTableModel
{
	Q_OBJECT
	QList<EnvironmentSet*> _sets;
	QList<Environment*> _envs;
public:
	explicit Environments(QObject *parent = 0);
	EnvironmentSet* findSet(QString name) const;
	Environment*    at(int idx);

	int index(const Environment* e) const;

	const QList<Environment*>& environments() const;
	const QList<EnvironmentSet*>& sets() const;

	EnvironmentSet* addSet(QString name);

	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int columnCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	bool setData(const QModelIndex &index, const QVariant &value, int role);
	virtual bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex());
	virtual bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
	Qt::ItemFlags flags(const QModelIndex &index) const;

signals:
	void addedEnvironment(const Environment* env);
	void addedSet(const EnvironmentSet* set);
	void removingSet(const EnvironmentSet* set);
	void renamingSet(QString oldName, QString newName);
public slots:
	void addEnvironment(Environment* env);
	void removeSet(EnvironmentSet* set);
	void removeEnvironment(Environment* env);
};

#endif // ENVIRONMENTS_H
