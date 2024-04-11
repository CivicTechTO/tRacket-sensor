/* noisemeter-device - Firmware for CivicTechTO's Noisemeter Device
 * Copyright (C) 2024  Clyne Sullivan, Nick Barnard
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
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

