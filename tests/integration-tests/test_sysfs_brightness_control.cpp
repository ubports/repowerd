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

#include "wait_condition.h"
#include "virtual_filesystem.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <thread>

namespace rt = repowerd::test;

using namespace testing;
using namespace std::chrono_literals;

namespace
{

struct ASysfsBrightnessControl : Test
{
    void set_up_sysfs_backlight()
    {
        vfs.add_directory("/class");
        vfs.add_directory("/class/backlight");
        vfs.add_directory("/class/backlight/acpi0");
        vfs.add_file(
            "/class/backlight/acpi0/brightness",
            [this] (auto path, auto buf, auto size, auto offset)
            {
                return backlight_brightness.read(path, buf, size, offset);
            },
            [this] (auto path, auto buf, auto size, auto offset)
            {
                return backlight_brightness.write(path, buf, size, offset);
            });
        vfs.add_file(
            "/class/backlight/acpi0/max_brightness",
            [this] (auto path, auto buf, auto size, auto offset)
            {
                return backlight_max_brightness.read(path, buf, size, offset);
            },
            [this] (auto path, auto buf, auto size, auto offset)
            {
                return backlight_max_brightness.write(path, buf, size, offset);
            });
    }

    void set_up_sysfs_led_backlight()
    {
        vfs.add_directory("/class");
        vfs.add_directory("/class/leds");
        vfs.add_directory("/class/leds/lcd-backlight");
        vfs.add_file(
            "/class/leds/lcd-backlight/brightness",
            [this] (auto path, auto buf, auto size, auto offset)
            {
                return led_backlight_brightness.read(path, buf, size, offset);
            },
            [this] (auto path, auto buf, auto size, auto offset)
            {
                return led_backlight_brightness.write(path, buf, size, offset);
            });
        vfs.add_file(
            "/class/leds/lcd-backlight/max_brightness",
            [this] (auto path, auto buf, auto size, auto offset)
            {
                return led_backlight_max_brightness.read(path, buf, size, offset);
            },
            [this] (auto path, auto buf, auto size, auto offset)
            {
                return led_backlight_max_brightness.write(path, buf, size, offset);
            });
    }

    struct MockFileHandlers
    {
        MockFileHandlers()
        {
            ON_CALL(*this, read(_,_,_,_)).WillByDefault(ReturnArg<2>());
            ON_CALL(*this, write(_,_,_,_)).WillByDefault(ReturnArg<2>());
        }
        MOCK_METHOD4(read, int(char const* path, char* buf, size_t size, off_t offset));
        MOCK_METHOD4(write, int(char const* path, char const* buf, size_t size, off_t offset));
    };

    NiceMock<MockFileHandlers> backlight_brightness;
    NiceMock<MockFileHandlers> backlight_max_brightness;
    NiceMock<MockFileHandlers> led_backlight_brightness;
    NiceMock<MockFileHandlers> led_backlight_max_brightness;

    std::unique_ptr<repowerd::SysfsBrightnessControl> create_sysfs_brightness_control()
    {
        return std::make_unique<repowerd::SysfsBrightnessControl>(vfs.mount_point());
    }

    void set_sysfs_backlight_max_brightness(int max)
    {
        EXPECT_CALL(backlight_max_brightness, read(_, _, _, _))
            .WillOnce(Invoke(
                [max](auto, auto buf, auto size, auto) -> int
                {
                    auto const max_str = std::to_string(max);
                    if (size < max_str.size()) return 0;
                    std::copy(max_str.begin(), max_str.end(), buf);
                    return max_str.size();
                }))
            .WillRepeatedly(Return(0));
    }

    void expect_brightness_value(int value)
    {
        auto value_str = std::to_string(value);
        auto value_vec = std::vector<char>{value_str.begin(), value_str.end()};
        EXPECT_CALL(backlight_brightness, write(_, _, _, _))
            .With(Args<1,2>(ElementsAreArray(value_vec)));
    }

    rt::VirtualFilesystem vfs;
    int const max_brightness = 255;
};

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

    EXPECT_CALL(backlight_max_brightness, read(_, _, _, _));

    create_sysfs_brightness_control();
}

TEST_F(ASysfsBrightnessControl, uses_sysfs_led_backlight_if_present)
{
    set_up_sysfs_led_backlight();

    EXPECT_CALL(led_backlight_max_brightness, read(_, _, _, _));

    create_sysfs_brightness_control();
}

TEST_F(ASysfsBrightnessControl, prefers_sysfs_backlight_over_led_backlight_if_both_present)
{
    set_up_sysfs_backlight();
    set_up_sysfs_led_backlight();

    EXPECT_CALL(backlight_max_brightness, read(_, _, _, _));
    EXPECT_CALL(led_backlight_max_brightness, read(_, _, _, _)).Times(0);

    create_sysfs_brightness_control();
}

TEST_F(ASysfsBrightnessControl, writes_normal_brightness_half_of_max_on_startup)
{
    set_up_sysfs_backlight();
    set_sysfs_backlight_max_brightness(max_brightness);

    expect_brightness_value(max_brightness * 0.5);

    auto const bc = create_sysfs_brightness_control();
}

TEST_F(ASysfsBrightnessControl, writes_zero_brightness_value_for_off_brightness)
{
    set_up_sysfs_backlight();
    set_sysfs_backlight_max_brightness(max_brightness);

    auto const bc = create_sysfs_brightness_control();

    expect_brightness_value(0);

    bc->set_off_brightness();
}

TEST_F(ASysfsBrightnessControl,
       writes_one_tenth_max_brightness_value_for_dim_brightness_by_default)
{
    set_up_sysfs_backlight();
    set_sysfs_backlight_max_brightness(max_brightness);

    auto const bc = create_sysfs_brightness_control();

    expect_brightness_value(max_brightness * 0.1);

    bc->set_dim_brightness();
}

TEST_F(ASysfsBrightnessControl, sets_write_normal_brightness_value_immediately_if_in_normal_mode)
{
    set_up_sysfs_backlight();
    set_sysfs_backlight_max_brightness(max_brightness);

    auto const bc = create_sysfs_brightness_control();

    expect_brightness_value(max_brightness * 0.7);

    bc->set_normal_brightness_value(0.7);
}

TEST_F(ASysfsBrightnessControl, does_not_write_new_normal_brightness_value_if_not_in_normal_mode)
{
    set_up_sysfs_backlight();
    set_sysfs_backlight_max_brightness(max_brightness);

    auto const bc = create_sysfs_brightness_control();

    bc->set_off_brightness();

    EXPECT_CALL(backlight_brightness, write(_, _, _, _)).Times(0);

    bc->set_normal_brightness_value(0.7);
}
