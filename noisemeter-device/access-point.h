/// @file
/// @brief Captive portal implementation for device setup.
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

#include <optional>

/**
 * @brief Manages the WiFi access point, captive portal, and user setup form.
 *
 * The access point is used to provide the user with a way to configure and set
 * up their device. After the user connects to the network using the fixed SSID
 * and passkey, they will be redirected to a setup form via a captive portal.
 * The form collects the user's WiFi credentials and any other needed
 * information (e.g. email address for first-time setup), then allows the
 * firmware to store the submitted response and connect to the internet.
 *
 * At the moment, once the access point is started it cannot be exited. The
 * firmware needs to do a software reset after handling a form submission to
 * begin normal operation.
 */
class AccessPoint : public RequestHandler
{
    /** Hard-coded SSID for the access point. */
    static constexpr auto SSID = "tRacket Setup";
    /** Hard-coded passkey for the access point. */
    static constexpr auto Passkey = "noise123";

public:
    /**
     * Submission handler receives WebServer for input data and returns an
     * error message on failure.
     */
    using SubmissionHandler = std::optional<const char *> (*)(WebServer&);

    /**
     * Starts the WiFi access point using the fixed credentials.
     * @param func Callback to handle user setup form submission.
     */
    AccessPoint(SubmissionHandler func);

    /**
     * Enters an infinite loop to handle the access point and web server.
     */
    void run();

private:

    unsigned long timeout;
    /** DNS server object for captive portal redirection. */
    DNSServer dns;
    /** Web server object for running the setup form. */
    WebServer server;
    /** Callback for setup form completion. */
    SubmissionHandler onCredentialsReceived;

    /** Hard-coded IP address for the ESP32 when hosting the access point. */
    static const IPAddress IP;
    /** Hard-coded netmask for access point configuration. */
    static const IPAddress Netmask;
    /** Provides HTML for an error page with the given message. */
    static String htmlFromMsg(const char *msg);

    /** Determines which HTTP requests should be handled. */
    bool canHandle(HTTPMethod, String) override;
    /** Handles requests by redirecting to the setup page. */
    bool handle(WebServer&, HTTPMethod, String) override;
};

#endif // ACCESS_POINT_H

