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

#pragma once

#include "src/adapters/device_config.h"

#include <unordered_map>

namespace repowerd
{
namespace test
{

struct FakeDeviceConfig : repowerd::DeviceConfig
{
    FakeDeviceConfig();

    std::string get(
        std::string const& name, std::string const& default_prop_value) const override;

    void set(std::string const& name, std::string const& value);

    int const brightness_dim_value = 5;
    int const brightness_min_value = 2;
    int const brightness_max_value = 100;
    int const brightness_default_value = 50;
    bool const brightness_autobrightness_supported = true;
    int const shutdown_battery_temperature = 990;

private:
    std::unordered_map<std::string,std::string> properties;
};

}
}
