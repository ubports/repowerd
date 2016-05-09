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
    repowerd-acceptance-tests

    acceptance_test.cpp
    test_client_requests.cpp
    test_notification.cpp
    test_power_button.cpp
    test_proximity_sensor.cpp
    test_turn_on_display_at_startup.cpp
    test_user_activity.cpp
    test_voice_call.cpp
)

target_link_libraries(
    repowerd-acceptance-tests

    repowerd-core
    repowerd-test-common

    ${GTEST_BOTH_LIBRARIES}
    ${GMOCK_LIBRARY}
    ${GMOCK_MAIN_LIBRARY}
)

add_test(repowerd-acceptance-tests ${EXECUTABLE_OUTPUT_PATH}/repowerd-acceptance-tests)

add_dependencies(repowerd-acceptance-tests GMock)
