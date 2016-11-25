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

#include "src/adapters/fd.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <unordered_map>
#include <stdexcept>
#include <system_error>

#include <unistd.h>
#include <fcntl.h>

using namespace testing;

namespace
{

struct AFd : testing::Test
{
    ~AFd()
    {
        for (auto const& kv : pipes)
        {
            close(kv.second);
        }
    }

    int open_fd()
    {
        int pipefd[2];

        if (pipe2(pipefd, O_NONBLOCK) == -1)
        {
            throw std::system_error{
                errno, std::system_category(), "Failed to create pipe"};
        }

        pipes[pipefd[1]] = pipefd[0];

        return pipefd[1];
    }

    bool is_fd_closed(int fd)
    {
        auto const iter = pipes.find(fd);
        if (iter == pipes.end())
            throw std::runtime_error{"Untracked fd"};

        char c{0};
        return read(iter->second, &c, 1) == 0;
    }

    std::unordered_map<int,int> pipes;
};

}

TEST_F(AFd, destruction_closes_file)
{
    int const fd = open_fd();

    {
        repowerd::Fd rfd{fd};
        EXPECT_FALSE(is_fd_closed(fd));
    }

    EXPECT_TRUE(is_fd_closed(fd));
}

TEST_F(AFd, does_not_close_invalid_file_on_destruction)
{
    repowerd::Fd rfd{-1};
}

TEST_F(AFd, move_construction_moves_fd)
{
    int const fd = open_fd();

    {
        repowerd::Fd rfd1{fd};
        {
            repowerd::Fd rfd2{std::move(rfd1)};
            EXPECT_FALSE(is_fd_closed(fd));
        }
        EXPECT_TRUE(is_fd_closed(fd));
    }
}

TEST_F(AFd, move_assignment_moves_fd)
{
    int const fd = open_fd();

    {
        repowerd::Fd rfd1{fd};
        {
            repowerd::Fd rfd2{-1};
            rfd2 = std::move(rfd1);
            EXPECT_FALSE(is_fd_closed(fd));
        }
        EXPECT_TRUE(is_fd_closed(fd));
    }
}

TEST_F(AFd, move_assignment_closes_target_fd)
{
    int const fd1 = open_fd();
    int const fd2 = open_fd();

    repowerd::Fd rfd1{fd1};
    repowerd::Fd rfd2{fd2};

    rfd2 = std::move(rfd1);
    EXPECT_FALSE(is_fd_closed(fd1));
    EXPECT_TRUE(is_fd_closed(fd2));

    rfd2 = repowerd::Fd{-1};
    EXPECT_TRUE(is_fd_closed(fd1));
    EXPECT_TRUE(is_fd_closed(fd2));
}
