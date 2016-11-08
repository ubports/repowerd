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

#include "fake_session_tracker.h"
#include "default_pid.h"
#include <algorithm>

namespace rt = repowerd::test;

namespace
{
auto const null_arg1_handler = [](auto){};
auto const null_arg2_handler = [](auto, auto){};
}

rt::FakeSessionTracker::FakeSessionTracker()
    : active_session_changed_handler{null_arg2_handler},
      session_removed_handler{null_arg1_handler}
{
}

void rt::FakeSessionTracker::start_processing()
{
    mock.start_processing();
    add_session(default_session(), SessionType::RepowerdCompatible, default_pid);
    switch_to_session(default_session());
}

repowerd::HandlerRegistration rt::FakeSessionTracker::register_active_session_changed_handler(
    ActiveSessionChangedHandler const& handler)
{
    mock.register_active_session_changed_handler(handler);
    this->active_session_changed_handler = handler;
    return HandlerRegistration{
        [this]
        {
            mock.unregister_active_session_changed_handler();
            this->active_session_changed_handler = null_arg2_handler;
        }};
}

repowerd::HandlerRegistration rt::FakeSessionTracker::register_session_removed_handler(
    SessionRemovedHandler const& handler)
{
    mock.register_session_removed_handler(handler);
    this->session_removed_handler = handler;
    return HandlerRegistration{
        [this]
        {
            mock.unregister_session_removed_handler();
            this->session_removed_handler = null_arg1_handler;
        }};
}

std::string rt::FakeSessionTracker::session_for_pid(pid_t pid)
{
    auto const result = std::find_if(sessions.begin(), sessions.end(),
        [pid] (auto const& kv)
        {
            return kv.second.pid == pid;
        });

    return result != sessions.end() ? result->first : std::string{};
}

void rt::FakeSessionTracker::add_session(
    std::string const& session_id, SessionType type, pid_t pid)
{
    sessions[session_id] = {type, pid};
}

void rt::FakeSessionTracker::remove_session(std::string const& session_id)
{
    if (sessions.erase(session_id) > 0)
        session_removed_handler(session_id);
}

void rt::FakeSessionTracker::switch_to_session(std::string const& session_id)
{
    auto const iter = sessions.find(session_id);
    if (iter != sessions.end())
        active_session_changed_handler(session_id, iter->second.type);
}

std::string rt::FakeSessionTracker::default_session()
{
    return "InitialCompatibleSession";
}
