#ifndef ACCESS_POINT_H
#define ACCESS_POINT_H

#include <WebServer.h>

class AccessPoint
{
    static constexpr auto SSID = "Noise meter";
    static constexpr auto Passkey = "noisemeter";
    // IP address set in access-point.cpp

public:
    AccessPoint():
        server(80), funcOnCredentialsReceived(nullptr) {}

    // Configure the WiFi radio to be an access point.
    void begin();

    // Enter main loop for executing the access point and web server.
    [[noreturn]] void run();

    // Set handler for reception of WiFi credentials.
    void onCredentialsReceived(void (*func)(WebServer&));

private:
    WebServer server;
    void (*funcOnCredentialsReceived)(WebServer&);

    static const IPAddress IP;
    static const IPAddress Netmask;
    static const char *htmlSetup;
    static const char *htmlSubmit;
};

#endif // ACCESS_POINT_H

