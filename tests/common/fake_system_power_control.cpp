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

void rt::FakeSystemPowerControl::allow_automatic_suspend(std::string const& id)
{
    mock.allow_automatic_suspend(id);

    std::lock_guard<std::mutex> lock{mutex};

    automatic_suspend_disallowances.erase(id);
}

void rt::FakeSystemPowerControl::disallow_automatic_suspend(std::string const& id)
{
    mock.disallow_automatic_suspend(id);

    std::lock_guard<std::mutex> lock{mutex};

    automatic_suspend_disallowances.insert(id);
}

void rt::FakeSystemPowerControl::power_off()
{
    mock.power_off();
}

void rt::FakeSystemPowerControl::suspend()
{
    mock.suspend();
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

bool rt::FakeSystemPowerControl::is_automatic_suspend_allowed()
{
    std::lock_guard<std::mutex> lock{mutex};

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
