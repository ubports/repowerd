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

#include <chrono>
#include <string>

namespace repowerd
{

class TemporarySuspendInhibition
{
public:
    virtual ~TemporarySuspendInhibition() = default;

    virtual void inhibit_suspend_for(std::chrono::milliseconds timeout, std::string const& name) = 0;

protected:
    TemporarySuspendInhibition() = default;
    TemporarySuspendInhibition(TemporarySuspendInhibition const&) = delete;
    TemporarySuspendInhibition& operator=(TemporarySuspendInhibition const&) = delete;
};

}
