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

#include "src/core/system_power_control.h"

#include <gmock/gmock.h>

#include <unordered_set>

namespace repowerd
{
namespace test
{

class FakeSystemPowerControl : public SystemPowerControl
{
public:
    FakeSystemPowerControl();

    void allow_suspend(std::string const& id, SuspendType suspend_type) override;
    void disallow_suspend(std::string const& id, SuspendType suspend_type) override;

    void power_off() override;
    void suspend_if_allowed() override;

    void allow_default_system_handlers() override;
    void disallow_default_system_handlers() override;

    bool is_suspend_allowed(SuspendType suspend_type);
    bool are_default_system_handlers_allowed();

    struct MockMethods
    {
        MOCK_METHOD2(allow_suspend, void(std::string const&, SuspendType));
        MOCK_METHOD2(disallow_suspend, void(std::string const&, SuspendType));
        MOCK_METHOD0(power_off, void());
        MOCK_METHOD0(suspend_if_allowed, void());
        MOCK_METHOD0(allow_default_system_handlers, void());
        MOCK_METHOD0(disallow_default_system_handlers, void());
    };
    testing::NiceMock<MockMethods> mock;

private:
    std::unordered_set<std::string> automatic_suspend_disallowances;
    std::unordered_set<std::string> any_suspend_disallowances;
    bool are_default_system_handlers_allowed_;
};

}
}
