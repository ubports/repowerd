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

#include <glib.h>
#include <string>

namespace repowerd
{

struct ScopedGError
{
    ScopedGError() : g_error{nullptr} {}
    ~ScopedGError()
    {
        if (g_error) g_error_free(g_error);
    }

    std::string message_str() const
    {
        if (g_error && g_error->message)
            return g_error->message;
        else
            return {};
    }

    bool is_set() const
    {
        return g_error != nullptr;
    }

    operator GError**()
    {
        return &g_error;
    }

private:
    GError* g_error;

    ScopedGError(ScopedGError const&) = delete;
    ScopedGError& operator=(ScopedGError const&) = delete;
};

}
