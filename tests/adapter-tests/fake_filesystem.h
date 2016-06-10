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

#pragma once

#include "src/adapters/filesystem.h"

#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <deque>

namespace repowerd
{
namespace test
{

using FakeFilesystemIoctlHandler =
    std::function<int(char const* path, int cmd, void* arg)>;

class FakeFilesystem : public Filesystem
{
public:
    ~FakeFilesystem();

    bool is_regular_file(std::string const& path) const override;
    std::unique_ptr<std::istream> istream(std::string const& path) const override;
    std::unique_ptr<std::ostream> ostream(std::string const& path) const override;
    std::vector<std::string> subdirs(std::string const& path) const override;

    Fd open(char const* pathname, int flags) const override;
    int ioctl(int fd, unsigned long request, void* args) const override;

    void add_file_with_contents(std::string const& path, std::string const& contents);
    std::shared_ptr<std::deque<std::string>> add_file_with_live_contents(
        std::string const& path);
    void add_file_ioctl(std::string const& path, FakeFilesystemIoctlHandler const& handler);

private:
    void add_dir(std::string const& path);

    mutable std::unordered_map<std::string,std::shared_ptr<std::deque<std::string>>> files;
    std::unordered_set<std::string> directories;
    mutable std::unordered_map<int,std::string> paths;
    std::unordered_map<std::string,FakeFilesystemIoctlHandler> ioctl_handlers;
};

}
}
