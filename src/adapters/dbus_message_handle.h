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

#include <gio/gio.h>

namespace repowerd
{

class DBusMessageHandle
{
public:
    DBusMessageHandle(GDBusMessage* message);
    DBusMessageHandle(GDBusMessage* message, GVariant* params);
    DBusMessageHandle(DBusMessageHandle&&) noexcept;
    ~DBusMessageHandle();

    operator GDBusMessage*() const;
    operator bool() const;

private:
    DBusMessageHandle(DBusMessageHandle const&) = delete;
    DBusMessageHandle& operator=(DBusMessageHandle const&) = delete;
    GDBusMessage* message;
};

}
