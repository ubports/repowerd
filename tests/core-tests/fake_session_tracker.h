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

#pragma once

#include "src/core/session_tracker.h"

#include <gmock/gmock.h>

namespace repowerd
{
namespace test
{

class FakeSessionTracker : public SessionTracker
{
public:
    FakeSessionTracker();

    void start_processing() override;

    HandlerRegistration register_active_session_changed_handler(
        ActiveSessionChangedHandler const& handler) override;
    HandlerRegistration register_session_removed_handler(
        SessionRemovedHandler const& handler) override;

    struct Mock
    {
        MOCK_METHOD0(start_processing, void());
        MOCK_METHOD1(register_active_session_changed_handler,
                     void(ActiveSessionChangedHandler const&));
        MOCK_METHOD0(unregister_active_session_changed_handler, void());
        MOCK_METHOD1(register_session_removed_handler,
                     void(SessionRemovedHandler const&));
        MOCK_METHOD0(unregister_session_removed_handler, void());
    };
    testing::NiceMock<Mock> mock;

private:
    ActiveSessionChangedHandler active_session_changed_handler;
    SessionRemovedHandler session_removed_handler;
};

}
}
