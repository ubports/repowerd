/*
 * Copyright © 2017 Canonical Ltd.
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

#include <random>
#include <mutex>
#include <unordered_set>
#include <cstddef>
#include <limits>

namespace repowerd
{

template <typename T> class UniqueRandomPool
{
public:
    UniqueRandomPool() : UniqueRandomPool{0} {}
    UniqueRandomPool(T min_value) : uniform_dist{min_value} {}

    T generate()
    {
        std::lock_guard<std::mutex> lock{mutex};

        uintmax_t const num_generated = generated.size();

        // This doesn't work properly at the max limit when
        // sizeof(T) == sizeof(uintmax_t), but for such large types we
        // are not going to reach the max limit anyway.
        if (num_generated > 0 &&
            num_generated - 1 == std::numeric_limits<T>::max())
        {
            throw std::runtime_error("UniqueRandomPool exhausted");
        }

        T value;
        do
        {
            value = uniform_dist(random_dev);
        }
        while (generated.find(value) != generated.end());

        generated.insert(value);

        return value;
    }

    void remove(T value)
    {
        std::lock_guard<std::mutex> lock{mutex};
        generated.erase(value);
    }

    size_t size()
    {
        std::lock_guard<std::mutex> lock{mutex};
        return generated.size();
    }

private:
    std::random_device random_dev;
    std::uniform_int_distribution<T> uniform_dist;

    std::mutex mutex;
    std::unordered_set<T> generated;
};

}
