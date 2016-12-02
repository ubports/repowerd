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

#include "src/core/handler_registration.h"
#include <memory>

#include <gmock/gmock.h>

TEST(AHandlerRegistration, calls_unregistration_callback_when_destroyed)
{
    bool unregistered = false;

    {
        repowerd::HandlerRegistration reg{[&] { unregistered = true; }};
        EXPECT_FALSE(unregistered);
    }

    EXPECT_TRUE(unregistered);
}

TEST(AHandlerRegistration, calls_unregistration_callback_when_overwritten)
{
    bool unregistered = false;

    repowerd::HandlerRegistration reg{[&] { unregistered = true; }};
    EXPECT_FALSE(unregistered);

    reg = repowerd::HandlerRegistration{[]{}};
    EXPECT_TRUE(unregistered);
}

TEST(AHandlerRegistration, move_construction_moves_properly)
{
    bool unregistered = false;

    auto reg1 = std::make_unique<repowerd::HandlerRegistration>([&] { unregistered = true; });
    auto reg2 = std::make_unique<repowerd::HandlerRegistration>(std::move(*reg1));

    reg1.reset();
    EXPECT_FALSE(unregistered);

    reg2.reset();
    EXPECT_TRUE(unregistered);
}

TEST(AHandlerRegistration, move_assignment_moves_properly)
{
    bool unregistered1 = false;
    bool unregistered2 = false;

    auto reg1 = std::make_unique<repowerd::HandlerRegistration>([&] { unregistered1 = true; });
    auto reg2 = std::make_unique<repowerd::HandlerRegistration>([&] { unregistered2 = true; });

    EXPECT_FALSE(unregistered1);
    EXPECT_FALSE(unregistered2);

    *reg2 = std::move(*reg1);
    EXPECT_FALSE(unregistered1);
    EXPECT_TRUE(unregistered2);

    reg1.reset();
    EXPECT_FALSE(unregistered1);
    EXPECT_TRUE(unregistered2);

    reg2.reset();
    EXPECT_TRUE(unregistered1);
    EXPECT_TRUE(unregistered2);
}

TEST(AHandlerRegistration, move_assignment_handles_self_assignment)
{
    bool unregistered = false;

    {
        repowerd::HandlerRegistration reg{[&] { unregistered = true; }};
        reg = std::move(reg);
        EXPECT_FALSE(unregistered);
    }

    EXPECT_TRUE(unregistered);
}
