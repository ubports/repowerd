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

#include "dbus_bus.h"
#include "dbus_client.h"
#include "fake_device_quirks.h"
#include "fake_log.h"
#include "fake_shared.h"
#include "fake_logind.h"
#include "wait_condition.h"

#include "src/adapters/logind_session_tracker.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <chrono>

namespace rt = repowerd::test;
using namespace testing;
using namespace std::chrono_literals;

namespace
{

struct ALogindSessionTracker : testing::Test
{
    enum class WithQuirk { none, ignore_session_deactivation };

    ALogindSessionTracker()
    {
        fake_logind.add_session(session_id(0), "mir", session_pid(0));
        fake_logind.add_session(session_id(1), "mir", session_pid(1));
        fake_logind.activate_session(session_id(0));

        use_logind_session_tracker(WithQuirk::none);
    }

    void use_logind_session_tracker(WithQuirk quirk)
    {
        registrations.clear();
        active_session_history.clear();
        removed_session_history.clear();

        rt::FakeDeviceQuirks fake_device_quirks;
        if (quirk == WithQuirk::ignore_session_deactivation)
            fake_device_quirks.set_ignore_session_deactivation(true);

        logind_session_tracker =
            std::make_unique<repowerd::LogindSessionTracker>(
                rt::fake_shared(fake_log),
                fake_device_quirks,
                bus.address());

        registrations.push_back(
            logind_session_tracker->register_active_session_changed_handler(
                [this] (std::string const& session_id, repowerd::SessionType type)
                {
                    {
                        std::lock_guard<std::mutex> lock{session_mutex};
                        active_session_history.emplace_back(session_id, type);
                    }
                    session_cv.notify_all();

                    mock_handlers.active_session_changed(session_id, type);
                }));

        registrations.push_back(
            logind_session_tracker->register_session_removed_handler(
                [this] (std::string const& session_id)
                {
                    {
                        std::lock_guard<std::mutex> lock{session_mutex};
                        removed_session_history.push_back(session_id);
                    }
                    session_cv.notify_all();

                    mock_handlers.session_removed(session_id);
                }));


        logind_session_tracker->start_processing();
    }

    std::string session_id(int i)
    {
        return "session" + std::to_string(i);
    }

    pid_t session_pid(int i)
    {
        return 1000 + i;
    }

    void wait_until_active_session_is(std::string const& session_id)
    {
        std::unique_lock<std::mutex> lock{session_mutex};
        auto const success = session_cv.wait_for(
            lock, default_timeout,
            [&]
            {
                return !active_session_history.empty() &&
                       active_session_history.back().first == session_id;
            });
        if (!success)
            throw std::runtime_error{
                "Timeout while waiting for " + session_id + " to become the active session"};
    }

    void wait_until_active_session_is(
        std::string const& session_id, repowerd::SessionType session_type)
    {
        std::unique_lock<std::mutex> lock{session_mutex};
        auto const success = session_cv.wait_for(
            lock, default_timeout,
            [&]
            {
                return !active_session_history.empty() &&
                       active_session_history.back().first == session_id &&
                       active_session_history.back().second == session_type;
            });
        if (!success)
            throw std::runtime_error{
                "Timeout while waiting for " + session_id + " to become the active session"};
    }

    void wait_until_activated_sessions_are(std::vector<std::string> const& activated)
    {
        std::unique_lock<std::mutex> lock{session_mutex};
        auto const success = session_cv.wait_for(
            lock, default_timeout,
            [&]
            {
                std::vector<std::string> active_session_id_history;
                
                for (auto const& s : active_session_history)
                    active_session_id_history.push_back(s.first);

                return activated == active_session_id_history;
            });
        if (!success)
            throw std::runtime_error{
                "Timeout while waiting for activated sessions"};
    }

    void wait_until_removed_sessions_are(std::vector<std::string> const& removed)
    {
        std::unique_lock<std::mutex> lock{session_mutex};
        auto const success = session_cv.wait_for(
            lock, default_timeout,
            [&]
            {
                return removed_session_history == removed;
            });
        if (!success)
            throw std::runtime_error{
                "Timeout while waiting for removed sessions"};
    }

    struct MockHandlers
    {
        MOCK_METHOD2(active_session_changed, void(std::string const&, repowerd::SessionType));
        MOCK_METHOD1(session_removed, void(std::string const&));
    };
    testing::NiceMock<MockHandlers> mock_handlers;

    rt::DBusBus bus;
    rt::FakeLog fake_log;
    std::unique_ptr<repowerd::LogindSessionTracker> logind_session_tracker;
    rt::FakeLogind fake_logind{bus.address()};
    std::vector<repowerd::HandlerRegistration> registrations;

    std::chrono::seconds const default_timeout{3};

    std::mutex session_mutex;
    std::condition_variable session_cv;
    std::vector<std::pair<std::string,repowerd::SessionType>> active_session_history;
    std::vector<std::string> removed_session_history;
};

}

TEST_F(ALogindSessionTracker, notifies_of_active_session_at_startup)
{
    wait_until_active_session_is(session_id(0));
}

TEST_F(ALogindSessionTracker, notifies_of_change_in_active_session)
{
    fake_logind.activate_session(session_id(1));

    wait_until_active_session_is(session_id(1));
}

TEST_F(ALogindSessionTracker, notifies_of_removal_of_tracked_session)
{
    fake_logind.activate_session(session_id(1));

    wait_until_active_session_is(session_id(1));

    fake_logind.remove_session(session_id(0));
    fake_logind.remove_session(session_id(1));

    wait_until_removed_sessions_are({session_id(0), session_id(1)});
}

TEST_F(ALogindSessionTracker, does_not_notify_of_removal_of_untracked_session)
{
    wait_until_active_session_is(session_id(0));

    fake_logind.remove_session(session_id(1));
    fake_logind.remove_session(session_id(0));

    wait_until_removed_sessions_are({session_id(0)});
}

TEST_F(ALogindSessionTracker, notifies_of_session_deactivation)
{
    fake_logind.deactivate_session();

    wait_until_active_session_is(
        repowerd::invalid_session_id, repowerd::SessionType::RepowerdIncompatible);
}

TEST_F(ALogindSessionTracker, marks_mir_sessions_as_repowerd_compatible)
{
    fake_logind.add_session(session_id(2), "mir", session_pid(2));
    fake_logind.activate_session(session_id(2));

    wait_until_active_session_is(session_id(2), repowerd::SessionType::RepowerdCompatible);
}

TEST_F(ALogindSessionTracker, marks_non_mir_sessions_as_repowerd_incompatible)
{
    fake_logind.add_session(session_id(2), "x11", session_pid(2));
    fake_logind.activate_session(session_id(2));

    wait_until_active_session_is(session_id(2), repowerd::SessionType::RepowerdIncompatible);
}

TEST_F(ALogindSessionTracker, returns_session_id_for_pid_of_tracked_session)
{
    fake_logind.activate_session(session_id(1));

    EXPECT_THAT(logind_session_tracker->session_for_pid(session_pid(0)),
                StrEq(session_id(0)));
    EXPECT_THAT(logind_session_tracker->session_for_pid(session_pid(1)),
                StrEq(session_id(1)));
}

TEST_F(ALogindSessionTracker, returns_invalid_session_id_for_pid_of_untracked_session)
{
    EXPECT_THAT(logind_session_tracker->session_for_pid(session_pid(1)),
                StrEq(repowerd::invalid_session_id));
}

TEST_F(ALogindSessionTracker, notifies_of_same_session_activation_after_deactivation)
{
    fake_logind.deactivate_session();
    fake_logind.activate_session(session_id(0));

    wait_until_activated_sessions_are(
        {session_id(0), repowerd::invalid_session_id, session_id(0)});
}

TEST_F(ALogindSessionTracker, does_not_notify_of_same_session_activation)
{
    fake_logind.activate_session(session_id(0));
    fake_logind.activate_session(session_id(0));
    fake_logind.activate_session(session_id(1));
    fake_logind.activate_session(session_id(1));

    wait_until_activated_sessions_are({session_id(0), session_id(1)});
}

TEST_F(ALogindSessionTracker, with_quirk_does_not_notify_of_session_deactivation)
{
    use_logind_session_tracker(WithQuirk::ignore_session_deactivation);

    fake_logind.deactivate_session();
    fake_logind.activate_session(session_id(1));

    wait_until_activated_sessions_are({session_id(0), session_id(1)});
}

TEST_F(ALogindSessionTracker,
       with_quirk_does_not_notify_of_same_session_activation_after_deactivation)
{
    use_logind_session_tracker(WithQuirk::ignore_session_deactivation);

    fake_logind.deactivate_session();
    fake_logind.activate_session(session_id(0));
    fake_logind.activate_session(session_id(1));

    wait_until_activated_sessions_are({session_id(0), session_id(1)});
}

TEST_F(ALogindSessionTracker, logs_active_session_at_startup)
{
    EXPECT_TRUE(fake_log.contains_line({"track_session", session_id(0), "mir"}));
    EXPECT_TRUE(fake_log.contains_line({"activate_session", session_id(0)}));
}

TEST_F(ALogindSessionTracker, logs_change_in_active_session)
{
    fake_logind.add_session(session_id(2), "x11", session_pid(2));
    fake_logind.activate_session(session_id(2));

    wait_until_active_session_is(session_id(2));

    EXPECT_TRUE(fake_log.contains_line({"track_session", session_id(2), "x11"}));
    EXPECT_TRUE(fake_log.contains_line({"activate_session", session_id(2)}));
}

TEST_F(ALogindSessionTracker, logs_session_deactivation)
{
    fake_logind.deactivate_session();
    wait_until_active_session_is(repowerd::invalid_session_id);

    EXPECT_TRUE(fake_log.contains_line({"deactivate_session"}));
}

TEST_F(ALogindSessionTracker, logs_tracked_session_removal)
{
    wait_until_active_session_is(session_id(0));

    fake_logind.remove_session(session_id(1));
    fake_logind.remove_session(session_id(0));

    wait_until_removed_sessions_are({session_id(0)});

    EXPECT_TRUE(fake_log.contains_line({"remove_session", session_id(0)}));
    EXPECT_FALSE(fake_log.contains_line({"remove_session", session_id(1)}));
}
