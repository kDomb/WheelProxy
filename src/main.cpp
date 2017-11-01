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

#include <iostream>
#include <string>
#include <cstring>
#include <stdint.h>
#include <linux/input.h>
#include <string>
#include <fstream>
#include <streambuf>
#include <signal.h>
#include <unistd.h>

#ifndef NO_SECCOMP
#include <sys/prctl.h>
#include <linux/seccomp.h>
#include <linux/filter.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <linux/audit.h>
#include <sys/types.h>
#endif


#include "sourcedevice.h"
#include "targetdevice.h"
#include "axis.h"
#include "map.h"
#include "button.h"
#include "utils.h"
#include "application.h"


#include "3rdparty/json/json.hpp"


#define ArchField offsetof(struct seccomp_data, arch)
#define ALLOW_SYSCALL(name) \
    BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, __NR_##name, 0, 1), \
    BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ALLOW)


enum wp_json_type {
    JSON_TYPE_STRING,
    JSON_TYPE_BOOL,
    JSON_TYPE_NUMBER
};


namespace WP {


class DeviceConfig {
public:
    DeviceConfig() { }
    ~DeviceConfig()
    {
        DELETE_ALL(buttons);
        DELETE_ALL(axes);
    }

    std::string name;
    int32_t vendor;
    int32_t product;
    int32_t version;
    std::vector<WP::Axis*> axes;
    std::vector<WP::Button*> buttons;

};


}


static volatile bool g_stop = false;


static bool
json_get_value(const nlohmann::json &json, wp_json_type type, void *ptr, int32_t min = INT32_MIN, int32_t max = INT32_MAX)
{

    if (json.is_null()) {
        switch (type) {
        case JSON_TYPE_STRING: (*(std::string*) ptr) = ""; break;
        case JSON_TYPE_BOOL: (*(bool*) ptr) = false; break;
        case JSON_TYPE_NUMBER: (*(int32_t*) ptr) = 0; break;
        default: return false;
        }

        return true;
    }

    switch (type) {
    case JSON_TYPE_STRING:
        if (json.type() != nlohmann::json::value_t::string) {
            return false;
        }
        break;
    case JSON_TYPE_BOOL:
        if (json.type() != nlohmann::json::value_t::boolean) {
            return false;
        }
        break;
    case JSON_TYPE_NUMBER:
        if (json.type() != nlohmann::json::value_t::number_integer && json.type() != nlohmann::json::value_t::number_unsigned) {
            return false;
        }

        if (json.type() == nlohmann::json::value_t::number_integer) {
            if (((int64_t) json) > max || ((int64_t) json) < min) {
                return false;
            }
        } else {
            if (max < 0) {
                return false;
            } else if (min >= 0 && ((uint64_t) json) < min) {
                return false;
            } else if (((uint64_t) json) > max) {
                return false;
            }
        }
        break;
    default: return false;
    }

    switch (json.type()) {
    case nlohmann::json::value_t::string: (*(std::string*) ptr) = json; break;
    case nlohmann::json::value_t::boolean: (*(bool*) ptr) = json; break;
    case nlohmann::json::value_t::number_integer:
    case nlohmann::json::value_t::number_unsigned: (*(int32_t*) ptr) = json; break;
    default: return false;
    }

    return true;

}


static bool
json_get_value_from_obj(const nlohmann::json &obj, const std::string &key, wp_json_type type, void *ptr, int32_t min = INT32_MIN, int32_t max = INT32_MAX)
{

    const auto it = obj.find(key);
    if (it == obj.end()) return false;
    return json_get_value(*it, type, ptr, min, max);

}


static bool
json_find_value(const nlohmann::json &obj, const std::vector<std::string> &path, wp_json_type type, void *ptr, int32_t min = INT32_MIN, int32_t max = INT32_MAX)
{

    if (!obj.is_object()) return false;

    nlohmann::json current_obj = obj;
    for (size_t i = 0, s = path.size(); i < s; ++i) {
        if (i == s - 1) {
            return json_get_value_from_obj(current_obj, path[i], type, ptr, min, max);
        } else {
            const auto it = current_obj.find(path[i]);
            if (it == current_obj.end()) return false;
            if (!(*it).is_object()) return false;
            current_obj = *it;
        }
    }

    return false;

}


template <typename T>
static T *
get_event_source_by_code(const std::vector<T*> &vec, uint16_t code)
{

    for (size_t i = 0, s = vec.size(); i < s; ++i) {
        T *e = vec[i];
        if (e->get_code() == code) return e;
    }
    return nullptr;

}


template <typename T>
static T *
get_event_source_by_name(const std::vector<T*> &vec, const std::string &name)
{

    for (size_t i = 0, s = vec.size(); i < s; ++i) {
        T *e = vec[i];
        if (e->get_name().compare(name) == 0) return e;
    }
    return nullptr;

}



static bool
parse_axes(nlohmann::json &dev_obj, std::vector<WP::Axis*> *axes)
{

    const auto _axes_array = dev_obj.find("axes");
    if (_axes_array == dev_obj.end()) return false;

    const auto axes_array = *_axes_array;
    if (axes_array.is_null()) return true;
    if (!axes_array.is_array()) {
        printf("invalid or missing axes element\n");
        return false;
    }

    for (auto it = axes_array.begin(); it != axes_array.end(); ++it) {
        const auto axis_obj = *it;

        std::string name;
        int32_t code;
        int32_t min;
        int32_t max;
        bool invert;


        if (!json_find_value(axis_obj, { "name" }, JSON_TYPE_STRING, (void*) &name) ||
                !json_find_value(axis_obj, { "code" }, JSON_TYPE_NUMBER, (void*) &code, 0, ABS_MAX) ||
                !json_find_value(axis_obj, { "min" }, JSON_TYPE_NUMBER, (void*) &min, INT32_MIN, INT32_MAX) ||
                !json_find_value(axis_obj, { "max" }, JSON_TYPE_NUMBER, (void*) &max, INT32_MIN, INT32_MAX) ||
                !json_find_value(axis_obj, { "invert" }, JSON_TYPE_BOOL, (void*) &invert))
        {
            printf("invalid axis:\n%s\n", axis_obj.dump().c_str());
            return false;
        }


        if (get_event_source_by_code(*axes, code) != nullptr) {
            printf("duplicate axis code %d:\n%s\n", code, axis_obj.dump().c_str());
            return false;
        }


        WP::Axis *axis = new WP::Axis;
        axis->set_code(code);
        axis->set_min(min);
        axis->set_max(max);
        axis->set_name(name);
        axis->set_invert(invert);

        axes->push_back(axis);
    }

    return true;

}


static bool
parse_buttons(nlohmann::json &dev_obj, std::vector<WP::Button*> *buttons)
{

    const auto _button_array = dev_obj.find("buttons");
    if (_button_array == dev_obj.end()) return false;

    const auto button_array = *_button_array;
    if (button_array.is_null()) return true;
    if (!button_array.is_array()) {
        printf("invalid or missing buttons element\n");
        return false;
    }

    for (auto it = button_array.begin(); it != button_array.end(); ++it) {
        const auto button_obj = *it;

        std::string name;
        int32_t code;

        if (!json_find_value(button_obj, { "name" }, JSON_TYPE_STRING, (void*) &name) ||
                !json_find_value(button_obj, { "code" }, JSON_TYPE_NUMBER, (void*) &code, 0, KEY_MAX))
        {
            printf("invalid button:\n%s\n", button_obj.dump().c_str());
            return false;
        }

        if (get_event_source_by_code(*buttons, code) != nullptr) {
            printf("duplicate button code %d:\n%s\n", code, button_obj.dump().c_str());
            return false;
        }

        WP::Button *button = new WP::Button();
        button->set_code(code);
        button->set_name(name);

        buttons->push_back(button);
    }

    return true;

}



static bool
get_device_info(nlohmann::json &json, WP::DeviceConfig *in, WP::DeviceConfig *out)
{

    if (!json_find_value(json, { "input", "name" }, JSON_TYPE_STRING, (void*) &in->name) ||
            !json_find_value(json, { "input", "vendor" }, JSON_TYPE_NUMBER, (void*) &in->vendor, 0, INT32_MAX) ||
            !json_find_value(json, { "input", "product" }, JSON_TYPE_NUMBER, (void*) &in->product, 0, INT32_MAX) ||
            !json_find_value(json, { "input", "version" }, JSON_TYPE_NUMBER, (void*) &in->version, 0, INT32_MAX))
    {
        printf("invalid input device\n");
        return false;
    }

    if (!json_find_value(json, { "output", "name" }, JSON_TYPE_STRING, (void*) &out->name) ||
            !json_find_value(json, { "output", "vendor" }, JSON_TYPE_NUMBER, (void*) &out->vendor, 0, INT32_MAX) ||
            !json_find_value(json, { "output", "product" }, JSON_TYPE_NUMBER, (void*) &out->product, 0, INT32_MAX) ||
            !json_find_value(json, { "output", "version" }, JSON_TYPE_NUMBER, (void*) &out->version, 0, INT32_MAX))
    {
        printf("invalid output device\n");
        return false;
    }


    const auto input = json.find("input");
    const auto output = json.find("output");
    if (input == json.end() || output == json.end() || !(*input).is_object() || !(*output).is_object()) {
        return false;
    }
    return parse_buttons(*input, &in->buttons) && parse_buttons(*output, &out->buttons) && parse_axes(*input, &in->axes) && parse_axes(*output, &out->axes);

}


static WP::EventSource *
parse_map_entry_event_source(const nlohmann::json &entry, bool src, const WP::DeviceConfig *in, const WP::DeviceConfig *out)
{

    WP::EventSource *ret = nullptr;
    const std::string key = src ? "src" : "target";
    std::string name;
    std::string type;
    int32_t code = 0;


    if (!json_find_value(entry, { key, "type" }, JSON_TYPE_STRING, (void*) &type)) {
        printf("missing %s type:\n%s\n", key.c_str(), entry.dump().c_str());
        return nullptr;
    } else if (type.compare("button") != 0 && type.compare("axis") != 0) {
        printf("invalid %s type: %s\n%s\n", key.c_str(), type.c_str(), entry.dump().c_str());
        return nullptr;
    }


    const bool found_code = json_find_value(entry, { key, "code" }, JSON_TYPE_NUMBER, (void*) &code, 0, INT32_MAX);
    const bool found_name = json_find_value(entry, { key, "name" }, JSON_TYPE_STRING, (void*) &name);


    if (!found_code && !found_name) {
        printf("missing %s code or name:\n%s\n", key.c_str(), entry.dump().c_str());
        return nullptr;
    }


    if (type.compare("button") == 0) {
        if (found_name) {
            ret = get_event_source_by_name(src ? in->buttons : out->buttons, name);
            if (ret == nullptr) {
                printf("invalid %s button name: %s\n%s\n", key.c_str(), name.c_str(), entry.dump().c_str());
            }
        } else {
            ret = get_event_source_by_code(src ? in->buttons : out->buttons, code);
            if (ret == nullptr) {
                printf("invalid %s button code: %d\n%s\n", key.c_str(), code, entry.dump().c_str());
            }
        }
    } else if (type.compare("axis") == 0) {
        if (found_name) {
            ret = get_event_source_by_name(src ? in->axes : out->axes, name);
            if (ret == nullptr) {
                printf("invalid %s axis name: %s\n%s\n", key.c_str(), name.c_str(), entry.dump().c_str());
            }
        } else {
            ret = get_event_source_by_code(src ? in->axes : out->axes, code);
            if (ret == nullptr) {
                printf("invalid %s axis code: %d\n%s\n", key.c_str(), code, entry.dump().c_str());
            }
        }
    } else {
        printf("invalid src type: %s\n", type.c_str());
        return nullptr;
    }

    return ret;

}


static WP::MapEntry *
parse_map_entry(const nlohmann::json &entry_obj, const WP::DeviceConfig *in, const WP::DeviceConfig *out)
{

    if (!entry_obj.is_object()) {
        printf("invalid map entry:\n%s\n", entry_obj.dump().c_str());
        return nullptr;
    }


    WP::EventSource *src = parse_map_entry_event_source(entry_obj, true, in, out);
    WP::EventSource *target = parse_map_entry_event_source(entry_obj, false, in, out);
    WP::MapEntry::Data *data = nullptr;

    if (src == nullptr || target == nullptr) {
        goto failed;
    }

    if (src->get_type() == WP::EventSource::TYPE_AXIS) {
        if (target->get_type() == WP::EventSource::TYPE_BUTTON) {
            int32_t range_start;
            int32_t range_end;

            if (!json_find_value(entry_obj, { "src", "range", "start" }, JSON_TYPE_NUMBER, (void*) &range_start, INT32_MIN, INT32_MAX) ||
                    !json_find_value(entry_obj, { "src", "range", "end" }, JSON_TYPE_NUMBER, (void*) &range_end, INT32_MIN, INT32_MAX))
            {
                printf("missing or invalid axis to button range:\n%s\n", entry_obj.dump().c_str());
                goto failed;
            }

            if (range_end > range_start) {
                printf("invalid axis to button range: start > end\n%s\n", entry_obj.dump().c_str());
                goto failed;
            }

            data = new WP::AxisToButtonData(range_start, range_end);
        }
    } else if (src->get_type() == WP::EventSource::TYPE_BUTTON) {
        if (target->get_type() == WP::EventSource::TYPE_AXIS) {
            int32_t value_released;
            int32_t value_pressed;

            if (!json_find_value(entry_obj, { "target", "values", "released" }, JSON_TYPE_NUMBER, (void*) &value_released, INT32_MIN, INT32_MAX) ||
                    !json_find_value(entry_obj, { "target", "values", "pressed" }, JSON_TYPE_NUMBER, (void*) &value_pressed, INT32_MIN, INT32_MAX))
            {
                printf("missing or invalid button to axis values:\n%s\n", entry_obj.dump().c_str());
                goto failed;
            }

            data = new WP::ButtonToAxisData(value_released, value_pressed);
        }
    }

    return new WP::MapEntry(src, target, data);

failed:
    delete data;

    return nullptr;

}


static bool
parse_map(nlohmann::json &json, WP::Map *map, const WP::DeviceConfig *in, const WP::DeviceConfig *out)
{

    const auto _map_array = json.find("map");
    if (_map_array == json.end()) {
        printf("missing map element\n");
        return false;
    }

    const auto map_array = *_map_array;
    if (!map_array.is_array()) {
        printf("invalid map element\n");
        return false;
    }


    for (auto it = map_array.begin(); it != map_array.end(); ++it) {
        WP::MapEntry *entry = parse_map_entry(*it, in, out);
        if (entry == nullptr) return false;
        map->add(entry);
    }

    return true;

}


static bool
parse_config(const char *file, WP::Map *map, WP::DeviceConfig *in, WP::DeviceConfig *out)
{

    std::ifstream ifs(file);
    std::string str;

    if (ifs.fail()) {
        printf("\"%s\": %s\n", file, strerror(errno));
        return false;
    }

    ifs.seekg(0, std::ios::end);
    if (ifs.tellg() < 1) {
        printf("empty config...\n");
        return false;
    }
    str.reserve(ifs.tellg());
    ifs.seekg(0, std::ios::beg);

    str.assign((std::istreambuf_iterator<char>(ifs)),
                std::istreambuf_iterator<char>());
    if (str.length() < 1) {
        printf("empty config\n");
        return false;
    }


    try {
        auto json = nlohmann::json::parse(str);
        if (!json.is_object()) {
            return false;
        }

        if (!get_device_info(json, in, out)) return false;
        if (!parse_map(json, map, in, out)) return false;
    } catch (const std::exception &e) {
        printf("%s\n", e.what());
        return false;
    }

    return true;

}



static void
print_map_pretty(int m, const std::string &in, const std::string &out)
{

    const int c = (m - in.length()) - 3;
    printf("%*s\"%s\" -> \"%s\"\n", c > 0 ? c : 1, " ", in.c_str(), out.c_str());

}


static void
print_map_entry(const WP::MapEntry *entry, int m)
{

    const std::string in = entry->get_src()->get_name();
    const std::string out = entry->get_target()->get_name();
    const int c = (m - in.length()) - 3;
    printf("%*s\"%s\" -> \"%s\"\n", c > 0 ? c : 1, " ", in.c_str(), out.c_str());

}


static void
print_config(const WP::Map *map, const WP::DeviceConfig *in, const WP::DeviceConfig *out)
{

    printf("%s -> %s\n", in->name.c_str(), out->name.c_str());

    const int m = in->name.length() + 1;

    for (size_t i = 0, s = in->axes.size(); i < s; ++i) {
        WP::Axis *src = in->axes[i];

        auto entries = map->get_entries_for_src(src);
        if (entries == nullptr) continue;
        for (size_t j = 0, jS = entries->size(); j < jS; ++j) {
            print_map_entry((*entries)[j], m);
        }
    }

    for (size_t i = 0, s = in->buttons.size(); i < s; ++i) {
        WP::Button *src = in->buttons[i];

        auto entries = map->get_entries_for_src(src);
        if (entries == nullptr) continue;
        for (size_t j = 0, jS = entries->size(); j < jS; ++j) {
            print_map_entry((*entries)[j], m);
        }
    }

    printf("\n\n");

}


#ifndef NO_SECCOMP
static int
install_syscall_filter()
{
    struct sock_filter filter[] = {
        // validate arch
        BPF_STMT(BPF_LD+BPF_W+BPF_ABS, ArchField),
        BPF_JUMP( BPF_JMP+BPF_JEQ+BPF_K, AUDIT_ARCH_X86_64, 1, 0),
        BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_KILL),

        // load syscall
        BPF_STMT(BPF_LD+BPF_W+BPF_ABS, offsetof(struct seccomp_data, nr)),

        // list of allowed syscalls
        ALLOW_SYSCALL(fstat),
        ALLOW_SYSCALL(exit_group),
        ALLOW_SYSCALL(open),
        ALLOW_SYSCALL(read),
        ALLOW_SYSCALL(write),
        ALLOW_SYSCALL(ioctl),
        ALLOW_SYSCALL(close),
        ALLOW_SYSCALL(lseek),
        ALLOW_SYSCALL(rt_sigreturn),
        ALLOW_SYSCALL(poll),
        ALLOW_SYSCALL(nanosleep),
        ALLOW_SYSCALL(dup),
        ALLOW_SYSCALL(fcntl),

        // and if we don't match above, die
        BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_TRAP), // TRAP
    };
    struct sock_fprog prog = {
        .len = (unsigned short)(sizeof(filter)/sizeof(filter[0])),
        .filter = filter,
    };

    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0)) {
        perror("prctl(NO_NEW_PRIVS)");
        goto failed;
    }
    if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog)) {
        perror("prctl(SECCOMP)");
        goto failed;
    }
    return 0;

failed:
    if (errno == EINVAL)
        printf("SECCOMP_FILTER is not available.\n");
    return 1;
}
#endif


static void
sig_handler(int sig)
{

    if (sig == SIGINT) {
        g_stop = true;
    }

}


static void
print_usage(const char *cmd)
{

    printf("usage: %s [--verbose] --config FILE\n", cmd);

}



int main(int argc, char **argv)
{

    signal(SIGINT, sig_handler);

#ifndef NO_SECCOMP
    install_syscall_filter();
#endif

    const char *file = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            WP::Application::set_verbose(true);
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--config") == 0) {
            if (i + 1 >= argc) {
                print_usage(argv[0]);
                return 1;
            }

            file = argv[i + 1];
        }
    }

    if (file == nullptr) {
        print_usage(argv[0]);
        return 1;
    }

    WP::Map map;
    WP::DeviceConfig in, out;
    if (!parse_config(file, &map, &in, &out)) {
        return 1;
    }


    print_config(&map, &in, &out);


    WP::SourceDevice src(in.name, in.vendor, in.product, in.version, in.axes, in.buttons);
    WP::TargetDevice target(out.name, out.vendor, out.product, out.version, out.axes, out.buttons);

    if (!src.open()) return 1;
    if (!target.open()) return 1;

    target.init(&src, &map);

    printf("\n\nPress Ctrl+C to stop.\n");
    while (!g_stop) {
        if (!src.next_event()) {
            // device lost, try to open again until SIGINT
            printf("input device lost, trying to recover...\nPress Ctrl+C to stop.\n");
            src.close();
            usleep(1000000 * 2);
            if (src.open()) {
                target.init(&src, &map); // sync
            }
        }
    }

    if (g_stop) {
        printf("received SIGINT, exiting...\n");
    }

    return 0;

}
