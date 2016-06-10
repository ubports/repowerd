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

#include "filesystem.h"
#include "path.h"
#include "src/core/log.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace
{
char const* const log_tag = "SysfsBacklight";

struct CompareBacklightPriority
{
    CompareBacklightPriority(repowerd::Filesystem& filesystem)
        : filesystem{filesystem}
    {
    }

    int get_backlight_priority(repowerd::Path const& backlight_dir)
    {
        auto istream = filesystem.istream(backlight_dir/"type");
        std::string type;
        *istream >> type;

        if (type == "firmware")
            return 1;
        else if (type == "platform")
            return 2;
        else if (type == "raw")
            return 3;

        return 10;
    }

    bool operator()(repowerd::Path const& b1, repowerd::Path const& b2)
    {
        return get_backlight_priority(b1) < get_backlight_priority(b2);
    }

    repowerd::Filesystem& filesystem;
};

repowerd::Path determine_sysfs_backlight_dir(repowerd::Filesystem& filesystem)
{
    repowerd::Path const sys_backlight_root{"/sys/class/backlight"};
    repowerd::Path const sys_led_backlight{"/sys/class/leds/lcd-backlight"};

    std::vector<repowerd::Path> backlights;

    for (auto const& dir : filesystem.subdirs(sys_backlight_root))
    {
        if (filesystem.is_regular_file(repowerd::Path{dir}/"brightness"))
            backlights.push_back(dir);
    }

    if (!backlights.empty())
    {
        std::stable_sort(backlights.begin(), backlights.end(),
                         CompareBacklightPriority(filesystem));
        return backlights.front();
    }

    if (filesystem.is_regular_file(sys_led_backlight/"brightness"))
        return sys_led_backlight;

    throw std::runtime_error("Couldn't find backlight in sysfs");
}

int determine_max_brightness(
    repowerd::Filesystem& filesystem, repowerd::Path const& backlight_dir)
{
    auto istream = filesystem.istream(backlight_dir/"max_brightness");
    int max_brightness = 0;
    *istream >> max_brightness;
    return max_brightness;
}

}

repowerd::SysfsBacklight::SysfsBacklight(
    std::shared_ptr<Log> const& log,
    std::shared_ptr<Filesystem> const& filesystem)
    : filesystem{filesystem},
      sysfs_backlight_dir{determine_sysfs_backlight_dir(*filesystem)},
      sysfs_brightness_file{sysfs_backlight_dir/"brightness"},
      max_brightness{determine_max_brightness(*filesystem, sysfs_backlight_dir)},
      last_set_brightness{-1.0}
{
    log->log(log_tag, "Using backlight %s",
             std::string{sysfs_backlight_dir}.c_str());
}

void repowerd::SysfsBacklight::set_brightness(double value)
{
    auto ostream = filesystem->ostream(sysfs_brightness_file);
    *ostream << absolute_brightness_for(value);
    ostream->flush();
    last_set_brightness = value;
}

double repowerd::SysfsBacklight::get_brightness()
{
    auto istream = filesystem->istream(sysfs_brightness_file);
    int abs_brightness = 0;
    *istream >> abs_brightness;

    if (absolute_brightness_for(last_set_brightness) == abs_brightness)
        return last_set_brightness;
    else
        return static_cast<double>(abs_brightness) / max_brightness;
}

int repowerd::SysfsBacklight::absolute_brightness_for(double rel_brightness)
{
    return static_cast<int>(round(rel_brightness * max_brightness));
}
