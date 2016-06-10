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

#include "src/adapters/android_autobrightness_algorithm.h"
#include "src/adapters/event_loop.h"

#include "fake_device_config.h"

#include <gmock/gmock.h>

namespace rt = repowerd::test;
using namespace testing;

namespace
{

struct AnAndroidAutobrightnessAlgorithm : Test
{
    repowerd::EventLoop event_loop;
};

}

TEST_F(AnAndroidAutobrightnessAlgorithm,
       fails_to_initialize_without_autobrightness_curves)
{
    rt::FakeDeviceConfig device_config;

    repowerd::AndroidAutobrightnessAlgorithm ab_algorithm{device_config};

    EXPECT_FALSE(ab_algorithm.init(event_loop));
}

TEST_F(AnAndroidAutobrightnessAlgorithm,
       fails_to_initialize_with_autobrightness_curves_of_incorrect_size)
{
    rt::FakeDeviceConfig device_config;
    device_config.set("autoBrightnessLevels", "1,2,3");
    device_config.set("autoBrightnessLcdBacklightValues", "1,2,3");

    repowerd::AndroidAutobrightnessAlgorithm ab_algorithm{device_config};

    EXPECT_FALSE(ab_algorithm.init(event_loop));
}

TEST_F(AnAndroidAutobrightnessAlgorithm,
       initializes_with_autobrightness_curves_of_correct_size)
{
    rt::FakeDeviceConfig device_config;
    device_config.set("autoBrightnessLevels", "1,2,3");
    device_config.set("autoBrightnessLcdBacklightValues", "1,2,3,4");

    repowerd::AndroidAutobrightnessAlgorithm ab_algorithm{device_config};

    EXPECT_TRUE(ab_algorithm.init(event_loop));
}
