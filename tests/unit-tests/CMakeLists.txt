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

include_directories(
    ${CMAKE_SOURCE_DIR}
)

add_executable(
    repowerd-unit-tests

    test_fake_timer.cpp
    test_daemon.cpp

    $<TARGET_OBJECTS:repowerd-objects>
    $<TARGET_OBJECTS:repowerd-test-common>
)

target_link_libraries(
    repowerd-unit-tests

    ${GTEST_BOTH_LIBRARIES}
    ${GMOCK_LIBRARY}
    ${GMOCK_MAIN_LIBRARY}
)

add_test(repowerd-unit-tests ${EXECUTABLE_OUTPUT_PATH}/repowerd-unit-tests)

add_dependencies(repowerd-unit-tests GMock)

