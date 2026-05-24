/*
 * Catastrophe Hello World
 *
 * Minimal example: displays a list with a few items, lets the user select
 * one, and shows a confirmation dialog with the result.
 */

#define CAT_IMPLEMENTATION
#include "catastrophe.h"
#define CAT_WIDGETS_IMPLEMENTATION
#include "catastrophe_widgets.h"

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    /* Initialise Catastrophe — auto-detects screen size & loads theme */
    cat_config cfg = {
        .window_title = "Hello Catastrophe",
        .log_path     = cat_resolve_log_path("hello"),
    };
    if (cat_init(&cfg) != CAT_OK) {
        fprintf(stderr, "Failed to initialise Catastrophe\n");
        return 1;
    }
    cat_log("hello: startup");

    /* Build a simple list of items */
    cat_list_item items[] = {
        { .label = "Say Hello",       .metadata = NULL },
        { .label = "Greet the World", .metadata = NULL },
        { .label = "Wave Goodbye",    .metadata = NULL },
    };
    int item_count = sizeof(items) / sizeof(items[0]);

    /* Define footer hints */
    cat_footer_item footer[] = {
        { .button = CAT_BTN_B, .label = "BACK" },
        { .button = CAT_BTN_A, .label = "SELECT", .is_confirm = true },
    };

    /* Configure and show the list */
    cat_list_opts opts = cat_list_default_opts("Hello!", items, item_count);
    opts.footer       = footer;
    opts.footer_count = 2;

    cat_list_result result;
    int rc = cat_list(&opts, &result);

    if (rc == CAT_OK && result.selected_index >= 0) {
        /* Show a confirmation with the selected item */
        char msg[128];
        snprintf(msg, sizeof(msg), "You picked: %s", items[result.selected_index].label);

        cat_footer_item conf_footer[] = {
            { .button = CAT_BTN_A, .label = "OK", .is_confirm = true },
        };
        cat_message_opts conf = {
            .message      = msg,
            .image_path   = NULL,
            .footer       = conf_footer,
            .footer_count = 1,
        };
        cat_confirm_result conf_result;
        cat_confirmation(&conf, &conf_result);
    }

    cat_quit();
    return 0;
}
