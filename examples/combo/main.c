/*
 * Catastrophe Combo Demo
 *
 * Demonstrates the combo system with two modes selectable from a menu:
 *
 *  "Polling (classic)"  — register combos and poll cat_poll_combo() each frame.
 *  "Callbacks (_ex)"    — register combos with _ex variants so callbacks fire
 *                         automatically on trigger/release, without polling.
 *
 * Navigate the menu with D-Pad Up/Down, press A to enter, B to quit/go back.
 */

#define CAT_IMPLEMENTATION
#include "catastrophe.h"

/* ──────────────────────────────────────────────────────────────────────────
 * Shared render state (set up once after cat_init)
 * ────────────────────────────────────────────────────────────────────────── */

static TTF_Font *g_body_font;
static TTF_Font *g_hint_font;
static ap_color  g_fg;
static ap_color  g_accent;
static int       g_sw, g_sh, g_pad;

static void push_footer_overflow_disabled(cat_footer_overflow_opts *saved) {
    if (!saved) return;
    cat_get_footer_overflow_opts(saved);

    cat_footer_overflow_opts disabled = *saved;
    disabled.enabled = false;
    cat_set_footer_overflow_opts(&disabled);
}

static void pop_footer_overflow_opts(cat_footer_overflow_opts *saved) {
    if (!saved) return;
    cat_set_footer_overflow_opts(saved);
}

static void init_render_state(void) {
    g_body_font  = cat_get_font(CAT_FONT_LARGE);
    g_hint_font  = cat_get_font(CAT_FONT_SMALL);
    g_fg         = cat_get_theme()->text;
    g_accent     = cat_get_theme()->accent;
    g_sw         = cat_get_screen_width();
    g_sh         = cat_get_screen_height();
    g_pad        = CAT_DS(12);
}

/* Draw the live status text and return its wrapped height. */
static int draw_status(const char *status, int x, int y, int w, bool draw) {
    int status_h = cat_measure_wrapped_text_height(g_body_font, status, w);
    if (status_h < TTF_FontLineSkip(g_body_font)) {
        status_h = TTF_FontLineSkip(g_body_font);
    }

    if (draw) {
        cat_draw_text_wrapped(g_body_font, status, x, y, w, g_accent, CAT_ALIGN_CENTER);
    }

    return status_h;
}

/* Measure the total content height for a combo demo screen (shared by both demos). */
static int combo_content_height(const char *help_text, const char *status, int content_w) {
    int h = 0;
    /* 3 chord/sequence header lines + 3 value lines, each with spacing */
    h += TTF_FontLineSkip(g_hint_font); h += CAT_DS(16);
    h += TTF_FontLineSkip(g_hint_font); h += CAT_DS(14);
    h += TTF_FontLineSkip(g_hint_font); h += CAT_DS(20);
    h += TTF_FontLineSkip(g_hint_font); h += CAT_DS(16);
    h += TTF_FontLineSkip(g_hint_font); h += CAT_DS(14);
    h += TTF_FontLineSkip(g_hint_font); h += CAT_DS(20);
    /* help text + gap + status */
    h += cat_measure_wrapped_text_height(g_hint_font, help_text, content_w);
    h += CAT_DS(12);
    h += draw_status(status, 0, 0, content_w, false);
    return h;
}

static bool register_polling_combos(void) {
    bool ok = true;

    cat_button shoulders[] = { CAT_BTN_L1, CAT_BTN_R1 };
    if (cat_register_chord("shoulders", shoulders, 2, 150) != CAT_OK) {
        cat_log("Failed to register shoulders chord");
        ok = false;
    }

    cat_button triggers[] = { CAT_BTN_L2, CAT_BTN_R2 };
    if (cat_register_chord("triggers", triggers, 2, 150) != CAT_OK) {
        cat_log("Failed to register triggers chord");
        ok = false;
    }

    cat_button uudd[] = { CAT_BTN_UP, CAT_BTN_UP, CAT_BTN_DOWN, CAT_BTN_DOWN };
    if (cat_register_sequence("uudd", uudd, 4, 500, false) != CAT_OK) {
        cat_log("Failed to register uudd sequence");
        ok = false;
    }

    cat_button aba[] = { CAT_BTN_A, CAT_BTN_B, CAT_BTN_A };
    if (cat_register_sequence("aba_strict", aba, 3, 400, true) != CAT_OK) {
        cat_log("Failed to register aba_strict sequence");
        ok = false;
    }

    return ok;
}

/* ──────────────────────────────────────────────────────────────────────────
 * Demo A: Polling (classic)
 *
 * Standard usage: register combos with cat_register_chord / cat_register_sequence,
 * then drain cat_poll_combo() every frame just like cat_poll_input().
 * ────────────────────────────────────────────────────────────────────────── */

static void run_polling_demo(void) {
    cat_footer_overflow_opts saved_footer_overflow;
    push_footer_overflow_disabled(&saved_footer_overflow);

    bool triggers_registered = register_polling_combos();

    char status[256] = "Waiting for combos...";
    char last_status[256] = "Waiting for combos...";
    bool running = true;
    int scroll_offset = 0;

    cat_footer_item footer[] = {
        { .button = CAT_BTN_MENU, .label = "BACK" },
        { .button = CAT_BTN_X,    .label = "UNREG" },
        { .button = CAT_BTN_Y,    .label = "REREG" },
    };

    while (running) {
        /* Normal input — MENU to go back */
        cat_input_event ev;
        while (cat_poll_input(&ev)) {
            if (!ev.pressed) continue;
            if (ev.button == CAT_BTN_MENU) {
                running = false;
            } else if (ev.button == CAT_BTN_LEFT) {
                scroll_offset -= CAT_DS(40);
                if (scroll_offset < 0) scroll_offset = 0;
            } else if (ev.button == CAT_BTN_RIGHT) {
                scroll_offset += CAT_DS(40);
            } else if (ev.button == CAT_BTN_X && triggers_registered) {
                cat_unregister_combo("triggers");
                triggers_registered = false;
                snprintf(status, sizeof(status), "Unregistered chord: triggers");
                cat_log("poll combo: unregistered triggers");
            } else if (ev.button == CAT_BTN_Y && !triggers_registered) {
                cat_clear_combos();
                triggers_registered = register_polling_combos();
                snprintf(status, sizeof(status), "%s",
                         triggers_registered ? "Re-registered all combos." : "Re-register failed.");
                cat_log("poll combo: re-register %s", triggers_registered ? "ok" : "failed");
            }
        }

        /* Poll combo events — cat_combo_event.type distinguishes chord vs sequence */
        cat_combo_event combo;
        while (cat_poll_combo(&combo)) {
            const char *kind = (combo.type == CAT_COMBO_CHORD) ? "chord" : "seq";
            if (combo.triggered)
                snprintf(status, sizeof(status), "TRIGGERED [%s]: %s", kind, combo.id);
            else
                snprintf(status, sizeof(status), "Released  [%s]: %s", kind, combo.id);
            cat_log("poll combo: %s %s", combo.triggered ? "triggered" : "released", combo.id);
        }

        bool status_changed = strcmp(status, last_status) != 0;
        if (status_changed) {
            snprintf(last_status, sizeof(last_status), "%s", status);
        }

        cat_clear_screen();
        cat_draw_screen_title("Polling (classic)", NULL);
        SDL_Rect content_rect = cat_get_content_rect(true, true, false);
        int scrollbar_gutter = CAT_S(16);
        int content_x = g_pad;
        int content_right = g_sw - g_pad - scrollbar_gutter;
        if (content_right <= content_x) content_right = g_sw - g_pad;
        int content_w = content_right - content_x;
        if (content_w < 1) content_w = 1;
        const char *poll_help =
            "cat_poll_combo() returns chord and sequence events. "
            "Press X to unregister the triggers chord, then Y to restore all combos. "
            "Use LEFT/RIGHT to scroll.";

        int total_content_h = combo_content_height(poll_help, status, content_w);

        int max_scroll = total_content_h - content_rect.h;
        if (max_scroll < 0) max_scroll = 0;
        if (status_changed && max_scroll > 0) scroll_offset = max_scroll;
        if (scroll_offset > max_scroll) scroll_offset = max_scroll;

        SDL_Rect clip = { content_x, content_rect.y, content_w, content_rect.h };
        SDL_RenderSetClipRect(cat_get_renderer(), &clip);

        int y = content_rect.y - scroll_offset;
        cat_draw_text(g_hint_font, "Chords — press simultaneously:", content_x, y, g_fg);
        y += CAT_DS(16);
        cat_draw_text(g_hint_font, "  L1 + R1              \"shoulders\"", content_x, y, g_fg);
        y += CAT_DS(14);
        cat_draw_text(g_hint_font, "  L2 + R2              \"triggers\"", content_x, y, g_fg);
        y += CAT_DS(20);

        cat_draw_text(g_hint_font, "Sequences — press in order:", content_x, y, g_fg);
        y += CAT_DS(16);
        cat_draw_text(g_hint_font, "  UP UP DOWN DOWN      \"uudd\"", content_x, y, g_fg);
        y += CAT_DS(14);
        cat_draw_text(g_hint_font, "  A B A (strict)       \"aba_strict\"", content_x, y, g_fg);
        y += CAT_DS(20);

        cat_draw_text_wrapped(g_hint_font, poll_help, content_x, y, content_w, g_fg, CAT_ALIGN_LEFT);
        y += cat_measure_wrapped_text_height(g_hint_font, poll_help, content_w);
        y += CAT_DS(12);

        draw_status(status, content_x, y, content_w, true);

        SDL_RenderSetClipRect(cat_get_renderer(), NULL);

        if (max_scroll > 0) {
            cat_draw_scrollbar(content_right + CAT_S(6), content_rect.y, content_rect.h,
                              content_rect.h, total_content_h, scroll_offset);
        }

        cat_draw_footer(footer, 3);
        cat_present();
        SDL_Delay(16);
    }

    cat_clear_combos();
    pop_footer_overflow_opts(&saved_footer_overflow);
}

/* ──────────────────────────────────────────────────────────────────────────
 * Demo B: Callbacks (_ex variants)
 *
 * Register combos with cat_register_chord_ex / cat_register_sequence_ex.
 * Callbacks fire automatically at trigger/release time — no poll loop needed.
 * cat_poll_combo() still works alongside callbacks (events are enqueued either way).
 * ────────────────────────────────────────────────────────────────────────── */

static char g_cb_status[256] = "Waiting for combos...";

static void on_combo_trigger(const char *id, cat_combo_type type, void *userdata) {
    (void)userdata;
    const char *kind = (type == CAT_COMBO_CHORD) ? "chord" : "seq";
    snprintf(g_cb_status, sizeof(g_cb_status), "CB TRIGGERED [%s]: %s", kind, id);
    cat_log("callback triggered: %s (%s)", id, kind);
}

static void on_combo_release(const char *id, cat_combo_type type, void *userdata) {
    (void)userdata;
    const char *kind = (type == CAT_COMBO_CHORD) ? "chord" : "seq";
    snprintf(g_cb_status, sizeof(g_cb_status), "CB released  [%s]: %s", kind, id);
    cat_log("callback released: %s (%s)", id, kind);
}

static void run_callback_demo(void) {
    cat_footer_overflow_opts saved_footer_overflow;
    push_footer_overflow_disabled(&saved_footer_overflow);

    /* Chords: provide both on_trigger and on_release callbacks */
    cat_button shoulders[] = { CAT_BTN_L1, CAT_BTN_R1 };
    if (cat_register_chord_ex("shoulders_cb", shoulders, 2, 150,
                             on_combo_trigger, on_combo_release, NULL) != CAT_OK)
        cat_log("Failed to register shoulders_cb chord");

    cat_button triggers[] = { CAT_BTN_L2, CAT_BTN_R2 };
    if (cat_register_chord_ex("triggers_cb", triggers, 2, 150,
                             on_combo_trigger, on_combo_release, NULL) != CAT_OK)
        cat_log("Failed to register triggers_cb chord");

    /* Sequences: only on_trigger (no release event for sequences) */
    cat_button uudd[] = { CAT_BTN_UP, CAT_BTN_UP, CAT_BTN_DOWN, CAT_BTN_DOWN };
    if (cat_register_sequence_ex("uudd_cb", uudd, 4, 500, false,
                                on_combo_trigger, NULL) != CAT_OK)
        cat_log("Failed to register uudd_cb sequence");

    cat_button aba[] = { CAT_BTN_A, CAT_BTN_B, CAT_BTN_A };
    if (cat_register_sequence_ex("aba_cb", aba, 3, 400, true,
                                on_combo_trigger, NULL) != CAT_OK)
        cat_log("Failed to register aba_cb sequence");

    snprintf(g_cb_status, sizeof(g_cb_status), "Waiting for combos...");
    char last_status[256] = "Waiting for combos...";
    bool running = true;
    int scroll_offset = 0;

    cat_footer_item footer[] = {
        { .button = CAT_BTN_MENU, .label = "BACK" },
    };

    while (running) {
        cat_input_event ev;
        while (cat_poll_input(&ev)) {
            if (!ev.pressed) continue;
            if (ev.button == CAT_BTN_MENU)
                running = false;
            else if (ev.button == CAT_BTN_LEFT) {
                scroll_offset -= CAT_DS(40);
                if (scroll_offset < 0) scroll_offset = 0;
            } else if (ev.button == CAT_BTN_RIGHT) {
                scroll_offset += CAT_DS(40);
            }
        }

        /* Callbacks have already fired — drain the queue to keep it clean. */
        cat_combo_event combo;
        while (cat_poll_combo(&combo)) { /* events still enqueued alongside callbacks */ }

        bool status_changed = strcmp(g_cb_status, last_status) != 0;
        if (status_changed) {
            snprintf(last_status, sizeof(last_status), "%s", g_cb_status);
        }

        cat_clear_screen();
        cat_draw_screen_title("Callbacks (_ex)", NULL);
        SDL_Rect content_rect = cat_get_content_rect(true, true, false);
        int scrollbar_gutter = CAT_S(16);
        int content_x = g_pad;
        int content_right = g_sw - g_pad - scrollbar_gutter;
        if (content_right <= content_x) content_right = g_sw - g_pad;
        int content_w = content_right - content_x;
        if (content_w < 1) content_w = 1;
        const char *cb_help =
            "on_trigger/on_release fire automatically; the demo only drains cat_poll_combo() to keep the queue clean. "
            "Use LEFT/RIGHT to scroll.";

        int total_content_h = combo_content_height(cb_help, g_cb_status, content_w);

        int max_scroll = total_content_h - content_rect.h;
        if (max_scroll < 0) max_scroll = 0;
        if (status_changed && max_scroll > 0) scroll_offset = max_scroll;
        if (scroll_offset > max_scroll) scroll_offset = max_scroll;

        SDL_Rect clip = { content_x, content_rect.y, content_w, content_rect.h };
        SDL_RenderSetClipRect(cat_get_renderer(), &clip);

        int y = content_rect.y - scroll_offset;
        cat_draw_text(g_hint_font, "Chords — press simultaneously:", content_x, y, g_fg);
        y += CAT_DS(16);
        cat_draw_text(g_hint_font, "  L1 + R1              \"shoulders_cb\"", content_x, y, g_fg);
        y += CAT_DS(14);
        cat_draw_text(g_hint_font, "  L2 + R2              \"triggers_cb\"", content_x, y, g_fg);
        y += CAT_DS(20);

        cat_draw_text(g_hint_font, "Sequences — press in order:", content_x, y, g_fg);
        y += CAT_DS(16);
        cat_draw_text(g_hint_font, "  UP UP DOWN DOWN      \"uudd_cb\"", content_x, y, g_fg);
        y += CAT_DS(14);
        cat_draw_text(g_hint_font, "  A B A (strict)       \"aba_cb\"", content_x, y, g_fg);
        y += CAT_DS(20);

        cat_draw_text_wrapped(g_hint_font, cb_help, content_x, y, content_w, g_fg, CAT_ALIGN_LEFT);
        y += cat_measure_wrapped_text_height(g_hint_font, cb_help, content_w);
        y += CAT_DS(12);

        draw_status(g_cb_status, content_x, y, content_w, true);

        SDL_RenderSetClipRect(cat_get_renderer(), NULL);

        if (max_scroll > 0) {
            cat_draw_scrollbar(content_right + CAT_S(6), content_rect.y, content_rect.h,
                              content_rect.h, total_content_h, scroll_offset);
        }

        cat_draw_footer(footer, 1);
        cat_present();
        SDL_Delay(16);
    }

    cat_clear_combos();
    pop_footer_overflow_opts(&saved_footer_overflow);
}

/* ──────────────────────────────────────────────────────────────────────────
 * Main menu
 * ────────────────────────────────────────────────────────────────────────── */

static const struct {
    const char *label;
    void (*fn)(void);
} g_modes[] = {
    { "Polling (classic)",  run_polling_demo  },
    { "Callbacks (_ex)",    run_callback_demo },
};
#define MODE_COUNT 2

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    cat_config cfg = {
        .window_title = "Combo Demo",
        .log_path     = cat_resolve_log_path("combo"),
    };
    if (cat_init(&cfg) != CAT_OK) {
        fprintf(stderr, "Failed to initialise Catastrophe\n");
        return 1;
    }
    cat_log("combo demo: startup");

    init_render_state();

    int  sel     = 0;
    bool running = true;

    cat_footer_item footer[] = {
        { .button = CAT_BTN_MENU, .label = "QUIT" },
        { .button = CAT_BTN_A,    .label = "OPEN", .is_confirm = true },
    };

    while (running) {
        cat_input_event ev;
        while (cat_poll_input(&ev)) {
            if (!ev.pressed) continue;
            switch (ev.button) {
                case CAT_BTN_UP:   sel = (sel - 1 + MODE_COUNT) % MODE_COUNT; break;
                case CAT_BTN_DOWN: sel = (sel + 1) % MODE_COUNT;              break;
                case CAT_BTN_A:    g_modes[sel].fn();                          break;
                case CAT_BTN_MENU: running = false;                            break;
                default: break;
            }
        }

        cat_clear_screen();
        cat_draw_screen_title("Combo Demo", NULL);
        SDL_Rect content_rect = cat_get_content_rect(true, true, false);
        int y = content_rect.y;
        cat_draw_text(g_hint_font, "Choose a demo mode:", g_pad, y, g_fg);
        y += CAT_DS(24);

        for (int i = 0; i < MODE_COUNT; i++) {
            ap_color col = (i == sel) ? g_accent : g_fg;
            char line[128];
            snprintf(line, sizeof(line), "%s %s", (i == sel) ? ">" : " ", g_modes[i].label);
            cat_draw_text(g_body_font, line, g_pad, y, col);
            y += CAT_DS(22);
        }

        cat_draw_footer(footer, 2);
        cat_present();
        SDL_Delay(16);
    }

    cat_quit();
    return 0;
}
