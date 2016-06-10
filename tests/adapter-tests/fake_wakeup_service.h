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

#include "src/adapters/wakeup_service.h"

#include <vector>

#include <gmock/gmock.h>

namespace repowerd
{
namespace test
{

class FakeWakeupService : public repowerd::WakeupService
{
public:
    FakeWakeupService();

    std::string schedule_wakeup_at(std::chrono::system_clock::time_point tp) override;
    void cancel_wakeup(std::string const& cookie) override;
    repowerd::HandlerRegistration register_wakeup_handler(
        repowerd::WakeupHandler const& handler) override;

    std::chrono::system_clock::time_point emit_next_wakeup();

    struct MockMethods
    {
        MOCK_METHOD1(register_wakeup_handler, void(repowerd::WakeupHandler const&));
        MOCK_METHOD0(unregister_wakeup_handler, void());
    };
    testing::NiceMock<MockMethods> mock;

private:
    repowerd::WakeupHandler wakeup_handler;
    std::vector<std::chrono::system_clock::time_point> wakeups;
};

}
}
