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

#include "path.h"

#include <dirent.h>
#include <sys/stat.h>

repowerd::Path::Path(std::string const& path)
    : path{path}
{
}

bool repowerd::Path::is_directory() const
{
    struct stat sb;
    return stat(path.c_str(), &sb) == 0 &&
           S_ISDIR(sb.st_mode);
}

bool repowerd::Path::is_regular_file() const
{
    struct stat sb;
    return stat(path.c_str(), &sb) == 0 &&
           S_ISREG(sb.st_mode);
}

std::vector<repowerd::Path> repowerd::Path::subdirs() const
{
    auto dir = opendir(path.c_str());
    if (!dir)
        return {};

    dirent* dp;

    std::vector<Path> child_dirs;

    while ((dp = readdir(dir)) != nullptr)
    {
        std::string const entry_name{dp->d_name};
        if (entry_name == "." || entry_name == "..")
            continue;

        auto const entry = *this/entry_name;
        if (entry.is_directory())
            child_dirs.push_back(entry);
    }

    closedir(dir);

    return child_dirs;
}

repowerd::Path::operator std::string() const
{
    return path;
}

repowerd::Path repowerd::Path::operator/(std::string const& path) const
{
    return Path{this->path + "/" + path};
}
