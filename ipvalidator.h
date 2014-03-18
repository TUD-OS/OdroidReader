#ifndef IPVALIDATOR_H
#define IPVALIDATOR_H

#include <QValidator>

class IPValidator : public QValidator
{
  Q_OBJECT
public:
  explicit IPValidator(QObject *parent = 0);
  QValidator::State validate(QString &, int &) const;
signals:

public slots:

};

#endif // IPVALIDATOR_H
