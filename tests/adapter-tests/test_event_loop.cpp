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

#include "src/adapters/event_loop.h"
#include "src/adapters/fd.h"

#include "current_thread_name.h"
#include "spin_wait.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <mutex>
#include <system_error>

using namespace testing;

namespace rt = repowerd::test;

namespace
{

void wait_for_string_contents(
    std::string const& str,
    std::string const& expected,
    std::mutex& str_mutex)
{
    auto const result = rt::spin_wait_for_condition_or_timeout(
        [&]
        {
            std::lock_guard<std::mutex> lock{str_mutex};
            return str == expected;
        },
        std::chrono::seconds{3});

    if (!result)
        throw std::runtime_error("Timeout waiting for string " + expected);
}

}

TEST(AnEventLoop, sets_thread_name)
{
    std::string const thread_name{"MyThreadName"};
    repowerd::EventLoop event_loop{thread_name};

    std::string event_loop_thread_name;
    event_loop.enqueue(
        [&] { event_loop_thread_name = rt::current_thread_name(); }).wait();

    EXPECT_THAT(event_loop_thread_name, StrEq(thread_name));
}

TEST(AnEventLoop, truncates_long_thread_name)
{
    std::string const long_thread_name{"MyLongLongLongThreadName"};
    repowerd::EventLoop event_loop{long_thread_name};

    std::string event_loop_thread_name;
    event_loop.enqueue(
        [&] { event_loop_thread_name = rt::current_thread_name(); }).wait();

    EXPECT_THAT(event_loop_thread_name, StrEq(long_thread_name.substr(0, 15)));
}

TEST(AnEventLoop, watches_fd)
{
    int pipefd[2];
    if (pipe(pipefd) < 0)
        throw std::system_error{errno, std::system_category(), "Failed to create pipefd"};

    repowerd::Fd read_fd{pipefd[0]};
    repowerd::Fd write_fd{pipefd[1]};

    std::mutex data_mutex;
    std::string data;
    repowerd::EventLoop event_loop{"fd"};

    event_loop.watch_fd(
        read_fd,
        [&]
        {
            int c = 0;
            if (read(read_fd, &c, 1)) {}
            std::lock_guard<std::mutex> lock{data_mutex};
            data += c;
        });

    if (write(write_fd, "a", 1)) {}
    wait_for_string_contents(data, "a", data_mutex);

    if (write(write_fd, "b", 1)) {}
    wait_for_string_contents(data, "ab", data_mutex);
}
