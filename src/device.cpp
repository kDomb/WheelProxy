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

#include "device.h"
#include "event.h"
#include "axis.h"
#include "button.h"
#include "utils.h"


#define VENDOR_IDX   0
#define PRODUCT_IDXX 1
#define VERSION_IDX  2

#define INDEX(a, b) (a >= b ? a * a + a + b : a + b * b)



namespace WP {




Device::Device(const std::string &name, uint16_t vendor, uint16_t product, uint16_t version,
               std::vector<WP::Axis*> &axes, std::vector<WP::Button*> &buttons)
{

    m_name = name;
    m_data[VENDOR_IDX] = vendor;
    m_data[PRODUCT_IDXX] = product;
    m_data[VERSION_IDX] = version;
    m_axes.swap(axes);
    m_buttons.swap(buttons);
    m_listener = nullptr;

}


Device::~Device()
{

    DELETE_ALL(m_axes);
    DELETE_ALL(m_buttons);

}


std::string
Device::get_name() const
{

    return m_name;

}


uint16_t
Device::get_vendor() const
{

    return m_data[VENDOR_IDX];

}


uint16_t
Device::get_product() const
{

    return m_data[PRODUCT_IDXX];

}


uint16_t
Device::get_version() const
{

    return m_data[VERSION_IDX];

}


size_t
Device::get_axis_count() const
{

    return m_axes.size();

}


WP::Axis *
Device::get_axis_at(size_t i) const
{

    return m_axes[i];

}


WP::Axis *
Device::get_axis_by_code(uint16_t code) const
{

    for (size_t i = 0, s = m_axes.size(); i < s; ++i) {
        WP::Axis *axis = m_axes[i];

        if (axis->get_code() == code) return axis;
    }

    return nullptr;

}


void
Device::add_axis(WP::Axis *axis)
{

    m_axes.push_back(axis);

}


size_t
Device::get_button_count() const
{

    return m_buttons.size();

}


WP::Button *
Device::get_button_at(size_t i) const
{

    return m_buttons[i];

}


WP::Button *
Device::get_button_by_code(uint16_t code) const
{

    for (size_t i = 0, s = m_buttons.size(); i < s; ++i) {
        WP::Button *button = m_buttons[i];

        if (button->get_code() == code) return button;
    }

    return nullptr;

}


void
Device::add_button(WP::Button *button)
{

    m_buttons.push_back(button);

}


void
Device::set_listener(WP::Device::Listener *listener)
{

    m_listener = listener;

}


void
Device::onAxisChanged(WP::Axis *axis)
{

    m_listener->onDeviceAxisChanged(this, axis);

}


void
Device::onButtonChanged(WP::Button *button)
{

    m_listener->onDeviceButtonChanged(this, button);

}



} // namespace WP
