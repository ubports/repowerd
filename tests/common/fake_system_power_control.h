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
#include <mutex>

namespace repowerd
{
namespace test
{

class FakeSystemPowerControl : public SystemPowerControl
{
public:
    FakeSystemPowerControl();

    void start_processing() override;
    HandlerRegistration register_system_resume_handler(
        SystemResumeHandler const& systemd_resume_handler) override;

    void allow_suspend(std::string const& id, SuspendType suspend_type) override;
    void disallow_suspend(std::string const& id, SuspendType suspend_type) override;

    void power_off() override;
    void suspend() override;
    void suspend_if_allowed() override;
    void suspend_when_allowed(std::string const& id) override;
    void cancel_suspend_when_allowed(std::string const& id) override;

    void allow_default_system_handlers() override;
    void disallow_default_system_handlers() override;

    bool is_suspend_allowed(SuspendType suspend_type);
    bool are_default_system_handlers_allowed();

    void emit_system_resume();

    struct MockMethods
    {
        MOCK_METHOD0(start_processing, void());
        MOCK_METHOD1(register_system_resume_handler, void(SystemResumeHandler const&));
        MOCK_METHOD0(unregister_system_resume_handler, void());
        MOCK_METHOD2(allow_suspend, void(std::string const&, SuspendType));
        MOCK_METHOD2(disallow_suspend, void(std::string const&, SuspendType));
        MOCK_METHOD0(power_off, void());
        MOCK_METHOD0(suspend, void());
        MOCK_METHOD0(suspend_if_allowed, void());
        MOCK_METHOD1(suspend_when_allowed, void(std::string const&));
        MOCK_METHOD1(cancel_suspend_when_allowed, void(std::string const&));
        MOCK_METHOD0(allow_default_system_handlers, void());
        MOCK_METHOD0(disallow_default_system_handlers, void());
    };
    testing::NiceMock<MockMethods> mock;

private:
    std::mutex mutex;
    std::unordered_set<std::string> automatic_suspend_disallowances;
    std::unordered_set<std::string> any_suspend_disallowances;
    bool are_default_system_handlers_allowed_;
    SystemResumeHandler system_resume_handler;
};

}
}
