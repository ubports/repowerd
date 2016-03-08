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
#include "fake_power_button.h"
#include "fake_timer.h"

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
    MOCK_METHOD0(handle_power_key_press, void());
    MOCK_METHOD0(handle_power_key_release, void());
    MOCK_METHOD1(handle_alarm, void(repowerd::AlarmId));
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

TEST_F(ADaemon, sets_and_clears_timer_alarm_handler)
{
    using namespace testing;

    EXPECT_CALL(config.the_fake_timer()->mock, set_alarm_handler(_));
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_timer().get());

    EXPECT_CALL(config.the_fake_timer()->mock, clear_alarm_handler());
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

TEST_F(ADaemon, sets_and_clears_power_button_handler)
{
    using namespace testing;

    EXPECT_CALL(config.the_fake_power_button()->mock, set_power_button_handler(_));
    start_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_power_button().get());

    EXPECT_CALL(config.the_fake_power_button()->mock, clear_power_button_handler());
    stop_daemon();
    testing::Mock::VerifyAndClearExpectations(config.the_fake_power_button().get());
}

TEST_F(ADaemon, notifies_state_machine_of_power_button_press)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_power_key_press());
    config.the_fake_power_button()->press();
}

TEST_F(ADaemon, notifies_state_machine_of_power_button_release)
{
    start_daemon();

    EXPECT_CALL(*config.the_mock_state_machine(), handle_power_key_release());
    config.the_fake_power_button()->release();
}
