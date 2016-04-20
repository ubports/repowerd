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

#include "src/adapters/android_device_config.h"

#include "virtual_filesystem.h"

#include <algorithm>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace testing;
namespace rt = repowerd::test;

namespace
{

char const* const config_default = R"(
<resources xmlns:xliff='urn:oasis:names:tc:xliff:document:1.2'>
  <bool name='config_boolconfig'>false</bool>
  <integer name='config_integerconfig'>4</integer>
  <integer name='integerconfigwithoutprefix'>680</integer>
  <integer-array name='config_integerarrayconfig'>
    <item>2</item>
    <item>4</item>
    <item>6</item>
  </integer-array>
</resources>
)";

struct AnAndroidDeviceConfig : Test
{
    AnAndroidDeviceConfig()
    {
        vfs.add_file_with_contents("/config-default.xml", config_default);
    }

    rt::VirtualFilesystem vfs;
};

}

TEST_F(AnAndroidDeviceConfig, reads_default_config_file)
{
    repowerd::AndroidDeviceConfig config{vfs.mount_point()};

    EXPECT_THAT(config.get("boolconfig", ""), StrEq("false"));
    EXPECT_THAT(config.get("integerconfig", ""), StrEq("4"));
    EXPECT_THAT(config.get("integerconfigwithoutprefix", ""), StrEq("680"));
    EXPECT_THAT(config.get("integerarrayconfig", ""), StrEq("2,4,6"));
}

TEST_F(AnAndroidDeviceConfig, returns_default_value_for_unknown_key)
{
    repowerd::AndroidDeviceConfig config{vfs.mount_point()};

    EXPECT_THAT(config.get("unknown", "bla"), StrEq("bla"));
}
