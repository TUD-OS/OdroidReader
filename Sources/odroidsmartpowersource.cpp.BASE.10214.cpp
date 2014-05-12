#include "Sources/odroidsmartpowersource.h"

OdroidSmartPowerSource::OdroidSmartPowerSource(const char *path) :
    DataSource("Odroid"), _path(path)
{
}

QString OdroidSmartPowerSource::descriptor() {
    return "[DEV] Odroid Smart Power" + _path;
}

void OdroidSmartPowerSource::start() {
    //TODO
}
