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

#include "src/adapters/event_loop_timer.h"

#include "wait_condition.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <thread>

namespace rt = repowerd::test;

using namespace testing;
using namespace std::chrono_literals;

namespace
{

struct AnEventLoopTimer : testing::Test
{
    repowerd::EventLoopTimer timer;
    repowerd::HandlerRegistration const reg{
        timer.register_alarm_handler(
            [this](repowerd::AlarmId id) { alarm_handler(id); })};

    MOCK_METHOD1(alarm_handler, void(repowerd::AlarmId id));
};

}

TEST_F(AnEventLoopTimer, gives_different_ids_to_active_alarms)
{
    auto const id1 = timer.schedule_alarm_in(10s);
    auto const id2 = timer.schedule_alarm_in(10s);
    auto const id3 = timer.schedule_alarm_in(10s);

    EXPECT_THAT(id1, Ne(id2));
    EXPECT_THAT(id1, Ne(id3));
    EXPECT_THAT(id2, Ne(id3));
}

TEST_F(AnEventLoopTimer, notifies_when_alarm_triggers)
{
    auto const id = timer.schedule_alarm_in(100ms);

    rt::WaitCondition alarm_triggered;

    EXPECT_CALL(*this, alarm_handler(id))
        .WillOnce(WakeUp(&alarm_triggered));

    alarm_triggered.wait_for(120ms);
    EXPECT_TRUE(alarm_triggered.woken());
}

TEST_F(AnEventLoopTimer, notifies_in_order_when_multiple_alarms_trigger)
{
    auto const id1 = timer.schedule_alarm_in(100ms);
    auto const id2 = timer.schedule_alarm_in(110ms);
    auto const id3 = timer.schedule_alarm_in(120ms);
    auto const id4 = timer.schedule_alarm_in(130ms);

    rt::WaitCondition alarm_triggered;

    testing::InSequence s;
    EXPECT_CALL(*this, alarm_handler(id1));
    EXPECT_CALL(*this, alarm_handler(id2));
    EXPECT_CALL(*this, alarm_handler(id3));
    EXPECT_CALL(*this, alarm_handler(id4))
        .WillOnce(WakeUp(&alarm_triggered));

    alarm_triggered.wait_for(150ms);
    EXPECT_TRUE(alarm_triggered.woken());
}

TEST_F(AnEventLoopTimer, does_not_notify_for_alarms_not_triggered)
{
    auto const id1 = timer.schedule_alarm_in(50ms);
    auto const id2 = timer.schedule_alarm_in(100ms);
    auto const id3 = timer.schedule_alarm_in(10s);

    testing::InSequence s;
    EXPECT_CALL(*this, alarm_handler(id1));
    EXPECT_CALL(*this, alarm_handler(id2));
    EXPECT_CALL(*this, alarm_handler(id3)).Times(0);

    std::this_thread::sleep_for(150ms);
}

TEST_F(AnEventLoopTimer, reports_current_now)
{
    auto const wait = 100ms;

    auto const then = timer.now();
    std::this_thread::sleep_for(wait);
    auto const now = timer.now();

    EXPECT_THAT(now - then, Ge(wait));
    EXPECT_THAT(now - then, Le(wait + 20ms));
}

TEST_F(AnEventLoopTimer, does_not_notify_for_cancelled_alarms)
{
    auto const id1 = timer.schedule_alarm_in(50ms);
    auto const id2 = timer.schedule_alarm_in(100ms);
    auto const id3 = timer.schedule_alarm_in(150ms);
    auto const id4 = timer.schedule_alarm_in(200ms);

    testing::InSequence s;
    EXPECT_CALL(*this, alarm_handler(id1));
    EXPECT_CALL(*this, alarm_handler(id2)).Times(0);
    EXPECT_CALL(*this, alarm_handler(id3));
    EXPECT_CALL(*this, alarm_handler(id4)).Times(0);

    timer.cancel_alarm(id4);
    timer.cancel_alarm(id2);

    std::this_thread::sleep_for(250ms);
}
