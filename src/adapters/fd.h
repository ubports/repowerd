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

#include <unistd.h>
#include <functional>

namespace repowerd
{

using FdCloseFunc = std::function<int(int)>;

class Fd
{
public:
    Fd(int fd);
    Fd(int fd, FdCloseFunc const& close_func);
    Fd(Fd&&);
    Fd& operator=(Fd&&);
    ~Fd();

    operator int() const;

private:
    Fd(Fd const&) = delete;
    Fd& operator=(Fd const&) = delete;
    void close() const;

    int fd;
    FdCloseFunc close_func;
};

}
