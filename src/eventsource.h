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


#ifndef EVENTSOURCE_H
#define EVENTSOURCE_H


#include <stdint.h>
#include <string>


namespace WP {



class EventSource
{


public:
    enum Type {
        TYPE_BUTTON,
        TYPE_AXIS
    };


    EventSource(Type type);
    virtual ~EventSource();

    Type get_type() const;

    uint16_t get_code() const;
    void set_code(uint16_t code);

    std::string get_name() const;
    void set_name(const std::string &name);


private:
    Type m_type;
    std::string m_name;
    uint16_t m_code;


};



} // namespace WP



#endif // EVENTSOURCE_H
