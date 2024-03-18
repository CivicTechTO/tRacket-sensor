#ifndef ACCESS_POINT_H
#define ACCESS_POINT_H

#include <DNSServer.h>
#include <WebServer.h>

class AccessPoint : public RequestHandler
{
    static constexpr auto SSID = "Noise meter";
    static constexpr auto Passkey = "noisemeter";

public:
    // Begins hosting a WiFi access point with the credentials form as a
    // captive portal.
    AccessPoint(void (*func)(WebServer&)):
        server(80),
        onCredentialsReceived(func) {}

    // Enters an infinite loop to handle the access point and web server.
    [[noreturn]] void run();

    // RequestHandler implementation for web server functionality.
    bool canHandle(HTTPMethod, String) override;
    bool handle(WebServer&, HTTPMethod, String) override;

private:
    DNSServer dns;
    WebServer server;
    void (*onCredentialsReceived)(WebServer&);

    static const IPAddress IP;
    static const IPAddress Netmask;
    static const char *htmlSetup;
    static const char *htmlSubmit;
};

#endif // ACCESS_POINT_H

