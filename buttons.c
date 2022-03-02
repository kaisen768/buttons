#include <stdlib.h>
#include <string.h>

#include "buttons.h"

static void button_timer_handler(uv_timer_t *handle)
{
    int i;
    button_handle_t *btn_handle = handle->data;

    btn_handle->hold_time += 500;

    for (i = 0; i < BTN_HANDLE_EXENTS; i++) {
        if (btn_handle->events[i].routine) {
            btn_handle->events[i].routine(BTN_EVENT_PRESS, btn_handle->hold_time, btn_handle->events[i].arg);
        }
    }
}

static void button_irq_handler(uv_signal_t *handle, int signum)
{
    int i;
    btn_event_t ev;
    button_handle_t *btn_handle = handle->data;
    uv_timer_t *timer = &btn_handle->_timer;

    ev = buttons_common_get_value(btn_handle->id);

    if (ev == BTN_EVENT_PRESS) {
        btn_handle->counter++;
        btn_handle->press_timestamp = uv_now(timer->loop);
        uv_timer_start(timer, button_timer_handler, 500, 500);
    } else {
        btn_handle->hold_time = uv_now(timer->loop) - btn_handle->press_timestamp;
        uv_timer_stop(timer);
    }

    for (i = 0; i < BTN_HANDLE_EXENTS; i++) {
        if (btn_handle->events[i].routine) {
            btn_handle->events[i].routine(ev, btn_handle->hold_time, btn_handle->events[i].arg);
        }
    }

    if (ev == BTN_EVENT_RELEASE) {
        btn_handle->hold_time = 0;
        btn_handle->press_timestamp = 0;
    }

    return;
}

static void start(buttons_t *btn)
{
    int i;
    uv_timer_t *timer;
    uv_signal_t *signal;

    if (!btn || !btn->loop)
        return;

    for (i = 0; i < btn->number; i++) {
        btn->handle[i].id = button_common_ids[i];
        btn->handle[i].signum = button_common_signums[i];
        btn->handle[i].counter = 0;
        btn->handle[i].press_timestamp = 0;
        btn->handle[i].hold_time = 0;

        timer = &btn->handle[i]._timer;
        signal = &btn->handle[i]._signal;

        timer->data = &btn->handle[i];
        signal->data = &btn->handle[i];

        uv_timer_init(btn->loop, timer);
        uv_signal_init(btn->loop, signal);

        uv_signal_start(signal, button_irq_handler, btn->handle[i].signum);

        buttons_common_device_init(btn->handle[i].id);
    }
}

static void stop(buttons_t *btn)
{
    int i;

    if (!btn)
        return;

    for (i = 0; i < btn->number; i++) {
        uv_signal_stop(&btn->handle[i]._signal);
        uv_close((uv_handle_t*)&btn->handle[i]._signal, NULL);
        uv_close((uv_handle_t*)&btn->handle[i]._timer, NULL);

        buttons_common_device_deinit(btn->handle[i].id);
    }
}

static int button_event_register(buttons_t *btn, button_id_t id, button_handle_cb event, void *arg)
{
    int i;
    button_handle_t *btn_handle;

    if (!btn || !event) {
        return -1;
    }

    for (i = 0; i < btn->number; i++) {
        if (btn->handle[i].id == id) {
            btn_handle = &btn->handle[i];
        }
    }

    if (!btn_handle) {
        return -1;
    }

    if (btn_handle->events_num >= BTN_HANDLE_EXENTS)
        return -1;

    for (i = 0; i < BTN_HANDLE_EXENTS; i++) {
        if (btn_handle->events[i].routine == NULL) {
            btn_handle->events[i].routine = event;
            btn_handle->events[i].arg = arg;
            btn_handle->events_num++;
            break;
        }
    }

    return 0;
}

static int button_event_disregister(buttons_t *btn, button_id_t id, button_handle_cb event)
{
    int i;
    button_handle_t *btn_handle;

    if (!btn || !event) {
        return -1;
    }

    for (i = 0; i < btn->number; i++) {
        if (btn->handle[i].id == id) {
            btn_handle = &btn->handle[i];
        }
    }

    if (!btn_handle || !btn_handle->events_num) {
        return -1;
    }

    for (i = 0; i < BTN_HANDLE_EXENTS; i++) {
        if (btn_handle->events[i].routine == event) {
            btn_handle->events[i].routine = NULL;
            btn_handle->events[i].arg = NULL;
            btn_handle->events_num--;
            break;
        }
    }

    return 0;
}

int buttons_init(buttons_t *btn, uv_loop_t *loop)
{
    if (!btn || !loop)
        return -1;

    btn->loop = loop;

    btn->number = buttons_common_number();

    memset(btn->handle, 0, sizeof(btn->handle));

    btn->start = start;
    btn->stop = stop;
    btn->event_register = button_event_register;
    btn->event_disregister = button_event_disregister;

    return 0;
}
