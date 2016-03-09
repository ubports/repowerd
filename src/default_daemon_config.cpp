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
#include "default_state_machine.h"

#include "display_power_control.h"
#include "power_button.h"
#include "power_button_event_sink.h"
#include "timer.h"

namespace
{

struct NullDisplayPowerControl : repowerd::DisplayPowerControl
{
    void turn_on() override {}
    void turn_off() override {}
};

struct NullPowerButton : repowerd::PowerButton
{
    void set_power_button_handler(repowerd::PowerButtonHandler const&) override {}
    void clear_power_button_handler() override {}
};

struct NullPowerButtonEventSink : repowerd::PowerButtonEventSink
{
    void notify_long_press() override {}
};

struct NullTimer : repowerd::Timer
{
    void set_alarm_handler(repowerd::AlarmHandler const&) override {}
    void clear_alarm_handler() override {}
    repowerd::AlarmId schedule_alarm_in(std::chrono::milliseconds) override { return {}; };
};

}

std::shared_ptr<repowerd::DisplayPowerControl>
repowerd::DefaultDaemonConfig::the_display_power_control()
{
    if (!display_power_control)
        display_power_control = std::make_shared<NullDisplayPowerControl>();
    return display_power_control;
}

std::shared_ptr<repowerd::PowerButton>
repowerd::DefaultDaemonConfig::the_power_button()
{
    if (!power_button)
        power_button = std::make_shared<NullPowerButton>();
    return power_button;

}

std::shared_ptr<repowerd::PowerButtonEventSink>
repowerd::DefaultDaemonConfig::the_power_button_event_sink()
{
    if (!power_button_event_sink)
        power_button_event_sink = std::make_shared<NullPowerButtonEventSink>();
    return power_button_event_sink;

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
        timer = std::make_shared<NullTimer>();
    return timer;
}

std::chrono::milliseconds
repowerd::DefaultDaemonConfig::power_button_long_press_timeout()
{
    return std::chrono::seconds{2};
}

std::chrono::milliseconds
repowerd::DefaultDaemonConfig::user_inactivity_display_off_timeout()
{
    return std::chrono::seconds{60};
}
