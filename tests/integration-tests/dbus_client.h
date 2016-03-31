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

#include "src/adapters/dbus_connection_handle.h"
#include "src/adapters/dbus_event_loop.h"

#include <string>
#include <future>

#include <gio/gio.h>

namespace repowerd
{
class DBusMessageHandle;

namespace test
{

class DBusAsyncReply
{
public:
    DBusAsyncReply(DBusAsyncReply const&) = delete;
    DBusAsyncReply& operator=(DBusAsyncReply const&) = delete;

    DBusAsyncReply() = default;
    DBusAsyncReply(DBusAsyncReply&& other);
    ~DBusAsyncReply();

    DBusMessageHandle get();

    void set_pending();

    static void static_set_async_result(
        GDBusConnection* connection, GAsyncResult* result, DBusAsyncReply* reply);

protected:
    void throw_on_error_reply(GDBusMessage* reply);
    void throw_on_invalid_reply(GDBusMessage* reply);

private:
    void set_async_result(GDBusMessage* message);

    bool pending = false;
    std::promise<GDBusMessage*> promise;
};

class DBusAsyncReplyVoid : public DBusAsyncReply
{
public:
    using DBusAsyncReply::DBusAsyncReply;
    void get();
};

class DBusAsyncReplyInt : public DBusAsyncReply
{
public:
    using DBusAsyncReply::DBusAsyncReply;
    int get();
};

class DBusAsyncReplyBool : public DBusAsyncReply
{
public:
    using DBusAsyncReply::DBusAsyncReply;
    bool get();
};

class DBusAsyncReplyString : public DBusAsyncReply
{
public:
    using DBusAsyncReply::DBusAsyncReply;
    std::string get();
};

class DBusClient
{
public:
    DBusClient(
        std::string const& bus_address,
        std::string const& destination,
        std::string const& path);

    void disconnect();

    template <typename T>
    T invoke_with_reply(
        char const* interface, char const* method, GVariant* args)
    {
        T t;
        invoke_async(&t, interface, method, args);
        return t;
    }

protected:
    void invoke_async(
        DBusAsyncReply* reply, char const* interface, char const* method, GVariant* args);
    DBusConnectionHandle connection;
    DBusEventLoop event_loop;
    std::string const destination;
    std::string const path;
    std::string const interface;
};

}
}
