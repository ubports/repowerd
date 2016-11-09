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

#include "src/core/session_tracker.h"

#include "dbus_connection_handle.h"
#include "dbus_event_loop.h"

#include <memory>
#include <unordered_map>

namespace repowerd
{
class Log;

class LogindSessionTracker : public SessionTracker
{
public:
    LogindSessionTracker(
        std::shared_ptr<Log> const& log,
        std::string const& dbus_bus_address);

    void start_processing() override;

    HandlerRegistration register_active_session_changed_handler(
        ActiveSessionChangedHandler const& handler) override;
    HandlerRegistration register_session_removed_handler(
        SessionRemovedHandler const& handler) override;

    std::string session_for_pid(pid_t pid) override;

private:
    void handle_dbus_signal(
        GDBusConnection* connection,
        gchar const* sender,
        gchar const* object_path,
        gchar const* interface_name,
        gchar const* signal_name,
        GVariant* parameters);
    void handle_dbus_change_seat_properties(
        std::string const& seat,
        GVariantIter* properties_iter);
    void set_initial_active_session();
    void track_session(std::string const& session_id, std::string const& session_path);
    void remove_session(std::string const& session_path);
    void activate_session(std::string const& session_id, std::string const& session_path);
    void deactivate_session();
    std::pair<std::string,std::string> dbus_get_active_session();
    std::string dbus_get_session_type(std::string const& session_path);
    std::string dbus_get_session_path_by_pid(pid_t pid);
    std::string session_id_for_path(std::string const& session_path);

    std::shared_ptr<Log> const log;

    DBusConnectionHandle dbus_connection;
    DBusEventLoop dbus_event_loop;
    HandlerRegistration dbus_seat_signal_handler_registration;
    HandlerRegistration dbus_manager_signal_handler_registration;

    ActiveSessionChangedHandler active_session_changed_handler;
    SessionRemovedHandler session_removed_handler;

    struct SessionInfo
    {
        std::string path;
        SessionType type;
    };
    std::unordered_map<std::string,SessionInfo> tracked_sessions;
};

}
