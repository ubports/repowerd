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

#include "temporary_file.h"

#include <cstdlib>
#include <stdexcept>
#include <unistd.h>

namespace rt = repowerd::test;

rt::TemporaryFile::TemporaryFile()
{
    char name_template[] = "/tmp/repowerd-test-XXXXXX";
    fd = mkstemp(name_template);
    if (fd == -1)
        throw std::runtime_error("Failed to create temporary file");
    filename = name_template;
}

rt::TemporaryFile::~TemporaryFile()
{
    close(fd);
    unlink(filename.c_str());
}

std::string rt::TemporaryFile::name() const
{
    return filename;
}

void rt::TemporaryFile::write(std::string data) const
{
    ::write(fd, data.c_str(), data.size());
    fsync(fd);
}
