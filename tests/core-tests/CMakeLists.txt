# Copyright © 2016 Canonical Ltd.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>

add_executable(
    repowerd-core-tests

    acceptance_test.cpp
    daemon_config.cpp
    fake_client_requests.cpp
    fake_notification_service.cpp
    fake_power_button.cpp
    fake_power_source.cpp
    fake_proximity_sensor.cpp
    fake_timer.cpp
    fake_user_activity.cpp
    fake_voice_call_service.cpp

    test_client_requests.cpp
    test_daemon.cpp
    test_fake_timer.cpp
    test_modem_power_control.cpp
    test_notification.cpp
    test_performance_booster.cpp
    test_power_button.cpp
    test_power_source.cpp
    test_proximity_sensor.cpp
    test_suspend_control.cpp
    test_turn_on_display_at_startup.cpp
    test_user_activity.cpp
    test_voice_call.cpp
)

target_link_libraries(
    repowerd-core-tests

    repowerd-core
    repowerd-test-common

    ${GTEST_BOTH_LIBRARIES}
    ${GMOCK_LIBRARY}
    ${GMOCK_MAIN_LIBRARY}
)

add_test(repowerd-core-tests ${EXECUTABLE_OUTPUT_PATH}/repowerd-core-tests)

add_dependencies(repowerd-core-tests GMock)
