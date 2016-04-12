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

#include "event_loop_timer.h"
#include "event_loop_handler_registration.h"

namespace
{
auto const null_handler = [](auto){};
}

repowerd::EventLoopTimer::EventLoopTimer()
    : alarm_handler{null_handler},
      next_alarm_id{1}
{
}

repowerd::EventLoopTimer::~EventLoopTimer()
{
    event_loop.stop();

    for (auto const& alarm : alarms)
        alarm.second();
}

repowerd::HandlerRegistration repowerd::EventLoopTimer::register_alarm_handler(
    AlarmHandler const& handler)
{
    return EventLoopHandlerRegistration{
        event_loop,
        [this, &handler] { alarm_handler = handler; },
        [this] { alarm_handler = null_handler; }};
}

repowerd::AlarmId repowerd::EventLoopTimer::schedule_alarm_in(
    std::chrono::milliseconds t)
{
    repowerd::AlarmId alarm_id;

    {
        std::lock_guard<std::mutex> lock{alarms_mutex};
        alarm_id = next_alarm_id++;
    }

    event_loop.schedule_with_cancellation_in(
        t,
        [this, alarm_id]
        {
            alarm_handler(alarm_id);
            cancel_alarm_unqueued(alarm_id);
        },
        [this, alarm_id] (EventLoopCancellation const& cancellation)
        {
            std::lock_guard<std::mutex> lock{alarms_mutex};
            alarms[alarm_id] = cancellation;
        });

    return alarm_id;
}

void repowerd::EventLoopTimer::cancel_alarm(AlarmId id)
{
    event_loop.enqueue([this,id] { cancel_alarm_unqueued(id); }).get();
}

std::chrono::steady_clock::time_point repowerd::EventLoopTimer::now()
{
    return std::chrono::steady_clock::now();
}

void repowerd::EventLoopTimer::cancel_alarm_unqueued(AlarmId id)
{
    std::lock_guard<std::mutex> lock{alarms_mutex};

    auto const iter = alarms.find(id);
    if (iter != alarms.end())
    {
        iter->second();
        alarms.erase(iter);
    }
}
