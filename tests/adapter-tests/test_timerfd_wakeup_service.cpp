/*
 * Copyright © 2017 Canonical Ltd.
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

#include "src/adapters/timerfd_wakeup_service.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <chrono>
#include <mutex>
#include <condition_variable>

using namespace testing;
using namespace std::chrono_literals;

namespace
{

bool about_equal(std::chrono::system_clock::time_point expected,
                 std::chrono::system_clock::time_point actual)
{
    return actual >= expected && (actual - expected) <= 10ms;
}

struct ATimerfdWakeupService : Test
{
    ATimerfdWakeupService()
    {
        handler_registration = wakeup_service.register_wakeup_handler(
            [this] (std::string const& cookie)
            {
                wakeup_handler(cookie);
            });
    }

    std::chrono::system_clock::time_point system_now()
    {
        return std::chrono::system_clock::now();
    }

    void wakeup_handler(std::string const& cookie)
    {
        std::unique_lock<std::mutex> lock{wakeup_mutex};
        wakeup_time_points.push_back(system_now());
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
            [&]
            {
                return cookies == wakeup_cookies &&
                       time_points.size() == wakeup_time_points.size();
            });

        if (!result)
            throw std::runtime_error("Timeout waiting for wakeups");

        for (auto i = 0u; i < time_points.size(); ++i)
        {
            if (!about_equal(time_points[i], wakeup_time_points[i]))
            {
                throw std::runtime_error(
                    "Time point " + std::to_string(i) + " mismatch: " +
                    "expected=" +
                    std::to_string(time_points[i].time_since_epoch().count()) +
                    " actual=" +
                    std::to_string(wakeup_time_points[i].time_since_epoch().count()));
            }
        }
    }

    repowerd::TimerfdWakeupService wakeup_service;
    std::mutex wakeup_mutex;
    std::condition_variable wakeup_cv;
    std::vector<std::string> wakeup_cookies;
    std::vector<std::chrono::system_clock::time_point> wakeup_time_points;

    // Registration needs to be last, so that we unregister the wakeup handler
    // before destroying the test, to avoid accessing test variables in the
    // handler while the test is being destroyed.
    repowerd::HandlerRegistration handler_registration;
};

}

TEST_F(ATimerfdWakeupService, returns_different_cookies)
{
    auto const tp1 = system_now() + 100ms;
    auto const tp2 = system_now() + 200ms;

    auto const cookie1 = wakeup_service.schedule_wakeup_at(tp1);
    auto const cookie2 = wakeup_service.schedule_wakeup_at(tp2);

    EXPECT_THAT(cookie1, StrNe(""));
    EXPECT_THAT(cookie2, StrNe(""));
    EXPECT_THAT(cookie1, StrNe(cookie2));
}

TEST_F(ATimerfdWakeupService, schedules_wakeup)
{
    auto const tp = system_now() + 100ms;

    auto cookie = wakeup_service.schedule_wakeup_at(tp);

    wait_for_wakeups({cookie}, {tp});
}

TEST_F(ATimerfdWakeupService, cancels_wakeup)
{
    auto const tp = system_now() + 50ms;

    auto const cookie = wakeup_service.schedule_wakeup_at(tp);
    wakeup_service.cancel_wakeup(cookie);

    std::this_thread::sleep_for(100ms);

    wait_for_wakeups({}, {});
}

TEST_F(ATimerfdWakeupService, schedules_multiple_wakeups)
{
    auto const tp1 = system_now() + 50ms;
    auto const tp2 = system_now() + 100ms;
    auto const tp3 = system_now() + 150ms;

    auto const cookie1 = wakeup_service.schedule_wakeup_at(tp1);
    auto const cookie2 = wakeup_service.schedule_wakeup_at(tp2);
    auto const cookie3 = wakeup_service.schedule_wakeup_at(tp3);

    wait_for_wakeups({cookie1, cookie2, cookie3}, {tp1, tp2, tp3});
}

TEST_F(ATimerfdWakeupService, cancels_one_of_many_wakeups)
{
    auto const tp1 = system_now() + 50ms;
    auto const tp2 = system_now() + 100ms;
    auto const tp3 = system_now() + 150ms;

    auto const cookie1 = wakeup_service.schedule_wakeup_at(tp1);
    auto const cookie2 = wakeup_service.schedule_wakeup_at(tp2);
    auto const cookie3 = wakeup_service.schedule_wakeup_at(tp3);
    wakeup_service.cancel_wakeup(cookie2);

    wait_for_wakeups({cookie1, cookie3}, {tp1, tp3});
}

TEST_F(ATimerfdWakeupService, schedules_and_cancels_multiple_wakeups_with_same_time)
{
    auto const tp1 = system_now() + 50ms;
    auto const tp2 = system_now() + 50ms;
    auto const tp3 = system_now() + 50ms;

    auto const cookie1 = wakeup_service.schedule_wakeup_at(tp1);
    auto const cookie2 = wakeup_service.schedule_wakeup_at(tp2);
    auto const cookie3 = wakeup_service.schedule_wakeup_at(tp3);
    wakeup_service.cancel_wakeup(cookie2);

    wait_for_wakeups({cookie1, cookie3}, {tp1, tp3});
}
