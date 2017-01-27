/*
 * Copyright © 2017 Canonical Ltd.
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

#include "src/core/client_settings.h"
#include "default_pid.h"

#include <gmock/gmock.h>

namespace repowerd
{
namespace test
{

class FakeClientSettings : public ClientSettings
{
public:
    FakeClientSettings();

    void start_processing() override;

    HandlerRegistration register_set_inactivity_behavior_handler(
        SetInactivityBehaviorHandler const& handler) override;

    void emit_set_inactivity_behavior(
        PowerAction power_action,
        PowerSupply power_supply,
        std::chrono::milliseconds timeout,
        pid_t pid = default_pid);

    struct Mock
    {
        MOCK_METHOD0(start_processing, void());
        MOCK_METHOD1(register_set_inactivity_behavior_handler,
                     void(SetInactivityBehaviorHandler const&));
        MOCK_METHOD0(unregister_set_inactivity_behavior_handler, void());
    };
    testing::NiceMock<Mock> mock;

private:
    SetInactivityBehaviorHandler set_inactivity_behavior_handler;
};

}
}
