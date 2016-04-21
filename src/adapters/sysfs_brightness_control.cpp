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

#include "sysfs_brightness_control.h"
#include "brightness_params.h"

#include <vector>
#include <string>
#include <fstream>

#include <dirent.h>

namespace
{

std::vector<std::string> directories_in(std::string const& parent_dir)
{
    auto dir = opendir(parent_dir.c_str());
    if (!dir)
        return {};

    dirent* dp;

    std::vector<std::string> child_dirs;

    while  ((dp = readdir(dir)) != nullptr)
    {
        if (dp->d_type == DT_DIR)
        {
            child_dirs.push_back(parent_dir + "/" + dp->d_name);
        }
    }

    closedir(dir);

    return child_dirs;
}

bool file_exists(std::string const& file)
{
    std::ifstream fs{file};
    return fs.good();
}

std::string determine_sysfs_backlight_dir(std::string const& sysfs_base_dir)
{
    std::string const sys_backlight_root{sysfs_base_dir + "/class/backlight"};
    std::string const sys_led_backlight{sysfs_base_dir + "/class/leds/lcd-backlight"};

    for (auto const& dir : directories_in(sys_backlight_root))
    {
        if (file_exists(dir + "/brightness"))
            return dir;
    }

    if (file_exists(sys_led_backlight + "/brightness"))
        return sys_led_backlight;

    throw std::runtime_error("Couldn't find backlight in sysfs");
}

int determine_max_brightness(std::string const& backlight_dir)
{
    std::ifstream fs{backlight_dir + "/max_brightness"};
    int max_brightness = 0;
    fs >> max_brightness;
    return max_brightness;
}

float normal_brightness_percent(repowerd::DeviceConfig const& device_config)
{
    auto brightness_params = repowerd::BrightnessParams::from_device_config(device_config);
    return static_cast<float>(brightness_params.default_value) / brightness_params.max_value;
}

float dim_brightness_percent(repowerd::DeviceConfig const& device_config)
{
    auto const brightness_params = repowerd::BrightnessParams::from_device_config(device_config);
    return static_cast<float>(brightness_params.dim_value) / brightness_params.max_value;
}

}

repowerd::SysfsBrightnessControl::SysfsBrightnessControl(
    std::string const& sysfs_base_dir,
    DeviceConfig const& device_config)
    : sysfs_backlight_dir{determine_sysfs_backlight_dir(sysfs_base_dir)},
      max_brightness{determine_max_brightness(sysfs_backlight_dir)},
      dim_brightness{
          static_cast<int>(dim_brightness_percent(device_config) * max_brightness)},
      normal_brightness{
          static_cast<int>(normal_brightness_percent(device_config) * max_brightness)}
{
}

void repowerd::SysfsBrightnessControl::disable_autobrightness()
{
}

void repowerd::SysfsBrightnessControl::enable_autobrightness()
{
}

void repowerd::SysfsBrightnessControl::set_dim_brightness()
{
    write_brightness_value(dim_brightness);
    normal_brightness_active = false;
}

void repowerd::SysfsBrightnessControl::set_normal_brightness()
{
    write_brightness_value(normal_brightness);
    normal_brightness_active = true;
}

void repowerd::SysfsBrightnessControl::set_normal_brightness_value(float v)
{
    normal_brightness = v * max_brightness;
    if (normal_brightness_active)
        set_normal_brightness();
}

void repowerd::SysfsBrightnessControl::set_off_brightness()
{
    write_brightness_value(0);
    normal_brightness_active = false;
}

void repowerd::SysfsBrightnessControl::write_brightness_value(int brightness)
{
    std::ofstream fs{sysfs_backlight_dir + "/brightness"};
    fs << brightness;
    fs.flush();
}
