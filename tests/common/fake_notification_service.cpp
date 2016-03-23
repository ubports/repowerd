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

#include "fake_notification_service.h"

namespace rt = repowerd::test;

rt::FakeNotificationService::FakeNotificationService()
    : all_notifications_done_handler{[]{}},
      notification_handler{[]{}}
{
}

repowerd::HandlerRegistration rt::FakeNotificationService::register_notification_handler(
    NotificationHandler const& handler)
{
    mock.register_notification_handler(handler);
    notification_handler = handler;
    return HandlerRegistration{
        [this]
        {
            mock.unregister_notification_handler();
            notification_handler = []{};
        }};
}

void rt::FakeNotificationService::emit_notification()
{
    notification_handler();
}

repowerd::HandlerRegistration rt::FakeNotificationService::register_all_notifications_done_handler(
    AllNotificationsDoneHandler const& handler)
{
    mock.register_all_notifications_done_handler(handler);
    all_notifications_done_handler = handler;
    return HandlerRegistration{
        [this]
        {
            mock.unregister_all_notifications_done_handler();
            all_notifications_done_handler = []{};
        }};
}

void rt::FakeNotificationService::emit_all_notifications_done()
{
    all_notifications_done_handler();
}
