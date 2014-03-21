#include "datapoint.h"
#include <QtEndian>

DatapointBase::DatapointBase(QByteArray ba)
{
	min_item = new QTableWidgetItem("undef");
	min_item->setBackgroundColor(Qt::yellow);
	last_item = new QTableWidgetItem(*min_item);
	max_item = new QTableWidgetItem(*min_item);
	avg_item = new QTableWidgetItem(*min_item);

	//Initialize from bytearray
	quint8 name_len = static_cast<quint8>(ba.left(1).constData()[0]);
	ba.remove(0,1);
	_type = static_cast<FIELD_TYPE>(ba.left(1).constData()[0]);
	ba.remove(0,1);
	_factor = qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(ba.left(4).constData()));
	ba.remove(0,4);
	_name = QString(ba.left(name_len).constData());
	ba.remove(0,name_len);
	_unit = QString(ba.constData());
	initialized = false;

	pc = new QwtPlotCurve(_name);
	unit_item = new QTableWidgetItem(_unit);
	unit_item->setBackgroundColor(Qt::yellow);
	name_item = new QTableWidgetItem(_name);
	name_item->setBackgroundColor(Qt::yellow);
}

DatapointBase::~DatapointBase() {
	delete name_item;
	delete unit_item;
	delete min_item;
	delete max_item;
	delete avg_item;
	delete last_item;
}
