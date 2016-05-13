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

add_library(
    repowerd-test-common STATIC

    daemon_config.cpp
    fake_client_requests.cpp
    fake_log.cpp
    fake_notification_service.cpp
    fake_power_button.cpp
    fake_proximity_sensor.cpp
    fake_timer.cpp
    fake_user_activity.cpp
    fake_voice_call_service.cpp
)

target_link_libraries(
    repowerd-test-common

    repowerd-core

    ${GTEST_BOTH_LIBRARIES}
    ${GMOCK_LIBRARY}
    ${GMOCK_MAIN_LIBRARY}
)
