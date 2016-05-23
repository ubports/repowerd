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

#include "console_log.h"

#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <string>

void repowerd::ConsoleLog::log(char const* tag, char const* format, ...)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    char now[32];
    auto const offset = strftime(now, sizeof(now), "%F %T", localtime(&ts.tv_sec));
    snprintf(now + offset, sizeof(now) - offset, ".%06ld", ts.tv_nsec / 1000);

    std::string format_str;
    format_str += "[";
    format_str += now;
    format_str += "] ";
    format_str += tag;
    format_str += ": ";
    format_str += format;
    format_str += "\n";

    va_list ap;
    va_start(ap, format);

    vprintf(format_str.c_str(), ap);

    va_end(ap);

    fflush(stdout);
}
