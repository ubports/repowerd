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

#include "fake_android_properties.h"

#include <stdexcept>
#include <cstring>

namespace rt = repowerd::test;

extern "C" int property_get(char const* key, char* value, char const* default_value)
{
    return rt::FakeAndroidProperties::static_property_get(key, value, default_value);
}

rt::FakeAndroidProperties* rt::FakeAndroidProperties::FakeAndroidProperties::fake_android_properties_instance = nullptr;

rt::FakeAndroidProperties::FakeAndroidProperties()
{
    fake_android_properties_instance = this;
}

rt::FakeAndroidProperties::~FakeAndroidProperties()
{
    fake_android_properties_instance = nullptr;
}

int rt::FakeAndroidProperties::static_property_get(
    char const* key, char* value, char const* default_value)
{
    if (!fake_android_properties_instance)
        throw std::runtime_error("Fake AndroidProperties instance not initialized");
    return fake_android_properties_instance->property_get(key, value, default_value);
}

void rt::FakeAndroidProperties::set(std::string const& key, std::string const& value)
{
    properties[key] = value;
}

int rt::FakeAndroidProperties::property_get(
    char const* key, char* value, char const* default_value)
{
    std::string out_value;

    if (properties.find(key) == properties.end())
    {
        out_value = default_value;
    }
    else
    {
        out_value = properties[key];
    }

    memcpy(value, out_value.c_str(), out_value.size() + 1);

    return out_value.size();
}
