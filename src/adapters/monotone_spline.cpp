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

#include "monotone_spline.h"
#include <algorithm>
#include <stdexcept>

namespace
{

std::vector<double> secant_slopes(
    std::vector<repowerd::MonotoneSpline::Point> const& points)
{
    std::vector<double> slopes;

    for (auto i = 0u; i < points.size() - 1; ++i)
    {
        auto dy = points[i+1].y - points[i].y;
        auto dx = points[i+1].x - points[i].x;
        slopes.push_back(dy/dx);
    }

    slopes.push_back(0);

    return slopes;
}

std::vector<double> point_tangents(std::vector<double> const& slopes)
{
    std::vector<double> tangents;

    tangents.push_back(slopes.front());
    for (auto i = 1u; i < slopes.size() - 1; ++i)
    {
        if (slopes[i-1]*slopes[i] < 0)
            tangents.push_back(0);
        else
            tangents.push_back((slopes[i-1] + slopes[i]) / 2);
    }
    tangents.push_back(slopes[slopes.size() - 2]);

    return tangents;
}

void ensure_monotonicity(std::vector<double>& tangents,
                         std::vector<double> const& slopes)
{
    auto const alpha =
        [&](int i) { return tangents[i] / slopes[i]; };
    auto const beta =
        [&](int i) { return (i < 0) ? 0 : tangents[i+1] / slopes[i]; };

    for (auto i = 0u; i < slopes.size() - 1; ++i)
    {
        if (slopes[i] == 0.0)
        {
            tangents[i] = 0;
            tangents[++i] = 0;
            continue;
        }

        auto const alpha_i = alpha(i);

        if (alpha_i < 0 || beta(i-1) < 0)
        {
            tangents[i] = 0;
            continue;
        }

        if (alpha_i > 3)
            tangents[i] = 3 * slopes[i];

        if (beta(i) > 3)
            tangents[i+1] = 3 * slopes[i];
    }
}

std::vector<double> calculate_monotone_point_tangents(
    std::vector<repowerd::MonotoneSpline::Point> const& points)
{
    if (points.size() < 2)
        throw std::logic_error("Cannot create spline with fewer than two points");

    auto const slopes = secant_slopes(points);
    auto tangents = point_tangents(slopes);
    ensure_monotonicity(tangents, slopes);
    return tangents;
}

std::vector<repowerd::MonotoneSpline::Point> sorted(
    std::vector<repowerd::MonotoneSpline::Point> const& points)
{
    auto const cmp = [](auto const& p1, auto const& p2) { return p1.x < p2.x; };
    auto sorted_points = points;

    std::sort(sorted_points.begin(), sorted_points.end(), cmp);

    return sorted_points;
}

}

repowerd::MonotoneSpline::MonotoneSpline(
    std::vector<Point> const& points)
    : points{sorted(points)},
      tangents{calculate_monotone_point_tangents(this->points)}
{
}

double repowerd::MonotoneSpline::interpolate(double x) const
{
    auto const i = find_index(x);

    if (i < 0)
        return points.front().y;
    if (i >= static_cast<int>(points.size() - 1))
        return points.back().y;

    auto const h = points[i+1].x - points[i].x;
    auto const t = (x - points[i].x) / h;
    auto const h00 = t * t * (2 * t - 3) + 1;
    auto const h10 = t * (1 + t * (t - 2));
    auto const h01 = t * t * (3 - 2 * t);
    auto const h11 = t * t * (t - 1);

    return h00 * points[i].y +
           h10 * h * tangents[i] +
           h01 * points[i+1].y +
           h11 * h * tangents[i+1];
}

int repowerd::MonotoneSpline::find_index(double x) const
{
    if (x < points[0].x)
        return -1;

    for (auto i = 0u; i < points.size() - 1; ++i)
    {
        if (x >= points[i].x && x < points[i+1].x)
            return i;
    }

    return points.size() - 1;
}
