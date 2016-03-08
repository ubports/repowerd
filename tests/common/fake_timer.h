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

#include "src/timer.h"

#include <gmock/gmock.h>

#include <vector>

namespace repowerd
{
namespace test
{

class FakeTimer : public Timer
{
public:
    FakeTimer();

    void set_alarm_handler(AlarmHandler const& handler) override;
    void clear_alarm_handler() override;
    AlarmId schedule_alarm_in(std::chrono::milliseconds t) override;

    void advance_by(std::chrono::milliseconds advance);

    struct Mock
    {
        MOCK_METHOD1(set_alarm_handler, void(AlarmHandler const&));
        MOCK_METHOD0(clear_alarm_handler, void());
    };
    testing::NiceMock<Mock> mock;

private:
    struct Alarm
    {
        AlarmId id;
        std::chrono::milliseconds time;
    };

    AlarmHandler handler;
    AlarmId next_alarm_id;
    std::chrono::milliseconds now;
    std::vector<Alarm> alarms;
};

}
}
