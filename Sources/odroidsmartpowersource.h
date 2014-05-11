#ifndef ODROIDSMARTPOWERSOURCE_H
#define ODROIDSMARTPOWERSOURCE_H

#include <Data/datasource.h>
#include <hidapi/hidapi.h>

class OdroidSmartPowerSource : public DataSource
{
    QString _path;
    hid_device* device;
public:
    OdroidSmartPowerSource(const char* path);
    QString path() { return _path; }
    bool canExecute() const { return false; }
    bool isRunning() const { return false; }
    QString descriptor();

public slots:
    void start();

};

#endif // ODROIDSMARTPOWERSOURCE_H
