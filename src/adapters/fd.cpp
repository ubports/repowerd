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

#include "fd.h"

#include <unistd.h>

repowerd::Fd::Fd(int fd)
    : fd{fd},
      close_func{::close}
{
}

repowerd::Fd::Fd(int fd, FdCloseFunc const& close_func)
    : fd{fd},
      close_func{close_func}
{
}

repowerd::Fd::Fd(Fd&& other)
    : fd{other.fd},
      close_func{std::move(other.close_func)}
{
    other.fd = -1;
}

repowerd::Fd& repowerd::Fd::operator=(Fd&& other)
{
    if (&other != this)
    {
        close();
        fd = other.fd;
        close_func = std::move(other.close_func);
        other.fd = -1;
    }

    return *this;
}

repowerd::Fd::~Fd()
{
    close();
}

repowerd::Fd::operator int() const
{
    return fd;
}

void repowerd::Fd::close() const
{
    if (fd >= 0) close_func(fd);
}
