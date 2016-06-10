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

#include "fake_filesystem.h"
#include "fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <chrono>
#include <mutex>
#include <condition_variable>

#include <android/linux/android_alarm.h>

using namespace testing;
using namespace std::chrono_literals;

namespace rt = repowerd::test;

namespace
{

struct FakeDevAlarm
{
    FakeDevAlarm(rt::FakeFilesystem& fake_fs)
    {
        fake_fs.add_file_ioctl(
            "/dev/alarm",
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

        while (now < next_wakeup_tp)
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

    void advance_time_by(std::chrono::system_clock::duration advance)
    {
        std::lock_guard<std::mutex> lock{next_wakeup_tp_mutex};
        now = now + advance;
    }

    std::chrono::system_clock::time_point system_now()
    {
        std::lock_guard<std::mutex> lock{next_wakeup_tp_mutex};
        return now;
    }

    std::mutex next_wakeup_tp_mutex;
    std::chrono::system_clock::time_point next_wakeup_tp;
    std::chrono::system_clock::time_point now;
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
        std::unique_lock<std::mutex> lock{wakeup_mutex};
        wakeup_time_points.push_back(fake_dev_alarm.system_now());
        wakeup_cookies.push_back(cookie);
        wakeup_cv.notify_all();
    }

    void wait_for_wakeups(
        std::vector<std::string> const& cookies,
        std::vector<std::chrono::system_clock::time_point> const& time_points)
    {
        std::unique_lock<std::mutex> lock{wakeup_mutex};
        auto const result = wakeup_cv.wait_for(
            lock,
            3s,
            [&] { return cookies == wakeup_cookies && time_points == wakeup_time_points;} );
        if (!result)
            throw std::runtime_error("Timeout waiting for wakeups");
    }

    rt::FakeFilesystem fake_fs;
    FakeDevAlarm fake_dev_alarm{fake_fs};
    repowerd::DevAlarmWakeupService wakeup_service{rt::fake_shared(fake_fs)};
    repowerd::HandlerRegistration handler_registration;
    std::mutex wakeup_mutex;
    std::condition_variable wakeup_cv;
    std::vector<std::string> wakeup_cookies;
    std::vector<std::chrono::system_clock::time_point> wakeup_time_points;
};

}

TEST_F(ADevAlarmWakeupService, returns_different_cookies)
{
    auto const tp1 = fake_dev_alarm.system_now() + 100ms;
    auto const tp2 = fake_dev_alarm.system_now() + 200ms;

    auto const cookie1 = wakeup_service.schedule_wakeup_at(tp1);
    auto const cookie2 = wakeup_service.schedule_wakeup_at(tp2);

    EXPECT_THAT(cookie1, StrNe(""));
    EXPECT_THAT(cookie2, StrNe(""));
    EXPECT_THAT(cookie1, StrNe(cookie2));
}

TEST_F(ADevAlarmWakeupService, schedules_wakeup)
{
    auto const tp = fake_dev_alarm.system_now() + 100ms;

    auto cookie = wakeup_service.schedule_wakeup_at(tp);

    fake_dev_alarm.advance_time_by(100ms);

    wait_for_wakeups({cookie}, {tp});
}

TEST_F(ADevAlarmWakeupService, cancels_wakeup)
{
    auto const tp = fake_dev_alarm.system_now() + 100ms;

    auto const cookie = wakeup_service.schedule_wakeup_at(tp);
    wakeup_service.cancel_wakeup(cookie);

    fake_dev_alarm.advance_time_by(100ms);

    wait_for_wakeups({}, {});
}

TEST_F(ADevAlarmWakeupService, schedules_multiple_wakeups)
{
    auto const tp1 = fake_dev_alarm.system_now() + 50ms;
    auto const tp2 = fake_dev_alarm.system_now() + 100ms;
    auto const tp3 = fake_dev_alarm.system_now() + 150ms;

    auto const cookie1 = wakeup_service.schedule_wakeup_at(tp1);
    auto const cookie2 = wakeup_service.schedule_wakeup_at(tp2);
    auto const cookie3 = wakeup_service.schedule_wakeup_at(tp3);

    fake_dev_alarm.advance_time_by(50ms);
    wait_for_wakeups({cookie1}, {tp1});

    fake_dev_alarm.advance_time_by(50ms);
    wait_for_wakeups({cookie1, cookie2}, {tp1, tp2});

    fake_dev_alarm.advance_time_by(50ms);
    wait_for_wakeups({cookie1, cookie2, cookie3}, {tp1, tp2, tp3});
}

TEST_F(ADevAlarmWakeupService, cancels_one_of_many_wakeups)
{
    auto const tp1 = fake_dev_alarm.system_now() + 50ms;
    auto const tp2 = fake_dev_alarm.system_now() + 100ms;
    auto const tp3 = fake_dev_alarm.system_now() + 150ms;

    auto const cookie1 = wakeup_service.schedule_wakeup_at(tp1);
    auto const cookie2 = wakeup_service.schedule_wakeup_at(tp2);
    auto const cookie3 = wakeup_service.schedule_wakeup_at(tp3);
    wakeup_service.cancel_wakeup(cookie2);

    fake_dev_alarm.advance_time_by(50ms);
    wait_for_wakeups({cookie1}, {tp1});

    fake_dev_alarm.advance_time_by(100ms);
    wait_for_wakeups({cookie1, cookie3}, {tp1, tp3});
}

TEST_F(ADevAlarmWakeupService, throws_if_cannot_open_dev_alarm_at_construction)
{
    EXPECT_THROW({
        rt::FakeFilesystem empty_fs;
        repowerd::DevAlarmWakeupService wakeup_service(rt::fake_shared(empty_fs));
    }, std::exception);
}

TEST_F(ADevAlarmWakeupService, throws_if_cannot_set_dev_alarm_at_construction)
{
    EXPECT_THROW({
        rt::FakeFilesystem local_fake_fs;
        FakeDevAlarm local_fake_dev_alarm(local_fake_fs);

        local_fake_dev_alarm.fail_on_next_alarm_set();
        repowerd::DevAlarmWakeupService wakeup_service(rt::fake_shared(local_fake_fs));
    }, std::exception);
}

TEST_F(ADevAlarmWakeupService, throws_if_cannot_set_dev_alarm)
{
    EXPECT_THROW({
        fake_dev_alarm.fail_on_next_alarm_set();
        wakeup_service.schedule_wakeup_at({});
    }, std::exception);
}
