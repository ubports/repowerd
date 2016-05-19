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

#include "src/core/voice_call_service.h"
#include "src/core/modem_power_control.h"

#include "dbus_connection_handle.h"
#include "dbus_event_loop.h"

#include <unordered_map>
#include <unordered_set>

namespace repowerd
{

enum class OfonoCallState {
    invalid,
    active,
    alerting,
    dialing,
    disconnected,
    held,
    incoming,
    waiting};

class OfonoVoiceCallService : public VoiceCallService, public ModemPowerControl
{
public:
    OfonoVoiceCallService(std::string const& dbus_bus_address);

    void start_processing() override;

    HandlerRegistration register_active_call_handler(
        ActiveCallHandler const& handler) override;
    HandlerRegistration register_no_active_call_handler(
        NoActiveCallHandler const& handler) override;

    void set_low_power_mode() override;
    void set_normal_power_mode() override;

    std::unordered_set<std::string> tracked_modems();

private:
    void handle_dbus_signal(
        GDBusConnection* connection,
        gchar const* sender,
        gchar const* object_path,
        gchar const* interface_name,
        gchar const* signal_name,
        GVariant* parameters);

    void dbus_CallAdded(std::string const& call_path, OfonoCallState call_state);
    void dbus_CallRemoved(std::string const& call_path);
    void dbus_CallStateChanged(std::string const& call_path, OfonoCallState call_state);
    void dbus_ModemAdded(std::string const& call_path);
    void dbus_ModemRemoved(std::string const& call_path);

    void update_call_state(
        std::string const& call_path, OfonoCallState call_state);
    bool is_any_call_active();
    void add_existing_modems();
    void set_fast_dormancy(bool fast_dormancy);

    DBusConnectionHandle dbus_connection;
    DBusEventLoop dbus_event_loop;
    HandlerRegistration manager_handler_registration;
    HandlerRegistration voice_call_manager_handler_registration;
    HandlerRegistration voice_call_handler_registration;

    ActiveCallHandler active_call_handler;
    NoActiveCallHandler no_active_call_handler;
    std::unordered_map<std::string,OfonoCallState> calls;
    std::unordered_set<std::string> modems;
};

}
