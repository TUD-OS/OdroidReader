#include "environmentset.h"

EnvironmentSet::EnvironmentSet(QString name, QObject *parent) :
	QObject(parent), _name(name)
{
}

void EnvironmentSet::addEnvironment(Environment *env) {
	_envs.append(env);
	emit changed();
}

void EnvironmentSet::removeEnvironment(Environment *env) {
	env->deleteLater();
	emit changed();
}

void EnvironmentSet::rename(QString newName) {
	emit renamingSet(_name,newName);
	_name = newName;
	emit changed();
}

void EnvironmentSet::clear() {
	_envs.clear();
	emit changed();
}
