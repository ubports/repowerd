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

#include "virtual_filesystem.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <dirent.h>

namespace rt = repowerd::test;

using namespace testing;

namespace
{

std::vector<std::string> entries_in(std::string const& parent_dir, int type)
{
    auto dir = opendir(parent_dir.c_str());
    if (!dir)
        return {};

    dirent* dp;

    std::vector<std::string> entries;

    while  ((dp = readdir(dir)) != nullptr)
    {
        if (dp->d_type == type)
        {
            entries.push_back(dp->d_name);
        }
    }

    closedir(dir);

    return entries;
}

std::vector<std::string> files_in(std::string const& parent_dir)
{
    return entries_in(parent_dir, DT_REG);
}

std::vector<std::string> directories_in(std::string const& parent_dir)
{
    return entries_in(parent_dir, DT_DIR);
}

auto const null_file_handler = [](auto, auto, auto, auto) { return 0; };

struct AVirtualFilesystem : Test
{
    rt::VirtualFilesystem vfs;
};

}

TEST_F(AVirtualFilesystem, lists_directories)
{
    vfs.add_directory("/dir");
    vfs.add_directory("/dir1");
    vfs.add_directory("/dir/subdir");
    vfs.add_directory("/dir/subdir1");

    EXPECT_THAT(
        directories_in(vfs.full_path("/")),
        UnorderedElementsAre("dir", "dir1"));

    EXPECT_THAT(
        directories_in(vfs.full_path("/dir")),
        UnorderedElementsAre("subdir", "subdir1"));
}

TEST_F(AVirtualFilesystem, lists_files)
{
    vfs.add_directory("/dir");
    vfs.add_directory("/dir/subdir");
    vfs.add_file("/dir/subdir/file", null_file_handler, null_file_handler);
    vfs.add_file("/dir/subdir/file1", null_file_handler, null_file_handler);

    EXPECT_THAT(
        files_in(vfs.full_path("/dir/subdir")),
        UnorderedElementsAre("file", "file1"));
}

TEST_F(AVirtualFilesystem, supports_file_reads)
{
    vfs.add_directory("/dir");
    vfs.add_directory("/dir/subdir");
    vfs.add_file(
        "/dir/subdir/file",
        [this] (auto path, auto buf, auto size, auto /*offset*/) -> int
        {
            if (std::string{path} != "/dir/subdir/file")
                return -1;
            for (auto i = 0u; i < size; ++i)
                *buf++ = 3 * i;
            return size;
        },
        null_file_handler);

    std::vector<char> data_read(5);

    auto fd = open(vfs.full_path("/dir/subdir/file").c_str(), O_RDWR);

    EXPECT_THAT(read(fd, data_read.data(), data_read.size()), Eq(data_read.size()));
    EXPECT_THAT(data_read, ElementsAre(0, 3, 6, 9, 12));

    close(fd);
}

TEST_F(AVirtualFilesystem, supports_file_writes)
{
    std::vector<char> file_contents;

    vfs.add_directory("/dir");
    vfs.add_directory("/dir/subdir");
    vfs.add_file(
        "/dir/subdir/file",
        null_file_handler,
        [this, &file_contents] (auto path, auto buf, auto size, auto /*offset*/) -> int
        {
            if (std::string{path} != "/dir/subdir/file")
                return -1;
            for (auto i = 0u; i < size; ++i)
                file_contents.push_back(*buf++);
            return size;
        });


    auto fd = open(vfs.full_path("/dir/subdir/file").c_str(), O_RDWR);

    std::vector<char> buf{'0', 'a', '1', 'b'};
    EXPECT_THAT(write(fd, buf.data(), buf.size()), Eq(buf.size()));
    EXPECT_THAT(file_contents, ContainerEq(buf));

    close(fd);
}
