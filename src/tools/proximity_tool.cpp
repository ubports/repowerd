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
#include "src/core/proximity_sensor.h"

#include <cstdlib>
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
    setenv("REPOWERD_LOG", "console", 1);

    repowerd::DefaultDaemonConfig config;
    auto const proximity_sensor = config.the_proximity_sensor();

    auto registration = proximity_sensor->register_proximity_handler(
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
            proximity_sensor->disable_proximity_events();
        }
        else if (line == "e")
        {
            std::cout << "Enabling proximity events" << std::endl;
            proximity_sensor->enable_proximity_events();
        }
        else if (line == "s")
        {
            print_proximity("STATE: ", proximity_sensor->proximity_state());
        }
    }
}
