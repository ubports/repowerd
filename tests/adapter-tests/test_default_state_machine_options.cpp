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

#include "src/adapters/default_state_machine_options.h"

#include "fake_android_properties.h"
#include "fake_log.h"

#include <gtest/gtest.h>

using namespace testing;
namespace rt = repowerd::test;

namespace
{

std::string ms_to_str(std::chrono::milliseconds ms)
{
    return std::to_string(ms.count());
}

std::string bool_to_str(bool b)
{
    return b ? "true" : "false";
}

struct ADefaultStateMachineOptions : Test
{
    void set_device_name()
    {
        fake_android_properties.set("ro.product.device", "device");
    }

    rt::FakeAndroidProperties fake_android_properties;
    rt::FakeLog fake_log;
};

}

TEST_F(ADefaultStateMachineOptions,
       treat_power_button_as_user_activity_is_false_if_device_has_a_name)
{
    set_device_name();
    repowerd::DefaultStateMachineOptions default_state_machine_options{fake_log};

    EXPECT_FALSE(default_state_machine_options.treat_power_button_as_user_activity());
}

TEST_F(ADefaultStateMachineOptions,
       treat_power_button_as_user_activity_is_true_if_device_does_not_have_a_name)
{
    repowerd::DefaultStateMachineOptions default_state_machine_options{fake_log};

    EXPECT_TRUE(default_state_machine_options.treat_power_button_as_user_activity());
}

TEST_F(ADefaultStateMachineOptions, logs_options)
{
    repowerd::DefaultStateMachineOptions options{fake_log};

    EXPECT_TRUE(fake_log.contains_line(
        {
            "notification_expiration_timeout",
            ms_to_str(options.notification_expiration_timeout())
        }));

    EXPECT_TRUE(fake_log.contains_line(
        {
            "power_button_long_press_timeout",
            ms_to_str(options.power_button_long_press_timeout())
        }));

    EXPECT_TRUE(fake_log.contains_line(
        {
            "user_inactivity_normal_display_dim_duration",
            ms_to_str(options.user_inactivity_normal_display_dim_duration())
        }));

    EXPECT_TRUE(fake_log.contains_line(
        {
            "user_inactivity_normal_display_off_timeout",
            ms_to_str(options.user_inactivity_normal_display_off_timeout())
        }));

    EXPECT_TRUE(fake_log.contains_line(
        {
            "user_inactivity_normal_suspend_timeout",
            ms_to_str(options.user_inactivity_normal_suspend_timeout())
        }));

    EXPECT_TRUE(fake_log.contains_line(
        {
            "user_inactivity_post_notification_display_off_timeout",
            ms_to_str(options.user_inactivity_post_notification_display_off_timeout())
        }));

    EXPECT_TRUE(fake_log.contains_line(
        {
            "user_inactivity_reduced_display_off_timeout",
            ms_to_str(options.user_inactivity_reduced_display_off_timeout())
        }));

    EXPECT_TRUE(fake_log.contains_line(
        {
            "treat_power_button_as_user_activity",
            bool_to_str(options.treat_power_button_as_user_activity())
        }));

    EXPECT_TRUE(fake_log.contains_line(
        {
            "turn_on_display_at_startup",
            bool_to_str(options.turn_on_display_at_startup())
        }));
}
