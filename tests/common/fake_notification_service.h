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

#include "src/core/notification_service.h"

#include <gmock/gmock.h>

namespace repowerd
{
namespace test
{

class FakeNotificationService : public NotificationService
{
public:
    FakeNotificationService();

    HandlerRegistration register_all_notifications_done_handler(
        AllNotificationsDoneHandler const& handler) override;
    HandlerRegistration register_notification_handler(
        NotificationHandler const& handler) override;

    void emit_all_notifications_done();
    void emit_notification();

    struct Mock
    {
        MOCK_METHOD1(register_all_notifications_done_handler,
                     void(AllNotificationsDoneHandler const&));
        MOCK_METHOD0(unregister_all_notifications_done_handler, void());
        MOCK_METHOD1(register_notification_handler, void(NotificationHandler const&));
        MOCK_METHOD0(unregister_notification_handler, void());
    };
    testing::NiceMock<Mock> mock;

private:
    AllNotificationsDoneHandler all_notifications_done_handler;
    NotificationHandler notification_handler;
};

}
}
