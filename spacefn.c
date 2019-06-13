/*
 * spacefn-evdev.c
 * James Laird-Wah (abrasive) 2018
 * This code is in the public domain.
 */

#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <libconfig.h>

// Global device handles {{{1
struct libevdev *idev;
struct libevdev_uinput *odev;
int fd;

// Ordered unique key buffer {{{1
#define MAX_BUFFER 8
static unsigned int buffer[MAX_BUFFER];
static unsigned int n_buffer = 0;

enum key_io_functions {
    V_RELEASE,
    V_PRESS,
    V_REPEAT
};

// State functions {{{1
enum {
    IDLE,
    DECIDE,
    SHIFT,
} state = IDLE;

/* keys map */
#define MAX_KEYS_NUM 104
static int keys_map [MAX_KEYS_NUM][3];


/********** 按键 buffer 辅助操作函数 **********/

static int buffer_contains(unsigned int code) {
    for (int i = 0; i < n_buffer; i++) {
        if (buffer[i] == code) {
            return 1;
        }
    }
    return 0;
}

static int buffer_remove(unsigned int code) {
    for (int i = 0; i < n_buffer; i++) {
        if (buffer[i] == code) {
            memcpy(&buffer[i], &buffer[i + 1], (n_buffer - i - 1) * sizeof(*buffer));
            n_buffer--;
            return 1;
        }
    }
    return 0;
}

static int buffer_append(unsigned int code) {
    if (n_buffer >= MAX_BUFFER) {
        return 1;
    }
    buffer[n_buffer++] = code;
    return 0;
}

// Key mapping {{{1
unsigned int key_map(unsigned int code, int* ext_code) {
    int keys_map_len = sizeof(keys_map) / sizeof(keys_map[0]);
    for (int i = 0; i < keys_map_len; i++) {
        if (code != keys_map[i][0]) {
            continue;
        }
        if (keys_map[i][2] != 0) {
            *ext_code = keys_map[i][2];
        }
        return keys_map[i][1];
    }
    return 0;
}

// Blacklist keys for which I have a mapping, to try and train myself out of using them
int blacklist(unsigned int code) {
    switch (code) {
        case KEY_UP:
        case KEY_DOWN:
        case KEY_RIGHT:
        case KEY_LEFT:
        case KEY_HOME:
        case KEY_END:
        case KEY_PAGEUP:
        case KEY_PAGEDOWN:
            return 1;
    }
    return 0;
}

static void send_key(unsigned int code, int value) {
    libevdev_uinput_write_event(odev, EV_KEY, code, value);
    libevdev_uinput_write_event(odev, EV_SYN, SYN_REPORT, 0);
}

static int send_mapped_key(unsigned int code, int value) {
    int ext_code = 0;
    unsigned int mapped_code = key_map(code, &ext_code);
    if (mapped_code) {
        code = mapped_code; 
    }
    // @todo 弹起时应该后发送
    if (ext_code) {
        send_key(ext_code, value); 
        ext_code = 0;
    }
    send_key(code, value); 
    return mapped_code;
}

static void send_press(unsigned int code) {
    send_key(code, 1);
}

static void send_release(unsigned int code) {
    send_key(code, 0);
}

static void send_repeat(unsigned int code) {
    send_key(code, 2);
}

// input {{{2
static int read_one_key(struct input_event *ev) {
    int err = libevdev_next_event(idev, LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_BLOCKING, ev);
    if (err) {
        fprintf(stderr, "Failed: (%d) %s\n", -err, strerror(err));
        exit(1);
    }
    if (ev->type != EV_KEY) {
        return -1;
    }
    // 取消原作者的退出快捷键
    //if (blacklist(ev->code)) {
    //    return -1;
    //}
    return 0;
}

static void state_idle(void) {  // {{{2
    struct input_event ev;
    for (;;) {
        while (read_one_key(&ev));
        // 按下空格后进入 FN 决策状态
        if (ev.code == KEY_SPACE && ev.value == V_PRESS) {
            state = DECIDE;
            return;
        }
        // 除按下空格键以外的都以正常键处理
        send_key(ev.code, ev.value);
    }
}

/* 空格键被按下，进入中间状态，需要判断是长按空格还是短按，长按才进入 FN 状态 */
static void state_decide(void) {    // {{{2
    n_buffer = 0;
    struct input_event ev;
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 200000;
    fd_set set;
    FD_ZERO(&set);

    while (timeout.tv_usec >= 0) {
        FD_SET(fd, &set);
        // 等待空格键之后的按键
        int nfds = select(fd+1, &set, NULL, &set, &timeout);
        // 指定时间内为获得新的按键，进入超时处理逻辑
        if (!nfds) {
            break;
        }

        while (read_one_key(&ev));

        //  记录空格状态下按下的所有键
        if (ev.value == V_PRESS) {
            buffer_append(ev.code);
            continue;
        }

        // 如果空格在指定时间内弹起，则认定行为为单击空格，不做按键转换，直接退回 IDLE 状态
        // 在该时间内按下的键都以正常按键处理
        if (ev.code == KEY_SPACE && ev.value == V_RELEASE) {
            send_key(KEY_SPACE, V_PRESS);
            send_key(KEY_SPACE, V_RELEASE);
            for (int i=0; i<n_buffer; i++)
                send_key(buffer[i], V_PRESS);
            state = IDLE;
            return;
        }

        // 在该状态下按下并弹起的所有按键都以正常键处理
        if (ev.value == V_RELEASE) {
            if (buffer_contains(ev.code)) {
                buffer_remove(ev.code);
            }
            send_key(ev.code, ev.value);
            continue;
        }
    }

    // 超时处理逻辑，依次将 buffer 中的按键发出
    for (int i=0; i<n_buffer; i++) {
        // 查找按键映射，如果未映射，将原键发出
        // 超时之前未弹起的按键都视为映射键
        send_mapped_key(buffer[i], V_PRESS);
    }

    // 正式进入 FN 状态
    state = SHIFT;
}

/* 正式进入 FN 状态 */
static void state_shift(void) {
    n_buffer = 0;
    struct input_event ev;
    for (;;) {
        while (read_one_key(&ev));
        // 空格键被松开
        if (ev.code == KEY_SPACE && ev.value == V_RELEASE) {
            // 遍历 buffer 中的按键，依次发从弹起事件
            for (int i=0; i<n_buffer; i++) {
                send_key(buffer[i], V_RELEASE);
            }
            // 重新进入 IDLE 状态
            state = IDLE;
            return;
        }
        if (ev.code == KEY_SPACE)
            continue;

        int mapped_code = send_mapped_key(ev.code, ev.value);
        if (mapped_code) {
            if (ev.value == V_PRESS) {
                buffer_append(mapped_code);
            } else if (ev.value == V_RELEASE) {
                buffer_remove(mapped_code);
            }
        }
    }
}

static void run_state_machine(void) {
    for (;;) {
        switch (state) {
            case IDLE:
                state_idle();
                break;
            case DECIDE:
                state_decide();
                break;
            case SHIFT:
                state_shift();
                break;
        }
    }
}


int main(int argc, char **argv) {   // {{{1
    // 检查运行参数
    if (argc < 2) {
        printf("usage: %s config.cfg...", argv[0]);
        return 1;
    }

    const config_setting_t *keys;
    const config_setting_t *keyboard;

    config_t cfg, *cf;
    cf = &cfg;
    config_init(cf);

    if (!config_read_file(cf, argv[1])) {
        fprintf(stderr, "read config file error %s:%d %s\n", config_error_file(cf), config_error_line(cf), config_error_text(cf)); 
        config_destroy(cf);
        return 1;
    }

    // 将配置文件中的参数存入 keys_map 数组
    keys = config_lookup(cf, "keys_map");
    int keys_count = config_setting_length(keys);
    int max_num = keys_count > MAX_KEYS_NUM ? MAX_KEYS_NUM : keys_count;
    for (int i = 0; i < max_num; i++) {
        config_setting_t* key = config_setting_get_elem(keys, i);
        if (config_setting_length(key) < 3) {
            fprintf(stderr, "key map error for the %dth key, three code required\n", i + 1);
            config_destroy(cf);
            return 1;
        }
        for (int j = 0; j < 3; j++){
            keys_map[i][j] = config_setting_get_int_elem(key, j);
        }
    }

    // 键盘读取
    keyboard = config_lookup(cf, "keyboard");
    fd = open(config_setting_get_string(keyboard), O_RDONLY);
    if (fd < 0) {
        config_destroy(cf);
        perror("open input");
        return 1;
    }

    // 释放配置文件
    config_destroy(cf);

    int err = libevdev_new_from_fd(fd, &idev);
    if (err) {
        fprintf(stderr, "Failed: (%d) %s\n", -err, strerror(err));
        return 1;
    }

    int uifd = open("/dev/uinput", O_RDWR);
    if (uifd < 0) {
        perror("open /dev/uinput");
        return 1;
    }

    err = libevdev_uinput_create_from_device(idev, uifd, &odev);
    if (err) {
        fprintf(stderr, "Failed: (%d) %s\n", -err, strerror(err));
        return 1;
    }

    // 修正无限回车的问题
    usleep(200000);

    err = libevdev_grab(idev, LIBEVDEV_GRAB);
    if (err) {
        fprintf(stderr, "Failed: (%d) %s\n", -err, strerror(err));
        return 1;
    }

    run_state_machine();
}
