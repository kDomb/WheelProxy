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

#ifndef SOURCEDEVICE_H
#define SOURCEDEVICE_H

#include "device.h"

#include <poll.h>


namespace WP {



class SourceDevice : public WP::Device
{


public:
    SourceDevice(const std::string &name, uint16_t vendor, uint16_t product, uint16_t version,
                 std::vector<WP::Axis*> &axes, std::vector<WP::Button*> &buttons);
    ~SourceDevice();

    bool open();
    void close();

    bool next_event();


private:
    int m_fd;
    struct pollfd m_fds[1];

    std::string get_handler() const;

    inline void handle_key(uint16_t code, int32_t value);
    inline void handle_abs(uint16_t code, int32_t value);


};



} // namespace WP



#endif // SOURCEDEVICE_H
