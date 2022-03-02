#include <stdlib.h>
#include "uv.h"
#include "buttons.h"

static void button0_test_handler_1(int status, uint64_t hold_time, void *param)
{
    fprintf(stderr, "Button 0 status : %d, hold time : %llums\n", status, hold_time);
}

int main(int argc, const char *argv[])
{
    buttons_t *button;
    uv_loop_t *loop;

    loop = uv_default_loop();
    button = malloc(sizeof(buttons_t));

    buttons_init(button, loop);

    button->event_register(button, BUTTON_0, button0_test_handler_1, NULL);

    button->start(button);

    uv_run(loop, UV_RUN_DEFAULT);

    return 0;
}
