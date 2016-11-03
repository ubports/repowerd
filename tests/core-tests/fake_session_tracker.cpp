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
    active_session_changed_handler("FakeSession", SessionType::RepowerdCompatible);
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
