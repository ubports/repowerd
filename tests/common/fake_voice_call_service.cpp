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

#include "fake_voice_call_service.h"

namespace rt = repowerd::test;

rt::FakeVoiceCallService::FakeVoiceCallService()
    : active_call_handler{[]{}},
      no_active_call_handler{[]{}}
{
}

repowerd::HandlerRegistration rt::FakeVoiceCallService::register_active_call_handler(
    ActiveCallHandler const& handler)
{
    mock.register_active_call_handler(handler);
    active_call_handler = handler;
    return HandlerRegistration{
        [this]
        {
            mock.unregister_active_call_handler();
            active_call_handler = []{};
        }};
}

void rt::FakeVoiceCallService::emit_active_call()
{
    active_call_handler();
}

repowerd::HandlerRegistration rt::FakeVoiceCallService::register_no_active_call_handler(
    NoActiveCallHandler const& handler)
{
    mock.register_no_active_call_handler(handler);
    no_active_call_handler = handler;
    return HandlerRegistration{
        [this]
        {
            mock.unregister_no_active_call_handler();
            no_active_call_handler = []{};
        }};
}

void rt::FakeVoiceCallService::emit_no_active_call()
{
    no_active_call_handler();
}
