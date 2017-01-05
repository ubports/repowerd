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

#include "ofono_voice_call_service.h"
#include "event_loop_handler_registration.h"

#include "src/core/log.h"

#include <algorithm>

namespace
{
char const* const log_tag = "OfonoVoiceCallService";
auto const null_handler = []{};
char const* const ofono_manager_interface = "org.ofono.Manager";
char const* const ofono_radio_settings_interface = "org.ofono.RadioSettings";
char const* const ofono_service_name = "org.ofono";
char const* const ofono_voice_call_manager_interface = "org.ofono.VoiceCallManager";
char const* const ofono_voice_call_interface = "org.ofono.VoiceCall";

repowerd::OfonoCallState ofono_call_state_from_string(std::string const& state_str)
{
    repowerd::OfonoCallState state;

    if (state_str == "active")
        state = repowerd::OfonoCallState::active;
    else if (state_str == "alerting")
        state = repowerd::OfonoCallState::alerting;
    else if (state_str == "dialing")
        state = repowerd::OfonoCallState::dialing;
    else if (state_str == "disconnected")
        state = repowerd::OfonoCallState::disconnected;
    else if (state_str == "held")
        state = repowerd::OfonoCallState::held;
    else if (state_str == "incoming")
        state = repowerd::OfonoCallState::incoming;
    else if (state_str == "waiting")
        state = repowerd::OfonoCallState::waiting;
    else
        state = repowerd::OfonoCallState::invalid;

    return state;
}

std::string ofono_call_state_to_string(repowerd::OfonoCallState state)
{
    switch (state)
    {
    case repowerd::OfonoCallState::active:
        return "active";
    case repowerd::OfonoCallState::alerting:
        return "alerting";
    case repowerd::OfonoCallState::dialing:
        return "dialing";
    case repowerd::OfonoCallState::disconnected:
        return "disconnected";
    case repowerd::OfonoCallState::held:
        return "held";
    case repowerd::OfonoCallState::incoming:
        return "incoming";
    case repowerd::OfonoCallState::waiting:
        return "waiting";
    case repowerd::OfonoCallState::invalid:
    default:
        return "";
    };

    return "";
}

repowerd::OfonoCallState get_call_state_from_properties(GVariantIter* properties)
{
    char const* key_cstr{""};
    GVariant* value{nullptr};
    std::string state;
    bool done = false;

    while (!done && g_variant_iter_next(properties, "{&sv}", &key_cstr, &value))
    {
        std::string const key{key_cstr ? key_cstr : ""};
        if (key == "State")
        {
            state = g_variant_get_string(value, nullptr);
            done = true;
        }
        g_variant_unref(value);
    }

    return ofono_call_state_from_string(state);
}

bool is_call_state_active(repowerd::OfonoCallState call_state)
{
    return call_state == repowerd::OfonoCallState::active ||
           call_state == repowerd::OfonoCallState::alerting ||
           call_state == repowerd::OfonoCallState::dialing;
}

}

repowerd::OfonoVoiceCallService::OfonoVoiceCallService(
    std::shared_ptr<Log> const& log,
    std::string const& dbus_bus_address)
    : log{log},
      dbus_connection{dbus_bus_address},
      dbus_event_loop{"Ofono"},
      active_call_handler{null_handler},
      no_active_call_handler{null_handler}
{
}

void repowerd::OfonoVoiceCallService::start_processing()
{
    manager_handler_registration = dbus_event_loop.register_signal_handler(
        dbus_connection,
        ofono_service_name,
        ofono_manager_interface,
        nullptr,
        nullptr,
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

    voice_call_manager_handler_registration = dbus_event_loop.register_signal_handler(
        dbus_connection,
        ofono_service_name,
        ofono_voice_call_manager_interface,
        nullptr,
        nullptr,
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

    voice_call_handler_registration = dbus_event_loop.register_signal_handler(
        dbus_connection,
        ofono_service_name,
        ofono_voice_call_interface,
        nullptr,
        nullptr,
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

    dbus_event_loop.enqueue([this] { add_existing_modems(); }).get();
}

repowerd::HandlerRegistration repowerd::OfonoVoiceCallService::register_active_call_handler(
    ActiveCallHandler const& handler)
{
    return EventLoopHandlerRegistration{
        dbus_event_loop,
            [this, &handler] { this->active_call_handler = handler; },
            [this] { this->active_call_handler = null_handler; }};
}

repowerd::HandlerRegistration repowerd::OfonoVoiceCallService::register_no_active_call_handler(
    NoActiveCallHandler const& handler)
{
    return EventLoopHandlerRegistration{
        dbus_event_loop,
            [this, &handler] { this->no_active_call_handler = handler; },
            [this] { this->no_active_call_handler = null_handler; }};
}

void repowerd::OfonoVoiceCallService::set_low_power_mode()
{
    dbus_event_loop.enqueue([this] { set_fast_dormancy(true); });
}

void repowerd::OfonoVoiceCallService::set_normal_power_mode()
{
    dbus_event_loop.enqueue([this] { set_fast_dormancy(false); });
}

std::unordered_set<std::string> repowerd::OfonoVoiceCallService::tracked_modems()
{
    decltype(modems) ret_modems;
    dbus_event_loop.enqueue([this,&ret_modems] { ret_modems = modems; }).get();
    return ret_modems;
}

void repowerd::OfonoVoiceCallService::handle_dbus_signal(
    GDBusConnection* /*connection*/,
    gchar const* /*sender*/,
    gchar const* object_path_cstr,
    gchar const* interface_name_cstr,
    gchar const* signal_name_cstr,
    GVariant* parameters)
{
    std::string const signal_name{signal_name_cstr ? signal_name_cstr : ""};
    std::string const object_path{object_path_cstr ? object_path_cstr : ""};
    std::string const interface_name{interface_name_cstr ? interface_name_cstr : ""};

    if (signal_name == "CallAdded")
    {
        char const* call_path{""};

        GVariantIter* properties;
        g_variant_get(parameters, "(&oa{sv})", &call_path, &properties);
        auto const call_state = get_call_state_from_properties(properties);
        g_variant_iter_free(properties);

        dbus_CallAdded(call_path, call_state);
    }
    else if (signal_name == "CallRemoved")
    {
        char const* call_path{""};
        g_variant_get(parameters, "(&o)", &call_path);

        dbus_CallRemoved(call_path);
    }
    else if (interface_name == ofono_voice_call_interface &&
             signal_name == "PropertyChanged")
    {
        char const* property{""};
        GVariant* value;
        g_variant_get(parameters, "(&sv)", &property, &value);

        if (std::string{property} == "State")
        {
            auto const state_str = g_variant_get_string(value, nullptr);

            dbus_CallStateChanged(object_path, ofono_call_state_from_string(state_str));
        }

        g_variant_unref(value);
    }
    else if (signal_name == "ModemAdded")
    {
        char const* modem_path{""};
        g_variant_get(parameters, "(&oa{sv})", &modem_path, nullptr);

        dbus_ModemAdded(modem_path);
    }
    else if (signal_name == "ModemRemoved")
    {
        char const* modem_path{""};
        g_variant_get(parameters, "(&o)", &modem_path);

        dbus_ModemRemoved(modem_path);
    }
}

void repowerd::OfonoVoiceCallService::dbus_CallAdded(
    std::string const& call_path, OfonoCallState call_state)
{
    log->log(log_tag, "dbus_CallAdded(%s,%s)",
             call_path.c_str(), ofono_call_state_to_string(call_state).c_str());

    update_call_state(call_path, call_state);
}

void repowerd::OfonoVoiceCallService::dbus_CallRemoved(
    std::string const& call_path)
{
    log->log(log_tag, "dbus_CallRemoved(%s)", call_path.c_str());

    update_call_state(call_path, repowerd::OfonoCallState::invalid);
}

void repowerd::OfonoVoiceCallService::dbus_CallStateChanged(
    std::string const& call_path, OfonoCallState call_state)
{
    log->log(log_tag, "dbus_CallStateChanged(%s,%s)",
             call_path.c_str(), ofono_call_state_to_string(call_state).c_str());

    update_call_state(call_path, call_state);
}

void repowerd::OfonoVoiceCallService::dbus_ModemAdded(
    std::string const& modem_path)
{
    log->log(log_tag, "dbus_ModemAdded(%s)", modem_path.c_str());

    modems.insert(modem_path);
}

void repowerd::OfonoVoiceCallService::dbus_ModemRemoved(
    std::string const& modem_path)
{
    log->log(log_tag, "dbus_ModemRemoved(%s)", modem_path.c_str());

    modems.erase(modem_path);
}

void repowerd::OfonoVoiceCallService::update_call_state(
    std::string const& call_path, OfonoCallState call_state)
{
    auto const old_state = calls[call_path];

    if (call_state == repowerd::OfonoCallState::invalid)
        calls.erase(call_path);
    else
        calls[call_path] = call_state;

    if (!is_call_state_active(old_state) && is_call_state_active(call_state))
    {
        active_call_handler();
    }
    else if (is_call_state_active(old_state) && !is_call_state_active(call_state))
    {
        if (!is_any_call_active())
            no_active_call_handler();
    }
}

bool repowerd::OfonoVoiceCallService::is_any_call_active()
{
    return std::any_of(
        calls.begin(), calls.end(),
        [this](auto const& kv)
        {
            return is_call_state_active(kv.second);
        });
}

void repowerd::OfonoVoiceCallService::add_existing_modems()
{
    int constexpr timeout_default = -1;
    auto constexpr null_cancellable = nullptr;
    auto constexpr null_args = nullptr;
    auto constexpr null_error = nullptr;

    auto const result = g_dbus_connection_call_sync(
        dbus_connection,
        ofono_service_name,
        "/",
        ofono_manager_interface,
        "GetModems",
        null_args,
        G_VARIANT_TYPE("(a(oa{sv}))"),
        G_DBUS_CALL_FLAGS_NONE,
        timeout_default,
        null_cancellable,
        null_error);

    if (!result) return;

    GVariantIter* result_modems;
    g_variant_get(result, "(a(oa{sv}))", &result_modems);

    char const* modem{""};
    while (g_variant_iter_next(result_modems, "(&oa{sv})", &modem, nullptr))
    {
        log->log(log_tag, "add_existing_modems(), %s", modem);
        modems.insert(modem);
    }

    g_variant_iter_free(result_modems);
    g_variant_unref(result);
}

void repowerd::OfonoVoiceCallService::set_fast_dormancy(bool fast_dormancy)
{
    int constexpr timeout_default = -1;
    auto constexpr null_cancellable = nullptr;
    auto constexpr null_reply_type = nullptr;
    auto constexpr null_callback = nullptr;
    auto constexpr null_user_data = nullptr;

    for (auto const& modem : modems)
    {
        log->log(log_tag, "set_fast_dormancy(%s,%s)",
                 modem.c_str(), fast_dormancy ? "true" : "false");

        g_dbus_connection_call(
            dbus_connection,
            ofono_service_name,
            modem.c_str(),
            ofono_radio_settings_interface,
            "SetProperty",
            g_variant_new("(sv)", "FastDormancy", g_variant_new_boolean(fast_dormancy)),
            null_reply_type,
            G_DBUS_CALL_FLAGS_NONE,
            timeout_default,
            null_cancellable,
            null_callback,
            null_user_data);
    }
}
