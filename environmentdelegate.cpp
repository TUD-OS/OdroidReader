#include "environmentdelegate.h"
#include <QComboBox>
#include <QPushButton>

EnvironmentDelegate::EnvironmentDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{
}

QWidget* EnvironmentDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const {
	if (index.column() > 0 && index.column() < 4) {
		QComboBox *cb = new QComboBox(parent);
		//250,300,350,400,450,500,550,600,800,900,1000,1100,1200,1300,1400,1500,1600
		for (int i = 250; i <= 600; i += 50)
			cb->addItem(QString("%1 MHz").arg(i),i);
		for (int i = 800; i <= 1600; i+= 100)
			cb->addItem(QString("%1 MHz").arg(i),i);
		return cb;
	}
	if (index.column() == 4) {
		QComboBox *cb = new QComboBox(parent);
		cb->addItem("ondemand");
		cb->addItem("interactive");
		cb->addItem("conservative");
		cb->addItem("userspace");
		cb->addItem("powersave");
		cb->addItem("performance");
		return cb;
	}

	return QStyledItemDelegate::createEditor(parent,option,index);
}

void EnvironmentDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
	if (QComboBox *cb = qobject_cast<QComboBox*>(editor)) {
		int cv = index.data(Qt::EditRole).toInt();
		int idx = cb->findData(cv);
		if (idx >= 0) cb->setCurrentIndex(idx);
	} else {
		QStyledItemDelegate::setEditorData(editor,index);
	}
}

void EnvironmentDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
	if (QComboBox* cb = qobject_cast<QComboBox*>(editor))
		if (index.column() != 4)
			model->setData(index,cb->currentData(Qt::UserRole));
		else
			model->setData(index,cb->currentText());
	else
		QStyledItemDelegate::setModelData(editor,model,index);
}
