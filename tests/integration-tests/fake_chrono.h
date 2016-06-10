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

#include "src/adapters/chrono.h"
#include <mutex>

namespace repowerd
{
namespace test
{

class FakeChrono : public Chrono
{
public:
    FakeChrono();

    void sleep_for(std::chrono::nanoseconds t) override;

    std::chrono::steady_clock::time_point steady_now();

private:
    std::mutex now_mutex;
    std::chrono::nanoseconds now;
};

}
}
