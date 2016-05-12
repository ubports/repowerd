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

#include "src/adapters/brightness_notification.h"

#include <gmock/gmock.h>

namespace repowerd
{
namespace test
{

class FakeBrightnessNotification : public repowerd::BrightnessNotification
{
public:
    FakeBrightnessNotification();

    repowerd::HandlerRegistration register_brightness_handler(
        repowerd::BrightnessHandler const& handler) override;

    void emit_brightness(double b);

    struct MockMethods
    {
        MOCK_METHOD1(register_brightness_handler, void(repowerd::BrightnessHandler const&));
        MOCK_METHOD0(unregister_brightness_handler, void());
    };
    testing::NiceMock<MockMethods> mock;

private:
    repowerd::BrightnessHandler brightness_handler;
};

}
}
