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

FILE(GLOB device_config_files "${CMAKE_CURRENT_SOURCE_DIR}/device-configs/*.xml")
FILE(GLOB dbus_config_files "${CMAKE_CURRENT_SOURCE_DIR}/dbus-configs/*.conf")
FILE(GLOB systemd_file "${CMAKE_CURRENT_SOURCE_DIR}/repowerd.service")

install(
  FILES ${device_config_files}
  DESTINATION ${REPOWERD_DEVICE_CONFIG_DIR}
)

install(
  FILES ${dbus_config_files}
  DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/dbus-1/system.d
)

pkg_check_modules(SYSTEMD systemd)
if (${SYSTEMD_FOUND})
  pkg_get_variable(SYSTEMD_SYSTEM_DIR systemd systemdsystemunitdir)
  message (STATUS "${SYSTEMD_SYSTEM_DIR} is the systemd system unit file install dir")
  install (FILES "${systemd_file}"  DESTINATION "${SYSTEMD_SYSTEM_DIR}")
endif()
