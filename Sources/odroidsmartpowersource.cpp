#include "Sources/odroidsmartpowersource.h"

OdroidSmartPowerSource::OdroidSmartPowerSource(const char *path) :
    DataSource("Odroid"), _running(false), reconnect(false), _path(path)
{
    descs.append(new DataDescriptor("U_ext","V",1,DataDescriptor::Type::FLOAT));
    descs.append(new DataDescriptor("I_ext","A",1,DataDescriptor::Type::FLOAT));
    descs.append(new DataDescriptor("P_ext","W",1,DataDescriptor::Type::FLOAT));
}

QString OdroidSmartPowerSource::descriptor() {
    return "[DEV] Odroid Smart Power " + _path;
}

void OdroidSmartPowerSource::start() {
    if (!_running) {
        if (!reconnect)
            emit descriptorsAvailable(descs);
        _running = true;
        reconnect = true;
        emit connected();
    } else  {
        _running = false;
        emit disconnected();
        //TODO: Implement stop
    }
}
