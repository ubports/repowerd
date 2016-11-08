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

#include "fake_logind.h"
#include <algorithm>

namespace rt = repowerd::test;
using namespace std::literals::string_literals;

namespace
{

char const* const logind_introspection = R"(<!DOCTYPE node PUBLIC '-//freedesktop//DTD D-BUS Object Introspection 1.0//EN' 'http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd'>
<node>
    <interface name='org.freedesktop.login1.Manager'>
        <method name='GetSessionByPID'>
            <arg name='pid' type='u'/>
            <arg name='session' type='o' direction='out'/>
        </method>
        <signal name='SessionAdded'>
            <arg name='name' type='s'/>
            <arg name='path' type='o'/>
        </signal>
        <signal name='SessionRemoved'>
            <arg name='name' type='s'/>
            <arg name='path' type='o'/>
        </signal>
    </interface>
</node>)";

char const* const logind_seat_introspection = R"(<!DOCTYPE node PUBLIC '-//freedesktop//DTD D-BUS Object Introspection 1.0//EN' 'http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd'>
<node>
    <interface name='org.freedesktop.login1.Seat'>
        <property type='(so)' name='ActiveSession' access='read'/>
    </interface>
</node>)";

char const* const logind_session_introspection = R"(<!DOCTYPE node PUBLIC '-//freedesktop//DTD D-BUS Object Introspection 1.0//EN' 'http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd'>
<node>
    <interface name='org.freedesktop.login1.Session'>
        <property type='s' name='Type' access='read'/>
    </interface>
</node>)";

char const* const seat_path = "/org/freedesktop/login1/seat/seat0";
}

rt::FakeLogind::FakeLogind(
    std::string const& dbus_address)
    : rt::DBusClient{dbus_address, "org.freedesktop.login1", "/org/freedesktop/login1"}
{
    connection.request_name("org.freedesktop.login1");

    logind_handler_registration = event_loop.register_object_handler(
        connection,
        "/org/freedesktop/login1",
        logind_introspection,
        [this] (
            GDBusConnection* connection,
            gchar const* sender,
            gchar const* object_path,
            gchar const* interface_name,
            gchar const* method_name,
            GVariant* parameters,
            GDBusMethodInvocation* invocation)
        {
            dbus_method_call(
                connection, sender, object_path, interface_name,
                method_name, parameters, invocation);
        });

    logind_seat_handler_registration = event_loop.register_object_handler(
        connection,
        seat_path,
        logind_seat_introspection,
        [this] (
            GDBusConnection* connection,
            gchar const* sender,
            gchar const* object_path,
            gchar const* interface_name,
            gchar const* method_name,
            GVariant* parameters,
            GDBusMethodInvocation* invocation)
        {
            dbus_method_call(
                connection, sender, object_path, interface_name,
                method_name, parameters, invocation);
        });
}

void rt::FakeLogind::add_session(
    std::string const& session_path, std::string const& session_type, pid_t pid)
{
    {
        std::lock_guard<std::mutex> lock{sessions_mutex};
        sessions[session_path] = {session_type, pid};
    }

    session_handler_registrations[session_path] = event_loop.register_object_handler(
        connection,
        session_path.c_str(),
        logind_session_introspection,
        [this] (
            GDBusConnection* connection,
            gchar const* sender,
            gchar const* object_path,
            gchar const* interface_name,
            gchar const* method_name,
            GVariant* parameters,
            GDBusMethodInvocation* invocation)
        {
            dbus_method_call(
                connection, sender, object_path, interface_name,
                method_name, parameters, invocation);
        });

    auto const session_name = session_path.substr(session_path.rfind("/") + 1);
    auto const params =
        g_variant_new_parsed("(@s %s, @o %o)",
            session_name.c_str(),session_path.c_str());

    emit_signal_full(
        "/org/freedesktop/login1",
        "org.freedesktop.login1.Manager",
        "SessionAdded", params);
}

void rt::FakeLogind::remove_session(std::string const& session_path)
{
    {
        std::lock_guard<std::mutex> lock{sessions_mutex};
        sessions.erase(session_path);
    }

    session_handler_registrations.erase(session_path);

    auto const session_name = session_path.substr(session_path.rfind("/") + 1);

    auto const params =
        g_variant_new_parsed("(@s %s, @o %o)",
            session_name.c_str(),session_path.c_str());

    emit_signal_full(
        "/org/freedesktop/login1",
        "org.freedesktop.login1.Manager",
        "SessionRemoved", params);
}

void rt::FakeLogind::activate_session(std::string const& session_path)
{
    {
        std::lock_guard<std::mutex> lock{sessions_mutex};

        if (sessions.find(session_path) == sessions.end())
            throw std::runtime_error("Cannot activate non-existent session " + session_path);
        active_session_path = session_path;
    }

    auto const session_name = session_path.substr(session_path.rfind("/") + 1);

    auto const changed_properties_str =
        "'ActiveSession': <(@s '"s + session_name + "', @o '" + session_path + "')>";
    auto const params_str =
        "(@s 'org.freedesktop.login1.Seat',"s +
        " @a{sv} {" + changed_properties_str + "}," +
        " @as [])";

    auto const params = g_variant_new_parsed(params_str.c_str());

    emit_signal_full(
        seat_path,
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged", params);
}

void rt::FakeLogind::deactivate_session()
{
    auto const changed_properties_str =
        "'ActiveSession': <(@s '', @o '/')>";
    auto const params_str =
        "(@s 'org.freedesktop.login1.Seat',"s +
        " @a{sv} {" + changed_properties_str + "}," +
        " @as [])";

    auto const params = g_variant_new_parsed(params_str.c_str());

    emit_signal_full(
        seat_path,
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged", params);
}

void rt::FakeLogind::dbus_method_call(
    GDBusConnection* /*connection*/,
    gchar const* /*sender_cstr*/,
    gchar const* object_path_cstr,
    gchar const* interface_name_cstr,
    gchar const* method_name_cstr,
    GVariant* parameters,
    GDBusMethodInvocation* invocation)
{
    std::string const object_path{object_path_cstr ? object_path_cstr : ""};
    std::string const method_name{method_name_cstr ? method_name_cstr : ""};
    std::string const interface_name{interface_name_cstr ? interface_name_cstr : ""};

    if (interface_name == "org.freedesktop.DBus.Properties" &&
        method_name == "Get" &&
        object_path == seat_path)
    {
        std::string local_active_session_path;
        {
            std::lock_guard<std::mutex> lock{sessions_mutex};
            local_active_session_path = active_session_path;
        }

        auto const active_session_name =
            local_active_session_path.substr(local_active_session_path.rfind("/") + 1);
        auto const properties =
            g_variant_new_parsed("(<(@s %s, @o %o)>,)",
                active_session_name.c_str(), local_active_session_path.c_str());

        g_dbus_method_invocation_return_value(invocation, properties);
    }
    else if (interface_name == "org.freedesktop.DBus.Properties" &&
             method_name == "Get")
    {
        std::string session_type;
        {
            std::lock_guard<std::mutex> lock{sessions_mutex};
            auto const iter = sessions.find(object_path);
            if (iter != sessions.end())
                session_type = iter->second.type;
        }

        auto const properties =
            g_variant_new_parsed("(<%s>,)", session_type.c_str());

        g_dbus_method_invocation_return_value(invocation, properties);
    }
    else if (interface_name == "org.freedesktop.login1.Manager" &&
             method_name == "GetSessionByPID")
    {
        guint gpid = -1;
        g_variant_get(parameters, "(u)", &gpid);
        pid_t const pid = gpid;

        std::string session_path;
        {
            std::lock_guard<std::mutex> lock{sessions_mutex};
            auto const iter = std::find_if(
                sessions.begin(), sessions.end(),
                [pid] (auto const& kv) { return kv.second.pid == pid; });
            if (iter != sessions.end())
                session_path = iter->first;
        }

        if (session_path.empty())
        {
            g_dbus_method_invocation_return_error_literal(
                invocation, G_DBUS_ERROR, G_DBUS_ERROR_UNIX_PROCESS_ID_UNKNOWN, "");
        }
        else
        {
            auto const session_variant =
                g_variant_new_parsed("(%o,)", session_path.c_str());

            g_dbus_method_invocation_return_value(invocation, session_variant);
        }
    }
    else
    {
        g_dbus_method_invocation_return_error_literal(
            invocation, G_DBUS_ERROR, G_DBUS_ERROR_NOT_SUPPORTED, "");
    }
}
