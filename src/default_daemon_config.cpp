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

#include "default_daemon_config.h"
#include "core/default_state_machine.h"

#include "adapters/android_device_config.h"
#include "adapters/android_device_quirks.h"
#include "adapters/event_loop_timer.h"
#include "adapters/sysfs_brightness_control.h"
#include "adapters/ubuntu_proximity_sensor.h"
#include "adapters/unity_display_power_control.h"
#include "adapters/unity_power_button.h"
#include "adapters/unity_screen_service.h"
#include "adapters/unity_user_activity.h"
#include "adapters/dev_alarm_wakeup_service.h"

#include "core/voice_call_service.h"

using namespace std::chrono_literals;

namespace
{

struct NullHandlerRegistration : repowerd::HandlerRegistration
{
    NullHandlerRegistration() : HandlerRegistration{[]{}} {}
};

struct NullBrightnessControl : repowerd::BrightnessControl
{
    void disable_autobrightness() override {}
    void enable_autobrightness() override {}
    void set_dim_brightness() override {}
    void set_normal_brightness() override {}
    void set_normal_brightness_value(float)  override {}
    void set_off_brightness() override {}
};

struct NullProximitySensor : repowerd::ProximitySensor
{
    void start_processing() override {}
    repowerd::HandlerRegistration register_proximity_handler(
        repowerd::ProximityHandler const&) override
    {
        return NullHandlerRegistration{};
    }
    repowerd::ProximityState proximity_state() override { return {}; }
    void enable_proximity_events() override {}
    void disable_proximity_events() override {}
};

struct NullVoiceCallService : repowerd::VoiceCallService
{
    void start_processing() override {}
    repowerd::HandlerRegistration register_active_call_handler(
        repowerd::ActiveCallHandler const&) override
    {
        return NullHandlerRegistration{};
    }

    repowerd::HandlerRegistration register_no_active_call_handler(
        repowerd::NoActiveCallHandler const&) override
    {
        return NullHandlerRegistration{};
    }
};

struct NullWakeupService : repowerd::WakeupService
{
    std::string schedule_wakeup_at(std::chrono::system_clock::time_point) override
    {
        return {};
    }

    void cancel_wakeup(std::string const&) override {}

    repowerd::HandlerRegistration register_wakeup_handler(
        repowerd::WakeupHandler const&) override
    {
        return NullHandlerRegistration{};
    }
};

}

std::shared_ptr<repowerd::BrightnessControl>
repowerd::DefaultDaemonConfig::the_brightness_control()
{
    if (!brightness_control)
    try
    {
        brightness_control = std::make_shared<SysfsBrightnessControl>(
            "/sys",
            *the_device_config());
    }
    catch(...)
    {
        brightness_control = std::make_shared<NullBrightnessControl>();
    }

    return brightness_control;
}

std::shared_ptr<repowerd::ClientRequests>
repowerd::DefaultDaemonConfig::the_client_requests()
{
    return the_unity_screen_service();
}

std::shared_ptr<repowerd::DisplayPowerControl>
repowerd::DefaultDaemonConfig::the_display_power_control()
{
    if (!display_power_control)
        display_power_control = std::make_shared<UnityDisplayPowerControl>(the_dbus_bus_address());
    return display_power_control;
}

std::shared_ptr<repowerd::DisplayPowerEventSink>
repowerd::DefaultDaemonConfig::the_display_power_event_sink()
{
    return the_unity_screen_service();
}

std::shared_ptr<repowerd::NotificationService>
repowerd::DefaultDaemonConfig::the_notification_service()
{
    return the_unity_screen_service();
}

std::shared_ptr<repowerd::PowerButton>
repowerd::DefaultDaemonConfig::the_power_button()
{
    return the_unity_power_button();
}

std::shared_ptr<repowerd::PowerButtonEventSink>
repowerd::DefaultDaemonConfig::the_power_button_event_sink()
{
    return the_unity_power_button();
}

std::shared_ptr<repowerd::ProximitySensor>
repowerd::DefaultDaemonConfig::the_proximity_sensor()
{
    if (!proximity_sensor)
    try
    {
        proximity_sensor = std::make_shared<UbuntuProximitySensor>(AndroidDeviceQuirks());
    }
    catch (...)
    {
        proximity_sensor = std::make_shared<NullProximitySensor>();
    }

    return proximity_sensor;
}

std::shared_ptr<repowerd::StateMachine>
repowerd::DefaultDaemonConfig::the_state_machine()
{
    if (!state_machine)
        state_machine = std::make_shared<DefaultStateMachine>(*this);
    return state_machine;
}

std::shared_ptr<repowerd::Timer>
repowerd::DefaultDaemonConfig::the_timer()
{
    if (!timer)
        timer = std::make_shared<EventLoopTimer>();
    return timer;
}

std::shared_ptr<repowerd::UserActivity>
repowerd::DefaultDaemonConfig::the_user_activity()
{
    if (!user_activity)
        user_activity = std::make_shared<UnityUserActivity>(the_dbus_bus_address());
    return user_activity;
}

std::shared_ptr<repowerd::VoiceCallService>
repowerd::DefaultDaemonConfig::the_voice_call_service()
{
    if (!voice_call_service)
        voice_call_service = std::make_shared<NullVoiceCallService>();
    return voice_call_service;
}

std::chrono::milliseconds
repowerd::DefaultDaemonConfig::power_button_long_press_timeout()
{
    return 2s;
}

std::chrono::milliseconds
repowerd::DefaultDaemonConfig::user_inactivity_normal_display_dim_duration()
{
    return 10s;
}

std::chrono::milliseconds
repowerd::DefaultDaemonConfig::user_inactivity_normal_display_off_timeout()
{
    return 60s;
}

std::chrono::milliseconds
repowerd::DefaultDaemonConfig::user_inactivity_post_notification_display_off_timeout()
{
    return 3s;
}

std::chrono::milliseconds
repowerd::DefaultDaemonConfig::user_inactivity_reduced_display_off_timeout()
{
    return 8s;
}

bool repowerd::DefaultDaemonConfig::turn_on_display_at_startup()
{
    return true;
}

std::string repowerd::DefaultDaemonConfig::the_dbus_bus_address()
{
    auto const address = std::unique_ptr<gchar, decltype(&g_free)>{
        g_dbus_address_get_for_bus_sync(G_BUS_TYPE_SYSTEM, nullptr, nullptr),
        g_free};

    return address ? address.get() : std::string{};
}

std::shared_ptr<repowerd::DeviceConfig>
repowerd::DefaultDaemonConfig::the_device_config()
{
    if (!device_config)
    {
        device_config = std::make_shared<AndroidDeviceConfig>(
            "/usr/share/powerd/device_configs");
    }

    return device_config;
}

std::shared_ptr<repowerd::UnityScreenService>
repowerd::DefaultDaemonConfig::the_unity_screen_service()
{
    if (!unity_screen_service)
    {
        unity_screen_service = std::make_shared<UnityScreenService>(
            the_wakeup_service(),
            *the_device_config(),
            the_dbus_bus_address());
    }

    return unity_screen_service;
}

std::shared_ptr<repowerd::UnityPowerButton>
repowerd::DefaultDaemonConfig::the_unity_power_button()
{
    if (!unity_power_button)
        unity_power_button = std::make_shared<UnityPowerButton>(the_dbus_bus_address());
    return unity_power_button;
}

std::shared_ptr<repowerd::WakeupService>
repowerd::DefaultDaemonConfig::the_wakeup_service()
{
    if (!wakeup_service)
    try
    {
        wakeup_service = std::make_shared<DevAlarmWakeupService>("/dev");
    }
    catch (...)
    {
        wakeup_service = std::make_shared<NullWakeupService>();
    }

    return wakeup_service;
}
