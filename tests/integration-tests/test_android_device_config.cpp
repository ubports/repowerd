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

#include "fake_android_properties.h"
#include "fake_log.h"
#include "fake_filesystem.h"

#include <algorithm>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace testing;
namespace rt = repowerd::test;

namespace
{

char const* const config_default1 = R"(
<resources xmlns:xliff='urn:oasis:names:tc:xliff:document:1.2'>
  <bool name='config_boolconfig'>false</bool>
  <integer name='config_integerconfig'>4</integer>
  <integer name='integerconfigwithoutprefix'>680</integer>
  <integer-array name='config_integerarrayconfig'>
    <item>2</item>
    <item>4</item>
    <item>6</item>
  </integer-array>
  <integer name='config_id'>1</integer>
</resources>
)";

char const* const config_default2 = R"(
<resources xmlns:xliff='urn:oasis:names:tc:xliff:document:1.2'>
  <bool name='config_boolconfig'>false</bool>
  <integer name='config_integerconfig'>4</integer>
  <integer name='integerconfigwithoutprefix'>680</integer>
  <integer-array name='config_integerarrayconfig'>
    <item>2</item>
    <item>4</item>
    <item>6</item>
  </integer-array>
  <integer name='config_id'>2</integer>
</resources>
)";

char const* const config_device = R"(
<resources xmlns:xliff='urn:oasis:names:tc:xliff:document:1.2'>
  <bool name='config_boolconfig'>true</bool>
  <integer name='config_integerconfig'>5</integer>
  <integer name='integerconfigwithoutprefixnew'>123</integer>
  <integer-array name='config_integerarrayconfig'>
    <item>1</item>
    <item>3</item>
    <item>5</item>
  </integer-array>
  <integer name='config_id'>3</integer>
</resources>
)";

struct AnAndroidDeviceConfig : Test
{
    AnAndroidDeviceConfig()
    {
        fake_fs->add_file_with_contents("/configdir1/config-default.xml", config_default1);
        fake_fs->add_file_with_contents("/configdir1/config-device.xml", config_device);
        fake_fs->add_file_with_contents("/configdir2/config-default.xml", config_default2);
    }

    void set_device_name()
    {
        fake_android_properties.set("ro.product.device", "device");
    }

    rt::FakeAndroidProperties fake_android_properties;
    std::shared_ptr<rt::FakeLog> fake_log{std::make_shared<rt::FakeLog>()};
    std::shared_ptr<rt::FakeFilesystem> fake_fs{std::make_shared<rt::FakeFilesystem>()};
    std::string const config_dir_1{"/configdir1"};
    std::string const config_dir_2{"/configdir2"};
    std::string const config_dir_empty{"/configdirempty"};
    rt::FakeFilesystem fs;
};

}

TEST_F(AnAndroidDeviceConfig, reads_default_config_file)
{
    repowerd::AndroidDeviceConfig config{fake_log, fake_fs, {config_dir_1}};

    EXPECT_THAT(config.get("boolconfig", ""), StrEq("false"));
    EXPECT_THAT(config.get("integerconfig", ""), StrEq("4"));
    EXPECT_THAT(config.get("integerconfigwithoutprefix", ""), StrEq("680"));
    EXPECT_THAT(config.get("integerarrayconfig", ""), StrEq("2,4,6"));
}

TEST_F(AnAndroidDeviceConfig, returns_default_value_for_unknown_key)
{
    repowerd::AndroidDeviceConfig config{fake_log, fake_fs, {config_dir_1}};

    EXPECT_THAT(config.get("unknown", "bla"), StrEq("bla"));
}

TEST_F(AnAndroidDeviceConfig, reads_first_default_config_file_from_config_dirs)
{
    repowerd::AndroidDeviceConfig config{
        fake_log,
        fake_fs,
        {config_dir_empty, config_dir_2, config_dir_1}};

    EXPECT_THAT(config.get("id", ""), StrEq("2"));
}

TEST_F(AnAndroidDeviceConfig, logs_config_files_read)
{
    repowerd::AndroidDeviceConfig config{fake_log, fake_fs, {config_dir_1}};

    EXPECT_TRUE(fake_log->contains_line({config_dir_1 + "/config-default.xml"}));
}

TEST_F(AnAndroidDeviceConfig, overwrites_value_from_default_with_device_specific)
{
    set_device_name();
    repowerd::AndroidDeviceConfig config{fake_log, fake_fs, {config_dir_1}};

    EXPECT_THAT(config.get("boolconfig", ""), StrEq("true"));
    EXPECT_THAT(config.get("integerconfig", ""), StrEq("5"));
    EXPECT_THAT(config.get("integerarrayconfig", ""), StrEq("1,3,5"));
}

TEST_F(AnAndroidDeviceConfig, keeps_value_only_in_default_or_device_specific)
{
    set_device_name();
    repowerd::AndroidDeviceConfig config{fake_log, fake_fs, {config_dir_1}};

    EXPECT_THAT(config.get("integerconfigwithoutprefix", ""), StrEq("680"));
    EXPECT_THAT(config.get("integerconfigwithoutprefixnew", ""), StrEq("123"));
}

TEST_F(AnAndroidDeviceConfig, logs_device_specific_file_path)
{
    set_device_name();
    repowerd::AndroidDeviceConfig config{fake_log, fake_fs, {config_dir_1}};

    EXPECT_TRUE(fake_log->contains_line({config_dir_1 + "/config-default.xml"}));
    EXPECT_TRUE(fake_log->contains_line({config_dir_1 + "/config-device.xml"}));
}

TEST_F(AnAndroidDeviceConfig, logs_properties)
{
    set_device_name();
    repowerd::AndroidDeviceConfig config{fake_log, fake_fs, {config_dir_1}};

    EXPECT_TRUE(fake_log->contains_line({"Property", "boolconfig", "true"}));
    EXPECT_TRUE(fake_log->contains_line({"Property", "integerconfig", "5"}));
    EXPECT_TRUE(fake_log->contains_line({"Property", "integerarrayconfig", "1,3,5"}));
    EXPECT_TRUE(fake_log->contains_line({"Property", "integerconfigwithoutprefix", "680"}));
    EXPECT_TRUE(fake_log->contains_line({"Property", "integerconfigwithoutprefixnew", "123"}));
}
