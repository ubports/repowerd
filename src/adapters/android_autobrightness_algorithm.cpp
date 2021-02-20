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

#include "android_autobrightness_algorithm.h"
#include "brightness_params.h"
#include "device_config.h"
#include "event_loop.h"
#include "event_loop_handler_registration.h"
#include "src/core/log.h"
#include "monotone_spline.h"

#include <cmath>
#include <stdexcept>
#include <sstream>
#include <vector>
#include <algorithm>

namespace
{

char const* const log_tag = "AndroidAutobrightnessAlgorithm";
auto const null_handler = [](auto){};
auto constexpr smoothing_factor_slow = 2000.0;
auto constexpr smoothing_factor_fast = 200.0;
auto constexpr hysteresis_factor = 0.1;
auto constexpr debounce_delay = std::chrono::seconds{4};

std::vector<int> parse_int_array(std::string const& str)
{
    std::vector<int> elems;
    std::stringstream ss{str};

    std::string item;

    while (std::getline(ss, item, ','))
        elems.push_back(std::stoi(item));

    return elems;
}

std::unique_ptr<repowerd::MonotoneSpline>
create_brightness_spline(repowerd::DeviceConfig const& device_config)
try
{
    auto const ab_light_levels_str = device_config.get("autoBrightnessLevels", "");
    std::string ab_brightness_levels_str = device_config.get("autoBrightnessLcdBacklightValues", "");
    if (ab_brightness_levels_str == "")
        ab_brightness_levels_str = device_config.get("screenBrightnessBacklight", "");
    if (ab_brightness_levels_str == "")
        throw std::runtime_error{"Autobrightness not supported"};
    auto ab_light_levels = parse_int_array(ab_light_levels_str);
    auto const ab_brightness_levels = parse_int_array(ab_brightness_levels_str);

    if (ab_light_levels.size() == 0 || ab_brightness_levels.size() == 0 ||
        ab_brightness_levels.size() != ab_light_levels.size() + 1)
    {
        throw std::runtime_error{"Autobrightness not supported"};
    }

    ab_light_levels.insert(ab_light_levels.begin(), 0);

    std::vector<repowerd::MonotoneSpline::Point> points;
    auto l_iter = ab_light_levels.begin();
    auto b_iter = ab_brightness_levels.begin();
    for (;
         l_iter != ab_light_levels.end() && b_iter != ab_brightness_levels.end();
         ++l_iter, ++b_iter)
    {
        points.push_back({static_cast<double>(*l_iter), static_cast<double>(*b_iter)});
    }

    return std::make_unique<repowerd::MonotoneSpline>(points);
}
catch (...)
{
    return {};
}

double get_max_brightness(repowerd::DeviceConfig const& device_config)
{
    auto const brightness_params = repowerd::BrightnessParams::from_device_config(device_config);
    return brightness_params.max_value;
}

double exponential_smoothing(
    double old_average, double new_value, double smoothing_factor)
{
    return old_average + smoothing_factor * (new_value - old_average);
}

}

repowerd::AndroidAutobrightnessAlgorithm::AndroidAutobrightnessAlgorithm(
    DeviceConfig const& device_config,
    std::shared_ptr<Log> const& log)
    : brightness_spline{create_brightness_spline(device_config)},
      max_brightness{get_max_brightness(device_config)},
      log{log},
      started{false},
      debouncing_seqnum{0}
{
    reset();
}

repowerd::AndroidAutobrightnessAlgorithm::~AndroidAutobrightnessAlgorithm() = default;

bool repowerd::AndroidAutobrightnessAlgorithm::init(EventLoop& event_loop)
{
    if (!brightness_spline) return false;

    this->event_loop = &event_loop;
    return true;
}

void repowerd::AndroidAutobrightnessAlgorithm::new_light_value(double light)
{
    if (!started)
        return;

    auto const is_first_light_value = !have_previous_light_values();

    log->log(log_tag, "process_new_light_value(%.2f), is_first_light_value=%d",
             light, is_first_light_value);

    update_averages(light);

    if (is_first_light_value)
    {
        notify_brightness(brightness_spline->interpolate(fast_average));
        applied_light = fast_average;
    }
    else
    {
        schedule_debounce();
    }
}

void repowerd::AndroidAutobrightnessAlgorithm::start()
{
    if (!started)
    {
        reset();
        started = true;
    }
}

void repowerd::AndroidAutobrightnessAlgorithm::stop()
{
    if (started)
    {
        reset();
        started = false;
    }
}

repowerd::HandlerRegistration repowerd::AndroidAutobrightnessAlgorithm::register_autobrightness_handler(
    AutobrightnessHandler const& handler)
{
    return EventLoopHandlerRegistration{
        *event_loop,
        [this, &handler]{ this->autobrightness_handler = handler; },
        [this]{ this->autobrightness_handler = null_handler; }};
}

void repowerd::AndroidAutobrightnessAlgorithm::reset()
{
    last_light = 0.0;
    last_light_tp = {};
    applied_light = 0.0;
    fast_average = 0.0;
    slow_average = 0.0;
    debouncing = false;
    ++debouncing_seqnum;
}

bool repowerd::AndroidAutobrightnessAlgorithm::have_previous_light_values()
{
    return last_light_tp != std::chrono::steady_clock::time_point{};
}

void repowerd::AndroidAutobrightnessAlgorithm::update_averages(double light)
{
    auto const now = std::chrono::steady_clock::now();

    if (!have_previous_light_values())
    {
        fast_average = light;
        slow_average = light;
    }
    else
    {
        auto const dt = now - last_light_tp;
        double const dt_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(dt).count();

        auto const fast_factor = dt_ms / (dt_ms + smoothing_factor_fast);
        auto const slow_factor = dt_ms / (dt_ms + smoothing_factor_slow);

        fast_average = exponential_smoothing(fast_average, light, fast_factor);
        slow_average = exponential_smoothing(slow_average, light, slow_factor);
    }

    last_light_tp = now;
    last_light = light;
}

void repowerd::AndroidAutobrightnessAlgorithm::schedule_debounce()
{
    if (debouncing)
        return;

    debouncing = true;
    ++debouncing_seqnum;

    log->log(log_tag, "schedule_debounce(), seqnum=%d", debouncing_seqnum);

    event_loop->schedule_in(
        debounce_delay,
        [this, expected_debouncing_seqnum=debouncing_seqnum]
        {
            if (debouncing_seqnum != expected_debouncing_seqnum)
            {
                log->log(log_tag, "debounce() ignored, expected_seqnum=%d, actual_seqnum=%d",
                         expected_debouncing_seqnum, debouncing_seqnum);
                return;
            }

            auto constexpr min_hysteresis = 2.0;

            debouncing = false;
            update_averages(last_light);

            auto const hysteresis = std::max(applied_light * hysteresis_factor, min_hysteresis);
            auto const slow_delta = slow_average - applied_light;
            auto const fast_delta = fast_average - applied_light;
            log->log(log_tag,
                     "debounce(), seqnum=%d, applied_light=%.2f, hysteresis=%.2f, "
                     "slow_average=%.2f, fast_average=%.2f, slow_delta=%.2f, "
                     "fast_delta=%.2f",
                     expected_debouncing_seqnum, applied_light, hysteresis, slow_average,
                     fast_average, slow_delta, fast_delta);

            if ((slow_delta >= hysteresis && fast_delta >= hysteresis) ||
                (-slow_delta >= hysteresis && -fast_delta >= hysteresis))
            {
                log->log(log_tag, "debounce(), apply light %.2f", fast_average);
                notify_brightness(brightness_spline->interpolate(fast_average));
                applied_light = fast_average;
            }

            auto const hysteresis_last_light =
                std::max(last_light * hysteresis_factor, min_hysteresis);

            if (fabs(fast_average - last_light) >= hysteresis_last_light)
                schedule_debounce();
        });
}

void repowerd::AndroidAutobrightnessAlgorithm::notify_brightness(double brightness)
{
    return autobrightness_handler(brightness / max_brightness);
}
