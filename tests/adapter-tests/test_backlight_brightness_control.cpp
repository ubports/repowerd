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

#include "src/adapters/autobrightness_algorithm.h"
#include "src/adapters/backlight_brightness_control.h"
#include "src/adapters/backlight.h"
#include "src/adapters/event_loop_handler_registration.h"
#include "src/adapters/light_sensor.h"

#include "fake_chrono.h"
#include "fake_device_config.h"
#include "fake_device_quirks.h"
#include "fake_log.h"
#include "fake_shared.h"
#include "spin_wait.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <thread>
#include <algorithm>
#include <functional>
#include <numeric>
#include <cmath>

namespace rt = repowerd::test;

using namespace testing;
using namespace std::chrono_literals;

namespace
{

class FakeBacklight : public repowerd::Backlight
{
public:
    void set_brightness(double v) override
    {
        brightness_history.push_back(v);
    }

    double get_brightness() override
    {
        return brightness_history.back();
    }

    void clear_brightness_history()
    {
        auto const last = brightness_history.back();
        brightness_history.clear();
        brightness_history.push_back(last);
    }

    std::vector<double> brightness_steps()
    {
        auto steps = brightness_history;
        std::adjacent_difference(steps.begin(), steps.end(), steps.begin());
        steps.erase(steps.begin());
        std::transform(steps.begin(), steps.end(), steps.begin(),
                       [](auto s) { return std::fabs(s); });
        return steps;
    }

    double brightness_steps_stddev()
    {
        auto const steps = brightness_steps();

        auto const sum = std::accumulate(steps.begin(), steps.end(), 0.0);
        auto const mean = sum / steps.size();

        auto accum = 0.0;
        for (auto const& d : steps)
            accum += (d - mean) * (d - mean);

        return sqrt(accum / (steps.size() - 1));
    }

    double const starting_brightness = 0.5;
    std::vector<double> brightness_history{starting_brightness};
};

class FakeLightSensor : public repowerd::LightSensor
{
public:
    repowerd::HandlerRegistration register_light_handler(
        repowerd::LightHandler const& handler) override
    {
        light_handler = handler;
        return repowerd::HandlerRegistration(
            [this] { light_handler = [](double){}; });
    }

    void enable_light_events() override { enabled = true; }
    void disable_light_events() override { enabled = false; }

    void emit_light_if_enabled(double light)
    {
        if (enabled)
            light_handler(light);
    }

    repowerd::LightHandler light_handler{[](double){}};
    bool enabled{false};
};

class FakeAutobrightnessAlgorithm : public repowerd::AutobrightnessAlgorithm
{
public:
    bool init(repowerd::EventLoop& event_loop) override
    {
        this->event_loop = &event_loop;
        return true;
    }

    void new_light_value(double light) override
    {
        light_history.push_back(light);
    }

    void start() override
    {
        mock.start();
    }

    void stop() override
    {
        mock.stop();
    }

    struct MockMethods
    {
        MOCK_METHOD0(start, void());
        MOCK_METHOD0(stop, void());
    };
    NiceMock<MockMethods> mock;

    void emit_autobrightness(double brightness)
    {
        event_loop->enqueue([this,brightness] { autobrightness_handler(brightness); }).get();
    }

    repowerd::HandlerRegistration register_autobrightness_handler(
        repowerd::AutobrightnessHandler const& handler) override
    {
        return repowerd::EventLoopHandlerRegistration(
            *event_loop,
            [this,&handler] { autobrightness_handler = handler; },
            [this] { autobrightness_handler = [](double){}; });
    }

    repowerd::EventLoop* event_loop;
    repowerd::AutobrightnessHandler autobrightness_handler{[](double){}};
    std::vector<double> light_history;
};

struct ABacklightBrightnessControl : Test
{
    void expect_brightness_value(double brightness)
    {
        EXPECT_THAT(backlight.brightness_history.back(), Eq(brightness));
    }

    std::chrono::milliseconds fake_chrono_duration_of(std::function<void()> const& func)
    {
        auto start = fake_chrono.steady_now();
        func();
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            fake_chrono.steady_now() - start);
    }

    rt::FakeDeviceConfig fake_device_config;
    FakeBacklight backlight;
    FakeLightSensor light_sensor;
    FakeAutobrightnessAlgorithm autobrightness_algorithm;
    rt::FakeChrono fake_chrono;
    rt::FakeLog fake_log;
    rt::FakeDeviceQuirks fake_device_quirks;
    repowerd::BacklightBrightnessControl brightness_control{
        rt::fake_shared(backlight), 
        rt::fake_shared(light_sensor), 
        rt::fake_shared(autobrightness_algorithm),
        rt::fake_shared(fake_chrono),
        rt::fake_shared(fake_log),
        fake_device_config,
        fake_device_quirks};

    double const normal_percent =
        static_cast<double>(fake_device_config.brightness_default_value) /
            fake_device_config.brightness_max_value;

    double const dim_percent =
        static_cast<double>(fake_device_config.brightness_dim_value) /
            fake_device_config.brightness_max_value;

    std::chrono::seconds default_timeout{3};
};

MATCHER_P(IsAbout, a, "")
{
    return arg >= a && arg <= a + 10ms;
}

}

TEST_F(ABacklightBrightnessControl,
       writes_normal_brightness_based_on_device_config)
{
    brightness_control.set_normal_brightness();

    expect_brightness_value(normal_percent);
}

TEST_F(ABacklightBrightnessControl, writes_zero_brightness_value_for_off_brightness)
{
    brightness_control.set_normal_brightness();
    brightness_control.set_off_brightness();

    expect_brightness_value(0);
}

TEST_F(ABacklightBrightnessControl,
       writes_default_dim_brightness_based_on_device_config)
{
    brightness_control.set_dim_brightness();

    expect_brightness_value(dim_percent);
}

TEST_F(ABacklightBrightnessControl, sets_write_normal_brightness_value_immediately_if_in_normal_mode)
{
    brightness_control.set_normal_brightness();
    brightness_control.set_normal_brightness_value(0.7);

    expect_brightness_value(0.7);
}

TEST_F(ABacklightBrightnessControl, does_not_write_new_normal_brightness_value_if_not_in_normal_mode)
{
    brightness_control.set_off_brightness();
    brightness_control.set_normal_brightness_value(0.7);

    expect_brightness_value(0);
}

TEST_F(ABacklightBrightnessControl,
       applies_set_normal_brightness_value_when_changing_to_normal_mode)
{
    brightness_control.set_normal_brightness_value(0.7);
    brightness_control.set_normal_brightness();

    expect_brightness_value(0.7);
}

TEST_F(ABacklightBrightnessControl, transitions_smoothly_between_brightness_values_when_increasing)
{
    brightness_control.set_off_brightness();
    brightness_control.set_normal_brightness();

    EXPECT_THAT(backlight.brightness_history.size(), Ge(20));
    EXPECT_THAT(backlight.brightness_steps_stddev(), Le(0.01));
}

TEST_F(ABacklightBrightnessControl, transitions_smoothly_between_brightness_values_when_decreasing)
{
    brightness_control.set_off_brightness();
    brightness_control.set_normal_brightness();
    backlight.clear_brightness_history();

    brightness_control.set_off_brightness();

    EXPECT_THAT(backlight.brightness_history.size(), Ge(20));
    EXPECT_THAT(backlight.brightness_steps_stddev(), Le(0.01));
}

TEST_F(ABacklightBrightnessControl,
       transitions_between_zero_and_non_zero_brightness_in_100ms)
{
    brightness_control.set_off_brightness();

    EXPECT_THAT(fake_chrono_duration_of([&]{brightness_control.set_normal_brightness();}),
                IsAbout(100ms));
    EXPECT_THAT(fake_chrono_duration_of([&]{brightness_control.set_off_brightness();}),
                IsAbout(100ms));
    EXPECT_THAT(fake_chrono_duration_of([&]{brightness_control.set_dim_brightness();}),
                IsAbout(100ms));
}

TEST_F(ABacklightBrightnessControl,
       does_not_transition_to_dim_if_current_brightness_is_positive_but_lower_than_dim)
{
    auto const half_dim_percent = dim_percent / 2.0;
    ASSERT_THAT(half_dim_percent, Lt(dim_percent));

    backlight.set_brightness(half_dim_percent);

    brightness_control.set_dim_brightness();

    expect_brightness_value(half_dim_percent);
}

TEST_F(ABacklightBrightnessControl,
       transitions_to_dim_if_current_brightness_is_zero)
{
    backlight.set_brightness(0.0);

    brightness_control.set_dim_brightness();

    expect_brightness_value(dim_percent);
}

TEST_F(ABacklightBrightnessControl,
       transitions_to_dim_if_current_brightness_is_unknown)
{
    backlight.set_brightness(repowerd::Backlight::unknown_brightness);

    brightness_control.set_dim_brightness();

    expect_brightness_value(dim_percent);
}

TEST_F(ABacklightBrightnessControl,
       disables_light_events_when_autobrightness_is_disabled)
{
    light_sensor.emit_light_if_enabled(500.0);

    // BacklightBrightnessControl handles light events asynchronously,
    // so we need to allow some time for the request to be processed
    std::this_thread::sleep_for(100ms);

    EXPECT_THAT(autobrightness_algorithm.light_history.size(), Eq(0));
}

TEST_F(ABacklightBrightnessControl,
       enables_light_events_when_autobrightness_enabled_while_in_normal_mode)
{
    brightness_control.set_normal_brightness();
    brightness_control.enable_autobrightness();
    light_sensor.emit_light_if_enabled(500.0);

    // BacklightBrightnessControl handles light events asynchronously,
    // so we need to allow some time for the request to be processed
    rt::spin_wait_for_condition_or_timeout(
        [&] { return autobrightness_algorithm.light_history.size() == 1; },
        default_timeout);
    EXPECT_THAT(autobrightness_algorithm.light_history.size(), Eq(1));
}

TEST_F(ABacklightBrightnessControl,
       enables_light_events_when_set_to_normal_mode_while_autobrightness_enabled)
{
    brightness_control.enable_autobrightness();
    brightness_control.set_normal_brightness();
    light_sensor.emit_light_if_enabled(500.0);

    // BacklightBrightnessControl handles light events asynchronously,
    // so we need to allow some time for the request to be processed
    rt::spin_wait_for_condition_or_timeout(
        [&] { return autobrightness_algorithm.light_history.size() == 1; },
        default_timeout);
    EXPECT_THAT(autobrightness_algorithm.light_history.size(), Eq(1));
}

TEST_F(ABacklightBrightnessControl,
       updates_brightness_from_autobrightness_algorithm_when_enabled_while_in_normal_mode)
{
    brightness_control.set_normal_brightness();
    brightness_control.enable_autobrightness();
    autobrightness_algorithm.emit_autobrightness(0.7);

    expect_brightness_value(0.7);
}

TEST_F(ABacklightBrightnessControl,
       updates_brightness_from_autobrightness_algorithm_when_set_to_normal_mode_while_enabled)
{
    brightness_control.enable_autobrightness();
    brightness_control.set_normal_brightness();
    autobrightness_algorithm.emit_autobrightness(0.7);

    expect_brightness_value(0.7);
}

TEST_F(ABacklightBrightnessControl,
       does_not_set_normal_brightness_when_set_to_normal_mode_if_autobrightness_enabled)
{
    auto const prev_history_size = backlight.brightness_history.size();

    brightness_control.enable_autobrightness();
    brightness_control.set_normal_brightness();

    EXPECT_THAT(backlight.brightness_history.size(), Eq(prev_history_size));

    brightness_control.set_off_brightness();
    brightness_control.set_normal_brightness();

    expect_brightness_value(0.0);
}

TEST_F(ABacklightBrightnessControl,
       sets_normal_brightness_if_autobrightness_enabled_when_set_from_off_to_normal_mode_with_quirk)
{
    fake_device_quirks.set_normal_before_display_on_autobrightness(true);
    repowerd::BacklightBrightnessControl quirked_brightness_control{
        rt::fake_shared(backlight), 
        rt::fake_shared(light_sensor), 
        rt::fake_shared(autobrightness_algorithm),
        rt::fake_shared(fake_chrono),
        rt::fake_shared(fake_log),
        fake_device_config,
        fake_device_quirks};

    quirked_brightness_control.enable_autobrightness();
    quirked_brightness_control.set_normal_brightness();
    quirked_brightness_control.set_off_brightness();

    quirked_brightness_control.set_normal_brightness();
    expect_brightness_value(normal_percent);
}

TEST_F(ABacklightBrightnessControl,
       ignores_brightness_from_autobrightness_algorithm_if_disabled)
{
    brightness_control.set_normal_brightness();
    autobrightness_algorithm.emit_autobrightness(0.7);

    expect_brightness_value(normal_percent);
}

TEST_F(ABacklightBrightnessControl,
       ignores_brightness_from_autobrightness_algorithm_if_not_in_normal_mode)
{
    brightness_control.enable_autobrightness();
    brightness_control.set_dim_brightness();
    autobrightness_algorithm.emit_autobrightness(0.7);

    expect_brightness_value(dim_percent);
}

TEST_F(ABacklightBrightnessControl,
       disabling_autobrightness_restores_user_brightness_if_in_normal_mode)
{
    brightness_control.set_normal_brightness();
    brightness_control.set_normal_brightness_value(0.7);

    brightness_control.enable_autobrightness();
    autobrightness_algorithm.emit_autobrightness(0.1);
    expect_brightness_value(0.1);

    brightness_control.disable_autobrightness();
    expect_brightness_value(0.7);
}

TEST_F(ABacklightBrightnessControl,
       disabling_autobrightness_leaves_brightness_unchanged_if_not_in_normal_mode)
{
    brightness_control.set_normal_brightness();

    brightness_control.enable_autobrightness();
    autobrightness_algorithm.emit_autobrightness(0.7);
    expect_brightness_value(0.7);

    brightness_control.set_dim_brightness();
    brightness_control.disable_autobrightness();

    expect_brightness_value(dim_percent);
}

TEST_F(ABacklightBrightnessControl,
       normal_brightness_value_set_by_user_not_applied_if_autobrightness_is_enabled)
{
    brightness_control.set_normal_brightness();

    brightness_control.enable_autobrightness();
    autobrightness_algorithm.emit_autobrightness(0.7);
    brightness_control.set_normal_brightness_value(0.9);

    expect_brightness_value(0.7);
}

TEST_F(ABacklightBrightnessControl,
       starts_autobrightness_algorithm_when_first_enabling_autobrightness)
{
    EXPECT_CALL(autobrightness_algorithm.mock, start()).Times(1);
    brightness_control.set_normal_brightness();
    brightness_control.enable_autobrightness();
    brightness_control.enable_autobrightness();
    Mock::VerifyAndClearExpectations(&autobrightness_algorithm.mock);
}

TEST_F(ABacklightBrightnessControl,
       stops_autobrightness_algorithm_when_first_disabling_autobrightness)
{
    EXPECT_CALL(autobrightness_algorithm.mock, start()).Times(1);
    EXPECT_CALL(autobrightness_algorithm.mock, stop()).Times(1);
    brightness_control.set_normal_brightness();
    brightness_control.enable_autobrightness();
    brightness_control.disable_autobrightness();
    brightness_control.disable_autobrightness();
    Mock::VerifyAndClearExpectations(&autobrightness_algorithm.mock);
}

TEST_F(ABacklightBrightnessControl,
       stops_autobrightness_algorithm_when_setting_off_brightness)
{
    EXPECT_CALL(autobrightness_algorithm.mock, stop()).Times(1);
    brightness_control.set_off_brightness();
    Mock::VerifyAndClearExpectations(&autobrightness_algorithm.mock);
}

TEST_F(ABacklightBrightnessControl,
       starts_autobrightness_algorithm_when_setting_normal_brightness_if_enabled)
{
    EXPECT_CALL(autobrightness_algorithm.mock, start()).Times(1);
    brightness_control.enable_autobrightness();
    brightness_control.set_normal_brightness();
    Mock::VerifyAndClearExpectations(&autobrightness_algorithm.mock);
}

TEST_F(ABacklightBrightnessControl,
       applies_last_autobrightness_if_enabled_when_setting_normal_brightness_after_dim)
{
    brightness_control.set_normal_brightness();
    brightness_control.enable_autobrightness();
    autobrightness_algorithm.emit_autobrightness(0.9);
    brightness_control.set_dim_brightness();
    expect_brightness_value(dim_percent);

    brightness_control.set_normal_brightness();

    expect_brightness_value(0.9);
}

TEST_F(ABacklightBrightnessControl, notifies_of_brightness_change)
{
    double notified_brightness{0.0};

    auto const handler_registration =
        brightness_control.register_brightness_handler(
            [&](double brightness) { notified_brightness = brightness; });

    brightness_control.set_normal_brightness();
    brightness_control.set_normal_brightness_value(0.9);

    EXPECT_THAT(notified_brightness, Eq(0.9));

    brightness_control.set_dim_brightness();

    EXPECT_THAT(notified_brightness, Eq(dim_percent));
}

TEST_F(ABacklightBrightnessControl, notifies_of_autobrightness_change)
{
    double notified_brightness{0.0};

    auto const handler_registration =
        brightness_control.register_brightness_handler(
            [&](double brightness) { notified_brightness = brightness; });

    brightness_control.set_normal_brightness();
    brightness_control.enable_autobrightness();
    autobrightness_algorithm.emit_autobrightness(0.9);

    EXPECT_THAT(notified_brightness, Eq(0.9));
}

TEST_F(ABacklightBrightnessControl, does_not_notify_if_brightness_does_not_change)
{
    double notified_brightness{-1.0};

    auto const handler_registration =
        brightness_control.register_brightness_handler(
            [&](double brightness) { notified_brightness = brightness; });

    expect_brightness_value(backlight.starting_brightness);

    brightness_control.set_normal_brightness();
    brightness_control.set_normal_brightness_value(backlight.starting_brightness);
    brightness_control.enable_autobrightness();
    autobrightness_algorithm.emit_autobrightness(backlight.starting_brightness);

    EXPECT_THAT(notified_brightness, Eq(-1.0));
}

TEST_F(ABacklightBrightnessControl, logs_brightness_transition)
{
    brightness_control.set_off_brightness();

    EXPECT_TRUE(fake_log.contains_line(
        {std::to_string(normal_percent).substr(0, 4), "0.00", "steps"}));
    EXPECT_TRUE(fake_log.contains_line(
        {std::to_string(normal_percent).substr(0, 4), "0.00", "done"}));
}

TEST_F(ABacklightBrightnessControl, does_not_log_null_brightness_transition)
{
    brightness_control.set_normal_brightness();

    EXPECT_FALSE(fake_log.contains_line({"steps"}));
    EXPECT_FALSE(fake_log.contains_line({"done"}));
}

TEST_F(ABacklightBrightnessControl, transitions_directly_to_new_value_if_current_is_unknown)
{
    backlight.set_brightness(repowerd::Backlight::unknown_brightness);
    auto const prev_history_size = backlight.brightness_history.size();

    brightness_control.set_dim_brightness();

    EXPECT_THAT(backlight.brightness_history.size(), Eq(prev_history_size + 1));
    expect_brightness_value(dim_percent);
}

TEST_F(ABacklightBrightnessControl, logs_autobrightness_values)
{
    auto const autobrightness_value = 0.08;

    brightness_control.set_normal_brightness();
    brightness_control.enable_autobrightness();
    autobrightness_algorithm.emit_autobrightness(autobrightness_value);

    EXPECT_TRUE(fake_log.contains_line(
        {"autobrightness", "value", std::to_string(autobrightness_value).substr(0, 4)}));
}
