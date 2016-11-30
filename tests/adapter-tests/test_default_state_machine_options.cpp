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

#include <gtest/gtest.h>

using namespace testing;
namespace rt = repowerd::test;

namespace
{

struct ADefaultStateMachineOptions : Test
{
    void set_device_name()
    {
        fake_android_properties.set("ro.product.device", "device");
    }

    rt::FakeAndroidProperties fake_android_properties;
};

}

TEST_F(ADefaultStateMachineOptions,
       treat_power_button_as_user_activity_is_false_if_device_has_a_name)
{
    set_device_name();
    repowerd::DefaultStateMachineOptions default_state_machine_options;

    EXPECT_FALSE(default_state_machine_options.treat_power_button_as_user_activity());
}

TEST_F(ADefaultStateMachineOptions,
       treat_power_button_as_user_activity_is_true_if_device_does_not_have_a_name)
{
    repowerd::DefaultStateMachineOptions default_state_machine_options;

    EXPECT_TRUE(default_state_machine_options.treat_power_button_as_user_activity());
}
