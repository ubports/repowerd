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

#include "fake_system_power_control.h"

namespace rt = repowerd::test;

rt::FakeSystemPowerControl::FakeSystemPowerControl()
    : are_default_system_handlers_allowed_{true},
      system_resume_handler{[]{}}
{
}

void rt::FakeSystemPowerControl::start_processing()
{
    mock.start_processing();
}

repowerd::HandlerRegistration rt::FakeSystemPowerControl::register_system_resume_handler(
    SystemResumeHandler const& handler)
{
    mock.register_system_resume_handler(handler);
    this->system_resume_handler = handler;
    return HandlerRegistration{
        [this]
        {
            mock.unregister_system_resume_handler();

            std::lock_guard<std::mutex> lock{mutex};
            this->system_resume_handler = []{};
        }};
}

void rt::FakeSystemPowerControl::allow_suspend(
    std::string const& id, SuspendType suspend_type)
{
    mock.allow_suspend(id, suspend_type);

    std::lock_guard<std::mutex> lock{mutex};

    if (suspend_type == SuspendType::any)
        any_suspend_disallowances.erase(id);
    else
        automatic_suspend_disallowances.erase(id);
}

void rt::FakeSystemPowerControl::disallow_suspend(
    std::string const& id, SuspendType suspend_type)
{
    mock.disallow_suspend(id, suspend_type);

    std::lock_guard<std::mutex> lock{mutex};

    if (suspend_type == SuspendType::any)
        any_suspend_disallowances.insert(id);
    else
        automatic_suspend_disallowances.insert(id);
}

void rt::FakeSystemPowerControl::power_off()
{
    mock.power_off();
}

void rt::FakeSystemPowerControl::suspend_if_allowed()
{
    mock.suspend_if_allowed();
}

void rt::FakeSystemPowerControl::suspend_when_allowed(std::string const& id)
{
    mock.suspend_when_allowed(id);
}

void rt::FakeSystemPowerControl::cancel_suspend_when_allowed(std::string const& id)
{
    mock.cancel_suspend_when_allowed(id);
}

void rt::FakeSystemPowerControl::allow_default_system_handlers()
{
    mock.allow_default_system_handlers();

    std::lock_guard<std::mutex> lock{mutex};

    are_default_system_handlers_allowed_ = true;
}

void rt::FakeSystemPowerControl::disallow_default_system_handlers()
{
    mock.disallow_default_system_handlers();

    std::lock_guard<std::mutex> lock{mutex};

    are_default_system_handlers_allowed_ = false;
}

bool rt::FakeSystemPowerControl::is_suspend_allowed(SuspendType suspend_type)
{
    std::lock_guard<std::mutex> lock{mutex};

    if (suspend_type == SuspendType::any)
        return any_suspend_disallowances.empty();
    else
        return automatic_suspend_disallowances.empty();
}

bool rt::FakeSystemPowerControl::are_default_system_handlers_allowed()
{
    std::lock_guard<std::mutex> lock{mutex};

    return are_default_system_handlers_allowed_;
}

void rt::FakeSystemPowerControl::emit_system_resume()
{
    SystemResumeHandler handler;

    {
        std::lock_guard<std::mutex> lock{mutex};
        handler = system_resume_handler;
    }

    handler();
}
