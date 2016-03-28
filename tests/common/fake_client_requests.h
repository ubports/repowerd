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

#include "src/client_requests.h"

#include <gmock/gmock.h>

namespace repowerd
{
namespace test
{

class FakeClientRequests : public ClientRequests
{
public:
    FakeClientRequests();

    HandlerRegistration register_disable_inactivity_timeout_handler(
        DisableInactivityTimeoutHandler const& handler) override;
    HandlerRegistration register_enable_inactivity_timeout_handler(
        EnableInactivityTimeoutHandler const& handler) override;

    HandlerRegistration register_disable_autobrightness_handler(
        DisableAutobrightnessHandler const& handler) override;
    HandlerRegistration register_enable_autobrightness_handler(
        EnableAutobrightnessHandler const& handler) override;
    HandlerRegistration register_set_normal_brightness_value_handler(
        SetNormalBrightnessValueHandler const& handler) override;

    void emit_disable_inactivity_timeout();
    void emit_enable_inactivity_timeout();
    void emit_disable_autobrightness();
    void emit_enable_autobrightness();
    void emit_set_normal_brightness_value(float f);

    struct Mock
    {
        MOCK_METHOD1(register_enable_inactivity_timeout_handler, void(EnableInactivityTimeoutHandler const&));
        MOCK_METHOD0(unregister_enable_inactivity_timeout_handler, void());
        MOCK_METHOD1(register_disable_inactivity_timeout_handler, void(EnableInactivityTimeoutHandler const&));
        MOCK_METHOD0(unregister_disable_inactivity_timeout_handler, void());

        MOCK_METHOD1(register_disable_autobrightness_handler, void(DisableAutobrightnessHandler const&));
        MOCK_METHOD0(unregister_disable_autobrightness_handler, void());
        MOCK_METHOD1(register_enable_autobrightness_handler, void(EnableAutobrightnessHandler const&));
        MOCK_METHOD0(unregister_enable_autobrightness_handler, void());
        MOCK_METHOD1(register_set_normal_brightness_value_handler, void(SetNormalBrightnessValueHandler const&));
        MOCK_METHOD0(unregister_set_normal_brightness_value_handler, void());
    };
    testing::NiceMock<Mock> mock;

private:
    DisableInactivityTimeoutHandler disable_inactivity_timeout_handler;
    EnableInactivityTimeoutHandler enable_inactivity_timeout_handler;
    DisableAutobrightnessHandler disable_autobrightness_handler;
    EnableAutobrightnessHandler enable_autobrightness_handler;
    SetNormalBrightnessValueHandler set_normal_brightness_value_handler;
};

}
}
