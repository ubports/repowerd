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

#include "real_filesystem.h"
#include "fd.h"

#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <sys/ioctl.h>
#include <sys/stat.h>

namespace
{

bool is_directory(std::string const& path)
{
    struct stat sb;
    return stat(path.c_str(), &sb) == 0 &&
           S_ISDIR(sb.st_mode);
}

}

bool repowerd::RealFilesystem::is_regular_file(
    std::string const& path) const
{
    struct stat sb;
    return stat(path.c_str(), &sb) == 0 &&
           S_ISREG(sb.st_mode);
}

std::unique_ptr<std::istream> repowerd::RealFilesystem::istream(
    std::string const& path) const
{
    return std::make_unique<std::ifstream>(path);
}

std::unique_ptr<std::ostream> repowerd::RealFilesystem::ostream(
    std::string const& path) const
{
    return std::make_unique<std::ofstream>(path);
}

std::vector<std::string> repowerd::RealFilesystem::subdirs(
    std::string const& path) const
{
    auto dir = opendir(path.c_str());
    if (!dir)
        return {};

    dirent* dp;

    std::vector<std::string> child_dirs;

    while ((dp = readdir(dir)) != nullptr)
    {
        std::string const entry_name{dp->d_name};
        if (entry_name == "." || entry_name == "..")
            continue;

        auto const entry = path + "/" + entry_name;
        if (is_directory(entry))
            child_dirs.push_back(entry);
    }

    closedir(dir);

    return child_dirs;
}

repowerd::Fd repowerd::RealFilesystem::open(
    char const* pathname, int flags) const
{
    return ::open(pathname, flags);
}

int repowerd::RealFilesystem::ioctl(
    int fd, unsigned long request, void* args) const
{
    if (args)
        return ::ioctl(fd, request, args);
    else
        return ::ioctl(fd, request);
}
