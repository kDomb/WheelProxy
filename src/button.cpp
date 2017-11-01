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

#include "button.h"



namespace WP {




Button::Button()
    : WP::EventSource(WP::EventSource::TYPE_BUTTON)
{

    m_down = false;

}


Button::Button(uint16_t code)
    : WP::EventSource(WP::EventSource::TYPE_BUTTON)
{

    set_code(code);
    m_down = false;

}


Button::~Button()
{

}



bool
Button::get_down() const
{

    return m_down;

}


void
Button::set_down(bool down)
{

    m_down = down;

}



} // namespace WP
