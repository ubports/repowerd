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

#include "src/core/timer.h"

#include <gmock/gmock.h>

#include <vector>
#include <mutex>

namespace repowerd
{
namespace test
{

class FakeTimer : public Timer
{
public:
    FakeTimer();

    HandlerRegistration register_alarm_handler(AlarmHandler const& handler) override;
    AlarmId schedule_alarm_in(std::chrono::milliseconds t) override;
    void cancel_alarm(AlarmId id) override;
    std::chrono::steady_clock::time_point now() override;

    void advance_by(std::chrono::milliseconds advance);

    struct Mock
    {
        MOCK_METHOD1(register_alarm_handler, void(AlarmHandler const&));
        MOCK_METHOD0(unregister_alarm_handler, void());
    };
    testing::NiceMock<Mock> mock;

private:
    struct Alarm
    {
        AlarmId id;
        std::chrono::milliseconds time;
    };

    std::mutex mutex;

    AlarmHandler handler;
    AlarmId next_alarm_id;
    std::chrono::milliseconds now_ms;
    std::vector<Alarm> alarms;
};

}
}
