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

#include <gmock/gmock.h>

namespace rt = repowerd::test;

using namespace std::chrono_literals;

namespace
{

struct AFakeTimer : testing::Test
{
    rt::FakeTimer fake_timer;
    repowerd::HandlerRegistration const reg{
        fake_timer.register_alarm_handler(
            [this](repowerd::AlarmId id) { alarm_handler(id); })};

    MOCK_METHOD1(alarm_handler, void(repowerd::AlarmId id));
};

}

TEST_F(AFakeTimer, gives_different_ids_to_active_alarms)
{
    using namespace testing;

    auto const id1 = fake_timer.schedule_alarm_in(10s);
    auto const id2 = fake_timer.schedule_alarm_in(10s);
    auto const id3 = fake_timer.schedule_alarm_in(10s);

    EXPECT_THAT(id1, Ne(id2));
    EXPECT_THAT(id1, Ne(id3));
    EXPECT_THAT(id2, Ne(id3));
}

TEST_F(AFakeTimer, notifies_when_alarm_triggers)
{
    auto const id = fake_timer.schedule_alarm_in(10s);

    EXPECT_CALL(*this, alarm_handler(id));

    fake_timer.advance_by(10s);
}

TEST_F(AFakeTimer, notifies_in_order_when_multiple_alarms_trigger)
{
    auto const id1 = fake_timer.schedule_alarm_in(10s);
    auto const id2 = fake_timer.schedule_alarm_in(10s);
    auto const id3 = fake_timer.schedule_alarm_in(20s);
    auto const id4 = fake_timer.schedule_alarm_in(30s);

    testing::InSequence s;
    EXPECT_CALL(*this, alarm_handler(id1));
    EXPECT_CALL(*this, alarm_handler(id2));
    EXPECT_CALL(*this, alarm_handler(id3));
    EXPECT_CALL(*this, alarm_handler(id4));

    fake_timer.advance_by(30s);
}

TEST_F(AFakeTimer, does_not_notify_for_alarms_not_triggered)
{
    auto const id1 = fake_timer.schedule_alarm_in(10s);
    auto const id2 = fake_timer.schedule_alarm_in(20s);
    auto const id3 = fake_timer.schedule_alarm_in(30s);

    testing::InSequence s;
    EXPECT_CALL(*this, alarm_handler(id1));
    EXPECT_CALL(*this, alarm_handler(id2));
    EXPECT_CALL(*this, alarm_handler(id3)).Times(0);

    fake_timer.advance_by(29s);
}

TEST_F(AFakeTimer, notifies_for_alarms_added_after_advancing)
{
    auto const id1 = fake_timer.schedule_alarm_in(10s);

    EXPECT_CALL(*this, alarm_handler(id1));

    fake_timer.advance_by(30s);

    auto const id2 = fake_timer.schedule_alarm_in(5s);
    auto const id3 = fake_timer.schedule_alarm_in(6s);

    testing::InSequence s;
    EXPECT_CALL(*this, alarm_handler(id2));
    EXPECT_CALL(*this, alarm_handler(id3));

    fake_timer.advance_by(10s);
}

TEST_F(AFakeTimer, updates_now_when_advanced)
{
    auto const advance = 10ms;
    auto const then = fake_timer.now();
    fake_timer.advance_by(advance);
    auto const now = fake_timer.now();
    EXPECT_THAT(now - then, testing::Eq(advance));
}
