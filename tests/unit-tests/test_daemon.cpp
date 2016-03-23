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
#include "fake_client_requests.h"
#include "fake_notification_service.h"
#include "fake_power_button.h"
#include "fake_proximity_sensor.h"
#include "fake_timer.h"
#include "fake_user_activity.h"

#include "src/daemon.h"
#include "src/state_machine.h"

#include <thread>

#include <gmock/gmock.h>

namespace rt = repowerd::test;

using namespace std::chrono_literals;

namespace
{

struct MockStateMachine : public repowerd::StateMachine
{
    MOCK_METHOD1(handle_alarm, void(repowerd::AlarmId));

    MOCK_METHOD0(handle_all_notifications_done, void());
    MOCK_METHOD0(handle_notification, void());

    MOCK_METHOD0(handle_power_button_press, void());
    MOCK_METHOD0(handle_power_button_release, void());

    MOCK_METHOD0(handle_proximity_far, void());
    MOCK_METHOD0(handle_proximity_near, void());

    MOCK_METHOD0(handle_enable_inactivity_timeout, void());
    MOCK_METHOD0(handle_disable_inactivity_timeout, void());

    MOCK_METHOD0(handle_user_activity_extending_power_state, void());
    MOCK_METHOD0(handle_user_activity_changing_power_state, void());
};

struct DaemonConfigWithMockStateMachine : rt::DaemonConfig
{
    std::shared_ptr<repowerd::StateMachine> the_state_machine() override
    {
        return the_mock_state_machine();
    }

    std::shared_ptr<MockStateMachine> the_mock_state_machine()
    {
        if (!mock_state_machine)
            mock_state_machine = std::make_shared<MockStateMachine>();
        return mock_state_machine;
    }

    std::shared_ptr<MockStateMachine> mock_state_machine;
};

struct ADaemon : testing::Test
{
    DaemonConfigWithMockStateMachine config;
    repowerd::Daemon daemon{config};
    std::thread daemon_thread;

    ~ADaemon()
    {
        if (daemon_thread.joinable())
            stop_daemon();
    }

    void start_daemon()
    {
        daemon_thread = std::thread{ [this] { daemon.run(); }};
        daemon.flush();
    }

    void stop_daemon()
    {
        daemon.flush();
        daemon.stop();
        daemon_thread.join();
    }
};

}

TEST_F(ADaemon, registers_and_unregisters_timer_alarm_handler)
{
    using namespace testing;

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

TEST_F(ADaemon, registers_and_unregisters_power_button_handler)
{
    using namespace testing;

    EXPECT_CALL(config.the_fake_power_button()->mock, register_power_button_handler(_));
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

TEST_F(ADaemon, registers_and_unregisters_user_activity_handler)
{
    using namespace testing;

    EXPECT_CALL(config.the_fake_user_activity()->mock, register_user_activity_handler(_));
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_user_activity().get());

    EXPECT_CALL(config.the_fake_user_activity()->mock, unregister_user_activity_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_user_activity().get());
}

TEST_F(ADaemon, notifies_state_machine_of_user_activity_changing_power_state)
{
    using namespace testing;

    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_user_activity_changing_power_state());

    config.the_fake_user_activity()->perform(repowerd::UserActivityType::change_power_state);
}

TEST_F(ADaemon, notifies_state_machine_of_user_activity_extending_power_state)
{
    using namespace testing;

    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_user_activity_extending_power_state());

    config.the_fake_user_activity()->perform(repowerd::UserActivityType::extend_power_state);
}

TEST_F(ADaemon, registers_and_unregisters_proximity_handler)
{
    using namespace testing;

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

TEST_F(ADaemon, registers_and_unregisters_enable_inactivity_timeout_handler)
{
    using namespace testing;

    EXPECT_CALL(config.the_fake_client_requests()->mock, register_enable_inactivity_timeout_handler(_));
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

    config.the_fake_client_requests()->emit_enable_inactivity_timeout();
}

TEST_F(ADaemon, registers_and_unregisters_disable_inactivity_timeout_handler)
{
    using namespace testing;

    EXPECT_CALL(config.the_fake_client_requests()->mock, register_disable_inactivity_timeout_handler(_));
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

    config.the_fake_client_requests()->emit_disable_inactivity_timeout();
}

TEST_F(ADaemon, registers_and_unregisters_notification_handler)
{
    using namespace testing;

    EXPECT_CALL(config.the_fake_notification_service()->mock, register_notification_handler(_));
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

    config.the_fake_notification_service()->emit_notification();
}

TEST_F(ADaemon, registers_and_unregisters_all_notifications_done_handler)
{
    using namespace testing;

    EXPECT_CALL(config.the_fake_notification_service()->mock, register_all_notifications_done_handler(_));
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_notification_service().get());

    EXPECT_CALL(config.the_fake_notification_service()->mock, unregister_all_notifications_done_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_notification_service().get());
}

TEST_F(ADaemon, notifies_state_machine_of_all_notifications_done)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_all_notifications_done());

    config.the_fake_notification_service()->emit_all_notifications_done();
}
