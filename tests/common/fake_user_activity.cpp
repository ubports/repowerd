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

#include "fake_user_activity.h"

namespace rt = repowerd::test;

rt::FakeUserActivity::FakeUserActivity()
    : handler{[](UserActivityType){}}
{
}

void rt::FakeUserActivity::start_processing()
{
    mock.start_processing();
}

repowerd::HandlerRegistration rt::FakeUserActivity::register_user_activity_handler(
    UserActivityHandler const& handler)
{
    mock.register_user_activity_handler(handler);
    this->handler = handler;
    return HandlerRegistration{
        [this]
        {
            mock.unregister_user_activity_handler();
            this->handler = [](UserActivityType){};
        }};
}

void rt::FakeUserActivity::perform(UserActivityType type)
{
    handler(type);
}
