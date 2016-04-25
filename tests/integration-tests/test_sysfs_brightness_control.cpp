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

#include "src/adapters/sysfs_brightness_control.h"

#include "fake_device_config.h"
#include "wait_condition.h"
#include "virtual_filesystem.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <thread>
#include <algorithm>

namespace rt = repowerd::test;

using namespace testing;
using namespace std::chrono_literals;

namespace
{

class FakeSysfsBacklight
{
public:
    FakeSysfsBacklight(rt::VirtualFilesystem& vfs, int max_brightness)
    {
        vfs.add_directory("/class");
        vfs.add_directory("/class/backlight");
        vfs.add_directory("/class/backlight/acpi0");
        brightness_contents = vfs.add_file_with_live_contents(
            "/class/backlight/acpi0/brightness");
        max_brightness_contents = vfs.add_file_with_live_contents(
            "/class/backlight/acpi0/max_brightness");
        brightness_contents->push_back("0");
        max_brightness_contents->push_back(std::to_string(max_brightness));
    }

    void clear_brightness_contents_history()
    {
        auto const last = brightness_contents->back();
        brightness_contents->clear();
        brightness_contents->push_back(last);
    }

    std::vector<int> brightness_steps()
    {
        std::vector<int> steps(brightness_contents->size());
        std::transform(
            brightness_contents->begin(),
            brightness_contents->end(),
            steps.begin(),
            [] (std::string const& s) { return std::stoi(s); });
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

    std::shared_ptr<std::vector<std::string>> brightness_contents;
    std::shared_ptr<std::vector<std::string>> max_brightness_contents;
};

class FakeSysfsLedBacklight
{
public:
    FakeSysfsLedBacklight(rt::VirtualFilesystem& vfs, int max_brightness)
    {
        vfs.add_directory("/class");
        vfs.add_directory("/class/leds");
        vfs.add_directory("/class/leds/lcd-backlight");
        brightness_contents = vfs.add_file_with_live_contents(
            "/class/leds/lcd-backlight/brightness");
        max_brightness_contents = vfs.add_file_with_live_contents(
            "/class/leds/lcd-backlight/max_brightness");
        brightness_contents->push_back("0");
        max_brightness_contents->push_back("255");
        max_brightness_contents->push_back(std::to_string(max_brightness));
    }

    std::shared_ptr<std::vector<std::string>> brightness_contents;
    std::shared_ptr<std::vector<std::string>> max_brightness_contents;
};

struct ASysfsBrightnessControl : Test
{
    void set_up_sysfs_backlight()
    {
        sysfs_backlight = std::make_unique<FakeSysfsBacklight>(vfs, max_brightness);
    }

    void set_up_sysfs_led_backlight()
    {
        sysfs_led_backlight = std::make_unique<FakeSysfsLedBacklight>(vfs, max_brightness);
    }

    std::unique_ptr<repowerd::SysfsBrightnessControl> create_sysfs_brightness_control()
    {
        return std::make_unique<repowerd::SysfsBrightnessControl>(
            vfs.mount_point(), fake_device_config);
    }

    void expect_brightness_value(int brightness)
    {
        ASSERT_THAT(sysfs_backlight, NotNull());
        EXPECT_THAT(sysfs_backlight->brightness_contents->back(),
                    StrEq(std::to_string(brightness)));
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
    int const max_brightness = 255;
    std::shared_ptr<std::vector<std::string>> sysfs_backlight_live_contents;
    std::unique_ptr<FakeSysfsBacklight> sysfs_backlight;
    std::unique_ptr<FakeSysfsLedBacklight> sysfs_led_backlight;
};

MATCHER_P(IsAbout, a, "")
{
    return arg >= a && arg <= a + 50ms;
}

}

TEST_F(ASysfsBrightnessControl, fails_if_no_backlight_present)
{
    EXPECT_THROW({
        create_sysfs_brightness_control();
    }, std::runtime_error);
}

TEST_F(ASysfsBrightnessControl, uses_sysfs_backlight_if_present)
{
    set_up_sysfs_backlight();

    auto const bc = create_sysfs_brightness_control();
    bc->set_dim_brightness();

    EXPECT_THAT(sysfs_backlight->brightness_contents->size(), Gt(1));
}

TEST_F(ASysfsBrightnessControl, uses_sysfs_led_backlight_if_present)
{
    set_up_sysfs_led_backlight();

    auto const bc = create_sysfs_brightness_control();
    bc->set_dim_brightness();

    EXPECT_THAT(sysfs_led_backlight->brightness_contents->size(), Gt(1));
}

TEST_F(ASysfsBrightnessControl, prefers_sysfs_backlight_over_led_backlight_if_both_present)
{
    set_up_sysfs_backlight();
    set_up_sysfs_led_backlight();

    auto const bc = create_sysfs_brightness_control();
    bc->set_dim_brightness();

    EXPECT_THAT(sysfs_backlight->brightness_contents->size(), Gt(1));
    EXPECT_THAT(sysfs_led_backlight->brightness_contents->size(), Eq(1));
}

TEST_F(ASysfsBrightnessControl,
       writes_normal_brightness_based_on_device_config)
{
    set_up_sysfs_backlight();

    auto const normal_percent =
        static_cast<float>(fake_device_config.brightness_default_value) /
            fake_device_config.brightness_max_value;

    auto const bc = create_sysfs_brightness_control();
    bc->set_normal_brightness();

    expect_brightness_value(max_brightness * normal_percent);
}

TEST_F(ASysfsBrightnessControl, writes_zero_brightness_value_for_off_brightness)
{
    set_up_sysfs_backlight();

    auto const bc = create_sysfs_brightness_control();
    bc->set_normal_brightness();
    bc->set_off_brightness();

    expect_brightness_value(0);
}

TEST_F(ASysfsBrightnessControl,
       writes_default_dim_brightness_based_on_device_config)
{
    set_up_sysfs_backlight();

    auto const dim_percent =
        static_cast<float>(fake_device_config.brightness_dim_value) /
            fake_device_config.brightness_max_value;

    auto const bc = create_sysfs_brightness_control();
    bc->set_dim_brightness();

    expect_brightness_value(max_brightness * dim_percent);
}

TEST_F(ASysfsBrightnessControl, sets_write_normal_brightness_value_immediately_if_in_normal_mode)
{
    set_up_sysfs_backlight();

    auto const bc = create_sysfs_brightness_control();
    bc->set_normal_brightness();
    bc->set_normal_brightness_value(0.7);

    expect_brightness_value(max_brightness * 0.7);
}

TEST_F(ASysfsBrightnessControl, does_not_write_new_normal_brightness_value_if_not_in_normal_mode)
{
    set_up_sysfs_backlight();

    auto const bc = create_sysfs_brightness_control();
    bc->set_off_brightness();
    bc->set_normal_brightness_value(0.7);

    expect_brightness_value(0);
}

TEST_F(ASysfsBrightnessControl, transitions_smoothly_between_brightness_values_when_increasing)
{
    set_up_sysfs_backlight();

    auto const bc = create_sysfs_brightness_control();
    bc->set_off_brightness();
    bc->set_normal_brightness();

    EXPECT_THAT(sysfs_backlight->brightness_contents->size(), Ge(20));
    EXPECT_THAT(sysfs_backlight->brightness_steps_stddev(), Le(1));
}

TEST_F(ASysfsBrightnessControl, transitions_smoothly_between_brightness_values_when_decreasing)
{
    set_up_sysfs_backlight();

    auto const bc = create_sysfs_brightness_control();
    bc->set_off_brightness();
    bc->set_normal_brightness();
    sysfs_backlight->clear_brightness_contents_history();

    bc->set_off_brightness();

    EXPECT_THAT(sysfs_backlight->brightness_contents->size(), Ge(20));
    EXPECT_THAT(sysfs_backlight->brightness_steps_stddev(), Le(1));
}

TEST_F(ASysfsBrightnessControl,
       transitions_between_zero_and_non_zero_brightness_in_100ms)
{
    set_up_sysfs_backlight();

    auto const bc = create_sysfs_brightness_control();
    bc->set_off_brightness();

    EXPECT_THAT(duration_of([&]{bc->set_normal_brightness();}), IsAbout(100ms));
    EXPECT_THAT(duration_of([&]{bc->set_off_brightness();}), IsAbout(100ms));
    EXPECT_THAT(duration_of([&]{bc->set_dim_brightness();}), IsAbout(100ms));
}
