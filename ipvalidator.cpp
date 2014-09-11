#include "ipvalidator.h"
#include <QStringList>
#include <QString>
IPValidator::IPValidator(QObject *parent) :  QValidator(parent) {}

QValidator::State IPValidator::validate(QString &in, int &) const {
	QStringList l = in.split('.');

	if (l.size() > 4)
		return QValidator::State::Invalid;

	bool test;
	for (int i = 0; i < l.size(); i++) {
		QString s = l.at(i);
		if (s == "" && i == l.size()-1) continue;
		int part = s.toInt(&test);
		if (part < 0 || part > 255 || !test) return QValidator::State::Invalid;
	}

	if (l.size() == 4 )
		return QValidator::State::Acceptable;
	return State::Intermediate;
}
