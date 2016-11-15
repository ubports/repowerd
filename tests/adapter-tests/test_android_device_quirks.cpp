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

#include "src/adapters/android_device_quirks.h"

#include "fake_android_properties.h"
#include "fake_log.h"
#include "fake_filesystem.h"

#include <gtest/gtest.h>

using namespace testing;
namespace rt = repowerd::test;

namespace
{

struct AnAndroidDeviceQuirks : Test
{
    void set_device_name()
    {
        fake_android_properties.set("ro.product.device", "device");
    }

    rt::FakeAndroidProperties fake_android_properties;
    rt::FakeLog fake_log;
};

}

TEST_F(AnAndroidDeviceQuirks,
       ignore_session_deactivation_is_false_if_device_does_not_have_a_name)
{
    repowerd::AndroidDeviceQuirks android_device_quirks{fake_log};

    EXPECT_FALSE(android_device_quirks.ignore_session_deactivation());
    EXPECT_TRUE(fake_log.contains_line({"ignore_session_deactivation", "false"}));
}

TEST_F(AnAndroidDeviceQuirks,
       ignore_session_deactivation_is_true_if_device_has_a_name)
{
    set_device_name();
    repowerd::AndroidDeviceQuirks android_device_quirks{fake_log};

    EXPECT_TRUE(android_device_quirks.ignore_session_deactivation());
    EXPECT_TRUE(fake_log.contains_line({"ignore_session_deactivation", "true"}));
}
