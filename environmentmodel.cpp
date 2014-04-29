#include "environmentmodel.h"

EnvironmentModel::EnvironmentModel(Experiment *exp, QObject *parent) :
	QAbstractTableModel(parent)
{
	experiment = exp;
}

int EnvironmentModel::rowCount(const QModelIndex&) const {
	return experiment->environments.size();
}

int EnvironmentModel::columnCount(const QModelIndex&) const {
	return 8;
}

QVariant EnvironmentModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
		switch (section) {
		  case 0: return "Label";
		  case 1: return "Frequency";
		  case 2: return "Min. Frequency";
		  case 3: return "Max. Frequency";
		  case 4: return "Governor";
		  case 5: return "big";
		  case 6: return "LITTLE";
		}
	}
	return QVariant();
}

QVariant EnvironmentModel::data(const QModelIndex &index, int role) const {
	const Experiment::Environment& e = experiment->environments.at(index.row());
	switch(role) {
	  case Qt::DecorationRole:
		if (index.column() == 7) return QIcon::fromTheme("list-remove");
		break;
	  case Qt::DisplayRole:
		switch (index.column()) {
			case 0: return QString::fromStdString(e.label);
			case 1: return e.freq;
			case 2: return e.freq_min;
			case 3: return e.freq_max;
			case 4: return QString::fromStdString(e.governor);
			default: return QVariant();
		}
	  case Qt::CheckStateRole:
		switch (index.column()) {
		  case 5: return (e.big)?Qt::Checked:Qt::Unchecked;
		  case 6: return (e.little)?Qt::Checked:Qt::Unchecked;
		  default: break;
		}
	}
	return QVariant();
}

bool EnvironmentModel::setData(const QModelIndex &index, const QVariant &value, int role) {
	Experiment::Environment &e = experiment->environments[index.row()];
	switch (role) {
	  case Qt::DecorationRole:

	  case Qt::EditRole:
		switch (index.column()) {
		  case 0: e.label = value.toString().toStdString(); break;
		  case 1: e.freq = value.toInt(); break;
		  case 2: e.freq_min = value.toInt(); break;
		  case 3: e.freq_max = value.toInt(); break;
		  case 4: e.governor = value.toString().toStdString(); break;
		  default: return false;
		}
	  case Qt::CheckStateRole:
		switch (index.column()) {
		  case 5:
			if (!e.little && !value.toBool()) return false;
			e.big = value.toBool();
			break;
		  case 6:
			if (!e.big && !value.toBool()) return false;
			e.little = value.toBool();
			break;
		}
	}
	return true;
}

Qt::ItemFlags EnvironmentModel::flags(const QModelIndex &index) const {
	if (index.column() < 5)
		return Qt::ItemIsEditable | QAbstractTableModel::flags(index);
	else
		return Qt::ItemIsUserCheckable| QAbstractTableModel::flags(index);
}

bool EnvironmentModel::insertRows(int row, int count, const QModelIndex &parent) {
	beginInsertRows(parent,row,row+count-1);
	for (int i = row; i < row+count; i++)
		experiment->environments.insert(i,Experiment::Environment());
	endInsertRows();
	return true;
}

bool EnvironmentModel::removeRows(int row, int count, const QModelIndex &parent) {
	if (row >= rowCount() || row+count > rowCount() || rowCount() == 1) return false;
	beginRemoveRows(parent,row,row+count-1);
	for (int i = 0; i < count; i ++) {
		qDebug() << "Removing " << row;
		experiment->environments.remove(row);
	}
	endRemoveRows();
	return true;
}
