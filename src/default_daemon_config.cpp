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
#include "core/default_state_machine_factory.h"

#include "adapters/android_autobrightness_algorithm.h"
#include "adapters/android_backlight.h"
#include "adapters/android_device_config.h"
#include "adapters/android_device_quirks.h"
#include "adapters/backlight_brightness_control.h"
#include "adapters/console_log.h"
#include "adapters/dev_alarm_wakeup_service.h"
#include "adapters/event_loop_timer.h"
#include "adapters/libsuspend_system_power_control.h"
#include "adapters/logind_session_tracker.h"
#include "adapters/null_log.h"
#include "adapters/ofono_voice_call_service.h"
#include "adapters/real_chrono.h"
#include "adapters/real_filesystem.h"
#include "adapters/sysfs_backlight.h"
#include "adapters/syslog_log.h"
#include "adapters/system_shutdown_control.h"
#include "adapters/ubuntu_light_sensor.h"
#include "adapters/ubuntu_performance_booster.h"
#include "adapters/ubuntu_proximity_sensor.h"
#include "adapters/unity_display_power_control.h"
#include "adapters/unity_power_button.h"
#include "adapters/unity_screen_service.h"
#include "adapters/unity_user_activity.h"
#include "adapters/upower_power_source.h"

using namespace std::chrono_literals;

namespace
{
char const* const log_tag = "DefaultDaemonConfig";

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
    void set_normal_brightness_value(double)  override {}
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

struct NullPerformanceBooster : repowerd::PerformanceBooster
{
    void enable_interactive_mode() override {}
    void disable_interactive_mode() override {}
};

struct NullProximitySensor : repowerd::ProximitySensor
{
    repowerd::HandlerRegistration register_proximity_handler(
        repowerd::ProximityHandler const&) override
    {
        return NullHandlerRegistration{};
    }
    repowerd::ProximityState proximity_state() override
    { 
        return repowerd::ProximityState::far;
    }
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
    catch (std::exception const& e)
    {
        the_log()->log(log_tag, "Failed to create BrightnessControl: %s", e.what());
        the_log()->log(log_tag, "Falling back to NullBrightnessControl");
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

std::shared_ptr<repowerd::PerformanceBooster>
repowerd::DefaultDaemonConfig::the_performance_booster()
{
    if (!performance_booster)
    try
    {
        performance_booster = std::make_shared<UbuntuPerformanceBooster>(
            the_log());
    }
    catch (std::exception const& e)
    {
        the_log()->log(log_tag, "Failed to create UbuntuPerformanceBooster: %s", e.what());
        the_log()->log(log_tag, "Falling back to NullPerformanceBooster");

        performance_booster = std::make_shared<NullPerformanceBooster>();
    }

    return performance_booster;
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

std::shared_ptr<repowerd::PowerSource>
repowerd::DefaultDaemonConfig::the_power_source()
{
    if (!power_source)
    {
        power_source = std::make_shared<UPowerPowerSource>(
            the_log(), *the_device_config(), the_dbus_bus_address());
    }

    return power_source;
}

std::shared_ptr<repowerd::ProximitySensor>
repowerd::DefaultDaemonConfig::the_proximity_sensor()
{
    if (!proximity_sensor)
    try
    {
        proximity_sensor = std::make_shared<UbuntuProximitySensor>(
            the_log(),
            *the_device_quirks());
    }
    catch (std::exception const& e)
    {
        the_log()->log(log_tag, "Failed to create UbuntuProximitySensor: %s", e.what());
        the_log()->log(log_tag, "Falling back to NullProximitySensor");

        proximity_sensor = std::make_shared<NullProximitySensor>();
    }

    return proximity_sensor;
}

std::shared_ptr<repowerd::SessionTracker>
repowerd::DefaultDaemonConfig::the_session_tracker()
{
    if (!session_tracker)
    {
        session_tracker = std::make_shared<LogindSessionTracker>(
            the_filesystem(), the_log(), *the_device_quirks(), the_dbus_bus_address());
    }

    return session_tracker;
}

std::shared_ptr<repowerd::ShutdownControl>
repowerd::DefaultDaemonConfig::the_shutdown_control()
{
    if (!shutdown_control)
        shutdown_control = std::make_shared<SystemShutdownControl>(the_log());
    return shutdown_control;
}

std::shared_ptr<repowerd::StateMachineFactory>
repowerd::DefaultDaemonConfig::the_state_machine_factory()
{
    if (!state_machine_factory)
        state_machine_factory = std::make_shared<DefaultStateMachineFactory>(*this);
    return state_machine_factory;
}

std::shared_ptr<repowerd::SystemPowerControl>
repowerd::DefaultDaemonConfig::the_system_power_control()
{
    if (!system_power_control)
    {
        system_power_control = std::make_shared<LibsuspendSystemPowerControl>(
            the_log());
    }
    return system_power_control;
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
repowerd::DefaultDaemonConfig::notification_expiration_timeout()
{
    return 60s;
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
    return 5s;
}

std::chrono::milliseconds
repowerd::DefaultDaemonConfig::user_inactivity_reduced_display_off_timeout()
{
    return 10s;
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
        catch (std::exception const& e)
        {
            the_log()->log(log_tag, "Failed to create AndroidBacklight: %s", e.what());
            the_log()->log(log_tag, "Trying SysfsBacklight");
        }

        try
        {
            if (!backlight)
            {
                backlight = std::make_shared<SysfsBacklight>(
                    the_log(), the_filesystem());
            }
        }
        catch (std::exception const& e)
        {
            the_log()->log(log_tag, "Failed to create SyfsBacklight: %s", e.what());
            throw std::runtime_error("Failed to create backlight");
        }
    }

    return backlight;
}

std::shared_ptr<repowerd::BacklightBrightnessControl>
repowerd::DefaultDaemonConfig::the_backlight_brightness_control()
{
    if (!backlight_brightness_control)
    {
        auto const ab_log_env_cstr = getenv("REPOWERD_LOG_AUTOBRIGHTNESS");
        std::string const ab_log_env{ab_log_env_cstr ? ab_log_env_cstr : ""};

        auto const ab_log = ab_log_env.empty() ? std::make_shared<NullLog>() : the_log();

        backlight_brightness_control = std::make_shared<BacklightBrightnessControl>(
            the_backlight(),
            the_light_sensor(),
            std::make_shared<AndroidAutobrightnessAlgorithm>(*the_device_config(), ab_log), 
            the_chrono(),
            the_log(),
            *the_device_config(),
            *the_device_quirks());
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
    catch (std::exception const& e)
    {
        the_log()->log(log_tag, "Failed to create BrightnessNotification: %s", e.what());
        the_log()->log(log_tag, "Falling back to NullBrightnessNotification");
        brightness_notification = std::make_shared<NullBrightnessNotification>();
    }

    return brightness_notification;
}

std::shared_ptr<repowerd::Chrono>
repowerd::DefaultDaemonConfig::the_chrono()
{
    if (!chrono)
        chrono = std::make_shared<RealChrono>();

    return chrono;
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
            the_log(),
            the_filesystem(),
            std::vector<std::string>{
                POWERD_DEVICE_CONFIGS_PATH, REPOWERD_DEVICE_CONFIGS_PATH});
    }

    return device_config;
}

std::shared_ptr<repowerd::DeviceQuirks>
repowerd::DefaultDaemonConfig::the_device_quirks()
{
    if (!device_quirks)
        device_quirks = std::make_shared<AndroidDeviceQuirks>(*the_log());

    return device_quirks;
}

std::shared_ptr<repowerd::Filesystem>
repowerd::DefaultDaemonConfig::the_filesystem()
{
    if (!filesystem)
        filesystem = std::make_shared<RealFilesystem>();

    return filesystem;
}

std::shared_ptr<repowerd::LightSensor>
repowerd::DefaultDaemonConfig::the_light_sensor()
{
    if (!light_sensor)
    try
    {
        light_sensor = std::make_shared<UbuntuLightSensor>();
    }
    catch (std::exception const& e)
    {
        the_log()->log(log_tag, "Failed to create UbuntuLightSensor: %s", e.what());
        the_log()->log(log_tag, "Falling back to NullLightSensor");
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
            the_brightness_notification(),
            the_log(),
            the_system_power_control(),
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
        wakeup_service = std::make_shared<DevAlarmWakeupService>(the_filesystem());
    }
    catch (std::exception const& e)
    {
        the_log()->log(log_tag, "Failed to create DevAlarmWakeupService: %s", e.what());
        the_log()->log(log_tag, "Falling back to NullWakeupService");
        wakeup_service = std::make_shared<NullWakeupService>();
    }

    return wakeup_service;
}
