#ifndef _COMMON_BUTTONS_H_
#define _COMMON_BUTTONS_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum button_id_e {
    BUTTON_0 = 0,
    BUTTON_1,
    BUTTON_2,
    BUTTON_3,
    BUTTON_4,
    BUTTON_NUM,
} button_id_t;

typedef enum btn_event_e {
    BTN_EVENT_PRESS,
    BTN_EVENT_RELEASE,
    BTN_EVENT_UNKNOW,
} btn_event_t;

extern button_id_t button_common_ids[BUTTON_NUM];
extern int button_common_signums[BUTTON_NUM];

uint16_t buttons_common_number(void);
btn_event_t buttons_common_get_value(button_id_t id);
int buttons_common_device_init(button_id_t id);
void buttons_common_device_deinit(button_id_t id);

#endif
