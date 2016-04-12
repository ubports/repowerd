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

#include <chrono>
#include <functional>

#include "alarm_id.h"
#include "handler_registration.h"

namespace repowerd
{

using AlarmHandler = std::function<void(AlarmId)>;

class Timer
{
public:
    virtual ~Timer() = default;

    virtual HandlerRegistration register_alarm_handler(AlarmHandler const& handler) = 0;
    virtual AlarmId schedule_alarm_in(std::chrono::milliseconds t) = 0;
    virtual void cancel_alarm(AlarmId id) = 0;
    virtual std::chrono::steady_clock::time_point now() = 0;

protected:
    Timer() = default;
    Timer(Timer const&) = delete;
    Timer& operator=(Timer const&) = delete;
};

}
