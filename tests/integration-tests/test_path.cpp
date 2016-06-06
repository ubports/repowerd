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

#include "src/adapters/path.h"

#include "virtual_filesystem.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace rt = repowerd::test;

using namespace testing;

namespace
{

struct APath : testing::Test
{
    APath()
    {
        vfs.add_directory("/dir");
        vfs.add_directory("/dir/subdir1");
        vfs.add_directory("/dir/subdir2");
        vfs.add_directory("/dir1");
        vfs.add_file_with_contents("/file", "abc");

        if (symlink(vfs.full_path("/dir1").c_str(),
                    vfs.full_path("/dir/subdirlink").c_str()) != 0)
        {
            throw std::runtime_error("Failed to create dir symlink");
        }

        if (symlink(vfs.full_path("/file").c_str(),
                    vfs.full_path("/dir1/filelink").c_str()) != 0)
        {
            throw std::runtime_error("Failed to create file symlink");
        }
    }

    rt::VirtualFilesystem vfs;
};

}

TEST_F(APath, can_be_converted_to_string)
{
    auto const path = repowerd::Path{vfs.full_path("/dir")};

    EXPECT_THAT(std::string{path}, Eq(vfs.full_path("/dir")));
}

TEST_F(APath, can_be_constructed_with_pathsep_operator)
{
    auto const path = repowerd::Path{vfs.full_path("/dir")}/"subdir1";

    EXPECT_THAT(std::string{path}, Eq(vfs.full_path("/dir/subdir1")));
}

TEST_F(APath, recognizes_regular_file)
{
    EXPECT_TRUE(repowerd::Path{vfs.full_path("/file")}.is_regular_file());
}

TEST_F(APath, does_not_recognize_regular_file_as_directory)
{
    EXPECT_FALSE(repowerd::Path{vfs.full_path("/file")}.is_directory());
}

TEST_F(APath, recognizes_regular_file_through_link)
{
    EXPECT_TRUE(repowerd::Path{vfs.full_path("/dir1/filelink")}.is_regular_file());
}

TEST_F(APath, recognizes_directory)
{
    EXPECT_TRUE(repowerd::Path{vfs.full_path("/dir")}.is_directory());
}

TEST_F(APath, does_not_recognize_directory_as_regular_file)
{
    EXPECT_FALSE(repowerd::Path{vfs.full_path("/dir")}.is_regular_file());
}

TEST_F(APath, recognizes_directory_through_link)
{
    EXPECT_TRUE(repowerd::Path{vfs.full_path("/dir/subdirlink")}.is_directory());
}

TEST_F(APath, does_not_recognize_non_existent_path)
{
    EXPECT_FALSE(repowerd::Path{vfs.full_path("/nonexistent")}.is_regular_file());
    EXPECT_FALSE(repowerd::Path{vfs.full_path("/nonexistent")}.is_directory());
}

TEST_F(APath, returns_subdirs)
{
    auto const subdirs = repowerd::Path{vfs.full_path("/dir")}.subdirs();
    std::unordered_set<std::string> subdirs_str_set;
    for (auto const& subdir : subdirs)
        subdirs_str_set.insert(std::string{subdir});

    EXPECT_THAT(subdirs_str_set,
                UnorderedElementsAre(
                    vfs.full_path("/dir/subdir1"),
                    vfs.full_path("/dir/subdir2"),
                    vfs.full_path("/dir/subdirlink")));
}
