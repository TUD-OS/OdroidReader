#ifndef ENVIRONMENTMODEL_H
#define ENVIRONMENTMODEL_H

#include <QAbstractTableModel>
#include <experiment.h>

class EnvironmentModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	explicit EnvironmentModel(Experiment *exp, QObject *parent = 0);
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int columnCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	bool setData(const QModelIndex &index, const QVariant &value, int role);
	virtual bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex());
	virtual bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
	Qt::ItemFlags flags(const QModelIndex &index) const;
signals:

public slots:


private:
	Experiment* experiment;
};

#endif // ENVIRONMENTMODEL_H
