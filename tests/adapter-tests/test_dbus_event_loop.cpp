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

#include "src/adapters/dbus_event_loop.h"

#include "current_thread_name.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace testing;

namespace rt = repowerd::test;

TEST(ADBusEventLoop, sets_thread_name)
{
    std::string const thread_name{"MyThreadName"};
    repowerd::EventLoop event_loop{thread_name};

    std::string event_loop_thread_name;
    event_loop.enqueue(
        [&] { event_loop_thread_name = rt::current_thread_name(); }).wait();

    EXPECT_THAT(event_loop_thread_name, StrEq(thread_name));
}

TEST(ADBusEventLoop, truncates_long_thread_name)
{
    std::string const long_thread_name{"MyLongLongLongThreadName"};
    repowerd::EventLoop event_loop{long_thread_name};

    std::string event_loop_thread_name;
    event_loop.enqueue(
        [&] { event_loop_thread_name = rt::current_thread_name(); }).wait();

    EXPECT_THAT(event_loop_thread_name, StrEq(long_thread_name.substr(0, 15)));
}

