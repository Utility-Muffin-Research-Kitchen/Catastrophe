/* Build on macOS:
 * cc -Iinclude tests/idle-wake.c include/cjson/cJSON.c -o /tmp/cat-idle-wake \
 *   $(pkg-config --cflags --libs sdl2 SDL2_ttf SDL2_image) -lobjc -lm -lpthread
 * For a device build, add its PLATFORM_* define and cross-compile instead.
 */
#define CAT_IMPLEMENTATION
#include "catastrophe.h"
#define CAT_WIDGETS_IMPLEMENTATION
#include "catastrophe_widgets.h"
#include <assert.h>
#include <pthread.h>
#include <unistd.h>

static int wake_pipe[2];

static bool cancelled(void) {
    struct pollfd pfd = { wake_pipe[0], POLLIN, 0 };
    return poll(&pfd, 1, 0) > 0;
}

static void *wake_later(void *unused) {
    (void)unused;
    SDL_Delay(250);
    assert(write(wake_pipe[1], "x", 1) == 1);
    return NULL;
}

static uint32_t wait_frame(void) {
    cat__g.next_redraw_ms = 0;
    cat_request_frame_in(500);
    uint32_t start = SDL_GetTicks();
    cat_present();
    return SDL_GetTicks() - start;
}

int main(int argc, char **argv) {
    assert(SDL_Init(SDL_INIT_TIMER) == 0);
    SDL_Surface *surface = SDL_CreateRGBSurface(0, 16, 16, 32, 0, 0, 0, 0);
    assert(surface);
    cat__g.renderer = SDL_CreateSoftwareRenderer(surface);
    assert(cat__g.renderer);
    assert(pipe(wake_pipe) == 0);
    cat_set_idle_wake_fd(wake_pipe[0]);
    cat__g.wake_rescan_ms = SDL_GetTicks() + 3000;

    /* A signal delivered before poll must remain observable, including when
       a helper thread received it. Catastrophe must not consume the byte. */
    assert(write(wake_pipe[1], "x", 1) == 1);
    assert(wait_frame() < 100);
    assert(wait_frame() < 100);
    char byte;
    assert(read(wake_pipe[0], &byte, 1) == 1);

    /* With no input, stay asleep beyond the old 100 ms heartbeat, but wake
       promptly when the application signals (even without any gamepads). */
    pthread_t thread;
    assert(pthread_create(&thread, NULL, wake_later, NULL) == 0);
    uint32_t elapsed = wait_frame();
    assert(elapsed >= 200 && elapsed < 450);
    assert(pthread_join(thread, NULL) == 0);
    assert(read(wake_pipe[0], &byte, 1) == 1);

    /* Short redraw deadlines must not trigger hotplug rescans. */
    uint32_t rescan = cat__g.wake_rescan_ms;
    cat__g.next_redraw_ms = 0;
    cat_request_frame_in(40);
    cat_present();
    assert(cat__g.wake_rescan_ms == rescan);

    /* The menu's nested list/confirmation must unwind on the same wakeup,
       without consuming it or treating cancellation as a selection. */
    assert(TTF_Init() == 0);
    TTF_Font *font = TTF_OpenFont(argc > 1 ? argv[1] : "res/font.ttf", 12);
    assert(font);
    for (int i = 0; i < CAT_FONT_TIER_COUNT; i++) cat__g.fonts[i] = font;
    cat__g.scale_factor = cat__g.device_scale = 1;
    cat__g.screen_w = 320;
    cat__g.screen_h = 240;
    cat_list_item item = { .label = "Test" };
    cat_list_opts list = cat_list_default_opts("Test", &item, 1);
    list.cancel_requested = cancelled;
    cat_list_result selection;
    assert(pthread_create(&thread, NULL, wake_later, NULL) == 0);
    assert(cat_list(&list, &selection) == CAT_CANCELLED);
    assert(selection.selected_index == -1);
    assert(pthread_join(thread, NULL) == 0);
    cat_message_opts message = { .message = "Test", .cancel_requested = cancelled };
    cat_confirm_result confirmation;
    assert(cat_confirmation(&message, &confirmation) == CAT_CANCELLED);
    assert(!confirmation.confirmed);
    assert(read(wake_pipe[0], &byte, 1) == 1);
    TTF_CloseFont(font);
    TTF_Quit();

    cat_set_idle_wake_fd(-1);
    close(wake_pipe[0]);
    close(wake_pipe[1]);
    SDL_DestroyRenderer(cat__g.renderer);
    SDL_FreeSurface(surface);
    SDL_Quit();
    puts("idle wake: queued/sleeping signals, redraw deadline, and modal cancellation passed");
    return 0;
}
