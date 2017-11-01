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

#ifndef TARGETDEVICE_H
#define TARGETDEVICE_H


#include "device.h"



namespace WP {


class AxisToButtonData;
class ButtonToAxisData;
class Map;
class TargetDevice : public WP::Device, public WP::Device::Listener
{


public:
    TargetDevice(const std::string &name, uint16_t vendor, uint16_t product, uint16_t version,
                 std::vector<WP::Axis*> &axes, std::vector<WP::Button*> &buttons);
    ~TargetDevice();

    bool open();
    void close();
    void init(WP::Device *src, const WP::Map *map);


private:
    const WP::Map *m_map;
    int m_fd;

    void onDeviceAxisChanged(WP::Device *device, WP::Axis *axis);
    void onDeviceButtonChanged(WP::Device *device, WP::Button *button);

    inline void handle_axis_to_axis(const WP::Axis *src_axis, WP::Axis *target_axis);
    inline void handle_axis_to_button(const WP::Axis *src_axis, WP::Button *target_button, const WP::AxisToButtonData *data);

    inline void handle_button_to_button(const WP::Button *src_button, WP::Button *target_button);
    inline void handle_button_to_axis(const WP::Button *src_button, WP::Axis *target_axis, const WP::ButtonToAxisData *data);


};



} // bnamespace WP


#endif // TARGETDEVICE_H
