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
    ${GIO_INCLUDE_DIRS}
)

set(
    REPOWERD_ADAPTER_SRCS

    dbus_connection_handle.cpp
    dbus_event_loop.cpp
    dbus_message_handle.cpp
    unity_screen_service.cpp
)

add_library(
    repowerd-adapters OBJECT
    ${REPOWERD_ADAPTER_SRCS}
)
