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

#include "fake_timer.h"

#include <algorithm>

namespace rt = repowerd::test;

rt::FakeTimer::FakeTimer()
    : handler{[](AlarmId){}},
      next_alarm_id{1},
      now{0}
{
}

void rt::FakeTimer::set_alarm_handler(AlarmHandler const& handler)
{
    mock.set_alarm_handler(handler);
    this->handler = handler;
}

void rt::FakeTimer::clear_alarm_handler()
{
    mock.clear_alarm_handler();
    this->handler = [](AlarmId){};
}

repowerd::AlarmId rt::FakeTimer::schedule_alarm_in(std::chrono::milliseconds t)
{
    alarms.push_back({next_alarm_id, now + t});

    return next_alarm_id++;
}
void rt::FakeTimer::advance_by(std::chrono::milliseconds advance)
{
    now += advance;

    for (auto const& alarm : alarms)
    {
        if (now >= alarm.time)
            handler(alarm.id);
    }

    alarms.erase(
        std::remove_if(
            alarms.begin(),
            alarms.end(),
            [this](auto const& alarm) { return now >= alarm.time; }),
        alarms.end());
}
