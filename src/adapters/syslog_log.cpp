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

#include "syslog_log.h"

#include <cstdarg>
#include <string>

#include <syslog.h>

repowerd::SyslogLog::SyslogLog()
{
    openlog("repowerd", LOG_PID, LOG_DAEMON);
}

repowerd::SyslogLog::~SyslogLog()
{
    closelog();
}

void repowerd::SyslogLog::log(char const* tag, char const* format, ...)
{
    std::string const format_str = std::string{tag} + ": " + format;

    va_list ap;
    va_start(ap, format);

    vsyslog(LOG_DEBUG, format_str.c_str(), ap);

    va_end(ap);
}
