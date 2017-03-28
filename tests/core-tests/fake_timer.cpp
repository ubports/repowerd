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
      now_ms{0}
{
}

repowerd::HandlerRegistration rt::FakeTimer::register_alarm_handler(AlarmHandler const& handler)
{
    mock.register_alarm_handler(handler);

    std::lock_guard<std::mutex> lock{mutex};
    this->handler = handler;

    return HandlerRegistration{
        [this]
        {
            mock.unregister_alarm_handler();

            std::lock_guard<std::mutex> lock{mutex};
            this->handler = [](AlarmId){};
        }};
}

repowerd::AlarmId rt::FakeTimer::schedule_alarm_in(std::chrono::milliseconds t)
{
    std::lock_guard<std::mutex> lock{mutex};

    alarms.push_back({next_alarm_id, now_ms + t});

    return next_alarm_id++;
}

void rt::FakeTimer::cancel_alarm(AlarmId id)
{
    std::lock_guard<std::mutex> lock{mutex};

    alarms.erase(
        std::remove_if(
            alarms.begin(),
            alarms.end(),
            [id](auto const& alarm) { return alarm.id == id; }),
        alarms.end());
}

std::chrono::steady_clock::time_point rt::FakeTimer::now()
{
    std::lock_guard<std::mutex> lock{mutex};
    return std::chrono::steady_clock::time_point{now_ms};
}

void rt::FakeTimer::advance_by(std::chrono::milliseconds advance)
{
    // It's not ideal to call the handlers under lock, but it's good/safe
    // enough for our testing scenarios and purposes. The only timer
    // handler we use in production code (in repowerd::Daemon) enqueues
    // work to other threads, so it can't deadlock.
    std::lock_guard<std::mutex> lock{mutex};

    now_ms += advance;

    for (auto const& alarm : alarms)
    {
        if (now_ms >= alarm.time)
            handler(alarm.id);
    }

    alarms.erase(
        std::remove_if(
            alarms.begin(),
            alarms.end(),
            [this](auto const& alarm) { return now_ms >= alarm.time; }),
        alarms.end());
}
