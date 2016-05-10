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

#include "dbus_event_loop.h"
#include "event_loop_handler_registration.h"
#include "scoped_g_error.h"

namespace
{

// Send a synchronous request to ensure all previous requests have
// reached the dbus daemon
void repowerd_g_dbus_connection_wait_for_requests(GDBusConnection* connection)
{
    int const timeout_default = -1;
    auto const null_cancellable = nullptr;
    auto const null_args = nullptr;
    auto const null_return_args_types = nullptr;
    auto const null_error = nullptr;

    auto result = g_dbus_connection_call_sync(
        connection,
        "org.freedesktop.DBus",
        "/org/freedesktop/DBus",
        "org.freedesktop.DBus",
        "GetId",
        null_args,
        null_return_args_types,
        G_DBUS_CALL_FLAGS_NONE,
        timeout_default,
        null_cancellable,
        null_error);

    g_variant_unref(result);
}

}

repowerd::HandlerRegistration repowerd::DBusEventLoop::register_object_handler(
    GDBusConnection* dbus_connection,
    char const* dbus_path,
    char const* introspection_xml,
    DBusEventLoopMethodCallHandler const& handler)
{
    struct ObjectContext
    {
        static void static_call(
            GDBusConnection* connection,
            char const* sender,
            char const* object_path,
            char const* interface_name,
            char const* method_name,
            GVariant* parameters,
            GDBusMethodInvocation* invocation,
            ObjectContext* ctx)
        {
            ctx->handler(
                connection, sender, object_path, interface_name,
                method_name, parameters, invocation);
        }
        static void static_destroy(ObjectContext* ctx) { delete ctx; }
        DBusEventLoopMethodCallHandler const handler;
    };

    unsigned int registration_id = 0;
    auto done = enqueue(
        [&]
        {
            auto const introspection_data = g_dbus_node_info_new_for_xml(
                introspection_xml, NULL);

            GDBusInterfaceVTable interface_vtable;
            interface_vtable.method_call =
                reinterpret_cast<GDBusInterfaceMethodCallFunc>(&ObjectContext::static_call);
            interface_vtable.get_property = nullptr;
            interface_vtable.set_property = nullptr;

            ScopedGError error;

            registration_id = g_dbus_connection_register_object(
                dbus_connection,
                dbus_path,
                introspection_data->interfaces[0],
                &interface_vtable,
                new ObjectContext{handler},
                reinterpret_cast<GDestroyNotify>(&ObjectContext::static_destroy),
                error);

            g_dbus_node_info_unref(introspection_data);
            if (error.is_set())
            {
                throw std::runtime_error{
                    "Failed to register DBus object '" + std::string{dbus_path} + "': " +
                        error.message_str()};
            }
        });

    done.wait();

    // g_dbus_connection_register_object() is not synchronous, so wait for
    // the registration (really a DBus AddMatch request) to be processed
    // by the server
    repowerd_g_dbus_connection_wait_for_requests(dbus_connection);

    return EventLoopHandlerRegistration(
        *this,
        [dbus_connection,registration_id]
        {
            g_dbus_connection_unregister_object(
                dbus_connection,
                registration_id);
        });
}


repowerd::HandlerRegistration repowerd::DBusEventLoop::register_signal_handler(
    GDBusConnection* dbus_connection,
    char const* dbus_sender,
    char const* dbus_interface,
    char const* dbus_member,
    char const* dbus_path,
    DBusEventLoopSignalHandler const& handler)
{
    struct SignalContext
    {
        static void static_call(
            GDBusConnection* connection,
            char const* sender,
            char const* object_path,
            char const* interface_name,
            char const* signal_name,
            GVariant* parameters,
            SignalContext* ctx)
        {
            ctx->handler(
                connection, sender, object_path, interface_name,
                signal_name, parameters);
        }
        static void static_destroy(SignalContext* ctx) { delete ctx; }
        DBusEventLoopSignalHandler const handler;
    };

    unsigned int registration_id = 0;
    auto done = enqueue(
        [&]
        {
            registration_id = g_dbus_connection_signal_subscribe(
                dbus_connection,
                dbus_sender,
                dbus_interface,
                dbus_member,
                dbus_path,
                nullptr,
                G_DBUS_SIGNAL_FLAGS_NONE,
                reinterpret_cast<GDBusSignalCallback>(&SignalContext::static_call),
                new SignalContext{handler},
                reinterpret_cast<GDestroyNotify>(&SignalContext::static_destroy));
        });

    done.wait();

    // g_dbus_connection_signal_subscribe() is not synchronous, so wait for
    // the subscription (really a DBus AddMatch request) to be processed
    // by the server
    repowerd_g_dbus_connection_wait_for_requests(dbus_connection);

    return EventLoopHandlerRegistration(
        *this,
        [dbus_connection,registration_id]
        {
            g_dbus_connection_signal_unsubscribe(
                dbus_connection,
                registration_id);
        });
}
