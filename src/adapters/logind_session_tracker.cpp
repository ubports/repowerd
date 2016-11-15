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

#include "logind_session_tracker.h"
#include "android_device_quirks.h"
#include "event_loop_handler_registration.h"
#include "scoped_g_error.h"

#include "src/core/log.h"

#include <algorithm>

namespace
{

char const* const log_tag = "LogindSessionTracker";

auto null_arg1_handler = [](auto){};
auto null_arg2_handler = [](auto,auto){};
char const* const dbus_logind_name = "org.freedesktop.login1";
char const* const dbus_manager_path = "/org/freedesktop/login1";
char const* const dbus_manager_interface = "org.freedesktop.login1.Manager";
char const* const dbus_seat_path = "/org/freedesktop/login1/seat/seat0";
char const* const dbus_seat_interface = "org.freedesktop.login1.Seat";
char const* const dbus_session_interface = "org.freedesktop.login1.Session";

repowerd::SessionType logind_session_type_to_repowerd_type(
    std::string const& logind_session_type)
{
    if (logind_session_type == "mir")
        return repowerd::SessionType::RepowerdCompatible;
    else
        return repowerd::SessionType::RepowerdIncompatible;
}

}

repowerd::LogindSessionTracker::LogindSessionTracker(
    std::shared_ptr<Log> const& log,
    DeviceQuirks const& quirks,
    std::string const& dbus_bus_address)
    : log{log},
      ignore_session_deactivation{quirks.ignore_session_deactivation()},
      dbus_connection{dbus_bus_address},
      active_session_changed_handler{null_arg2_handler},
      session_removed_handler{null_arg1_handler},
      active_session_id{invalid_session_id}
{
}

void repowerd::LogindSessionTracker::start_processing()
{
    dbus_seat_signal_handler_registration = dbus_event_loop.register_signal_handler(
        dbus_connection,
        dbus_logind_name,
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged",
        dbus_seat_path,
        [this] (
            GDBusConnection* connection,
            gchar const* sender,
            gchar const* object_path,
            gchar const* interface_name,
            gchar const* signal_name,
            GVariant* parameters)
        {
            handle_dbus_signal(
                connection, sender, object_path, interface_name,
                signal_name, parameters);
        });

    dbus_manager_signal_handler_registration = dbus_event_loop.register_signal_handler(
        dbus_connection,
        dbus_logind_name,
        dbus_manager_interface,
        "SessionRemoved",
        dbus_manager_path,
        [this] (
            GDBusConnection* connection,
            gchar const* sender,
            gchar const* object_path,
            gchar const* interface_name,
            gchar const* signal_name,
            GVariant* parameters)
        {
            handle_dbus_signal(
                connection, sender, object_path, interface_name,
                signal_name, parameters);
        });

    dbus_event_loop.enqueue([this] { set_initial_active_session(); }).get();
}

repowerd::HandlerRegistration
repowerd::LogindSessionTracker::register_active_session_changed_handler(
    ActiveSessionChangedHandler const& handler)
{
    return EventLoopHandlerRegistration{
        dbus_event_loop,
        [this, &handler] { this->active_session_changed_handler = handler; },
        [this] { this->active_session_changed_handler = null_arg2_handler; }};
}

repowerd::HandlerRegistration
repowerd::LogindSessionTracker::register_session_removed_handler(
    SessionRemovedHandler const& handler)
{
    return EventLoopHandlerRegistration{
        dbus_event_loop,
        [this, &handler] { this->session_removed_handler = handler; },
        [this] { this->session_removed_handler = null_arg1_handler; }};
}

std::string repowerd::LogindSessionTracker::session_for_pid(pid_t pid)
{
    auto const session_path = dbus_get_session_path_by_pid(pid);
    return session_id_for_path(session_path);

}

void repowerd::LogindSessionTracker::handle_dbus_signal(
    GDBusConnection* /*connection*/,
    gchar const* /*sender*/,
    gchar const* object_path_cstr,
    gchar const* /*interface_name*/,
    gchar const* signal_name_cstr,
    GVariant* parameters)
{
    std::string const signal_name{signal_name_cstr ? signal_name_cstr : ""};
    std::string const object_path{object_path_cstr ? object_path_cstr : ""};

    if (signal_name == "PropertiesChanged")
    {
        char const* properties_interface_cstr{""};
        GVariantIter* properties_iter;
        g_variant_get(parameters, "(&sa{sv}as)",
                      &properties_interface_cstr, &properties_iter, nullptr);

        std::string const properties_interface{properties_interface_cstr};

        if (properties_interface == "org.freedesktop.login1.Seat")
            handle_dbus_change_seat_properties(object_path, properties_iter);

        g_variant_iter_free(properties_iter);
    }
    else if (signal_name == "SessionRemoved")
    {
        char const* session_id_cstr{""};

        g_variant_get(parameters, "(&s&o)", &session_id_cstr, nullptr);

        remove_session(session_id_cstr);
    }
}

void repowerd::LogindSessionTracker::handle_dbus_change_seat_properties(
    std::string const& seat, GVariantIter* properties_iter)
{
    char const* key_cstr{""};
    GVariant* value{nullptr};

    while (g_variant_iter_next(properties_iter, "{&sv}", &key_cstr, &value))
    {
        auto const key_str = std::string{key_cstr};

        if (key_str == "ActiveSession")
        {
            char const* session_id_cstr{""};
            char const* session_path_cstr{""};

            g_variant_get(value, "(&s&o)", &session_id_cstr, &session_path_cstr);

            log->log(log_tag, "change_seat_properties(%s), ActiveSession=(%s,%s)",
                     seat.c_str(), session_id_cstr, session_path_cstr);

            if (std::string{session_id_cstr}.empty())
                deactivate_session();
            else
                activate_session(session_id_cstr, session_path_cstr);
        }

        g_variant_unref(value);
    }
}

void repowerd::LogindSessionTracker::set_initial_active_session()
{
    auto const active_session = dbus_get_active_session();
    if (!active_session.first.empty())
        activate_session(active_session.first, active_session.second);
}

void repowerd::LogindSessionTracker::track_session(
    std::string const& session_id,
    std::string const& session_path)
{
    auto const session_type = dbus_get_session_type(session_path);

    log->log(log_tag, "track_session(%s), type=%s", session_id.c_str(), session_type.c_str());

    tracked_sessions[session_id] =
        { session_path, logind_session_type_to_repowerd_type(session_type) };
}

void repowerd::LogindSessionTracker::remove_session(std::string const& session_id)
{
    auto const iter = tracked_sessions.find(session_id);
    if (iter != tracked_sessions.end())
    {
        log->log(log_tag, "remove_session(%s)", session_id.c_str());

        tracked_sessions.erase(iter);
        session_removed_handler(session_id);
    }
}

void repowerd::LogindSessionTracker::activate_session(
    std::string const& session_id, std::string const& session_path)
{
    auto iter = tracked_sessions.find(session_id);
    if (iter == tracked_sessions.end())
    {
        track_session(session_id, session_path);
        iter = tracked_sessions.find(session_id);
    }

    if (iter != tracked_sessions.end() && iter->first != active_session_id)
    {
        log->log(log_tag, "activate_session(%s)", session_id.c_str());
        active_session_id = iter->first;
        active_session_changed_handler(iter->first, iter->second.type);
    }
}

void repowerd::LogindSessionTracker::deactivate_session()
{
    log->log(log_tag, "deactivate_session()");

    if (ignore_session_deactivation)
        return;

    active_session_id = invalid_session_id;

    active_session_changed_handler(
        repowerd::invalid_session_id,
        repowerd::SessionType::RepowerdIncompatible);
}

std::pair<std::string,std::string> repowerd::LogindSessionTracker::dbus_get_active_session()
{
    int constexpr timeout_default = -1;
    auto constexpr null_cancellable = nullptr;
    ScopedGError error;

    auto const result = g_dbus_connection_call_sync(
        dbus_connection,
        dbus_logind_name,
        dbus_seat_path,
        "org.freedesktop.DBus.Properties",
        "Get",
        g_variant_new("(ss)", dbus_seat_interface, "ActiveSession"),
        G_VARIANT_TYPE("(v)"),
        G_DBUS_CALL_FLAGS_NONE,
        timeout_default,
        null_cancellable,
        error);

    if (!result)
    {
        log->log(log_tag, "dbus_get_active_session() failed to get ActiveSession: %s",
                 error.message_str().c_str());
        return {"",""};
    }

    GVariant* active_session_variant{nullptr};
    g_variant_get(result, "(v)", &active_session_variant);

    char const* session_id_cstr{""};
    char const* session_path_cstr{""};
    g_variant_get(active_session_variant, "(&s&o)", &session_id_cstr, &session_path_cstr);

    std::string const session_id{session_id_cstr};
    std::string const session_path{session_path_cstr};

    g_variant_unref(active_session_variant);
    g_variant_unref(result);

    return {session_id, session_path};
}

std::string repowerd::LogindSessionTracker::dbus_get_session_type(std::string const& session_path)
{
    int constexpr timeout_default = -1;
    auto constexpr null_cancellable = nullptr;
    ScopedGError error;

    auto const result = g_dbus_connection_call_sync(
        dbus_connection,
        dbus_logind_name,
        session_path.c_str(),
        "org.freedesktop.DBus.Properties",
        "Get",
        g_variant_new("(ss)", dbus_session_interface, "Type"),
        G_VARIANT_TYPE("(v)"),
        G_DBUS_CALL_FLAGS_NONE,
        timeout_default,
        null_cancellable,
        error);

    if (!result)
    {
        log->log(log_tag, "dbus_get_session_type() failed to get session Type: %s",
                 error.message_str().c_str());
        return "";
    }

    GVariant* type_variant{nullptr};
    g_variant_get(result, "(v)", &type_variant);

    char const* session_type_cstr{""};
    g_variant_get(type_variant, "&s", &session_type_cstr);

    std::string const session_type{session_type_cstr};

    g_variant_unref(type_variant);
    g_variant_unref(result);

    return session_type;
}

std::string repowerd::LogindSessionTracker::dbus_get_session_path_by_pid(pid_t pid)
{
    int constexpr timeout_default = 1000;
    auto constexpr null_cancellable = nullptr;
    ScopedGError error;

    auto const result = g_dbus_connection_call_sync(
        dbus_connection,
        dbus_logind_name,
        dbus_manager_path,
        dbus_manager_interface,
        "GetSessionByPID",
        g_variant_new("(u)", pid),
        G_VARIANT_TYPE("(o)"),
        G_DBUS_CALL_FLAGS_NONE,
        timeout_default,
        null_cancellable,
        error);

    if (!result)
    {
        log->log(log_tag, "dbus_get_session_by_pid() failed: %s",
                 error.message_str().c_str());
        return invalid_session_id;
    }

    char const* session_path_cstr{""};
    g_variant_get(result, "(&o)", &session_path_cstr);

    std::string const session_path{session_path_cstr};

    g_variant_unref(result);

    return session_path;
}

std::string repowerd::LogindSessionTracker::session_id_for_path(
    std::string const& session_path)
{
    if (session_path.empty())
        return invalid_session_id;

    auto const iter = std::find_if(
        tracked_sessions.begin(), tracked_sessions.end(),
        [&session_path] (auto const& kv)
        {
            return kv.second.path == session_path;
        });

    return iter != tracked_sessions.end() ? iter->first : invalid_session_id;
}
