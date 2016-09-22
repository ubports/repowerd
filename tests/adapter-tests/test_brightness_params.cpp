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

#include "src/adapters/brightness_params.h"

#include "fake_device_config.h"

#include <gmock/gmock.h>

namespace rt = repowerd::test;
using namespace testing;

namespace
{

struct ABrightnessParams : Test
{
    rt::FakeDeviceConfig device_config;
};

}

TEST_F(ABrightnessParams, uses_values_from_device_config)
{
    auto const brightness_params = repowerd::BrightnessParams::from_device_config(device_config);

    EXPECT_THAT(brightness_params.dim_value, Eq(device_config.brightness_dim_value));
    EXPECT_THAT(brightness_params.min_value, Eq(device_config.brightness_min_value));
    EXPECT_THAT(brightness_params.max_value, Eq(device_config.brightness_max_value));
    EXPECT_THAT(brightness_params.default_value, Eq(device_config.brightness_default_value));
    EXPECT_THAT(brightness_params.autobrightness_supported,
                Eq(device_config.brightness_autobrightness_supported));
}

TEST_F(ABrightnessParams, falls_back_to_sensible_values_if_config_entries_are_missing)
{
    device_config.clear();
    auto const brightness_params = repowerd::BrightnessParams::from_device_config(device_config);

    EXPECT_THAT(brightness_params.dim_value, Eq(10));
    EXPECT_THAT(brightness_params.min_value, Eq(10));
    EXPECT_THAT(brightness_params.max_value, Eq(255));
    EXPECT_THAT(brightness_params.default_value, Eq(102));
    EXPECT_THAT(brightness_params.autobrightness_supported, Eq(false));
}
