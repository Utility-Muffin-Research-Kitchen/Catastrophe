#define CAT_IMPLEMENTATION
#include "catastrophe.h"

#include <assert.h>
#include <stdio.h>

int main(void) {
    assert(SDL_Init(SDL_INIT_EVENTS) == 0);
    cat__g.input_backend_ready = true;
    cat__g.input_delay_ms = 20;

    cat_input_event event;

    /* The first press must work even during SDL's first 20ms. */
    cat__input_push(CAT_BTN_X, true);
    assert(cat_poll_input(&event) && event.button == CAT_BTN_X && event.pressed);

    /* A release must not consume the following press's debounce window. */
    cat__g.buttons_held[CAT_BTN_A] = true;
    cat__input_push(CAT_BTN_A, false);
    cat__input_push(CAT_BTN_A, true);
    assert(cat_poll_input(&event) && event.button == CAT_BTN_A && !event.pressed);
    assert(cat_poll_input(&event) && event.button == CAT_BTN_A && event.pressed);

    /* Duplicate presses debounce per button, without blocking another button. */
    cat__input_push(CAT_BTN_A, true);
    assert(!cat_poll_input(&event));
    cat__input_push(CAT_BTN_B, true);
    assert(cat_poll_input(&event) && event.button == CAT_BTN_B && event.pressed);

    /* Releases and generated hold-repeat bypass fresh-press debounce. */
    cat__input_push(CAT_BTN_B, false);
    assert(cat_poll_input(&event) && event.button == CAT_BTN_B && !event.pressed);
    int next = (cat__input_head + 1) % 64;
    assert(next != cat__input_tail);
    cat__input_queue[cat__input_head] =
        (cat_input_event){ CAT_BTN_UP, true, true };
    cat__input_head = next;
    assert(cat_poll_input(&event) && event.button == CAT_BTN_UP && event.repeated);

    SDL_Quit();
    puts("input debounce checks passed");
    return 0;
}
