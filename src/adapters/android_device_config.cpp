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

#include "android_device_config.h"

#include <memory>
#include <fstream>
#include <cstring>
#include <cctype>
#include <algorithm>
#include <array>
#include <sys/stat.h>

#include <hybris/properties/properties.h>

namespace
{

std::string determine_device_name()
{
    char name[PROP_VALUE_MAX] = "";
    property_get("ro.product.device", name, "");
    return name;
}

bool file_exists(std::string const& filename)
{
    struct stat sb;
    return stat(filename.c_str(), &sb) == 0 &&
           S_ISREG(sb.st_mode);
}

}

repowerd::AndroidDeviceConfig::AndroidDeviceConfig(
    std::vector<std::string> const& config_dirs)
{
    parse_first_matching_file_in_dirs(config_dirs, "config-default.xml");

    auto const device_name = determine_device_name();
    if (device_name != "")
        parse_first_matching_file_in_dirs(config_dirs, "config-" + device_name + ".xml");
}

std::string repowerd::AndroidDeviceConfig::get(
    std::string const& name, std::string const& default_value) const
{
    auto const iter = config.find(name);
    if (iter != config.end())
        return iter->second;
    else
        return default_value;
}

void repowerd::AndroidDeviceConfig::parse_first_matching_file_in_dirs(
    std::vector<std::string> const& config_dirs, std::string const& filename)
{
    for (auto const& config_dir : config_dirs)
    {
        auto const full_file_path = config_dir + "/" + filename;
        if (file_exists(full_file_path))
        {
            parse_file(full_file_path);
            break;
        }
    }
}

void repowerd::AndroidDeviceConfig::parse_file(std::string const& file)
{
    GMarkupParser parser;
    parser.start_element = static_xml_start_element;
    parser.end_element = static_xml_end_element;
    parser.text = static_xml_text;
    parser.passthrough = nullptr;
    parser.error = nullptr;

    auto const ctx = std::unique_ptr<GMarkupParseContext, void(*)(GMarkupParseContext*)>{
        g_markup_parse_context_new(
            &parser,
            static_cast<GMarkupParseFlags>(0),
            this,
            nullptr),
        g_markup_parse_context_free};

    std::ifstream ifs{file};

    std::array<char,4096> text;

    while (ifs)
    {
        ifs.read(text.data(), text.size());
        auto text_len = ifs.gcount();

        g_markup_parse_context_parse(
            ctx.get(),
            text.data(),
            text_len,
            nullptr);
    }
}

void repowerd::AndroidDeviceConfig::static_xml_start_element(
    GMarkupParseContext* /*context*/,
    char const* element_name,
    char const** attribute_names,
    char const** attribute_values,
    gpointer user_data,
    GError** /*error*/)
{
    auto const device_config = static_cast<AndroidDeviceConfig*>(user_data);
    std::unordered_map<std::string,std::string> attribs;

    auto current_attrib = attribute_names;
    auto current_value = attribute_values;
    while (*current_attrib != nullptr)
    {
        attribs.emplace(*current_attrib, *current_value);
        ++current_attrib;
        ++current_value;
    }

    device_config->xml_start_element(element_name, attribs);
}

void repowerd::AndroidDeviceConfig::static_xml_end_element(
    GMarkupParseContext* /*context*/,
    char const* element_name,
    void* user_data,
    GError** /*error*/)
{
    auto const device_config = static_cast<AndroidDeviceConfig*>(user_data);
    device_config->xml_end_element(element_name);
}

void repowerd::AndroidDeviceConfig::static_xml_text(
    GMarkupParseContext* /*context*/,
    char const* text,
    gsize text_len,
    gpointer user_data,
    GError** /*error*/)
{
    auto const device_config = static_cast<AndroidDeviceConfig*>(user_data);
    if (text && text_len > 0)
        device_config->xml_text(std::string{text, text_len});
    else
        device_config->xml_text("");
}

void repowerd::AndroidDeviceConfig::xml_start_element(
    std::string const& element_name,
    std::unordered_map<std::string,std::string> const& attribs)
{
    if (element_name != "integer" && element_name != "integer-array" && element_name != "bool")
        return;

    auto iter = attribs.find("name");
    if (iter == attribs.end())
        return;

    auto config_name = iter->second;
    if (config_name.find("config_") == 0)
        config_name = config_name.substr(strlen("config_"));
    last_config_name = config_name;
}

void repowerd::AndroidDeviceConfig::xml_end_element(
    std::string const& element_name)
{
    if (element_name != "integer" && element_name != "integer-array" && element_name != "bool")
        return;

    last_config_name = "";
}

void repowerd::AndroidDeviceConfig::xml_text(std::string const& text)
{
    auto clean_text = text;
    clean_text.erase(
        std::remove_if(clean_text.begin(), clean_text.end(), [](int c) { return isspace(c); }),
        clean_text.end());
    if (last_config_name == "" || clean_text == "")
        return;

    auto iter = config.find(last_config_name);
    if (iter != config.end())
        iter->second += "," + clean_text;
    else
        config[last_config_name] = clean_text;
}

