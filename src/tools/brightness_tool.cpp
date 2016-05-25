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
#include "src/adapters/android_backlight.h"
#include "src/adapters/android_device_config.h"
#include "src/adapters/backlight_brightness_control.h"
#include "src/adapters/console_log.h"
#include "src/adapters/sysfs_backlight.h"
#include "src/adapters/ubuntu_light_sensor.h"

#include <iostream>
#include <string>

struct NullLightSensor : repowerd::LightSensor
{
    repowerd::HandlerRegistration register_light_handler(
        repowerd::LightHandler const&) override
    {
        return repowerd::HandlerRegistration{[]{}};
    }

    void enable_light_events() override {}
    void disable_light_events() override {}
};

auto create_backlight()
{
    std::shared_ptr<repowerd::Backlight> backlight;

    try
    {
        backlight = std::make_shared<repowerd::AndroidBacklight>();
    }
    catch (...)
    {
    }

    if (!backlight)
        backlight = std::make_shared<repowerd::SysfsBacklight>("/sys");

    return backlight;
}

auto create_light_sensor()
{
    std::shared_ptr<repowerd::LightSensor> light_sensor;

    try
    {
        light_sensor = std::make_shared<repowerd::UbuntuLightSensor>();
    }
    catch (...)
    {
    }

    if (!light_sensor)
        light_sensor = std::make_shared<NullLightSensor>();

    return light_sensor;
}

int main()
{
    repowerd::AndroidDeviceConfig device_config{REPOWERD_DEVICE_CONFIGS_PATH};
    auto const backlight = create_backlight();
    auto const light_sensor = create_light_sensor();
    auto const log = std::make_shared<repowerd::ConsoleLog>();
    auto const ab_algorithm =
        std::make_shared<repowerd::AndroidAutobrightnessAlgorithm>(device_config);
    repowerd::BacklightBrightnessControl brightness_control{
        backlight, light_sensor, ab_algorithm, log, device_config};

    bool running = true;

    std::cout << "Commands (press enter after command letter): "<< std::endl
              << "  d => dim brightness" << std::endl
              << "  n => normal brightness" << std::endl
              << "  o => off brightness" << std::endl
              << "  ae => autobrightness enable" << std::endl
              << "  ad => autobrightness disable" << std::endl
              << "  1-9,0 => set normal brightness value 10%-90%,100%" << std::endl
              << "  q => quit" << std::endl;

    while (running)
    {
        std::string line;
        std::getline(std::cin, line);

        if (line == "q")
        {
            running = false;
        }
        else if (line == "d")
        {
            std::cout << "Setting dim brightness" << std::endl;
            brightness_control.set_dim_brightness();
        }
        else if (line == "n")
        {
            std::cout << "Setting normal brightness" << std::endl;
            brightness_control.set_normal_brightness();
        }
        else if (line == "o")
        {
            std::cout << "Setting off brightness" << std::endl;
            brightness_control.set_off_brightness();
        }
        else if (line == "ae")
        {
            std::cout << "Enabling autobrightness" << std::endl;
            brightness_control.enable_autobrightness();
        }
        else if (line == "ad")
        {
            std::cout << "Disabling autobrightness" << std::endl;
            brightness_control.disable_autobrightness();
        }
        else if (line == "0")
        {
            std::cout << "Setting normal brightness value to 100%" << std::endl;
            brightness_control.set_normal_brightness_value(1.0);
        }
        else if (line.size() == 1 && line[0] >= '1' && line[0] <= '9')
        {
            auto b = line[0] - '0';
            std::cout << "Setting normal brightness value to " << b*10 << "%" << std::endl;
            brightness_control.set_normal_brightness_value(0.1*b);
        }
    }
}

