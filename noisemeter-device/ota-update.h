#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H

#include <WString.h>
#include <WiFiClientSecure.h>

struct OTAUpdate
{
    String version;

    // Root CA needs to be passed to the object due to linking issues.
    // TODO this is not a good solution, fix it.
    OTAUpdate(const char *rootCA_):
        rootCA(rootCA_) {}

    // Checks if a new OTA update is available, returns true if so.
    bool available();

    // Downloads and applies the latest OTA update, returns true if successful.
    bool download();

private:
    String url;
    const char *rootCA;

    // Writes the received OTA update to flash memory, returns true on success.
    bool applyUpdate(WiFiClientSecure& client, int totalSize);
};

#endif // OTA_UPDATE_H

