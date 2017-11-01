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


#include "eventsource.h"

namespace WP {

EventSource::EventSource(Type type)
{

    m_type = type;
    m_code = 0;

}


EventSource::~EventSource()
{


}


EventSource::Type
EventSource::get_type() const
{

    return m_type;

}


uint16_t
EventSource::get_code() const
{

    return m_code;

}


void
EventSource::set_code(uint16_t code)
{

    m_code = code;

}


std::string
EventSource::get_name() const
{

    return m_name;

}


void
EventSource::set_name(const std::string &name)
{

    m_name = name;

}



} // namespace WP
