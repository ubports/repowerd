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

#include "src/adapters/ofono_voice_call_service.h"

#include "dbus_client.h"

#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <chrono>

namespace repowerd
{
namespace test
{

class FakeOfono : private DBusClient
{
public:
    enum class ModemPowerState{normal, low};
    using Modems = std::unordered_map<std::string,ModemPowerState>;

    FakeOfono(std::string const& dbus_address);

    void add_call(std::string const& call_path, repowerd::OfonoCallState call_state);
    void remove_call(std::string const& call_path);
    void change_call_state(std::string const& call_path, repowerd::OfonoCallState call_state);

    void add_modem(std::string const& modem_path);
    void remove_modem(std::string const& modem_path);

    bool wait_for_modems_condition(
        std::function<bool(Modems const&)> const& condition,
        std::chrono::milliseconds timeout);

private:
    void dbus_method_call(
        GDBusConnection* connection,
        gchar const* sender_cstr,
        gchar const* object_path_cstr,
        gchar const* interface_name_cstr,
        gchar const* method_name_cstr,
        GVariant* parameters,
        GDBusMethodInvocation* invocation);

    repowerd::HandlerRegistration manager_handler_registration;

    std::mutex modems_mutex;
    std::condition_variable modems_cv;
    Modems modems;
    std::unordered_map<std::string,HandlerRegistration> modem_handler_registrations;
};

}
}
