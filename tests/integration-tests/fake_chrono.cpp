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

#include "fake_chrono.h"

namespace rt = repowerd::test;

rt::FakeChrono::FakeChrono() : now{0}
{
}

void rt::FakeChrono::sleep_for(std::chrono::nanoseconds t)
{
    std::lock_guard<std::mutex> lock{now_mutex};
    now += t;
}

std::chrono::steady_clock::time_point rt::FakeChrono::steady_now()
{
    std::lock_guard<std::mutex> lock{now_mutex};
    using namespace std::chrono;
    return steady_clock::time_point{duration_cast<steady_clock::duration>(now)};
}
