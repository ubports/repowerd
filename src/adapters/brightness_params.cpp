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

#include "brightness_params.h"

#include "device_config.h"

int string_to_int(std::string const& value, int default_value)
{
    try { return std::stoi(value); }
    catch (...) { return default_value; }
}

repowerd::BrightnessParams repowerd::BrightnessParams::from_device_config(
    DeviceConfig const& device_config)
{
    auto dim_str = device_config.get("screenBrightnessDim", "10");
    auto min_str = device_config.get("screenBrightnessSettingMinimum", "10");
    auto max_str = device_config.get("screenBrightnessSettingMaximum", "255");
    auto default_str = device_config.get("screenBrightnessSettingDefault", "102");
    auto ab_str = device_config.get("automatic_brightness_available", "false");

    BrightnessParams params;
    params.dim_value = string_to_int(dim_str, 10);
    params.min_value = string_to_int(min_str, 10);
    params.max_value = string_to_int(max_str, 255);
    params.default_value = string_to_int(default_str, 102);
    params.autobrightness_supported = (ab_str == "true");

    return params;
}
