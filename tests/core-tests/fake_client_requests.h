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

#include "src/core/client_requests.h"
#include "default_pid.h"

#include <gmock/gmock.h>

namespace repowerd
{
namespace test
{

class FakeClientRequests : public ClientRequests
{
public:
    FakeClientRequests();

    void start_processing() override;

    HandlerRegistration register_disable_inactivity_timeout_handler(
        DisableInactivityTimeoutHandler const& handler) override;
    HandlerRegistration register_enable_inactivity_timeout_handler(
        EnableInactivityTimeoutHandler const& handler) override;
    HandlerRegistration register_set_inactivity_timeout_handler(
        SetInactivityTimeoutHandler const& handler) override;

    HandlerRegistration register_disable_autobrightness_handler(
        DisableAutobrightnessHandler const& handler) override;
    HandlerRegistration register_enable_autobrightness_handler(
        EnableAutobrightnessHandler const& handler) override;
    HandlerRegistration register_set_normal_brightness_value_handler(
        SetNormalBrightnessValueHandler const& handler) override;

    HandlerRegistration register_allow_suspend_handler(
        AllowSuspendHandler const& handler) override;
    HandlerRegistration register_disallow_suspend_handler(
        DisallowSuspendHandler const& handler) override;

    void emit_disable_inactivity_timeout(std::string const& id, pid_t pid = default_pid);
    void emit_enable_inactivity_timeout(std::string const& id, pid_t pid = default_pid);
    void emit_set_inactivity_timeout(std::chrono::milliseconds timeout, pid_t pid = default_pid);
    void emit_disable_autobrightness(pid_t pid = default_pid);
    void emit_enable_autobrightness(pid_t pid = default_pid);
    void emit_set_normal_brightness_value(double f, pid_t pid = default_pid);
    void emit_allow_suspend(std::string const& id, pid_t pid = default_pid);
    void emit_disallow_suspend(std::string const& id, pid_t pid = default_pid);

    struct Mock
    {
        MOCK_METHOD0(start_processing, void());
        MOCK_METHOD1(register_disable_inactivity_timeout_handler, void(DisableInactivityTimeoutHandler const&));
        MOCK_METHOD0(unregister_disable_inactivity_timeout_handler, void());
        MOCK_METHOD1(register_enable_inactivity_timeout_handler, void(EnableInactivityTimeoutHandler const&));
        MOCK_METHOD0(unregister_enable_inactivity_timeout_handler, void());
        MOCK_METHOD1(register_set_inactivity_timeout_handler, void(SetInactivityTimeoutHandler const&));
        MOCK_METHOD0(unregister_set_inactivity_timeout_handler, void());

        MOCK_METHOD1(register_disable_autobrightness_handler, void(DisableAutobrightnessHandler const&));
        MOCK_METHOD0(unregister_disable_autobrightness_handler, void());
        MOCK_METHOD1(register_enable_autobrightness_handler, void(EnableAutobrightnessHandler const&));
        MOCK_METHOD0(unregister_enable_autobrightness_handler, void());
        MOCK_METHOD1(register_set_normal_brightness_value_handler, void(SetNormalBrightnessValueHandler const&));
        MOCK_METHOD0(unregister_set_normal_brightness_value_handler, void());
        MOCK_METHOD1(register_allow_suspend_handler, void(AllowSuspendHandler const&));
        MOCK_METHOD0(unregister_allow_suspend_handler, void());
        MOCK_METHOD1(register_disallow_suspend_handler, void(DisallowSuspendHandler const&));
        MOCK_METHOD0(unregister_disallow_suspend_handler, void());
    };
    testing::NiceMock<Mock> mock;

private:
    DisableInactivityTimeoutHandler disable_inactivity_timeout_handler;
    EnableInactivityTimeoutHandler enable_inactivity_timeout_handler;
    SetInactivityTimeoutHandler set_inactivity_timeout_handler;
    DisableAutobrightnessHandler disable_autobrightness_handler;
    EnableAutobrightnessHandler enable_autobrightness_handler;
    SetNormalBrightnessValueHandler set_normal_brightness_value_handler;
    AllowSuspendHandler allow_suspend_handler;
    DisallowSuspendHandler disallow_suspend_handler;
};

}
}
