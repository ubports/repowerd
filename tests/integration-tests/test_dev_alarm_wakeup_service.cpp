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

#include "src/adapters/dev_alarm_wakeup_service.h"

#include "virtual_filesystem.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <mutex>

#include <android/linux/android_alarm.h>

using namespace testing;
using namespace std::chrono_literals;

namespace rt = repowerd::test;

namespace
{

struct FakeDevAlarm
{
    FakeDevAlarm()
    {
        vfs.add_file_read_write_ioctl(
            "/alarm",
            [](auto,auto,auto,auto){ return 0; },
            [](auto,auto,auto,auto){ return 0; },
            [this](auto, auto cmd, auto arg)
            {
                if (cmd == ANDROID_ALARM_WAIT)
                {
                    this->alarm_wait();
                }
                else if (cmd == ANDROID_ALARM_SET(ANDROID_ALARM_RTC_WAKEUP))
                {
                    if (fail_on_next_alarm_set_)
                    {
                        fail_on_next_alarm_set_ = false;
                        return -1;
                    }
                    this->alarm_set(static_cast<timespec*>(arg));
                }
                return 0;
            });
    }

    void fail_on_next_alarm_set()
    {
        fail_on_next_alarm_set_ = true;
    }

    void alarm_wait()
    {
        std::unique_lock<std::mutex> lock{next_wakeup_tp_mutex};

        while (std::chrono::system_clock::now() < next_wakeup_tp)
        {
            lock.unlock();
            std::this_thread::sleep_for(10ms);
            lock.lock();
        }

        next_wakeup_tp = std::chrono::system_clock::time_point::max();
    }

    void alarm_set(timespec* ts)
    {
        using namespace std::chrono;
        std::lock_guard<std::mutex> lock{next_wakeup_tp_mutex};

        auto const duration = seconds{ts->tv_sec} + nanoseconds{ts->tv_nsec};
        next_wakeup_tp = system_clock::time_point{duration_cast<system_clock::duration>(duration)};
    }

    rt::VirtualFilesystem vfs;
    std::mutex next_wakeup_tp_mutex;
    std::chrono::system_clock::time_point next_wakeup_tp;
    bool fail_on_next_alarm_set_ = false;
};

struct ADevAlarmWakeupService : Test
{
    ADevAlarmWakeupService()
    {
        handler_registration = wakeup_service.register_wakeup_handler(
            [this] (std::string const& cookie)
            {
                wakeup_handler(cookie);
            });
    }

    void wakeup_handler(std::string const& cookie)
    {
        wakeup_time_points.push_back(std::chrono::system_clock::now());
        wakeup_cookies.push_back(cookie);
    }

    FakeDevAlarm fake_dev_alarm;
    repowerd::DevAlarmWakeupService wakeup_service{fake_dev_alarm.vfs.mount_point()};
    repowerd::HandlerRegistration handler_registration;
    std::vector<std::string> wakeup_cookies;
    std::vector<std::chrono::system_clock::time_point> wakeup_time_points;
};

MATCHER_P(TimePointIsAbout, a, "")
{
    return arg >= a && arg <= a + 50ms;
}

}

TEST_F(ADevAlarmWakeupService, returns_different_cookies)
{
    auto const tp1 = std::chrono::system_clock::now() + 100ms;
    auto const tp2 = std::chrono::system_clock::now() + 200ms;

    auto const cookie1 = wakeup_service.schedule_wakeup_at(tp1);
    auto const cookie2 = wakeup_service.schedule_wakeup_at(tp2);

    EXPECT_THAT(cookie1, StrNe(""));
    EXPECT_THAT(cookie2, StrNe(""));
    EXPECT_THAT(cookie1, StrNe(cookie2));
}

TEST_F(ADevAlarmWakeupService, schedules_wakeup)
{
    auto const tp = std::chrono::system_clock::now() + 100ms;

    auto cookie = wakeup_service.schedule_wakeup_at(tp);

    std::this_thread::sleep_for(150ms);

    EXPECT_THAT(wakeup_cookies, ElementsAre(cookie));
    EXPECT_THAT(wakeup_time_points, ElementsAre(TimePointIsAbout(tp)));
}

TEST_F(ADevAlarmWakeupService, cancels_wakeup)
{
    auto const tp = std::chrono::system_clock::now() + 100ms;

    auto const cookie = wakeup_service.schedule_wakeup_at(tp);
    wakeup_service.cancel_wakeup(cookie);

    std::this_thread::sleep_for(150ms);

    EXPECT_THAT(wakeup_cookies, IsEmpty());
    EXPECT_THAT(wakeup_time_points, IsEmpty());
}

TEST_F(ADevAlarmWakeupService, schedules_multiple_wakeups)
{
    auto const tp1 = std::chrono::system_clock::now() + 50ms;
    auto const tp2 = std::chrono::system_clock::now() + 100ms;
    auto const tp3 = std::chrono::system_clock::now() + 150ms;

    auto const cookie1 = wakeup_service.schedule_wakeup_at(tp1);
    auto const cookie2 = wakeup_service.schedule_wakeup_at(tp2);
    auto const cookie3 = wakeup_service.schedule_wakeup_at(tp3);

    std::this_thread::sleep_for(200ms);

    EXPECT_THAT(wakeup_cookies, ElementsAre(cookie1, cookie2, cookie3));
    EXPECT_THAT(wakeup_time_points,
        ElementsAre(
            TimePointIsAbout(tp1),
            TimePointIsAbout(tp2),
            TimePointIsAbout(tp3)));
}

TEST_F(ADevAlarmWakeupService, cancels_one_of_many_wakeups)
{
    auto const tp1 = std::chrono::system_clock::now() + 50ms;
    auto const tp2 = std::chrono::system_clock::now() + 100ms;
    auto const tp3 = std::chrono::system_clock::now() + 150ms;

    auto const cookie1 = wakeup_service.schedule_wakeup_at(tp1);
    auto const cookie2 = wakeup_service.schedule_wakeup_at(tp2);
    auto const cookie3 = wakeup_service.schedule_wakeup_at(tp3);
    wakeup_service.cancel_wakeup(cookie2);

    std::this_thread::sleep_for(200ms);

    EXPECT_THAT(wakeup_cookies, ElementsAre(cookie1, cookie3));
    EXPECT_THAT(wakeup_time_points,
        ElementsAre(
            TimePointIsAbout(tp1),
            TimePointIsAbout(tp3)));
}

TEST_F(ADevAlarmWakeupService, throws_if_cannot_open_dev_alarm_at_construction)
{
    EXPECT_THROW({
        rt::VirtualFilesystem empty_vfs;
        repowerd::DevAlarmWakeupService wakeup_service{empty_vfs.mount_point()};
    }, std::exception);
}

TEST_F(ADevAlarmWakeupService, throws_if_cannot_set_dev_alarm_at_construction)
{
    EXPECT_THROW({
        fake_dev_alarm.fail_on_next_alarm_set();
        repowerd::DevAlarmWakeupService wakeup_service{fake_dev_alarm.vfs.mount_point()};
    }, std::exception);
}

TEST_F(ADevAlarmWakeupService, throws_if_cannot_set_dev_alarm)
{
    EXPECT_THROW({
        fake_dev_alarm.fail_on_next_alarm_set();
        wakeup_service.schedule_wakeup_at({});
    }, std::exception);
}
