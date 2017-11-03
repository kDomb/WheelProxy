#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <cstring>
#include <stdint.h>

#include "../../3rdparty/json/json.hpp"



#define BITS_PER_LONG (sizeof(long) * 8)
#define OFF(x)  ((x)%BITS_PER_LONG)
#define BIT(x)  (1UL<<OFF(x))
#define LONG(x) ((x)/BITS_PER_LONG)
#define test_bit(bit, array)    ((array[LONG(bit)] >> OFF(bit)) & 1)



static void
add_name(int fd, nlohmann::json &json)
{
    char name[1024];
    if (ioctl(fd, EVIOCGNAME(sizeof(name) - 1), name) < 0) {
        perror("ioctl EVIOCGNAME");
        return;
    }

    json["name"] = name;
}


static void
add_info(int fd, nlohmann::json &json)
{
    struct input_id info;
    if (ioctl(fd, EVIOCGID, &info) < 0) {
         perror("ioctl EVIOCGID");
         return;
    }

    json["vendor"] = info.vendor;
    json["product"] = info.product;
    json["version"] = info.version;
}


static void
add_buttons(int fd, nlohmann::json &json)
{
    unsigned long keys[KEY_MAX / 8 + 1];
    memset(keys, 0, sizeof(keys));
    ioctl(fd, EVIOCGBIT(EV_KEY, KEY_MAX), keys);

    for (int event_code = 0; event_code < KEY_MAX; ++event_code) {
        if (test_bit(event_code, keys)) {
            nlohmann::json btn;
            btn["name"] = nullptr;
            btn["code"] = event_code;
            json.push_back(btn);
        }
    }
}


static void
add_axes(int fd, nlohmann::json &json)
{
    unsigned long axes[ABS_MAX / 8 + 1];
    memset(axes, 0, sizeof(axes));
    ioctl(fd, EVIOCGBIT(EV_ABS, ABS_MAX), axes);

    for (int event_code = 0; event_code < ABS_MAX; ++event_code) {
        if (test_bit(event_code, axes)) {

            struct input_absinfo abs;
            if (ioctl(fd, EVIOCGABS(event_code), &abs) < 0) {
                perror("ioctl EVIOCGABS");
                continue;
            }

            nlohmann::json axis;
            axis["name"] = nullptr;
            axis["code"] = event_code;
            axis["min"] = abs.minimum;
            axis["max"] = abs.maximum;
            axis["invert"] = false;
            json.push_back(axis);
        }
    }
}


int main(int argc, char **argv)
{

    if (argc < 2) {
        printf("usage: %s /dev/input/eventXX\n", argv[0]);
        return 1;
    }

    const int fd = open(argv[1], O_RDONLY);
    if (fd < 1) {
        perror("open");
        return 1;
    }

    nlohmann::json json;
    add_name(fd, json["input"]);
    add_info(fd, json["input"]);
    add_buttons(fd, json["input"]["buttons"]);
    add_axes(fd, json["input"]["axes"]);
    printf("%s\n", json.dump(4).c_str());

    close(fd);

    return 0;

}

