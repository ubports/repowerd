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

#include "src/adapters/real_filesystem.h"
#include "src/adapters/fd.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <system_error>
#include <sys/stat.h>
#include <linux/fs.h>

using namespace testing;

namespace
{

struct TemporaryDirectory
{
    TemporaryDirectory()
    {
        char dir_tmpl[] = "/tmp/repowerd-test-tmpdir-XXXXXX";
        if (!mkdtemp(dir_tmpl))
            throw std::system_error{errno, std::system_category()};

        path = dir_tmpl;
    }

    ~TemporaryDirectory()
    {
        if (system((std::string{"rm -rf "} + path).c_str())) 
        {
            std::cerr << "Failed to remove temp. dir " << path << std::endl;
        }
    }

    std::string path;
};

struct ARealFilesystem : Test
{
    ARealFilesystem()
    {
        mkdir_or_throw("/dir");
        mkdir_or_throw("/dir/subdir1");
        mkdir_or_throw("/dir/subdir2");
        mkdir_or_throw("/dir1");
        create_file_or_throw("/file", "abc");
        symlink_or_throw("/dir1", "/dir/subdirlink");
        symlink_or_throw("/file", "/dir1/filelink");
    }

    void mkdir_or_throw(std::string const& path)
    {
        if (mkdir(full_path(path).c_str(), 0700) != 0)
            throw std::system_error{errno, std::system_category(), "Failed to mkdir"};
    }

    void create_file_or_throw(std::string const& path, std::string const& contents)
    {
        std::ofstream fs{full_path(path)};
        fs << contents;
        fs.flush();
    }

    void symlink_or_throw(std::string const& to, std::string const& from)
    {
        if (symlink(full_path(to).c_str(),
                    full_path(from).c_str()) != 0)
        {
            throw std::system_error{errno, std::system_category(), "Failed to symlink"};
        }
    }

    std::string full_path(std::string const& path)
    {
        return temp_dir.path + path;
    }

    std::string file_contents(std::string const& path)
    {
        std::ifstream fs{full_path(path)};
        std::string contents;
        fs >> contents;
        return contents;
    }

    TemporaryDirectory temp_dir;
    repowerd::RealFilesystem fs;
};

}

TEST_F(ARealFilesystem, recognizes_regular_file)
{
    EXPECT_TRUE(fs.is_regular_file(full_path("/file")));
}

TEST_F(ARealFilesystem, recognizes_regular_file_through_link)
{
    EXPECT_TRUE(fs.is_regular_file(full_path("/dir1/filelink")));
}

TEST_F(ARealFilesystem, does_not_recognizes_directory_as_regular_file)
{
    EXPECT_FALSE(fs.is_regular_file(full_path("/dir")));
}

TEST_F(ARealFilesystem, does_not_recognize_non_existent_path)
{
    EXPECT_FALSE(fs.is_regular_file(full_path("/nonexistent")));
}

TEST_F(ARealFilesystem, lists_subdirs)
{
    auto const subdirs = fs.subdirs(full_path("/dir"));

    EXPECT_THAT(subdirs,
                UnorderedElementsAre(
                    full_path("/dir/subdir1"),
                    full_path("/dir/subdir2"),
                    full_path("/dir/subdirlink")));
}

TEST_F(ARealFilesystem, performs_ioctl)
{
    auto const fd = fs.open(full_path("/file").c_str(), O_RDONLY);
    EXPECT_THAT(fd, Ge(0));

    int bsz{0};
    EXPECT_THAT(fs.ioctl(fd, FIGETBSZ, &bsz), Eq(0));
    EXPECT_THAT(bsz, Gt(0));
}

TEST_F(ARealFilesystem, reads_through_istream)
{
    auto const istream = fs.istream(full_path("/file"));
    std::string contents;
    *istream >> contents;

    EXPECT_THAT(contents, StrEq("abc"));
}

TEST_F(ARealFilesystem, writes_through_ostream)
{
    {
        auto const ostream = fs.ostream(full_path("/file"));
        std::string const new_contents{"123"};
        *ostream << new_contents;
    }

    EXPECT_THAT(file_contents("/file"), StrEq("123"));
}
