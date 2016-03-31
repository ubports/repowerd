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

#include "src/core/client_requests.h"

#include "dbus_connection_handle.h"
#include "dbus_event_loop.h"

#include <string>
#include <thread>
#include <unordered_map>

#include <gio/gio.h>

namespace repowerd
{

class UnityScreenService : public ClientRequests
{
public:
    UnityScreenService(std::string const& dbus_bus_address);
    ~UnityScreenService();

    HandlerRegistration register_disable_inactivity_timeout_handler(
        DisableInactivityTimeoutHandler const& handler) override;
    HandlerRegistration register_enable_inactivity_timeout_handler(
        EnableInactivityTimeoutHandler const& handler) override;
    HandlerRegistration register_set_inactivity_timeout_handler(
        SetInactivityTimeoutHandler const& handler) override;

    HandlerRegistration register_disable_autobrightness_handler(
        DisableAutobrightnessHandler const& handler) override;
    HandlerRegistration register_enable_autobrightness_handler(
        EnableAutobrightnessHandler const& handler) override;
    HandlerRegistration register_set_normal_brightness_value_handler(
        SetNormalBrightnessValueHandler const& handler) override;

private:
    void dbus_method_call(
        GDBusConnection* connection,
        gchar const* sender,
        gchar const* object_path,
        gchar const* interface_name,
        gchar const* method_name,
        GVariant* parameters,
        GDBusMethodInvocation* invocation);
    void dbus_signal(
        GDBusConnection* connection,
        gchar const* sender,
        gchar const* object_path,
        gchar const* interface_name,
        gchar const* signal_name,
        GVariant* parameters);
    int32_t dbus_keepDisplayOn(std::string const& sender);
    void dbus_removeDisplayOnRequest(std::string const& sender, int32_t id);
    void dbus_setUserBrightness(int32_t brightness);
    void dbus_setInactivityTimeouts(int32_t poweroff_timeout, int32_t dimmer_timeout);
    void dbus_userAutobrightnessEnable(bool enable);
    void dbus_NameOwnerChanged(
        std::string const& name,
        std::string const& old_owner,
        std::string const& new_owner);

    DBusConnectionHandle dbus_connection;
    DBusEventLoop dbus_event_loop;

    DisableInactivityTimeoutHandler disable_inactivity_timeout_handler;
    EnableInactivityTimeoutHandler enable_inactivity_timeout_handler;
    SetInactivityTimeoutHandler set_inactivity_timeout_handler;
    DisableAutobrightnessHandler disable_autobrightness_handler;
    EnableAutobrightnessHandler enable_autobrightness_handler;
    SetNormalBrightnessValueHandler set_normal_brightness_value_handler;

    std::unordered_multimap<std::string,int32_t> keep_display_on_ids;
    int32_t next_keep_display_on_id;
};

}
