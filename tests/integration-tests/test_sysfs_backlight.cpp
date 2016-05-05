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

#include "src/adapters/sysfs_backlight.h"

#include "virtual_filesystem.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace rt = repowerd::test;

using namespace testing;

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

struct ASysfsBacklight : Test
{
    void set_up_sysfs_backlight()
    {
        sysfs_backlight = std::make_unique<FakeSysfsBacklight>(vfs, max_brightness);
    }

    void set_up_sysfs_led_backlight()
    {
        sysfs_led_backlight = std::make_unique<FakeSysfsLedBacklight>(vfs, max_brightness);
    }

    std::unique_ptr<repowerd::SysfsBacklight> create_sysfs_backlight()
    {
        return std::make_unique<repowerd::SysfsBacklight>(
            vfs.mount_point(), fake_device_config);
    }

    void expect_brightness_value(int brightness)
    {
        ASSERT_THAT(sysfs_backlight, NotNull());
        EXPECT_THAT(sysfs_backlight->brightness_contents->back(),
                    StrEq(std::to_string(brightness)));
    }

    rt::VirtualFilesystem vfs;
    int const max_brightness = 255;
    std::shared_ptr<std::vector<std::string>> sysfs_backlight_live_contents;
    std::unique_ptr<FakeSysfsBacklight> sysfs_backlight;
    std::unique_ptr<FakeSysfsLedBacklight> sysfs_led_backlight;
};

}

TEST_F(ASysfsBacklight, fails_if_no_backlight_present)
{
    EXPECT_THROW({
        create_sysfs_backlight();
    }, std::runtime_error);
}

TEST_F(ASysfsBacklight, uses_sysfs_backlight_if_present)
{
    set_up_sysfs_backlight();

    auto const backlight = create_sysfs_backlight();
    backlight->set_brightness(0.5f);

    EXPECT_THAT(sysfs_backlight->brightness_contents->size(), Gt(1));
}

TEST_F(ASysfsBacklight, uses_sysfs_led_backlight_if_present)
{
    set_up_sysfs_led_backlight();

    auto const backlight = create_sysfs_backlight();
    backlight->set_brightness(0.5f);

    EXPECT_THAT(sysfs_led_backlight->brightness_contents->size(), Gt(1));
}

TEST_F(ASysfsBacklight, prefers_sysfs_backlight_over_led_backlight_if_both_present)
{
    set_up_sysfs_backlight();
    set_up_sysfs_led_backlight();

    auto const backlight = create_sysfs_backlight();
    backlight->set_brightness(0.5f);

    EXPECT_THAT(sysfs_backlight->brightness_contents->size(), Gt(1));
    EXPECT_THAT(sysfs_led_backlight->brightness_contents->size(), Eq(1));
}

TEST_F(ASysfsBacklight, writes_brightness_value_based_on_max_brightness)
{
    set_up_sysfs_backlight();

    auto const normalized_brightness = 0.7f;

    auto const backlight = create_sysfs_backlight();
    backlight->set_brightness(normalized_brightness);

    expect_brightness_value(max_brightness * normalized_brightness);
}

TEST_F(ASysfsBacklight, writes_zero_brightness_value_for_zero_brightness)
{
    set_up_sysfs_backlight();

    auto const normalized_brightness = 0.0f;

    auto const backlight = create_sysfs_backlight();
    backlight->set_brightness(normalized_brightness);

    expect_brightness_value(0);
}
