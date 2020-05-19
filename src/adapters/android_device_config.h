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

#pragma once

#include "device_config.h"
#include "filesystem.h"

#include <glib.h>

#include <memory>
#include <unordered_map>
#include <vector>

namespace repowerd
{
class Log;

class AndroidDeviceConfig : public DeviceConfig
{
public:
    AndroidDeviceConfig(
        std::shared_ptr<Log> const& log,
        std::shared_ptr<Filesystem> const& filesystem,
        std::vector<std::string> const& config_dirs);

    std::string get(
        std::string const& name, std::string const& default_value) const override;

private:
    static void static_xml_start_element(
        GMarkupParseContext* context,
        char const* element_name,
        char const** attribute_names,
        char const** attribute_values,
        gpointer user_data,
        GError** error);
    static void static_xml_end_element(
        GMarkupParseContext* context,
        char const* element_name,
        void* user_data,
        GError** error);
    static void static_xml_text(
        GMarkupParseContext* context,
        char const* text,
        gsize text_len,
        gpointer user_data,
        GError** error);

    void parse_first_matching_file_in_dirs(
        std::vector<std::string> const& dirs, std::string const& filename);
    void parse_file(std::string const& file);
    void xml_start_element(
        std::string const& element_name,
        std::unordered_map<std::string,std::string> const& attribs);
    void xml_end_element(std::string const& element_name);
    void xml_text(std::string const& text);
    void log_properties();

    std::shared_ptr<Log> const log;
    std::shared_ptr<Filesystem> const filesystem;
    std::string last_config_name;
    std::unordered_map<std::string,std::string> config;
    bool use_vendor_overlay = false;
};

}
