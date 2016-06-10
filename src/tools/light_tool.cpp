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

#include "src/default_daemon_config.h"
#include "src/adapters/light_sensor.h"

#include <cstdlib>
#include <iostream>
#include <string>

int main()
{
    setenv("REPOWERD_LOG", "console", 1);

    repowerd::DefaultDaemonConfig config;
    auto const light_sensor = config.the_light_sensor();

    auto registration = light_sensor->register_light_handler(
        [] (double light)
        {
            std::cout << "LIGHT: " << light << std::endl;
        });

    bool running = true;

    std::cout << "Commands (press enter after command letter): "<< std::endl
              << "  e => enable light events" << std::endl
              << "  d => disable light events" << std::endl
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
            std::cout << "Disabling light events" << std::endl;
            light_sensor->disable_light_events();
        }
        else if (line == "e")
        {
            std::cout << "Enabling light events" << std::endl;
            light_sensor->enable_light_events();
        }
    }
}
