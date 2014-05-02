#include "Sources/networksource.h"

NetworkSource::NetworkSource(QString name, QString address, quint16 port, QObject *parent) :
	DataSource(name,parent), _address(address), _port(port)
{
	networkThread = new QThread();
	this->moveToThread(networkThread); //TODO: this should not work if we have a parent. Enforce?
}

void NetworkSource::connect() {

}
