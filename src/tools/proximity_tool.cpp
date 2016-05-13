/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "src/adapters/android_device_quirks.h"
#include "src/adapters/console_log.h"
#include "src/adapters/ubuntu_proximity_sensor.h"

#include <iostream>
#include <string>

void print_proximity(std::string const& s, repowerd::ProximityState state)
{
    if (state == repowerd::ProximityState::near)
        std::cout << s << "near" << std::endl;
    else
        std::cout << s << "far" << std::endl;
}

int main()
{
    auto log = std::make_shared<repowerd::ConsoleLog>();
    repowerd::AndroidDeviceQuirks quirks;
    repowerd::UbuntuProximitySensor sensor{log, quirks};

    auto registration = sensor.register_proximity_handler(
        [] (repowerd::ProximityState state)
        {
            print_proximity("EVENT: ", state);
        });

    bool running = true;

    std::cout << "Commands (press enter after command letter): "<< std::endl
              << "  e => enable proximity events" << std::endl
              << "  d => disable proximity events" << std::endl
              << "  s => display proximity state" << std::endl
              << "  q => quit" << std::endl;

    while (running)
    {
        std::string line;
        std::getline(std::cin, line);

        if (line == "q")
        {
            running = false;
        }
        else if (line == "d")
        {
            std::cout << "Disabling proximity events" << std::endl;
            sensor.disable_proximity_events();
        }
        else if (line == "e")
        {
            std::cout << "Enabling proximity events" << std::endl;
            sensor.enable_proximity_events();
        }
        else if (line == "s")
        {
            print_proximity("STATE: ", sensor.proximity_state());
        }
    }
}
