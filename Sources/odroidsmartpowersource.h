#ifndef ODROIDSMARTPOWERSOURCE_H
#define ODROIDSMARTPOWERSOURCE_H

#include <Data/datasource.h>
#include <Data/datadescriptor.h>
#include <hidapi/hidapi.h>

class OdroidSmartPowerSource : public DataSource
{
    bool _running, reconnect;
    QString _path;
    hid_device* device;
    QVector<const DataDescriptor*> descs;
public:
    OdroidSmartPowerSource(const char* path);
    QString path() { return _path; }
    bool canExecute() const { return false; }
    bool isRunning() const { return _running; }
    QString descriptor();

public slots:
    void start();

};

#endif // ODROIDSMARTPOWERSOURCE_H
