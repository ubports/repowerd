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

#include "src/adapters/monotone_spline.h"

#include <gmock/gmock.h>

using namespace testing;

namespace
{

struct AMonotoneSpline : Test
{
    std::vector<repowerd::MonotoneSpline::Point> const points{
        {0.1, 3.0},
        {0.2, 2.9},
        {0.3, 2.5},
        {0.4, 1.0},
        {0.5, 0.9},
        {0.6, 0.8},
        {0.7, 0.5},
        {0.8, 0.2},
        {0.9, 0.1}};

    int find_index(double x)
    {
        if (x < points[0].x) return -1;

        for (auto i = 0u; i < points.size() - 1; ++i)
        {
            if (x >= points[i].x && x < points[i+1].x)
                return i;
        }

        return points.size() - 1;
    }

    repowerd::MonotoneSpline const spline{points};
};

}

TEST_F(AMonotoneSpline, returns_lowest_value_if_out_of_lower_bound)
{
    EXPECT_THAT(spline.interpolate(0.09), Eq(points.front().y));
}

TEST_F(AMonotoneSpline, returns_highest_value_if_out_of_upper_bound)
{
    EXPECT_THAT(spline.interpolate(0.91), Eq(points.back().y));
}

TEST_F(AMonotoneSpline, returns_original_points)
{
    for (auto const& point : points)
        EXPECT_THAT(spline.interpolate(point.x), Eq(point.y));
}

TEST_F(AMonotoneSpline, interpolated_points_lie_between_original_points)
{
    for (auto x = 0.1; x < 0.9; x += 0.01)
    {
        auto i = find_index(x);
        auto y = spline.interpolate(x);
        auto min_y = points[i].y;
        auto max_y = points[i+1].y;
        if (min_y > max_y)
        {
            std::swap(min_y, max_y);
            EXPECT_THAT(y, Gt(min_y));
            EXPECT_THAT(y, Le(max_y));
        }
        else
        {
            EXPECT_THAT(y, Ge(min_y));
            EXPECT_THAT(y, Lt(max_y));
        }
    }
}

TEST_F(AMonotoneSpline, with_fewer_than_two_point_cannot_be_created)
{
    EXPECT_THROW({
        repowerd::MonotoneSpline({});
    }, std::logic_error);

    EXPECT_THROW({
        repowerd::MonotoneSpline({{1,1}});
    }, std::logic_error);
}
