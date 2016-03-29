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

#pragma once

#include "src/core/brightness_control.h"

#include <gmock/gmock.h>

namespace repowerd
{
namespace test
{

class MockBrightnessControl : public BrightnessControl
{
public:
    MOCK_METHOD0(disable_autobrightness, void());
    MOCK_METHOD0(enable_autobrightness, void());
    MOCK_METHOD0(set_dim_brightness, void());
    MOCK_METHOD0(set_normal_brightness, void());
    MOCK_METHOD1(set_normal_brightness_value, void(float));
    MOCK_METHOD0(set_off_brightness, void());
};

}
}
