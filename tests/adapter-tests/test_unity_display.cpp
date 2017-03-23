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

#include "dbus_bus.h"

#include "src/adapters/dbus_connection_handle.h"
#include "src/adapters/dbus_event_loop.h"
#include "src/adapters/unity_display.h"

#include "duration_of.h"
#include "fake_log.h"
#include "fake_shared.h"
#include "spin_wait.h"
#include "wait_condition.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <chrono>

namespace rt = repowerd::test;

using namespace testing;
using namespace std::chrono_literals;

namespace
{

char const* const unity_display_service_introspection = R"(
<node>
  <interface name='com.canonical.Unity.Display'>
    <method name='TurnOn'>
      <arg type='s' name='filter' direction='in'/>
    </method>
    <method name='TurnOff'>
      <arg type='s' name='filter' direction='in'/>
    </method>
    <property name='ActiveOutputs' type='(ii)' access='read'/>
  </interface>
</node>)";

class FakeUnityDisplayDBusService
{
public:
    FakeUnityDisplayDBusService(std::string const& bus_address)
        : dbus_connection{bus_address},
          dbus_event_loop{"FakeUnityDisplay"}
    {
        dbus_connection.request_name("com.canonical.Unity.Display");
        unity_display_handler_registation = dbus_event_loop.register_object_handler(
            dbus_connection,
            "/com/canonical/Unity/Display",
            unity_display_service_introspection,
            [this] (
                GDBusConnection* connection,
                gchar const* sender,
                gchar const* object_path,
                gchar const* interface_name,
                gchar const* method_name,
                GVariant* parameters,
                GDBusMethodInvocation* invocation)
            {
                dbus_method_call(
                    connection, sender, object_path, interface_name,
                    method_name, parameters, invocation);
            });
    }

    void emit_active_outputs(int internal, int external)
    {
        this->internal = internal;
        this->external = external;

        g_dbus_connection_emit_signal(
            dbus_connection,
            nullptr,
            "/com/canonical/Unity/Display",
            "org.freedesktop.DBus.Properties",
            "PropertiesChanged",
            g_variant_new_parsed(
                "(@s 'com.canonical.Unity.Display',"
                " @a{sv} { 'ActiveOutputs' : <(%i,%i)> },"
                " @as [])",
                internal,
                external),
            nullptr);
    }

    struct MockDBusCalls
    {
        MOCK_METHOD1(turn_on, void(std::string const&));
        MOCK_METHOD1(turn_off, void(std::string const&));
    };

    testing::NiceMock<MockDBusCalls> mock_dbus_calls;

private:
    void dbus_method_call(
        GDBusConnection* /*connection*/,
        gchar const* /*sender*/,
        gchar const* /*object_path*/,
        gchar const* /*interface_name*/,
        gchar const* method_name_cstr,
        GVariant* parameters,
        GDBusMethodInvocation* invocation)
    {
        std::string const method_name{method_name_cstr ? method_name_cstr : ""};
        GVariant* reply{nullptr};

        if (method_name == "TurnOn")
        {
            char const* filter{""};
            g_variant_get(parameters, "(&s)", &filter);

            mock_dbus_calls.turn_on(filter);
        }
        else if (method_name == "TurnOff")
        {
            char const* filter{""};
            g_variant_get(parameters, "(&s)", &filter);

            mock_dbus_calls.turn_off(filter);
        }
        else if (method_name == "Get")
        {
            reply = g_variant_new_parsed("(<(%i,%i)>,)", internal.load(), external.load());
        }

        g_dbus_method_invocation_return_value(invocation, reply);
    }

    repowerd::DBusConnectionHandle dbus_connection;
    repowerd::DBusEventLoop dbus_event_loop;
    repowerd::HandlerRegistration unity_display_handler_registation;
    std::atomic<int> internal{0};
    std::atomic<int> external{0};
};

struct AUnityDisplay : testing::Test
{
    void wait_for_have_external(repowerd::UnityDisplay& unity_display, bool value)
    {
        auto const result = rt::spin_wait_for_condition_or_timeout(
            [&] { return unity_display.has_active_external_displays() == value; },
            default_timeout);
        if (!result)
        {
            throw std::runtime_error(
                "Timeout while waiting for has_active_external_displays=" + std::to_string(value));
        }
    }

    rt::DBusBus bus;
    rt::FakeLog fake_log;
    FakeUnityDisplayDBusService service{bus.address()};
    repowerd::UnityDisplay unity_display{
        rt::fake_shared(fake_log),
        bus.address()};

    std::chrono::seconds const default_timeout{3};
};

}

TEST_F(AUnityDisplay, turn_on_request_contacts_dbus_service)
{
    rt::WaitCondition called;

    InSequence s;
    EXPECT_CALL(service.mock_dbus_calls, turn_on("all"));
    EXPECT_CALL(service.mock_dbus_calls, turn_on("internal"));
    EXPECT_CALL(service.mock_dbus_calls, turn_on("external"))
        .WillOnce(WakeUp(&called));

    unity_display.turn_on(repowerd::DisplayPowerControlFilter::all);
    unity_display.turn_on(repowerd::DisplayPowerControlFilter::internal);
    unity_display.turn_on(repowerd::DisplayPowerControlFilter::external);

    called.wait_for(default_timeout);
    EXPECT_TRUE(called.woken());
}

TEST_F(AUnityDisplay, turn_off_request_contacts_dbus_service)
{
    rt::WaitCondition called;

    InSequence s;
    EXPECT_CALL(service.mock_dbus_calls, turn_off("all"));
    EXPECT_CALL(service.mock_dbus_calls, turn_off("internal"));
    EXPECT_CALL(service.mock_dbus_calls, turn_off("external"))
        .WillOnce(WakeUp(&called));

    unity_display.turn_off(repowerd::DisplayPowerControlFilter::all);
    unity_display.turn_off(repowerd::DisplayPowerControlFilter::internal);
    unity_display.turn_off(repowerd::DisplayPowerControlFilter::external);

    called.wait_for(default_timeout);
    EXPECT_TRUE(called.woken());
}

TEST_F(AUnityDisplay, handles_display_information_updates)
{
    EXPECT_FALSE(unity_display.has_active_external_displays());

    service.emit_active_outputs(1, 1);
    wait_for_have_external(unity_display, true);

    service.emit_active_outputs(1, 0);
    wait_for_have_external(unity_display, false);
}

TEST_F(AUnityDisplay, queries_initial_display_information)
{
    service.emit_active_outputs(1, 1);

    repowerd::UnityDisplay local_unity_display{
        rt::fake_shared(fake_log),
        bus.address()};

    wait_for_have_external(local_unity_display, true);
}

TEST_F(AUnityDisplay,
       does_not_hang_waiting_for_initial_display_information_if_unity_display_is_down)
{
    rt::DBusBus empty_bus;

    repowerd::UnityDisplay local_unity_display{
        rt::fake_shared(fake_log),
        empty_bus.address()};
}

TEST_F(AUnityDisplay, waits_at_most_one_second_for_turn_on_response)
{
    EXPECT_CALL(service.mock_dbus_calls, turn_on("all"))
        .WillOnce(InvokeWithoutArgs([]{ std::this_thread::sleep_for(1200ms); }));

    EXPECT_THAT(
        rt::duration_of(
            [this] { unity_display.turn_on(repowerd::DisplayPowerControlFilter::all); }),
        AllOf(Ge(1000ms), Le(1100ms)));
}

TEST_F(AUnityDisplay, logs_turn_on_request)
{
    unity_display.turn_on(repowerd::DisplayPowerControlFilter::all);

    EXPECT_TRUE(fake_log.contains_line({"turn_on"}));
}

TEST_F(AUnityDisplay, logs_turn_off_request)
{
    unity_display.turn_off(repowerd::DisplayPowerControlFilter::all);

    EXPECT_TRUE(fake_log.contains_line({"turn_off"}));
}
