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

#include "src/adapters/upower_power_source.h"
#include "src/adapters/console_log.h"
#include "src/adapters/android_device_config.h"

#include <gio/gio.h>
#include <iostream>
#include <string>

int main()
{
    repowerd::AndroidDeviceConfig device_config{
        {POWERD_DEVICE_CONFIGS_PATH, REPOWERD_DEVICE_CONFIGS_PATH}};

    auto bus_cstr = g_dbus_address_get_for_bus_sync(G_BUS_TYPE_SYSTEM, nullptr, nullptr);
    std::string const bus{bus_cstr};
    g_free(bus_cstr);

    auto const log = std::make_shared<repowerd::ConsoleLog>();

    repowerd::UPowerPowerSource power_source{log, device_config, bus};
    power_source.start_processing();

    auto registration = power_source.register_power_source_change_handler(
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
