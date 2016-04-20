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

#include "fake_device_config.h"

std::string repowerd::test::FakeDeviceConfig::get(
    std::string const& name, std::string const& default_prop_value) const
{
    if (name == "screenBrightnessDim")
        return std::to_string(brightness_dim_value);
    else if (name == "screenBrightnessSettingMininum")
        return std::to_string(brightness_min_value);
    else if (name == "screenBrightnessSettingMaximum")
        return std::to_string(brightness_max_value);
    else if (name == "screenBrightnessSettingDefault")
        return std::to_string(brightness_default_value);
    else if (name == "automatic_brightness_available")
        return "true";
    else
        return default_prop_value;
}
