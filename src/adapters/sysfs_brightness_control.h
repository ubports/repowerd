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

#include "src/core/brightness_control.h"

namespace repowerd
{

class SysfsBrightnessControl : public BrightnessControl
{
public:
    SysfsBrightnessControl(std::string const& sysfs_base_dir);

    void disable_autobrightness() override;
    void enable_autobrightness() override;
    void set_dim_brightness() override;
    void set_normal_brightness() override;
    void set_normal_brightness_value(float) override;
    void set_off_brightness() override;

private:
    void write_brightness_value(int brightness);

    std::string const sysfs_backlight_dir;
    int const max_brightness;
    int dim_brightness;
    int normal_brightness;
    bool normal_brightness_active;
};

}
