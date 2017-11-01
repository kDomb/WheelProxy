/*
   Copyright (C) 2017 Kai Dombrowe <just89@gmx.de>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include "axis.h"

#include <stdlib.h>



namespace WP {



Axis::Axis()
    : WP::EventSource(WP::EventSource::TYPE_AXIS)
{

    m_min_max[0] = 0;
    m_min_max[1] = 0;
    m_invert = false;
    m_value = 0;
    m_percent = 0.0f;

}


Axis::~Axis()
{


}


int32_t
Axis::get_min() const
{

    return m_min_max[0];

}


void
Axis::set_min(int32_t min)
{

    m_min_max[0] = min;

}


int32_t
Axis::get_max() const
{

    return m_min_max[1];

}


void
Axis::set_max(int32_t max)
{

    m_min_max[1] = max;

}


bool
Axis::get_invert() const
{

    return m_invert;

}


void
Axis::set_invert(bool invert)
{

    m_invert = invert;

}


float
Axis::get_value_percent() const
{

    return m_percent;

}


int32_t
Axis::get_value() const
{

    return m_value;

}


void
Axis::set_value(int32_t value)
{

    m_value = value;

    const int32_t range = abs(get_max() - get_min());
    const int64_t abs_value = get_min() < 0 ? value + abs(get_min()) : value;
    m_percent = static_cast<float>(abs_value) / static_cast<float>(range);

}


void
Axis::set_value_percent(float p)
{

    if (get_invert()) {
        p = 1.0 - p;
    }

    const int32_t min = get_min();
    const int32_t max = get_max();
    const int64_t range = abs(max - min);
    m_value = (range * p) + (min > 0 ? min : (min < 0 ? min : 0 + max < 0 ? max : 0));
    m_percent = p;

}



} // namespace WP
