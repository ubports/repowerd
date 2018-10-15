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
#include "src/adapters/path.h"

#include "fake_filesystem.h"
#include "fake_log.h"
#include "fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cmath>
#include <unistd.h>

namespace rt = repowerd::test;

using namespace testing;

namespace
{

class FakeSysfsBacklight
{
public:
    FakeSysfsBacklight(
        rt::FakeFilesystem& fake_fs,
        std::string const& type,
        int max_brightness)
        : path{repowerd::Path{"/sys/class/backlight"}/type}
    {
        fake_fs.add_file_with_contents(path/"type" , type);
        brightness_contents = fake_fs.add_file_with_live_contents(
            path/"brightness");
        fake_fs.add_file_with_contents(
            path/"max_brightness",
            std::to_string(max_brightness));
        brightness_contents->push_back("0");
    }

    repowerd::Path const path;
    std::shared_ptr<std::deque<std::string>> brightness_contents;
};

class FakeSysfsLedBacklight
{
public:
    FakeSysfsLedBacklight(rt::FakeFilesystem& fake_fs, int max_brightness)
    {
        brightness_contents = fake_fs.add_file_with_live_contents(
            "/sys/class/leds/lcd-backlight/brightness");
        fake_fs.add_file_with_contents(
            "/sys/class/leds/lcd-backlight/max_brightness",
            std::to_string(max_brightness));

        brightness_contents->push_back("0");
    }

    std::shared_ptr<std::deque<std::string>> brightness_contents;
};

struct ASysfsBacklight : Test
{
    void set_up_sysfs_backlight()
    {
        sysfs_backlight = std::make_unique<FakeSysfsBacklight>(fake_fs, "firmware", max_brightness);
    }

    std::unique_ptr<FakeSysfsBacklight> set_up_sysfs_backlight_with_type(std::string const& type)
    {
        return std::make_unique<FakeSysfsBacklight>(fake_fs, type, max_brightness);
    }

    void set_up_sysfs_led_backlight()
    {
        sysfs_led_backlight = std::make_unique<FakeSysfsLedBacklight>(fake_fs, max_brightness);
    }

    std::unique_ptr<repowerd::SysfsBacklight> create_sysfs_backlight()
    {
        return std::make_unique<repowerd::SysfsBacklight>(
            rt::fake_shared(fake_log), rt::fake_shared(fake_fs));
    }

    void expect_brightness_value(int brightness)
    {
        ASSERT_THAT(sysfs_backlight, NotNull());
        EXPECT_THAT(sysfs_backlight->brightness_contents->back(),
                    StrEq(std::to_string(brightness)));
    }

    rt::FakeLog fake_log;
    rt::FakeFilesystem fake_fs;
    int const max_brightness = 255;
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
    backlight->set_brightness(0.5);

    EXPECT_THAT(sysfs_backlight->brightness_contents->size(), Gt(1u));
}

TEST_F(ASysfsBacklight, uses_sysfs_led_backlight_if_present)
{
    set_up_sysfs_led_backlight();

    auto const backlight = create_sysfs_backlight();
    backlight->set_brightness(0.5);

    EXPECT_THAT(sysfs_led_backlight->brightness_contents->size(), Gt(1u));
}

TEST_F(ASysfsBacklight, prefers_sysfs_backlight_over_led_backlight_if_both_present)
{
    set_up_sysfs_backlight();
    set_up_sysfs_led_backlight();

    auto const backlight = create_sysfs_backlight();
    backlight->set_brightness(0.5);

    EXPECT_THAT(sysfs_backlight->brightness_contents->size(), Gt(1u));
    EXPECT_THAT(sysfs_led_backlight->brightness_contents->size(), Eq(1u));
}

TEST_F(ASysfsBacklight, writes_brightness_value_based_on_max_brightness)
{
    set_up_sysfs_backlight();

    auto const normalized_brightness = 0.7;

    auto const backlight = create_sysfs_backlight();
    backlight->set_brightness(normalized_brightness);

    expect_brightness_value(round(max_brightness * normalized_brightness));
}

TEST_F(ASysfsBacklight, writes_zero_brightness_value_for_zero_brightness)
{
    set_up_sysfs_backlight();

    auto const normalized_brightness = 0.0;

    auto const backlight = create_sysfs_backlight();
    backlight->set_brightness(normalized_brightness);

    expect_brightness_value(0);
}

TEST_F(ASysfsBacklight, gets_last_set_brightness)
{
    set_up_sysfs_backlight();

    auto const normalized_brightness = 0.7123;

    auto const backlight = create_sysfs_backlight();
    backlight->set_brightness(normalized_brightness);

    EXPECT_THAT(backlight->get_brightness(), Eq(normalized_brightness));
}

TEST_F(ASysfsBacklight, gets_real_brightness_if_brightness_changed_externally)
{
    set_up_sysfs_backlight();

    auto const backlight = create_sysfs_backlight();
    backlight->set_brightness(0.7);

    sysfs_backlight->brightness_contents->push_back("102");
    EXPECT_THAT(backlight->get_brightness(), Eq(102.0/max_brightness));
}

TEST_F(ASysfsBacklight, logs_used_sysfs_backlight_dir)
{
    set_up_sysfs_backlight();

    auto const backlight = create_sysfs_backlight();

    EXPECT_TRUE(fake_log.contains_line({sysfs_backlight->path}));
}

TEST_F(ASysfsBacklight, prefers_firmware_backlight_over_all_others)
{
    auto const firmware = set_up_sysfs_backlight_with_type("firmware");
    auto const platform = set_up_sysfs_backlight_with_type("platform");
    auto const raw = set_up_sysfs_backlight_with_type("raw");

    auto const backlight = create_sysfs_backlight();
    backlight->set_brightness(0.5);

    EXPECT_THAT(firmware->brightness_contents->size(), Gt(1u));
}

TEST_F(ASysfsBacklight, prefers_platform_backlight_over_raw)
{
    auto const platform = set_up_sysfs_backlight_with_type("platform");
    auto const raw = set_up_sysfs_backlight_with_type("raw");

    auto const backlight = create_sysfs_backlight();
    backlight->set_brightness(0.5);

    EXPECT_THAT(platform->brightness_contents->size(), Gt(1u));
}

TEST_F(ASysfsBacklight, uses_raw_backlight_if_no_other_choice)
{
    auto const raw = set_up_sysfs_backlight_with_type("raw");

    auto const backlight = create_sysfs_backlight();
    backlight->set_brightness(0.5);

    EXPECT_THAT(raw->brightness_contents->size(), Gt(1u));
}
