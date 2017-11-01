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

#ifndef DEVICE_H
#define DEVICE_H

#include <string>
#include <stdint.h>
#include <vector>



namespace WP {


class Button;
class Axis;
class Event;
class Device
{


public:
    class Listener {
    public:
        virtual void onDeviceAxisChanged(WP::Device *device, WP::Axis *axis) = 0;
        virtual void onDeviceButtonChanged(WP::Device *device, WP::Button *button) = 0;
    };


    Device(const std::string &name, uint16_t vendor, uint16_t product, uint16_t version,
           std::vector<WP::Axis*> &axes, std::vector<WP::Button*> &buttons);
    virtual ~Device();

    std::string get_name() const;
    uint16_t get_vendor() const;
    uint16_t get_product() const;
    uint16_t get_version() const;

    size_t get_axis_count() const;
    WP::Axis *get_axis_at(size_t i) const;
    WP::Axis *get_axis_by_code(uint16_t code) const;
    void add_axis(WP::Axis *axis);

    size_t get_button_count() const;
    WP::Button *get_button_at(size_t i) const;
    WP::Button *get_button_by_code(uint16_t code) const;
    void add_button(WP::Button *button);


    virtual bool open() = 0;
    virtual void close() = 0;

    void set_listener(WP::Device::Listener *listener);


protected:
    void onAxisChanged(WP::Axis *axis);
    void onButtonChanged(WP::Button *button);


private:
    std::string m_name;
    uint16_t m_data[3];
    std::vector<WP::Button*> m_buttons;
    std::vector<WP::Axis*> m_axes;
    WP::Device::Listener *m_listener;

    bool src_event(WP::Event *event) const;
    bool dest_event(const WP::Event *event);




};


} // namespace WP



#endif // DEVICE_H
