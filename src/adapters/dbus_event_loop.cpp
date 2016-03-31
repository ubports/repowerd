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
#include "scoped_g_error.h"

#include <future>

repowerd::DBusEventLoop::DBusEventLoop()
    : main_context{g_main_context_new()},
      main_loop{g_main_loop_new(main_context, FALSE)}
{
    std::promise<void> started_promise;
    auto started_future = started_promise.get_future();

    dbus_thread = std::thread{
        [this, &started_promise]
        {
            g_main_context_push_thread_default(main_context);

            enqueue(
                [&started_promise]
                {
                    started_promise.set_value();
                });

            g_main_loop_run(main_loop);
        }};

    started_future.wait();
}

repowerd::DBusEventLoop::~DBusEventLoop()
{
    stop();
}

void repowerd::DBusEventLoop::stop()
{
    if (main_loop)
        g_main_loop_quit(main_loop);
    if (dbus_thread.joinable())
        dbus_thread.join();
    if (main_loop)
    {
        g_main_loop_unref(main_loop);
        main_loop = nullptr;
    }
    if (main_context)
    {
        g_main_context_unref(main_context);
        main_context = nullptr;
    }
}

void repowerd::DBusEventLoop::register_object_handler(
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

    std::promise<void> done_promise;
    auto done_future = done_promise.get_future();

    enqueue(
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

            g_dbus_connection_register_object(
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
                try
                { 
                    throw std::runtime_error{
                        "Failed to register DBus object '" + std::string{dbus_path} + "': " +
                            error.message_str()};
                }
                catch (...) 
                { 
                    done_promise.set_exception(std::current_exception());
                }
            }
            else
            {
                done_promise.set_value();
            }
        });

    done_future.wait();
}

void repowerd::DBusEventLoop::register_signal_handler(
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

    std::promise<void> done_promise;
    auto done_future = done_promise.get_future();

    enqueue(
        [&]
        {
            g_dbus_connection_signal_subscribe(
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

            done_promise.set_value();
        });

    done_future.wait();
}

void repowerd::DBusEventLoop::enqueue(std::function<void()> const& callback)
{
    struct IdleContext
    {
        static gboolean static_call(IdleContext* ctx)
        {
            ctx->callback();
            return G_SOURCE_REMOVE;
        }
        static void static_destroy(IdleContext* ctx) { delete ctx; }
        std::function<void()> const callback;
    };

    auto const gsource = g_idle_source_new();
    g_source_set_callback(
            gsource,
            reinterpret_cast<GSourceFunc>(&IdleContext::static_call),
            new IdleContext{callback},
            reinterpret_cast<GDestroyNotify>(&IdleContext::static_destroy));

    g_source_attach(gsource, main_context);
    g_source_unref(gsource);
}
