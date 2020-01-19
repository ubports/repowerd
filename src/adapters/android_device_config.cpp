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
#include "path.h"

#include "src/core/log.h"

#include <memory>
#include <fstream>
#include <cstring>
#include <cctype>
#include <algorithm>
#include <array>

#include <deviceinfo.h>

char const* const log_tag = "AndroidDeviceConfig";

repowerd::AndroidDeviceConfig::AndroidDeviceConfig(
    std::shared_ptr<Log> const& log,
    std::shared_ptr<Filesystem> const& filesystem,
    std::vector<std::string> const& config_dirs)
    : log{log},
      filesystem{filesystem}
{
    for (auto const& dir : config_dirs)
        log->log(log_tag, "Using config directory: %s", dir.c_str());

    parse_first_matching_file_in_dirs(config_dirs, "config-default.xml");

    DeviceInfo info;
    auto const device_name = info.name();
    if (device_name != "")
        parse_first_matching_file_in_dirs(config_dirs, "config-" + device_name + ".xml");

    log_properties();
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
        auto const full_file_path = Path{config_dir}/filename;
        if (filesystem->is_regular_file(full_file_path))
        {
            parse_file(full_file_path);
            break;
        }
    }
}

void repowerd::AndroidDeviceConfig::parse_file(std::string const& file)
{
    log->log(log_tag, "parse_file(%s)", file.c_str());

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

    auto const ifs_ptr = filesystem->istream(file);
    auto& ifs = *ifs_ptr;

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

    config.erase(last_config_name);
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

void repowerd::AndroidDeviceConfig::log_properties()
{
    for (auto const& property : config)
    {
        log->log(log_tag, "Property: %s=%s",
                 property.first.c_str(), property.second.c_str());
    }
}
