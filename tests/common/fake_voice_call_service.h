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

#include "src/voice_call_service.h"

#include <gmock/gmock.h>

namespace repowerd
{
namespace test
{

class FakeVoiceCallService : public VoiceCallService
{
public:
    FakeVoiceCallService();

    HandlerRegistration register_active_call_handler(
        ActiveCallHandler const& handler) override;
    HandlerRegistration register_no_active_call_handler(
        NoActiveCallHandler const& handler) override;

    void emit_active_call();
    void emit_no_active_call();

    struct Mock
    {
        MOCK_METHOD1(register_active_call_handler, void(ActiveCallHandler const&));
        MOCK_METHOD0(unregister_active_call_handler, void());
        MOCK_METHOD1(register_no_active_call_handler, void(NoActiveCallHandler const&));
        MOCK_METHOD0(unregister_no_active_call_handler, void());
    };
    testing::NiceMock<Mock> mock;

private:
    ActiveCallHandler active_call_handler;
    NoActiveCallHandler no_active_call_handler;
};

}
}
