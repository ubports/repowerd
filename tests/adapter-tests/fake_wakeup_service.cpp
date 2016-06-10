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

#include "fake_wakeup_service.h"

namespace rt = repowerd::test;

rt::FakeWakeupService::FakeWakeupService()
    : wakeup_handler{[](std::string const&){}}
{
}

std::string rt::FakeWakeupService::schedule_wakeup_at(
    std::chrono::system_clock::time_point tp)
{
    wakeups.push_back(tp);
    return std::to_string(wakeups.size() - 1);
}

void rt::FakeWakeupService::cancel_wakeup(std::string const& cookie)
{
    int const cookie_int = std::stoi(cookie);
    wakeups.at(cookie_int) = {};
}

std::chrono::system_clock::time_point rt::FakeWakeupService::emit_next_wakeup()
{
    std::chrono::system_clock::time_point next{};

    for (auto i = 0u; i < wakeups.size(); ++i)
    {
        if (wakeups[i] != std::chrono::system_clock::time_point{})
        {
            next = wakeups[i];
            wakeups[i] = {};
            wakeup_handler(std::to_string(i));
        }
    }

    return next;
}

repowerd::HandlerRegistration rt::FakeWakeupService::register_wakeup_handler(
    repowerd::WakeupHandler const& handler)
{
    mock.register_wakeup_handler(handler);
    this->wakeup_handler = handler;
    return repowerd::HandlerRegistration{
        [this]
        {
            mock.unregister_wakeup_handler();
            this->wakeup_handler = [](std::string const&){};
        }};
}
