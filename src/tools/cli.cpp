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

#include "src/adapters/scoped_g_error.h"

#include <gio/gio.h>

#include <csignal>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

std::string get_progname(int argc, char** argv)
{
    if (argc == 0)
        return "";
    else
        return argv[0];
}

std::vector<std::string> get_args(int argc, char** argv)
{
    std::vector<std::string> args;

    for (int i = 1; i < argc; ++i)
        args.push_back(argv[i]);

    return args;
}

void show_usage(std::string const& progname)
{
    std::cerr << "Usage: " << progname << " <command>" << std::endl;
    std::cerr << "Available commands: " << std::endl;
    std::cerr << "  display [on]: keep display on until program is terminated" << std::endl;
    std::cerr << "  active: inhibit device suspend until program is terminated" << std::endl;
    std::cerr << "  settings inactivity <power_action> <power_supply> <timeout>" << std::endl;
    std::cerr << "  settings lid <power_action> <power-supply>" << std::endl;
    std::cerr << "  settings critical-power <power_action>" << std::endl;
    std::cerr << std::endl;
    std::cerr << "<power_action>: none, display-off, suspend, power-off" << std::endl;
    std::cerr << "<power_supply>: battery, line-power" << std::endl;
}

std::unique_ptr<GDBusProxy,void(*)(void*)> create_unity_screen_proxy()
{
    repowerd::ScopedGError error;

    auto uscreen_proxy = g_dbus_proxy_new_for_bus_sync(
        G_BUS_TYPE_SYSTEM,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
        "com.canonical.Unity.Screen",
        "/com/canonical/Unity/Screen",
        "com.canonical.Unity.Screen",
        NULL,
        error);

    if (uscreen_proxy == nullptr)
    {
        throw std::runtime_error(
            "Failed to connect to com.canonical.Unity.Screen: " + error.message_str());
    }

    return {uscreen_proxy, g_object_unref};
}

std::unique_ptr<GDBusProxy,void(*)(void*)> create_powerd_proxy()
{
    repowerd::ScopedGError error;

    auto powerd_proxy = g_dbus_proxy_new_for_bus_sync(
        G_BUS_TYPE_SYSTEM,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
        "com.canonical.powerd",
        "/com/canonical/powerd",
        "com.canonical.powerd",
        NULL,
        error);

    if (powerd_proxy == nullptr)
    {
        throw std::runtime_error(
            "Failed to connect to com.canonical.powerd: " + error.message_str());
    }

    return {powerd_proxy, g_object_unref};
}

std::unique_ptr<GDBusProxy,void(*)(void*)> create_repowerd_proxy()
{
    repowerd::ScopedGError error;

    auto repowerd_proxy = g_dbus_proxy_new_for_bus_sync(
        G_BUS_TYPE_SYSTEM,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
        "com.canonical.repowerd",
        "/com/canonical/repowerd",
        "com.canonical.repowerd",
        NULL,
        error);

    if (repowerd_proxy == nullptr)
    {
        throw std::runtime_error(
            "Failed to connect to com.canonical.repowerd: " + error.message_str());
    }

    return {repowerd_proxy, g_object_unref};
}

int32_t keep_display_on(GDBusProxy* uscreen_proxy)
{
    repowerd::ScopedGError error;

    auto const ret = g_dbus_proxy_call_sync(
        uscreen_proxy,
        "keepDisplayOn",
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        error);

    if (ret == nullptr)
    {
        throw std::runtime_error(
            "com.canonical.Unity.Screen.keepDisplayOn() failed: " + error.message_str());
    }

    int32_t cookie{0};
    g_variant_get(ret, "(i)", &cookie);
    g_variant_unref(ret);

    return cookie;
}

void remove_display_on_request(GDBusProxy* uscreen_proxy, int32_t cookie)
{
    repowerd::ScopedGError error;

    auto const ret = g_dbus_proxy_call_sync(
        uscreen_proxy,
        "removeDisplayOnRequest",
        g_variant_new("(i)", cookie),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        error);

    if (ret == nullptr)
    {
        throw std::runtime_error(
            "com.canonical.Unity.Screen.removeDisplayOnRequest() failed: " + error.message_str());
    }

    g_variant_unref(ret);
}

std::string request_sys_state_active(GDBusProxy* powerd_proxy)
{
    static int constexpr active_state = 1;
    repowerd::ScopedGError error;

    auto const ret = g_dbus_proxy_call_sync(
        powerd_proxy,
        "requestSysState",
        g_variant_new("(si)", "repowerd-cli", active_state),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        error);

    if (ret == nullptr)
    {
        throw std::runtime_error(
            "com.canonical.powerd.requestSysState(active) failed: " + error.message_str());
    }

    char const* cookie_raw{""};
    g_variant_get(ret, "(&s)", &cookie_raw);
    std::string cookie{cookie_raw};
    g_variant_unref(ret);

    return cookie;
}

void clear_sys_state(GDBusProxy* powerd_proxy, std::string const& cookie)
{
    repowerd::ScopedGError error;

    auto const ret = g_dbus_proxy_call_sync(
        powerd_proxy,
        "clearSysState",
        g_variant_new("(s)", cookie.c_str()),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        error);

    if (ret == nullptr)
    {
        throw std::runtime_error(
            "com.canonical.powerd.clearSysState() failed: " + error.message_str());
    }

    g_variant_unref(ret);
}

void set_inactivity_behavior(
    GDBusProxy* repowerd_proxy,
    std::string const& action,
    std::string const& supply,
    int32_t timeout)
{
    repowerd::ScopedGError error;

    auto const ret = g_dbus_proxy_call_sync(
        repowerd_proxy,
        "SetInactivityBehavior",
        g_variant_new("(ssi)", action.c_str(), supply.c_str(), timeout),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        error);

    if (ret == nullptr)
    {
        throw std::runtime_error(
            "com.canonical.repowerd.SetInactivityBehavior() failed: " + error.message_str());
    }

    g_variant_unref(ret);
}

void set_lid_behavior(
    GDBusProxy* repowerd_proxy,
    std::string const& action,
    std::string const& supply)
{
    repowerd::ScopedGError error;

    auto const ret = g_dbus_proxy_call_sync(
        repowerd_proxy,
        "SetLidBehavior",
        g_variant_new("(ss)", action.c_str(), supply.c_str()),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        error);

    if (ret == nullptr)
    {
        throw std::runtime_error(
            "com.canonical.repowerd.SetLidBehavior() failed: " + error.message_str());
    }

    g_variant_unref(ret);
}

void set_critical_power_behavior(
    GDBusProxy* repowerd_proxy,
    std::string const& action)
{
    repowerd::ScopedGError error;

    auto const ret = g_dbus_proxy_call_sync(
        repowerd_proxy,
        "SetCriticalPowerBehavior",
        g_variant_new("(s)", action.c_str()),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        error);

    if (ret == nullptr)
    {
        throw std::runtime_error(
            "com.canonical.repowerd.SetCriticalPowerBehavior() failed: " + error.message_str());
    }

    g_variant_unref(ret);
}

void handle_display_command(GDBusProxy* uscreen_proxy)
{
    auto const cookie = keep_display_on(uscreen_proxy);

    std::cout << "keepDisplayOn requested, cookie is " << cookie << std::endl;
    std::cout << "Press ctrl-c to exit" << std::endl;

    pause();

    remove_display_on_request(uscreen_proxy, cookie);
}

void handle_active_command(GDBusProxy* uscreen_proxy)
{
    auto const cookie = request_sys_state_active(uscreen_proxy);

    std::cout << "requestSysState(active) requested, cookie is " << cookie << std::endl;
    std::cout << "Press ctrl-c to exit" << std::endl;

    pause();

    clear_sys_state(uscreen_proxy, cookie);
}

void handle_settings_command(
    GDBusProxy* repowerd_proxy,
    std::vector<std::string> const& args)
{
    if (args.size() < 2)
        throw std::invalid_argument{""};

    if (args[1] == "inactivity")
    {
        if (args.size() != 5)
            throw std::invalid_argument{""};

        auto const& action = args[2];
        auto const& supply = args[3];
        auto const timeout = std::stoi(args[4]);

        set_inactivity_behavior(repowerd_proxy, action, supply, timeout);
    }
    else if (args[1] == "lid")
    {
        if (args.size() != 4)
            throw std::invalid_argument{""};

        auto const& action = args[2];
        auto const& supply = args[3];

        set_lid_behavior(repowerd_proxy, action, supply);
    }
    else if (args[1] == "critical-power")
    {
        if (args.size() != 3)
            throw std::invalid_argument{""};

        auto const& action = args[2];

        set_critical_power_behavior(repowerd_proxy, action);
    }
    else
    {
        throw std::invalid_argument{""};
    }
}

void null_signal_handler(int) {}

int main(int argc, char** argv)
try
{
    signal(SIGINT, null_signal_handler);
    signal(SIGTERM, null_signal_handler);

    auto const args = get_args(argc, argv);

    if (args.size() == 0)
        throw std::invalid_argument{""};

    auto const uscreen_proxy = create_unity_screen_proxy();
    auto const powerd_proxy = create_powerd_proxy();
    auto const repowerd_proxy = create_repowerd_proxy();

    if (args[0] == "display")
    {
        handle_display_command(uscreen_proxy.get());
    }
    else if (args[0] == "active")
    {
        handle_active_command(powerd_proxy.get());
    }
    else if (args[0] == "settings")
    {
        handle_settings_command(repowerd_proxy.get(), args);
    }
    else
    {
        throw std::invalid_argument{""};
    }
}
catch (std::invalid_argument const& e)
{
    show_usage(get_progname(argc, argv));
    return -1;
}
catch (std::exception const& e)
{
    std::cerr << e.what() << std::endl;
    return -1;
}
