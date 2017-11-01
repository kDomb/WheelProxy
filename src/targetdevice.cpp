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

#include "targetdevice.h"
#include "axis.h"
#include "map.h"
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



static void
send_event(int fd, uint16_t type, uint16_t code, int32_t value, bool syn)
{

    if (fd < 0) return;


    static const size_t s = sizeof(struct input_event);
    static struct input_event event;

    memset(&event, 0, s);

    event.type = type;
    event.code = code;
    event.value = value;
    if (write(fd, &event, s) != s) {
        perror("write");
        return;
    }

    if (syn) {
        event.type = EV_SYN;
        event.code = 0;
        event.value = 0;

        if (write(fd, &event, s) != s) {
            perror("write");
            return;
        }
    }

}



namespace WP {



TargetDevice::TargetDevice(const std::string &name, uint16_t vendor, uint16_t product, uint16_t version,
                           std::vector<WP::Axis*> &axes, std::vector<WP::Button*> &buttons)
    : WP::Device(name, vendor, product, version, axes, buttons)
{

    m_map = nullptr;
    m_fd = -1;

}


TargetDevice::~TargetDevice()
{

    close();

}


bool
TargetDevice::open()
{


    m_fd = ::open("/dev/uinput", O_RDWR);
    if (m_fd < 0) {
        perror("open /dev/uinput");
        return false;
    }


    const std::vector<int> events = { EV_SYN, EV_KEY, EV_ABS };
    for (size_t i = 0, s = events.size(); i < s; ++i) {
        if (ioctl(m_fd, UI_SET_EVBIT, events[i]) < 0) {
            perror("ioctl UI_SET_EVBIT");
            goto failed;
        }
    }


    for (size_t i = 0, c = get_button_count(); i < c; ++i) {
        if (ioctl(m_fd, UI_SET_KEYBIT, get_button_at(i)->get_code()) < 0) {
            perror("ioctl UI_SET_KEYBIT");
            goto failed;
        }
    }


    for (size_t i = 0, c = get_axis_count(); i < c; ++i) {
        if (ioctl(m_fd, UI_SET_ABSBIT, get_axis_at(i)->get_code()) < 0) {
            perror("ioctl UI_SET_ABSBIT");
            goto failed;
        }
    }


    struct uinput_user_dev uidev;
    memset(&uidev, 0, sizeof(uidev));

    if (get_name().length() > UINPUT_MAX_NAME_SIZE - 1) {
        printf("output device name too long!\n");
        goto failed;
    }

    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "%s", get_name().c_str());
    uidev.id.bustype = BUS_USB;
    uidev.id.vendor  = get_vendor();
    uidev.id.product = get_product();
    uidev.id.version = get_version();


    for (size_t i = 0, c = get_axis_count(); i < c; ++i) {
        const WP::Axis *axis = get_axis_at(i);

        uidev.absmin[axis->get_code()] = axis->get_min();
        uidev.absmax[axis->get_code()] = axis->get_max();
    }


    if (write(m_fd, &uidev, sizeof(uidev)) < 0) {
        perror("write");
        goto failed;
    }
    if (ioctl(m_fd, UI_DEV_CREATE) < 0) {
        perror("ioctl UI_DEV_CREATE");
        goto failed;
    }

    goto success;
failed:
    close();
    return false;

success:
    return true;

}


void
TargetDevice::close()
{

    if (m_fd != -1) {
        ::close(m_fd);
        m_fd = -1;
    }

}

void
TargetDevice::init(WP::Device *src, const WP::Map *map)
{

    m_map = map;
    src->set_listener(this);

    if (WP::Application::get_verbose()) {
        printf("[Output] sync with input device...\n");
    }

    for (size_t i = 0, c = src->get_button_count(); i < c; ++i) {
        WP::Button *button = src->get_button_at(i);
        if (button->get_down()) onDeviceButtonChanged(src, button);
    }

    for (size_t i = 0, c = src->get_axis_count(); i < c; ++i) {
        onDeviceAxisChanged(src, src->get_axis_at(i));
    }

    if (WP::Application::get_verbose()) {
        printf("[Output] sync done\n");
    }

}


void
TargetDevice::onDeviceAxisChanged(WP::Device *src_device, WP::Axis *axis)
{

    assert(m_map != nullptr);


    const auto entries = m_map->get_entries_for_src(axis);
    if (entries == nullptr) return;

    for (size_t i = 0, s = entries->size(); i < s; ++i) {
        const WP::MapEntry *entry = (*entries)[i];

        switch (entry->get_target()->get_type()) {
        case WP::EventSource::TYPE_BUTTON:
            handle_axis_to_button(axis, (WP::Button*) entry->get_target(), (WP::AxisToButtonData*) entry->get_data());
            break;
        case WP::EventSource::TYPE_AXIS:
            handle_axis_to_axis(axis, (WP::Axis*) entry->get_target());
            break;
        default: assert(false); break;
        }
    }

}


void
TargetDevice::onDeviceButtonChanged(WP::Device *src_device, WP::Button *button)
{

    assert(m_map != nullptr);


    const auto entries = m_map->get_entries_for_src(button);
    if (entries == nullptr) return;

    for (size_t i = 0, s = entries->size(); i < s; ++i) {
        const WP::MapEntry *entry = (*entries)[i];

        switch (entry->get_target()->get_type()) {
        case WP::EventSource::TYPE_BUTTON:
            handle_button_to_button(button, (WP::Button*) entry->get_target());
            break;
        case WP::EventSource::TYPE_AXIS:
            handle_button_to_axis(button, (WP::Axis*) entry->get_target(), (WP::ButtonToAxisData*) entry->get_data());
            break;
        default: assert(false); break;
        }
    }

}


void
TargetDevice::handle_axis_to_axis(const WP::Axis *src_axis, WP::Axis *target_axis)
{

    target_axis->set_value_percent(src_axis->get_value_percent());

    if (WP::Application::get_verbose()) {
        printf("[Input] axis: (name=\"%s\" code=\"%d\" value=\"%d\") -> [Output] axis: (name=\"%s\" code=\"%d\" value=\"%d\")\n",
               src_axis->get_name().c_str(), src_axis->get_code(), src_axis->get_value(),
               target_axis->get_name().c_str(), target_axis->get_code(), target_axis->get_value());
    }

    send_event(m_fd, EV_ABS, target_axis->get_code(), target_axis->get_value(), true);

}


void
TargetDevice::handle_axis_to_button(const WP::Axis *src_axis, WP::Button *target_button, const WP::AxisToButtonData *data)
{

    assert(data != nullptr);

    const bool in_range = src_axis->get_value() >= data->get_range_start() && src_axis->get_value() <= data->get_range_end();
    if (target_button->get_down() == in_range) return;

    target_button->set_down(in_range);

    if (WP::Application::get_verbose()) {
        printf("[Input] axis: (name=\"%s\" code=\"%d\" value=\"%d\") -> [Output] button: (name=\"%s\" code=\"%d\" down=\"%s\")\n",
               src_axis->get_name().c_str(), src_axis->get_code(), src_axis->get_value(),
               target_button->get_name().c_str(), target_button->get_code(),
               target_button->get_down() ? "true" : "false");
    }

    send_event(m_fd, EV_KEY, target_button->get_code(), target_button->get_down(), true);

}


void
TargetDevice::handle_button_to_button(const WP::Button *src_button, WP::Button *target_button)
{

    target_button->set_down(src_button->get_down());

    if (WP::Application::get_verbose()) {
        printf("[Input] button: (name=\"%s\" code=\"%d\") -> [Output] button: (name=\"%s\" code=\"%d\") down=\"%s\"\n",
               src_button->get_name().c_str(), src_button->get_code(), target_button->get_name().c_str(),
               target_button->get_code(), target_button->get_down() ? "true" : "false");
    }

    send_event(m_fd, EV_KEY, target_button->get_code(), target_button->get_down(), true);

}


void
TargetDevice::handle_button_to_axis(const WP::Button *src_button, WP::Axis *target_axis, const WP::ButtonToAxisData *data)
{

    if (src_button->get_down()) {
        target_axis->set_value(data->get_value_pressed());
    } else {
        target_axis->set_value(data->get_value_released());
    }

    if (WP::Application::get_verbose()) {
        printf("[Input] button: (name=\"%s\" code=\"%d\") -> [Output] axis: (name=\"%s\" code=\"%d\") value=\"%d\"\n",
               src_button->get_name().c_str(), src_button->get_code(), target_axis->get_name().c_str(),
               target_axis->get_code(), target_axis->get_value());
    }

    send_event(m_fd, EV_ABS, target_axis->get_code(), target_axis->get_value(), true);

}



} // namespace WP
