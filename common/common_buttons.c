#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "common_buttons.h"

#define BUTTON0_DEVICE "/dev/buttons"

button_id_t button_common_ids[BUTTON_NUM] = {
    [0] = BUTTON_0,
};

int button_common_signums[BUTTON_NUM] = {
    [0] = SIGIO,
};

static volatile int btnfd = -1;

uint16_t buttons_common_number(void)
{
    return 1;
}

btn_event_t buttons_common_get_value(button_id_t id)
{
    unsigned char value[1] = {0};

    if (id != BUTTON_0)
        return -1;

    if (btnfd < 0)
        return BTN_EVENT_UNKNOW;

    if (read(btnfd, value, sizeof(value)) < 0)
        return BTN_EVENT_UNKNOW;

    if (!value[0])
        return BTN_EVENT_PRESS;

    return BTN_EVENT_RELEASE;
}

int buttons_common_device_init(button_id_t id)
{
    int flag;

    if (id != BUTTON_0)
        return -1;

    if (btnfd > 0)
        return 0;

    btnfd = open(BUTTON0_DEVICE, O_RDWR);
    if (btnfd < 0)
        return -1;

    fcntl(btnfd, F_SETOWN, getpid());
    flag = fcntl(btnfd, F_GETFL);
    fcntl(btnfd, F_SETFL, flag|FASYNC);

    return 0;
}

void buttons_common_device_deinit(button_id_t id)
{
    if (id != BUTTON_0)
        return;

    if (btnfd > 0)
        close(btnfd);

    btnfd = -1;
}
