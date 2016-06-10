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
#include "src/core/power_source.h"

#include <cstdlib>
#include <iostream>
#include <string>

int main()
{
    setenv("REPOWERD_LOG", "console", 1);

    repowerd::DefaultDaemonConfig config;
    auto const power_source = config.the_power_source();

    power_source->start_processing();

    auto registration = power_source->register_power_source_change_handler(
        []
        {
            std::cout << "Power source changed" << std::endl;
        });

    bool running = true;

    std::cout << "Commands (press enter after command letter): "<< std::endl
              << "  q => quit" << std::endl;

    while (running)
    {
        std::string line;
        std::getline(std::cin, line);

        if (line == "q")
        {
            running = false;
        }
    }
}
