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

#include "autobrightness_algorithm.h"

#include <chrono>

namespace repowerd
{

class DeviceConfig;
class MonotoneSpline;

class AndroidAutobrightnessAlgorithm : public AutobrightnessAlgorithm
{
public:
    AndroidAutobrightnessAlgorithm(DeviceConfig const& device_config);
    ~AndroidAutobrightnessAlgorithm();

    bool init(EventLoop& event_loop) override;

    void new_light_value(double light) override;
    void reset() override;

    HandlerRegistration register_autobrightness_handler(
        AutobrightnessHandler const& handler) override;

private:
    void update_averages(double light);
    void schedule_debounce();
    void notify_brightness(double brightness);

    EventLoop* event_loop;
    std::unique_ptr<MonotoneSpline> const brightness_spline;
    double const max_brightness;
    AutobrightnessHandler autobrightness_handler;

    std::chrono::steady_clock::time_point last_light_tp;
    double last_light;
    double applied_light;
    double fast_average;
    double slow_average;
    bool debouncing;
};

}
