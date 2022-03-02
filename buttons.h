#ifndef _BUTTONS_H_
#define _BUTTONS_H_

#include "uv.h"
#include "common/common_buttons.h"

#define BTN_HANDLE_EXENTS   10

typedef void (*button_handle_cb)(int, uint64_t, void *);

typedef struct button_events_s {
    button_handle_cb routine;
    void *arg;
} button_events_t;

typedef struct button_handle_s {
    button_id_t id;
    int signum;
    uint32_t counter;
    uint64_t press_timestamp;
    uint64_t hold_time;
    button_events_t events[BTN_HANDLE_EXENTS];
    uint16_t events_num;
    uv_timer_t _timer;
    uv_signal_t _signal;
} button_handle_t;

typedef struct buttons_s {
    uv_loop_t *loop;

    /* private parameters */
    uint16_t number;
    button_handle_t handle[BUTTON_NUM];

    /**
     * @brief buttons module start working
     * 
     * @note it must be calling after "event_register"
     * @param btn buttons module structure
     */
    void (*start)(struct buttons_s *btn);

    /**
     * @brief buttons module stop work
     * 
     * @param btn buttons module structure
     */
    void (*stop)(struct buttons_s *btn);

    /**
     * @brief buttons module events handler register
     * 
     * @param btn buttons module structure
     * @param id button id
     * @param handle register event handler
     * @param arg is passed as the sole argument of handle()
     * @return 0 if handle register success, else return -1
     */
    int (*event_register)(struct buttons_s *btn, button_id_t id, button_handle_cb handle, void *arg);

    /**
     * @brief buttons module handler register 
     *
     * @param btn buttons module structure
     * @param id button id
     * @param handle register handler
     * @return 0 if handler disregister success, else return -1 
     */
    int (*event_disregister)(struct buttons_s *btn, button_id_t id, button_handle_cb handle);
} buttons_t;

/**
 * @brief button module structure initizal
 * 
 * @param btn buttons structure address (non-nil)
 * @param loop uv loop struture (non-nil)
 * @return 0 if initizal success, else return -1
 */
int buttons_init(buttons_t *btn, uv_loop_t *loop);

#endif
