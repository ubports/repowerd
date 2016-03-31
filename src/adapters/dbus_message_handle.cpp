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

#include "dbus_message_handle.h"

#include <stdexcept>

repowerd::DBusMessageHandle::DBusMessageHandle(GDBusMessage* message)
    : message{message}
{
}

repowerd::DBusMessageHandle::DBusMessageHandle(GDBusMessage* message, GVariant* params)
    : message{message}
{
    if (!message)
        throw std::runtime_error("Failed to create DBusMessageHandle with invalid dbus message");

    g_dbus_message_set_body(message, params);
}

repowerd::DBusMessageHandle::DBusMessageHandle(DBusMessageHandle&& other) noexcept
    : message{other.message}
{
    other.message = nullptr;
}

repowerd::DBusMessageHandle::~DBusMessageHandle()
{
    if (message)
        g_object_unref(message);
}

repowerd::DBusMessageHandle::operator GDBusMessage*() const
{
    return message;
}

repowerd::DBusMessageHandle::operator bool() const
{
    return message != nullptr;
}
