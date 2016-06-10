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

#include "filesystem.h"

namespace repowerd
{

class RealFilesystem : public Filesystem
{
public:
    bool is_regular_file(std::string const& path) const override;
    std::unique_ptr<std::istream> istream(std::string const& path) const override;
    std::unique_ptr<std::ostream> ostream(std::string const& path) const override;
    std::vector<std::string> subdirs(std::string const& path) const override;

    Fd open(char const* pathname, int flags) const override;
    int ioctl(int fd, unsigned long request, void* args) const override;
};

}
