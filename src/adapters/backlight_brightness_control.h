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
#include "brightness_notification.h"
#include "event_loop.h"

#include <memory>

namespace repowerd
{

class AutobrightnessAlgorithm;
class Backlight;
class Chrono;
class DeviceConfig;
class DeviceQuirks;
class LightSensor;
class Log;

class BacklightBrightnessControl : public BrightnessControl, public BrightnessNotification
{
public:
    BacklightBrightnessControl(
        std::shared_ptr<Backlight> const& backlight,
        std::shared_ptr<LightSensor> const& light_sensor,
        std::shared_ptr<AutobrightnessAlgorithm> const& autobrightness_algorithm,
        std::shared_ptr<Chrono> const& chrono,
        std::shared_ptr<Log> const& log,
        DeviceConfig const& device_config,
        DeviceQuirks const& device_quirks);

    void disable_autobrightness() override;
    void enable_autobrightness() override;
    void set_dim_brightness() override;
    void set_normal_brightness() override;
    void set_normal_brightness_value(double) override;
    void set_off_brightness() override;

    HandlerRegistration register_brightness_handler(
        BrightnessHandler const& handler) override;

private:
    enum class ActiveBrightnessType {normal, dim, off};
    enum class TransitionSpeed {normal, slow};
    void transition_to_brightness_value(double brightness, TransitionSpeed transition_speed);
    void set_brightness_value(double brightness);
    double get_brightness_value();

    std::shared_ptr<Backlight> const backlight;
    std::shared_ptr<LightSensor> const light_sensor;
    std::shared_ptr<AutobrightnessAlgorithm> const autobrightness_algorithm;
    std::shared_ptr<Chrono> const chrono;
    std::shared_ptr<Log> const log;
    bool const normal_before_display_on_autobrightness;
    bool const ab_supported;

    EventLoop event_loop;
    HandlerRegistration light_handler_registration;
    HandlerRegistration ab_handler_registration;
    BrightnessHandler brightness_handler;

    double dim_brightness;
    double normal_brightness;
    double user_normal_brightness;
    ActiveBrightnessType active_brightness_type;
    bool ab_active;
};

}
