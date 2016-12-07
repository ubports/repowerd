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

#include "fake_suspend_control.h"

namespace rt = repowerd::test;

void rt::FakeSuspendControl::allow_suspend(std::string const& id)
{
    mock.allow_suspend(id);

    std::lock_guard<std::mutex> lock{mutex};

    suspend_disallowances.erase(id);
}

void rt::FakeSuspendControl::disallow_suspend(std::string const& id)
{
    mock.disallow_suspend(id);

    std::lock_guard<std::mutex> lock{mutex};

    suspend_disallowances.insert(id);
}

bool rt::FakeSuspendControl::is_suspend_allowed()
{
    std::lock_guard<std::mutex> lock{mutex};
    return suspend_disallowances.empty();
}
