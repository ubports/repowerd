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

#include "adapters/android_autobrightness_algorithm.h"
#include "adapters/android_backlight.h"
#include "adapters/android_device_config.h"
#include "adapters/android_device_quirks.h"
#include "adapters/backlight_brightness_control.h"
#include "adapters/console_log.h"
#include "adapters/dev_alarm_wakeup_service.h"
#include "adapters/event_loop_timer.h"
#include "adapters/libsuspend_suspend_control.h"
#include "adapters/null_log.h"
#include "adapters/ofono_voice_call_service.h"
#include "adapters/sysfs_backlight.h"
#include "adapters/syslog_log.h"
#include "adapters/ubuntu_light_sensor.h"
#include "adapters/ubuntu_proximity_sensor.h"
#include "adapters/unity_display_power_control.h"
#include "adapters/unity_power_button.h"
#include "adapters/unity_screen_service.h"
#include "adapters/unity_user_activity.h"

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

struct NullBrightnessNotification : repowerd::BrightnessNotification
{
    repowerd::HandlerRegistration register_brightness_handler(
        repowerd::BrightnessHandler const&) override
    {
        return NullHandlerRegistration{};
    }
};

struct NullLightSensor : repowerd::LightSensor
{
    repowerd::HandlerRegistration register_light_handler(
        repowerd::LightHandler const&) override
    {
        return NullHandlerRegistration{};
    }

    void enable_light_events() override {}
    void disable_light_events() override {}
};

struct NullModemPowerControl : repowerd::ModemPowerControl
{
    void set_low_power_mode() override {}
    void set_normal_power_mode() override {}
};

struct NullProximitySensor : repowerd::ProximitySensor
{
    repowerd::HandlerRegistration register_proximity_handler(
        repowerd::ProximityHandler const&) override
    {
        return NullHandlerRegistration{};
    }
    repowerd::ProximityState proximity_state() override { return {}; }
    void enable_proximity_events() override {}
    void disable_proximity_events() override {}
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
        brightness_control = the_backlight_brightness_control();
    }
    catch (...)
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
    {
        display_power_control = std::make_shared<UnityDisplayPowerControl>(
            the_log(),
            the_dbus_bus_address());
    }
    return display_power_control;
}

std::shared_ptr<repowerd::DisplayPowerEventSink>
repowerd::DefaultDaemonConfig::the_display_power_event_sink()
{
    return the_unity_screen_service();
}

std::shared_ptr<repowerd::ModemPowerControl>
repowerd::DefaultDaemonConfig::the_modem_power_control()
{
    return the_ofono_voice_call_service();
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
        proximity_sensor = std::make_shared<UbuntuProximitySensor>(
            the_log(),
            AndroidDeviceQuirks());
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

std::shared_ptr<repowerd::SuspendControl>
repowerd::DefaultDaemonConfig::the_suspend_control()
{
    if (!suspend_control)
    {
        suspend_control = std::make_shared<LibsuspendSuspendControl>(
            the_log());
    }
    return suspend_control;
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
    return the_ofono_voice_call_service();
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

std::shared_ptr<repowerd::Backlight>
repowerd::DefaultDaemonConfig::the_backlight()
{
    if (!backlight)
    {
        try
        {
            backlight = std::make_shared<AndroidBacklight>();
        }
        catch (...)
        {
        }

        if (!backlight)
            backlight = std::make_shared<SysfsBacklight>("/sys");
    }

    return backlight;
}

std::shared_ptr<repowerd::BacklightBrightnessControl>
repowerd::DefaultDaemonConfig::the_backlight_brightness_control()
{
    if (!backlight_brightness_control)
    {
        backlight_brightness_control = std::make_shared<BacklightBrightnessControl>(
            the_backlight(),
            the_light_sensor(),
            std::make_shared<AndroidAutobrightnessAlgorithm>(*the_device_config()),
            *the_device_config());
    }

    return backlight_brightness_control;
}

std::shared_ptr<repowerd::BrightnessNotification>
repowerd::DefaultDaemonConfig::the_brightness_notification()
{
    if (!brightness_notification)
    try
    {
        brightness_notification = the_backlight_brightness_control();
    }
    catch (...)
    {
        brightness_notification = std::make_shared<NullBrightnessNotification>();
    }

    return brightness_notification;
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

std::shared_ptr<repowerd::LightSensor>
repowerd::DefaultDaemonConfig::the_light_sensor()
{
    if (!light_sensor)
    try
    {
        light_sensor = std::make_shared<UbuntuLightSensor>();
    }
    catch (...)
    {
        light_sensor = std::make_shared<NullLightSensor>();
    }

    return light_sensor;
}

std::shared_ptr<repowerd::OfonoVoiceCallService>
repowerd::DefaultDaemonConfig::the_ofono_voice_call_service()
{
    if (!ofono_voice_call_service)
    {
        ofono_voice_call_service = std::make_shared<OfonoVoiceCallService>(
            the_log(),
            the_dbus_bus_address());
    }
    return ofono_voice_call_service;
}

std::shared_ptr<repowerd::Log>
repowerd::DefaultDaemonConfig::the_log()
{
    if (!log)
    {
        auto const log_env_cstr = getenv("REPOWERD_LOG");
        std::string const log_env{log_env_cstr ? log_env_cstr : ""};
        if (log_env == "console")
            log = std::make_shared<ConsoleLog>();
        else if (log_env == "null")
            log = std::make_shared<NullLog>();
        else
            log = std::make_shared<SyslogLog>();
    }
    return log;
}

std::shared_ptr<repowerd::UnityScreenService>
repowerd::DefaultDaemonConfig::the_unity_screen_service()
{
    if (!unity_screen_service)
    {
        unity_screen_service = std::make_shared<UnityScreenService>(
            the_wakeup_service(),
            the_backlight_brightness_control(),
            the_log(),
            the_suspend_control(),
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
