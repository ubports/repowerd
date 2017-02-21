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

#include "src/adapters/unique_random_pool.h"

#include <gmock/gmock.h>
#include <limits>
#include <set>

using namespace testing;

namespace
{

struct AUniqueRandomPool : Test
{
    template <typename T> std::set<T> generate(
        repowerd::UniqueRandomPool<T>& pool,
        size_t n)
    {
        std::set<T> generated;

        for (size_t i = 0; i < n; ++i)
            generated.insert(pool.generate());

        return generated;
    }

    repowerd::UniqueRandomPool<uint8_t> unique_random_pool;
    size_t const nvalues = std::numeric_limits<uint8_t>::max() + 1;
};

}

TEST_F(AUniqueRandomPool, generates_different_values)
{
    auto const generated = generate(unique_random_pool, nvalues);

    EXPECT_THAT(generated.size(), Eq(nvalues));
}

TEST_F(AUniqueRandomPool, generates_value_again_if_removed)
{
    repowerd::UniqueRandomPool<uint8_t> unique_random_pool;

    auto const generated_partial = generate(unique_random_pool, 100);

    EXPECT_THAT(generated_partial.size(), Eq(100));

    for (auto g : generated_partial)
        unique_random_pool.remove(g);

    auto const generated_full = generate(unique_random_pool, nvalues);

    EXPECT_THAT(generated_full.size(), Eq(nvalues));
}

TEST_F(AUniqueRandomPool, generates_values_from_specified_min)
{
    size_t const min_value = 200;
    repowerd::UniqueRandomPool<uint8_t> unique_random_pool_with_min{min_value};
    auto const nvalues_with_min = nvalues - min_value;

    auto const generated = generate(unique_random_pool_with_min, nvalues_with_min);

    for (auto g : generated)
        EXPECT_THAT(g, Ge(min_value));

    EXPECT_THAT(generated.size(), Eq(nvalues_with_min));
}

TEST_F(AUniqueRandomPool, reports_size)
{
    generate(unique_random_pool, 100);
    EXPECT_THAT(unique_random_pool.size(), 100);
}

TEST_F(AUniqueRandomPool, throws_exception_if_exhausted)
{
    EXPECT_THROW({ generate(unique_random_pool, nvalues + 1); }, std::runtime_error);
}
