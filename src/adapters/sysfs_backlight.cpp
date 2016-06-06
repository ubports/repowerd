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

#include "sysfs_backlight.h"

#include <cmath>
#include <fstream>
#include <stdexcept>
#include <vector>

#include "path.h"

namespace
{

repowerd::Path determine_sysfs_backlight_dir(std::string const& sysfs_base_dir)
{
    repowerd::Path const sys_backlight_root{
        repowerd::Path{sysfs_base_dir}/"class"/"backlight"};
    repowerd::Path const sys_led_backlight{
        repowerd::Path{sysfs_base_dir}/"class"/"leds"/"lcd-backlight"};

    for (auto const& dir : sys_backlight_root.subdirs())
    {
        if ((dir/"brightness").is_regular_file())
            return dir;
    }

    if ((sys_led_backlight/"brightness").is_regular_file())
        return sys_led_backlight;

    throw std::runtime_error("Couldn't find backlight in sysfs");
}

int determine_max_brightness(repowerd::Path const& backlight_dir)
{
    std::ifstream fs{backlight_dir/"max_brightness"};
    int max_brightness = 0;
    fs >> max_brightness;
    return max_brightness;
}

}

repowerd::SysfsBacklight::SysfsBacklight(std::string const& sysfs_base_dir)
    : sysfs_backlight_dir{determine_sysfs_backlight_dir(sysfs_base_dir)},
      sysfs_brightness_file{sysfs_backlight_dir/"brightness"},
      max_brightness{determine_max_brightness(sysfs_backlight_dir)}
{
}

void repowerd::SysfsBacklight::set_brightness(double value)
{
    std::ofstream fs{sysfs_brightness_file};
    fs << static_cast<int>(round(value * max_brightness));
    fs.flush();
}

double repowerd::SysfsBacklight::get_brightness()
{
    std::ifstream fs{sysfs_brightness_file};
    int brightness = 0;
    fs >> brightness;
    return static_cast<double>(brightness) / max_brightness;
}
