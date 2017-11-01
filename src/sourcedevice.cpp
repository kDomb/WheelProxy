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

#include "sourcedevice.h"
#include "axis.h"
#include "button.h"
#include "application.h"

#include <assert.h>
#include <errno.h>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <vector>
#include <signal.h>
#include <algorithm>
#include <string>
#include <fstream>
#include <streambuf>
#include <sstream>
#include <iterator>
#include <algorithm>
#include <regex>
#include <poll.h>


static bool
is_button_pressed(int fd, int key)
{

    char key_b[KEY_MAX/8 + 1];
    memset(key_b, 0, sizeof(key_b));
    ioctl(fd, EVIOCGKEY(sizeof(key_b)), key_b);
    return !!(key_b[key/8] & (1<<(key % 8)));

}



static void
device_src_parse_i(const std::string &line, uint16_t *vendor, uint16_t *product, uint16_t *version)
{

    *vendor = 0;
    *product = 0;
    *version = 0;


    static const std::regex regex("^I: Bus=([0-9a-zA-Z]+)\\sVendor=([0-9a-zA-Z]+)\\sProduct=([0-9a-zA-Z]+)\\sVersion=([0-9a-zA-Z]+)$");
    std::smatch m;
    if (!std::regex_match(line, m, regex) || m.size() != 5) {
        return;
    }

    std::stringstream ss;

    ss << std::hex << m[2];
    ss >> *vendor;

    ss.clear();
    ss << std::hex << m[3];
    ss >> *product;

    ss.clear();
    ss << std::hex << m[4];
    ss >> *version;

}


static void
device_src_parse_h(const std::string &line, std::string *handler)
{

    static const std::regex regex("(event[0-9]+)");
    std::smatch m;
    if (!std::regex_search(line, m, regex)) {
        return;
    }

    *handler = m[0];

}



namespace WP {



SourceDevice::SourceDevice(const std::string &name, uint16_t vendor, uint16_t product, uint16_t version,
                           std::vector<WP::Axis*> &axes, std::vector<WP::Button*> &buttons)
    : WP::Device(name, vendor, product, version, axes, buttons)
{

}


SourceDevice::~SourceDevice()
{

    close();

}


bool
SourceDevice::open()
{

    const std::string handler = get_handler();
    if (handler.size() < 1) {
        printf("input device not found: name=\"%s\" vendor=\"%04x\" product=\"%04x\" version=\"%04x\"\n",
               get_name().c_str(), get_vendor(), get_product(), get_version());
        printf("please connect the device and try again\n");
        return false;
    }

    const std::string path = "/dev/input/" + handler;

    errno = 0;
    m_fd = ::open(path.c_str(), O_RDONLY/*O_RDWR*/);
    if (m_fd < 0) {
        m_fd = -1;
        perror("failed to open input device");
        return false;
    }

    m_fds[0].fd = m_fd;
    m_fds[0].events = POLLIN;


    if (WP::Application::get_verbose()) {
        printf("[Input] get initial state...\n");
    }

    unsigned long keys[KEY_MAX/8 + 1];
    memset(keys, 0, sizeof(keys));
    if (ioctl(m_fd, EVIOCGKEY(sizeof(keys)), &keys) < 0) {
        perror("ioctl EVIOCGBIT");
    } else {
        for (size_t i = 0, c = get_button_count(); i < c; ++i) {
            WP::Button *button = get_button_at(i);

            button->set_down(is_button_pressed(m_fd, button->get_code()));

            if (WP::Application::get_verbose()) {
                printf("[Input] button=\"%s\" state=\"%s\"\n", button->get_name().c_str(), button->get_down() ? "pressed" : "released");
            }
        }
    }


    for (size_t i = 0, c = get_axis_count(); i < c; ++i) {
        WP::Axis *axis = get_axis_at(i);

        struct input_absinfo abs;
        if (ioctl(m_fd, EVIOCGABS(axis->get_code()), &abs) < 0) {
            perror("ioctl EVIOCGABS");
            continue;
        }
        axis->set_value(abs.value);
        if (WP::Application::get_verbose()) {
            printf("[Input] axis=\"%s\" position=\"%d\"\n", axis->get_name().c_str(), axis->get_value());
        }
    }

    if (WP::Application::get_verbose()) {
        printf("[Input] done\n");
    }

    return true;

}


void
SourceDevice::close()
{

    if (m_fd != -1) {
        ::close(m_fd);
        m_fd = -1;
    }

}


bool
SourceDevice::next_event()
{

    if (m_fd < 0) return false;

    const int i = poll(m_fds, 1, -1);
    if (i == -1) {
        if (errno != EINTR) {
            perror("poll");
            return false;
        }
        return true;
    }

    if (m_fds[0].revents & POLLIN) {
        static struct input_event event;
        static const size_t s = sizeof(struct input_event);
        memset(&event, 0, s);

        if (read(m_fd, &event, sizeof(event)) != s) {
            if (m_fd != -1) {
                perror("read");
            }
            return false;
        }


        switch (event.type) {
        case EV_KEY: handle_key(event.code, event.value); break;
        case EV_ABS: handle_abs(event.code, event.value); break;
        default: break;
        }

        return true;
    }

    return false;

}



std::string
SourceDevice::get_handler() const
{

    static const char *i_line = "I: ";
    static const char *h_line = "H: ";


    std::string str;
    std::string line;

    uint16_t vendor = 0;
    uint16_t product = 0;
    uint16_t version = 0;
    std::string handler;


    std::ifstream stream("/proc/bus/input/devices");
    if (stream.fail()) {
        perror("/proc/bus/input/devices");
        return std::string();
    }
    str.assign((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());


    std::istringstream ss(str);
    while (std::getline(ss, line)) {
        if (line.compare(0, strlen(i_line), i_line) == 0) {
            device_src_parse_i(line, &vendor, &product, &version);
        } else if (line.compare(0, strlen(h_line), h_line) == 0) {
            device_src_parse_h(line, &handler);
        } else if (line.compare("") == 0) {
            vendor = 0;
            product = 0;
            version = 0;
            handler.clear();
        }


        if (vendor == get_vendor() && product == get_product() &&
                /*version == get_version() &&*/ handler.size() > 0)
        {
            return handler;
        }
    }

    return std::string();

}


void
SourceDevice::handle_key(uint16_t code, int32_t value)
{

    WP::Button *button = get_button_by_code(code);
    if (button != nullptr) {
        button->set_down(value > 0);
        onButtonChanged(button);
    } else {
        if (WP::Application::get_verbose()) {
            printf("[Input] button: code=\"%d\" value=\"%d\" -> ignored\n", code, value);
        }
    }

}


void
SourceDevice::handle_abs(uint16_t code, int32_t value)
{

    WP::Axis *axis = get_axis_by_code(code);
    if (axis != nullptr) {
        axis->set_value(value);
        onAxisChanged(axis);
    } else {
        if (WP::Application::get_verbose()) {
            printf("[Input] axis: code=\"%d\" value=\"%d\" -> ignored\n", code, value);
        }
    }
}



} // namespace WP
