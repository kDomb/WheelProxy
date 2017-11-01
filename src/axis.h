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
#ifndef AXIS_H
#define AXIS_H

#include "eventsource.h"

#include <stdint.h>
#include <string>


namespace WP {



class Axis : public WP::EventSource
{


public:
    Axis();
    ~Axis();

    int32_t get_min() const;
    void set_min(int32_t min);

    int32_t get_max() const;
    void set_max(int32_t max);

    bool get_invert() const;
    void set_invert(bool invert);

    float get_value_percent() const;
    int32_t get_value() const;

    void set_value_percent(float p);
    void set_value(int32_t value);



private:
    int32_t m_min_max[2];
    bool m_invert;
    int32_t m_value;
    float m_percent;

};



} // namespace WP



#endif // AXIS_H
