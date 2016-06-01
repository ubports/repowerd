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

#include "src/adapters/android_backlight.h"

#include "fake_libhardware.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace testing;

namespace
{

struct AnAndroidBacklight : Test
{
    repowerd::test::FakeLibhardware fake_libhardware;
    std::unique_ptr<repowerd::AndroidBacklight> create_backlight()
    {
        return std::make_unique<repowerd::AndroidBacklight>();
    }
};

MATCHER_P(LightState, brightness, "")
{
    return arg.flashMode == LIGHT_FLASH_NONE &&
           arg.brightnessMode == BRIGHTNESS_MODE_USER &&
           arg.color == ((0xffU << 24) | (brightness << 16) | (brightness << 8) | brightness);
}

}

TEST_F(AnAndroidBacklight, opens_and_closes_lights_module)
{
    EXPECT_THAT(fake_libhardware.is_lights_module_open(), Eq(false));
    auto backlight = create_backlight();
    EXPECT_THAT(fake_libhardware.is_lights_module_open(), Eq(true));
    backlight.reset();
    EXPECT_THAT(fake_libhardware.is_lights_module_open(), Eq(false));
}

TEST_F(AnAndroidBacklight, sets_brightness)
{
    auto const backlight = create_backlight();
    backlight->set_brightness(0.0);
    backlight->set_brightness(0.5);
    backlight->set_brightness(1.0);

    EXPECT_THAT(fake_libhardware.backlight_state_history(),
                ElementsAre(LightState(0), LightState(128), LightState(255)));
}
