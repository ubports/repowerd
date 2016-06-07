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

#include "path.h"
#include "src/core/log.h"

#include <cmath>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace
{
char const* const log_tag = "SysfsBacklight";

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

repowerd::SysfsBacklight::SysfsBacklight(
    std::shared_ptr<Log> const& log,
    std::string const& sysfs_base_dir)
    : sysfs_backlight_dir{determine_sysfs_backlight_dir(sysfs_base_dir)},
      sysfs_brightness_file{sysfs_backlight_dir/"brightness"},
      max_brightness{determine_max_brightness(sysfs_backlight_dir)},
      last_set_brightness{-1.0}
{
    log->log(log_tag, "Using backlight %s",
             std::string{sysfs_backlight_dir}.c_str());
}

void repowerd::SysfsBacklight::set_brightness(double value)
{
    std::ofstream fs{sysfs_brightness_file};
    fs << absolute_brightness_for(value);
    fs.flush();
    last_set_brightness = value;
}

double repowerd::SysfsBacklight::get_brightness()
{
    std::ifstream fs{sysfs_brightness_file};
    int abs_brightness = 0;
    fs >> abs_brightness;

    if (absolute_brightness_for(last_set_brightness) == abs_brightness)
        return last_set_brightness;
    else
        return static_cast<double>(abs_brightness) / max_brightness;
}

int repowerd::SysfsBacklight::absolute_brightness_for(double rel_brightness)
{
    return static_cast<int>(round(rel_brightness * max_brightness));
}
