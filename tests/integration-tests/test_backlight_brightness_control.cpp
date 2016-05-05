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

#include "src/adapters/backlight_brightness_control.h"
#include "src/adapters/backlight.h"

#include "fake_device_config.h"
#include "virtual_filesystem.h"
#include "fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <thread>
#include <algorithm>

namespace rt = repowerd::test;

using namespace testing;
using namespace std::chrono_literals;

namespace
{

class FakeBacklight : public repowerd::Backlight
{
public:
    void set_brightness(float v) override
    {
        brightness_history.push_back(v);
    }

    float get_brightness() override
    {
        return brightness_history.back();
    }

    void clear_brightness_history()
    {
        auto const last = brightness_history.back();
        brightness_history.clear();
        brightness_history.push_back(last);
    }

    std::vector<float> brightness_steps()
    {
        auto steps = brightness_history;
        std::adjacent_difference(steps.begin(), steps.end(), steps.begin());
        steps.erase(steps.begin());
        return steps;
    }

    float brightness_steps_stddev()
    {
        auto const steps = brightness_steps();

        auto const sum = std::accumulate(steps.begin(), steps.end(), 0.0);
        auto const mean = sum / steps.size();

        auto accum = 0.0;
        for (auto const& d : steps)
            accum += (d - mean) * (d - mean);

        return sqrt(accum / (steps.size() - 1));
    }

    std::vector<float> brightness_history{0.5f};
};

struct ABacklightBrightnessControl : Test
{
    void expect_brightness_value(float brightness)
    {
        EXPECT_THAT(backlight.brightness_history.back(), Eq(brightness));
    }

    std::chrono::milliseconds duration_of(std::function<void()> const& func)
    {
        auto start = std::chrono::steady_clock::now();
        func();
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start);
    }

    rt::VirtualFilesystem vfs;
    rt::FakeDeviceConfig fake_device_config;
    FakeBacklight backlight;
    repowerd::BacklightBrightnessControl brightness_control{
        rt::fake_shared(backlight), fake_device_config};
};

MATCHER_P(IsAbout, a, "")
{
    return arg >= a && arg <= a + 50ms;
}

}

TEST_F(ABacklightBrightnessControl,
       writes_normal_brightness_based_on_device_config)
{
    auto const normal_percent =
        static_cast<float>(fake_device_config.brightness_default_value) /
            fake_device_config.brightness_max_value;

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
    auto const dim_percent =
        static_cast<float>(fake_device_config.brightness_dim_value) /
            fake_device_config.brightness_max_value;

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

    EXPECT_THAT(duration_of([&]{brightness_control.set_normal_brightness();}), IsAbout(100ms));
    EXPECT_THAT(duration_of([&]{brightness_control.set_off_brightness();}), IsAbout(100ms));
    EXPECT_THAT(duration_of([&]{brightness_control.set_dim_brightness();}), IsAbout(100ms));
}
