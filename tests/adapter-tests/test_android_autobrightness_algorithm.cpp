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

#include "src/adapters/android_autobrightness_algorithm.h"
#include "src/adapters/event_loop.h"

#include "fake_device_config.h"
#include "fake_log.h"

#include <gmock/gmock.h>

namespace rt = repowerd::test;
using namespace testing;

namespace
{

struct AnAndroidAutobrightnessAlgorithm : Test
{
    AnAndroidAutobrightnessAlgorithm()
    {
        device_config_with_valid_curves.set("autoBrightnessLevels", "1,2,3");
        device_config_with_valid_curves.set("autoBrightnessLcdBacklightValues", "1,2,3,4");
    }

    void wait_for_event_loop_processing()
    {
        event_loop.enqueue([]{}).get();
    }

    repowerd::EventLoop event_loop;
    std::shared_ptr<rt::FakeLog> const fake_log{std::make_shared<rt::FakeLog>()};

    rt::FakeDeviceConfig device_config_with_valid_curves;

};

}

TEST_F(AnAndroidAutobrightnessAlgorithm,
       fails_to_initialize_without_autobrightness_curves)
{
    rt::FakeDeviceConfig device_config_without_curves;

    repowerd::AndroidAutobrightnessAlgorithm ab_algorithm{
        device_config_without_curves, fake_log};

    EXPECT_FALSE(ab_algorithm.init(event_loop));
}

TEST_F(AnAndroidAutobrightnessAlgorithm,
       fails_to_initialize_with_autobrightness_curves_of_incorrect_size)
{
    rt::FakeDeviceConfig device_config_with_invalid_curves;
    device_config_with_invalid_curves.set("autoBrightnessLevels", "1,2,3");
    device_config_with_invalid_curves.set("autoBrightnessLcdBacklightValues", "1,2,3");

    repowerd::AndroidAutobrightnessAlgorithm ab_algorithm{
        device_config_with_invalid_curves, fake_log};

    EXPECT_FALSE(ab_algorithm.init(event_loop));
}

TEST_F(AnAndroidAutobrightnessAlgorithm,
       initializes_with_autobrightness_curves_of_correct_size)
{
    repowerd::AndroidAutobrightnessAlgorithm ab_algorithm{
        device_config_with_valid_curves, fake_log};

    EXPECT_TRUE(ab_algorithm.init(event_loop));
}

TEST_F(AnAndroidAutobrightnessAlgorithm,
       reacts_immediately_to_first_light_value_after_started)
{
    repowerd::AndroidAutobrightnessAlgorithm ab_algorithm{
        device_config_with_valid_curves, fake_log};
    ASSERT_TRUE(ab_algorithm.init(event_loop));

    std::vector<double> ab_values;
    auto const reg = ab_algorithm.register_autobrightness_handler(
        [&] (double brightness) { ab_values.push_back(brightness); });

    ab_algorithm.start();
    ab_algorithm.new_light_value(2.0);

    wait_for_event_loop_processing();

    EXPECT_THAT(ab_values.size(), Eq(1));
}

TEST_F(AnAndroidAutobrightnessAlgorithm, ignores_light_values_when_stopped)
{
    repowerd::AndroidAutobrightnessAlgorithm ab_algorithm{
        device_config_with_valid_curves, fake_log};
    ASSERT_TRUE(ab_algorithm.init(event_loop));

    std::vector<double> ab_values;
    auto const reg = ab_algorithm.register_autobrightness_handler(
        [&] (double brightness) { ab_values.push_back(brightness); });

    ab_algorithm.new_light_value(2.0);
    wait_for_event_loop_processing();
    EXPECT_THAT(ab_values, IsEmpty());

    ab_algorithm.start();
    ab_algorithm.stop();
    ab_algorithm.new_light_value(2.0);
    wait_for_event_loop_processing();
    EXPECT_THAT(ab_values, IsEmpty());
}
