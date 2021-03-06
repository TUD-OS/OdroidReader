#include "environments.h"
#include <QIcon>

Environments::Environments(QObject *parent) :
	QAbstractTableModel(parent)
{
}

EnvironmentSet* Environments::addSet(QString name) {
	EnvironmentSet* ns = new EnvironmentSet(name);
	connect(ns,&EnvironmentSet::renamingSet, this,&Environments::renamingSet);
	_sets.append(ns);
	emit addedSet(ns);
	return ns;
}

void Environments::addEnvironment(Environment *env) {
	_envs.append(env);
	emit addedEnvironment(env);
}

EnvironmentSet* Environments::findSet(QString name) const {
	for (EnvironmentSet* s : _sets)
		if (s->name() == name) return s;
	return nullptr;
}

int Environments::index(const Environment *e) const {
	return _envs.indexOf(const_cast<Environment*>(e));
}

Environment* Environments::at(int idx) {
	return _envs.at(idx);
}

int Environments::rowCount(const QModelIndex &) const {
	return _envs.size();
}

int Environments::columnCount(const QModelIndex &) const {
	return 8;
}

QVariant Environments::headerData(int section, Qt::Orientation orientation, int role) const {
	QString data[] = {"Label", "Frequency", "Min. Frequency", "Max. Frequency", "Governor", "big", "LITTLE"};
	if (role == Qt::DisplayRole && orientation == Qt::Horizontal && section < 7) {
		return data[section];
	}
	return QVariant();
}

QVariant Environments::data(const QModelIndex &index, int role) const {
	const Environment* e = _envs.at(index.row());
	switch(role) {
	  case Qt::DecorationRole:
		if (index.column() == 7) return QIcon::fromTheme("list-remove");
		break;
	  case Qt::DisplayRole:
		switch (index.column()) {
			case 0: return e->label;
			case 1: return e->freq;
			case 2: return e->freq_min;
			case 3: return e->freq_max;
			case 4: return e->governor;
			default: return QVariant();
		}
	  case Qt::CheckStateRole:
		switch (index.column()) {
		  case 5: return (e->big)?Qt::Checked:Qt::Unchecked;
		  case 6: return (e->little)?Qt::Checked:Qt::Unchecked;
		  default: break;
		}
	}
	return QVariant();
}

bool Environments::setData(const QModelIndex &index, const QVariant &value, int role) {
	Environment* e = _envs[index.row()];
	switch (role) {
	  case Qt::EditRole:
		switch (index.column()) {
		  case 0: e->label = value.toString(); return true;
		  case 1: e->freq = value.toInt(); return true;
		  case 2: e->freq_min = value.toInt(); return true;
		  case 3: e->freq_max = value.toInt(); return true;
		  case 4: e->governor = value.toString(); return true;
		  default: return false;
		}
	  case Qt::CheckStateRole:
		switch (index.column()) {
		  case 5:
			if (!e->little && !value.toBool()) return false;
			e->big = value.toBool();
			return true;
		  case 6:
			if (!e->big && !value.toBool()) return false;
			e->little = value.toBool();
			return true;
		}
	}
	return false;
}

Qt::ItemFlags Environments::flags(const QModelIndex &index) const {
	if (index.column() < 5)
		return Qt::ItemIsEditable | QAbstractTableModel::flags(index);
	else
		return Qt::ItemIsUserCheckable| QAbstractTableModel::flags(index);
}

bool Environments::insertRows(int row, int count, const QModelIndex &parent) {
	beginInsertRows(parent,row,row+count-1);
	for (int i = row; i < row+count; i++)
		_envs.insert(i,new Environment());
	endInsertRows();
	return true;
}

bool Environments::removeRows(int row, int count, const QModelIndex &parent) {
	if (row >= rowCount() || row+count > rowCount() || rowCount() == 1) return false;
	beginRemoveRows(parent,row,row+count-1);
	for (int i = 0; i < count; i ++) {
		_envs.removeAt(row);
	}
	endRemoveRows();
	return true;
}

void Environments::removeSet(EnvironmentSet *set) {
	emit removingSet(set);
	_sets.removeAll(set);
	set->deleteLater();
}

void Environments::removeEnvironment(Environment *env) {
	_envs.removeAll(env);
	delete env;
}

const QList<Environment*>& Environments::environments() const {
	return _envs;
}

const QList<EnvironmentSet*>& Environments::sets() const {
	return _sets;
}

