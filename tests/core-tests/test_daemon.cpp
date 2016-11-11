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

#include "daemon_config.h"
#include "run_daemon.h"
#include "fake_client_requests.h"
#include "fake_notification_service.h"
#include "fake_power_button.h"
#include "fake_power_source.h"
#include "fake_proximity_sensor.h"
#include "fake_session_tracker.h"
#include "fake_timer.h"
#include "fake_user_activity.h"
#include "fake_voice_call_service.h"
#include "mock_brightness_control.h"

#include "src/core/daemon.h"
#include "src/core/state_machine.h"
#include "src/core/state_machine_factory.h"

#include <thread>

#include <gmock/gmock.h>

namespace rt = repowerd::test;

using namespace testing;
using namespace std::chrono_literals;

namespace
{

struct MockStateMachine : public repowerd::StateMachine
{
    MOCK_METHOD1(handle_alarm, void(repowerd::AlarmId));

    MOCK_METHOD0(handle_active_call, void());
    MOCK_METHOD0(handle_no_active_call, void());

    MOCK_METHOD0(handle_no_notification, void());
    MOCK_METHOD0(handle_notification, void());

    MOCK_METHOD0(handle_power_button_press, void());
    MOCK_METHOD0(handle_power_button_release, void());

    MOCK_METHOD0(handle_power_source_change, void());
    MOCK_METHOD0(handle_power_source_critical, void());

    MOCK_METHOD0(handle_proximity_far, void());
    MOCK_METHOD0(handle_proximity_near, void());

    MOCK_METHOD0(handle_enable_inactivity_timeout, void());
    MOCK_METHOD0(handle_disable_inactivity_timeout, void());
    MOCK_METHOD1(handle_set_inactivity_timeout, void(std::chrono::milliseconds));

    MOCK_METHOD0(handle_turn_on_display, void());

    MOCK_METHOD0(handle_user_activity_extending_power_state, void());
    MOCK_METHOD0(handle_user_activity_changing_power_state, void());

    MOCK_METHOD1(handle_set_normal_brightness_value, void(double));
    MOCK_METHOD0(handle_enable_autobrightness, void());
    MOCK_METHOD0(handle_disable_autobrightness, void());
};

struct MockStateMachineFactory : public repowerd::StateMachineFactory
{
    std::shared_ptr<repowerd::StateMachine> create_state_machine(std::string const&)
    {
        if (mock_state_machine)
            mock_state_machines.push_back(std::move(mock_state_machine));
        else
            mock_state_machines.push_back(std::make_shared<MockStateMachine>());
        return mock_state_machines.back();
    }

    std::shared_ptr<MockStateMachine> the_mock_state_machine()
    {
        if (!mock_state_machines.empty())
            return mock_state_machines.back();
        else if (mock_state_machine)
            return mock_state_machine;
        else
            return mock_state_machine = std::make_shared<MockStateMachine>();
    }

    std::shared_ptr<MockStateMachine> the_mock_state_machine(int index)
    {
        return mock_state_machines.at(index);
    }

    std::shared_ptr<MockStateMachine> mock_state_machine;
    std::vector<std::shared_ptr<MockStateMachine>> mock_state_machines;
};

struct DaemonConfigWithMockStateMachine : rt::DaemonConfig
{
    std::shared_ptr<repowerd::StateMachineFactory> the_state_machine_factory() override
    {
        if (!mock_state_machine_factory)
            mock_state_machine_factory = std::make_shared<MockStateMachineFactory>();
        return mock_state_machine_factory;
    }

    std::shared_ptr<MockStateMachine> the_mock_state_machine()
    {
        the_state_machine_factory();
        return mock_state_machine_factory->the_mock_state_machine();
    }

    std::shared_ptr<MockStateMachine> the_mock_state_machine(int index)
    {
        return mock_state_machine_factory->the_mock_state_machine(index);
    }

    std::shared_ptr<MockStateMachineFactory> mock_state_machine_factory;
};

struct ADaemon : testing::Test
{
    DaemonConfigWithMockStateMachine config;
    std::unique_ptr<repowerd::Daemon> daemon;
    std::thread daemon_thread;

    ~ADaemon()
    {
        if (daemon_thread.joinable())
            stop_daemon();
    }

    void start_daemon()
    {
        start_daemon_with_config(config);
    }

    void start_daemon_with_config(repowerd::DaemonConfig& config)
    {
        daemon = std::make_unique<repowerd::Daemon>(config);
        daemon_thread = rt::run_daemon(*daemon);
    }

    void stop_daemon()
    {
        daemon->flush();
        daemon->stop();
        daemon_thread.join();
    }

    void flush_daemon()
    {
        daemon->flush();
    }

    void start_daemon_with_second_session_active()
    {
        start_daemon();
        add_session("s1", repowerd::SessionType::RepowerdCompatible, 42);
        switch_to_session("s1");
    }

    void add_session(std::string const& session_id, repowerd::SessionType type, pid_t pid)
    {
        config.the_fake_session_tracker()->add_session(session_id, type, pid);
        daemon->flush();
    }

    void switch_to_session(std::string const& session_id)
    {
        config.the_fake_session_tracker()->switch_to_session(session_id);
        daemon->flush();
    }

    void remove_session(std::string const& session_id)
    {
        config.the_fake_session_tracker()->remove_session(session_id);
        daemon->flush();
    }
};

}

TEST_F(ADaemon, registers_and_unregisters_timer_alarm_handler)
{
    EXPECT_CALL(config.the_fake_timer()->mock, register_alarm_handler(_));
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_timer().get());

    EXPECT_CALL(config.the_fake_timer()->mock, unregister_alarm_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_timer().get());
}

TEST_F(ADaemon, notifies_state_machine_of_timer_alarm)
{
    start_daemon();

    auto const alarm_id = config.the_fake_timer()->schedule_alarm_in(1s);

    EXPECT_CALL(*config.the_mock_state_machine(), handle_alarm(alarm_id));

    config.the_fake_timer()->advance_by(1s);
}

TEST_F(ADaemon, registers_starts_and_unregisters_power_button_handler)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_power_button()->mock, register_power_button_handler(_));
    EXPECT_CALL(config.the_fake_power_button()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_power_button().get());

    EXPECT_CALL(config.the_fake_power_button()->mock, unregister_power_button_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_power_button().get());
}

TEST_F(ADaemon, notifies_state_machine_of_power_button_press)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_power_button_press());
    config.the_fake_power_button()->press();
}

TEST_F(ADaemon, notifies_state_machine_of_power_button_release)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_power_button_release());
    config.the_fake_power_button()->release();
}

TEST_F(ADaemon, registers_starts_and_unregisters_user_activity_handler)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_user_activity()->mock, register_user_activity_handler(_));
    EXPECT_CALL(config.the_fake_user_activity()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_user_activity().get());

    EXPECT_CALL(config.the_fake_user_activity()->mock, unregister_user_activity_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_user_activity().get());
}

TEST_F(ADaemon, notifies_state_machine_of_user_activity_changing_power_state)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_user_activity_changing_power_state());

    config.the_fake_user_activity()->perform(repowerd::UserActivityType::change_power_state);
}

TEST_F(ADaemon, notifies_state_machine_of_user_activity_extending_power_state)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_user_activity_extending_power_state());

    config.the_fake_user_activity()->perform(repowerd::UserActivityType::extend_power_state);
}

TEST_F(ADaemon, registers_starts_and_unregisters_proximity_handler)
{
    EXPECT_CALL(config.the_fake_proximity_sensor()->mock, register_proximity_handler(_));
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_proximity_sensor().get());

    EXPECT_CALL(config.the_fake_proximity_sensor()->mock, unregister_proximity_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_proximity_sensor().get());
}

TEST_F(ADaemon, notifies_state_machine_of_proximity_far)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_proximity_far());

    config.the_fake_proximity_sensor()->emit_proximity_state(repowerd::ProximityState::far);
}

TEST_F(ADaemon, notifies_state_machine_of_proximity_near)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_proximity_near());

    config.the_fake_proximity_sensor()->emit_proximity_state(repowerd::ProximityState::near);
}

TEST_F(ADaemon, registers_starts_and_unregisters_enable_inactivity_timeout_handler)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_client_requests()->mock, register_enable_inactivity_timeout_handler(_));
    EXPECT_CALL(config.the_fake_client_requests()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_client_requests().get());

    EXPECT_CALL(config.the_fake_client_requests()->mock, unregister_enable_inactivity_timeout_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_client_requests().get());
}

TEST_F(ADaemon, notifies_state_machine_of_enable_inactivity_timeout)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_enable_inactivity_timeout());

    config.the_fake_client_requests()->emit_enable_inactivity_timeout("id");
}

TEST_F(ADaemon, notifies_inactive_state_machine_of_enable_inactivity_timeout)
{
    start_daemon_with_second_session_active();

    EXPECT_CALL(*config.the_mock_state_machine(0), handle_enable_inactivity_timeout());
    EXPECT_CALL(*config.the_mock_state_machine(1), handle_enable_inactivity_timeout()).Times(0);
    config.the_fake_client_requests()->emit_enable_inactivity_timeout("id");
}

TEST_F(ADaemon, registers_and_unregisters_disable_inactivity_timeout_handler)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_client_requests()->mock, register_disable_inactivity_timeout_handler(_));
    EXPECT_CALL(config.the_fake_client_requests()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_client_requests().get());

    EXPECT_CALL(config.the_fake_client_requests()->mock, unregister_disable_inactivity_timeout_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_client_requests().get());
}

TEST_F(ADaemon, notifies_state_machine_of_disable_inactivity_timeout)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_disable_inactivity_timeout());

    config.the_fake_client_requests()->emit_disable_inactivity_timeout("id");
}

TEST_F(ADaemon, notifies_inactive_state_machine_of_disable_inactivity_timeout)
{
    start_daemon_with_second_session_active();

    EXPECT_CALL(*config.the_mock_state_machine(0), handle_disable_inactivity_timeout());
    EXPECT_CALL(*config.the_mock_state_machine(1), handle_disable_inactivity_timeout()).Times(0);
    config.the_fake_client_requests()->emit_disable_inactivity_timeout("id");
}

TEST_F(ADaemon, registers_and_unregisters_set_inactivity_timeout_handler)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_client_requests()->mock, register_set_inactivity_timeout_handler(_));
    EXPECT_CALL(config.the_fake_client_requests()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_client_requests().get());

    EXPECT_CALL(config.the_fake_client_requests()->mock, unregister_set_inactivity_timeout_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_client_requests().get());
}

TEST_F(ADaemon, notifies_state_machine_of_set_inactivity_timeout)
{
    start_daemon();

    auto const timeout = 10000ms;
    EXPECT_CALL(*config.the_mock_state_machine(), handle_set_inactivity_timeout(timeout));

    config.the_fake_client_requests()->emit_set_inactivity_timeout(timeout);
}

TEST_F(ADaemon, notifies_inactive_state_machine_of_set_inactivity_timeout)
{
    start_daemon_with_second_session_active();

    auto const timeout = 10000ms;
    EXPECT_CALL(*config.the_mock_state_machine(0), handle_set_inactivity_timeout(timeout));
    EXPECT_CALL(*config.the_mock_state_machine(1), handle_set_inactivity_timeout(_)).Times(0);
    config.the_fake_client_requests()->emit_set_inactivity_timeout(timeout);
}

TEST_F(ADaemon, registers_and_unregisters_disable_autobrightness)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_client_requests()->mock, register_disable_autobrightness_handler(_));
    EXPECT_CALL(config.the_fake_client_requests()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_client_requests().get());

    EXPECT_CALL(config.the_fake_client_requests()->mock, unregister_disable_autobrightness_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_client_requests().get());
}

TEST_F(ADaemon, notifies_state_machine_of_disable_autobrightness)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_disable_autobrightness());

    config.the_fake_client_requests()->emit_disable_autobrightness();
}

TEST_F(ADaemon, notifies_inactive_state_machine_of_disable_autobrightness)
{
    start_daemon_with_second_session_active();

    EXPECT_CALL(*config.the_mock_state_machine(0), handle_disable_autobrightness());
    EXPECT_CALL(*config.the_mock_state_machine(1), handle_disable_autobrightness()).Times(0);
    config.the_fake_client_requests()->emit_disable_autobrightness();
}

TEST_F(ADaemon, registers_and_unregisters_enable_autobrightness)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_client_requests()->mock, register_enable_autobrightness_handler(_));
    EXPECT_CALL(config.the_fake_client_requests()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_client_requests().get());

    EXPECT_CALL(config.the_fake_client_requests()->mock, unregister_enable_autobrightness_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_client_requests().get());
}

TEST_F(ADaemon, notifies_state_machine_of_enable_autobrightness)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_enable_autobrightness());

    config.the_fake_client_requests()->emit_enable_autobrightness();
}

TEST_F(ADaemon, notifies_inactive_state_machine_of_enable_autobrightness)
{
    start_daemon_with_second_session_active();

    EXPECT_CALL(*config.the_mock_state_machine(0), handle_enable_autobrightness());
    EXPECT_CALL(*config.the_mock_state_machine(1), handle_enable_autobrightness()).Times(0);
    config.the_fake_client_requests()->emit_enable_autobrightness();
}

TEST_F(ADaemon, registers_and_unregisters_set_normal_brightness_value)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_client_requests()->mock, register_set_normal_brightness_value_handler(_));
    EXPECT_CALL(config.the_fake_client_requests()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_client_requests().get());

    EXPECT_CALL(config.the_fake_client_requests()->mock, unregister_set_normal_brightness_value_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_client_requests().get());
}

TEST_F(ADaemon, notifies_state_machine_of_set_normal_brightness_value)
{
    start_daemon();

    auto const value = 0.7;
    EXPECT_CALL(*config.the_mock_state_machine(), handle_set_normal_brightness_value(value));

    config.the_fake_client_requests()->emit_set_normal_brightness_value(value);
}

TEST_F(ADaemon, notifies_inactive_state_machine_of_set_normal_brightness_value)
{
    start_daemon_with_second_session_active();

    auto const value = 0.7;
    EXPECT_CALL(*config.the_mock_state_machine(0), handle_set_normal_brightness_value(value));
    EXPECT_CALL(*config.the_mock_state_machine(1), handle_set_normal_brightness_value(_)).Times(0);

    config.the_fake_client_requests()->emit_set_normal_brightness_value(value);
}

TEST_F(ADaemon, registers_and_unregisters_notification_handler)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_notification_service()->mock, register_notification_handler(_));
    EXPECT_CALL(config.the_fake_notification_service()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_notification_service().get());

    EXPECT_CALL(config.the_fake_notification_service()->mock, unregister_notification_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_notification_service().get());
}

TEST_F(ADaemon, notifies_state_machine_of_notification)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_notification());

    config.the_fake_notification_service()->emit_notification("id");
}

TEST_F(ADaemon, notifies_inactive_state_machine_of_notification)
{
    start_daemon_with_second_session_active();

    EXPECT_CALL(*config.the_mock_state_machine(0), handle_notification());
    EXPECT_CALL(*config.the_mock_state_machine(1), handle_notification()).Times(0);

    config.the_fake_notification_service()->emit_notification("id");
}

TEST_F(ADaemon, registers_and_unregisters_no_notification_handler)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_notification_service()->mock, register_notification_done_handler(_));
    EXPECT_CALL(config.the_fake_notification_service()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_notification_service().get());

    EXPECT_CALL(config.the_fake_notification_service()->mock, unregister_notification_done_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_notification_service().get());
}

TEST_F(ADaemon, notifies_state_machine_of_no_notification)
{
    start_daemon();

    InSequence s;
    EXPECT_CALL(*config.the_mock_state_machine(), handle_notification());
    EXPECT_CALL(*config.the_mock_state_machine(), handle_no_notification());

    config.the_fake_notification_service()->emit_notification("id");
    config.the_fake_notification_service()->emit_notification_done("id");
}

TEST_F(ADaemon, notifies_inactive_state_machine_of_no_notification)
{
    start_daemon_with_second_session_active();

    InSequence s;
    EXPECT_CALL(*config.the_mock_state_machine(0), handle_notification());
    EXPECT_CALL(*config.the_mock_state_machine(0), handle_no_notification());
    EXPECT_CALL(*config.the_mock_state_machine(1), handle_no_notification()).Times(0);

    config.the_fake_notification_service()->emit_notification("id");
    config.the_fake_notification_service()->emit_notification_done("id");
}

TEST_F(ADaemon, notifies_inactive_state_machine_of_no_notification_if_notification_was_sent_while_active)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(0), handle_notification());
    config.the_fake_notification_service()->emit_notification("id");
    flush_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_voice_call_service().get());

    add_session("s1", repowerd::SessionType::RepowerdCompatible, 42);
    switch_to_session("s1");

    InSequence s;
    EXPECT_CALL(*config.the_mock_state_machine(0), handle_no_notification());
    EXPECT_CALL(*config.the_mock_state_machine(1), handle_no_notification()).Times(0);

    config.the_fake_notification_service()->emit_notification_done("id");
}

TEST_F(ADaemon, registers_and_unregisters_active_call_handler)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_voice_call_service()->mock, register_active_call_handler(_));
    EXPECT_CALL(config.the_fake_voice_call_service()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_voice_call_service().get());

    EXPECT_CALL(config.the_fake_voice_call_service()->mock, unregister_active_call_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_voice_call_service().get());
}

TEST_F(ADaemon, notifies_state_machine_of_active_call)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_active_call());

    config.the_fake_voice_call_service()->emit_active_call();
}

TEST_F(ADaemon, registers_and_unregisters_no_active_call_handler)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_voice_call_service()->mock, register_no_active_call_handler(_));
    EXPECT_CALL(config.the_fake_voice_call_service()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_voice_call_service().get());

    EXPECT_CALL(config.the_fake_voice_call_service()->mock, unregister_no_active_call_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_voice_call_service().get());
}

TEST_F(ADaemon, notifies_state_machine_of_no_active_call)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_no_active_call());

    config.the_fake_voice_call_service()->emit_no_active_call();
}

TEST_F(ADaemon, does_not_turn_on_display_at_startup_if_not_configured)
{
    EXPECT_CALL(*config.the_mock_state_machine(), handle_turn_on_display()).Times(0);

    start_daemon();
}

TEST_F(ADaemon, turns_on_display_at_startup_if_configured)
{
    struct DaemonConfigWithTurnOnDisplay : DaemonConfigWithMockStateMachine
    {
        bool turn_on_display_at_startup() override { return true; }
    };
    DaemonConfigWithTurnOnDisplay config_with_turn_on_display;

    EXPECT_CALL(*config_with_turn_on_display.the_mock_state_machine(),
                handle_turn_on_display());

    start_daemon_with_config(config_with_turn_on_display);
}

TEST_F(ADaemon, registers_and_unregisters_power_source_change_handler)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_power_source()->mock, register_power_source_change_handler(_));
    EXPECT_CALL(config.the_fake_power_source()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_power_source().get());

    EXPECT_CALL(config.the_fake_power_source()->mock, unregister_power_source_change_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_power_source().get());
}

TEST_F(ADaemon, notifies_state_machine_of_power_source_change)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_power_source_change());

    config.the_fake_power_source()->emit_power_source_change();
}

TEST_F(ADaemon, registers_and_unregisters_power_source_critical_handler)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_power_source()->mock, register_power_source_critical_handler(_));
    EXPECT_CALL(config.the_fake_power_source()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_power_source().get());

    EXPECT_CALL(config.the_fake_power_source()->mock, unregister_power_source_critical_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_power_source().get());
}

TEST_F(ADaemon, notifies_state_machine_of_power_source_critical)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_power_source_critical());

    config.the_fake_power_source()->emit_power_source_critical();
}

TEST_F(ADaemon, registers_and_unregisters_active_session_changed_handler)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_session_tracker()->mock,
                register_active_session_changed_handler(_));
    EXPECT_CALL(config.the_fake_session_tracker()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_session_tracker().get());

    EXPECT_CALL(config.the_fake_session_tracker()->mock,
                unregister_active_session_changed_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_session_tracker().get());
}

TEST_F(ADaemon, registers_and_unregisters_session_removed_handler)
{
    InSequence s;
    EXPECT_CALL(config.the_fake_session_tracker()->mock, register_session_removed_handler(_));
    EXPECT_CALL(config.the_fake_session_tracker()->mock, start_processing());
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_session_tracker().get());

    EXPECT_CALL(config.the_fake_session_tracker()->mock, unregister_session_removed_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_session_tracker().get());
}

TEST_F(ADaemon, starts_session_tracker_processing_before_per_session_components)
{
    Expectation session = EXPECT_CALL(config.the_fake_session_tracker()->mock, start_processing());
    EXPECT_CALL(config.the_fake_client_requests()->mock, start_processing())
        .After(session);
    EXPECT_CALL(config.the_fake_notification_service()->mock, start_processing())
        .After(session);

    start_daemon();
}

TEST_F(ADaemon, makes_null_session_active_if_active_is_removed)
{
    start_daemon();

    remove_session(
        config.the_fake_session_tracker()->default_session());

    EXPECT_CALL(*config.the_mock_state_machine(), handle_power_button_press()).Times(0);
    config.the_fake_power_button()->press();
}
