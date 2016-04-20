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

#include "dbus_connection_handle.h"
#include "dbus_event_loop.h"
#include "brightness_params.h"

#include <string>
#include <memory>

#include <gio/gio.h>

namespace repowerd
{

class UnityScreenService;
class DeviceProperties;

class PowerdService
{
public:
    PowerdService(
        std::shared_ptr<UnityScreenService> const& unity_screen_service,
        DeviceConfig const& device_config,
        std::string const& dbus_bus_address);
    ~PowerdService();

private:
    void dbus_method_call(
        GDBusConnection* connection,
        gchar const* sender,
        gchar const* object_path,
        gchar const* interface_name,
        gchar const* method_name,
        GVariant* parameters,
        GDBusMethodInvocation* invocation);

    std::string dbus_requestSysState(
        std::string const& sender,
        std::string const& name,
        int32_t state);
    void dbus_clearSysState(
        std::string const& sender,
        std::string const& cookie);
    BrightnessParams dbus_getBrightnessParams();

    std::shared_ptr<UnityScreenService> const unity_screen_service;
    DBusConnectionHandle dbus_connection;
    DBusEventLoop dbus_event_loop;
    BrightnessParams const brightness_params;
};

}
