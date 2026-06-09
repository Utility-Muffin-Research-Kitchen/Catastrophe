/*
 * Catastrophe Performance Demo
 *
 * Demonstrates CPU speed and fan control:
 *
 *  Main screen — live readout of CPU MHz, temperature, fan mode, and fan %.
 *  "CPU Speed"  — pick a preset; the change takes effect immediately.
 *  "Fan Speed"  — pick a fan mode (quiet/normal/performance) or fixed percentage.
 *
 * All values display "N/A" on desktop builds (non-device).
 * MENU quits from any screen.
 */

#define CAT_IMPLEMENTATION
#include "catastrophe.h"
#define CAT_WIDGETS_IMPLEMENTATION
#include "catastrophe_widgets.h"

/* ──────────────────────────────────────────────────────────────────────────
 * Shared render state
 * ────────────────────────────────────────────────────────────────────────── */

static TTF_Font *g_body_font;
static TTF_Font *g_hint_font;
static cat_draw_color  g_fg;
static cat_draw_color  g_accent;
static int       g_sw, g_sh, g_pad;

static void init_render_state(void) {
    g_body_font  = cat_get_font(CAT_FONT_LARGE);
    g_hint_font  = cat_get_font(CAT_FONT_SMALL);
    g_fg         = cat_get_theme()->text;
    g_accent     = cat_get_theme()->accent;
    g_sw         = cat_get_screen_width();
    g_sh         = cat_get_screen_height();
    g_pad        = CAT_DS(12);
}

/* Draw a labelled key/value row */
static int draw_kv(int x, int y, const char *key, const char *value) {
    char line[128];
    snprintf(line, sizeof(line), "%-18s %s", key, value);
    cat_draw_text(g_body_font, line, x, y, g_fg);
    return y + CAT_DS(22);
}

/* Format an integer sensor value, showing "N/A" when -1 */
static const char *fmt_sensor(char *buf, int val, const char *unit) {
    if (val < 0) { snprintf(buf, 32, "N/A"); }
    else         { snprintf(buf, 32, "%d %s", val, unit); }
    return buf;
}

/* ──────────────────────────────────────────────────────────────────────────
 * Sub-screen: CPU Speed preset picker
 * ────────────────────────────────────────────────────────────────────────── */

static const struct { const char *label; cat_cpu_speed preset; } g_cpu_presets[] = {
    { "Menu       (~600–672 MHz)",   CAT_CPU_SPEED_MENU        },
    { "Powersave  (~1200 MHz)",      CAT_CPU_SPEED_POWERSAVE   },
    { "Normal     (~1608–1680 MHz)", CAT_CPU_SPEED_NORMAL      },
    { "Performance (~2000–2160 MHz)",CAT_CPU_SPEED_PERFORMANCE },
};
#define CPU_PRESET_COUNT 4

#if defined(PLATFORM_TG5050)
static const char *fan_mode_label(cat_fan_mode mode) {
    switch (mode) {
        case CAT_FAN_MODE_MANUAL:           return "Manual";
        case CAT_FAN_MODE_AUTO_QUIET:       return "Auto Quiet";
        case CAT_FAN_MODE_AUTO_NORMAL:      return "Auto Normal";
        case CAT_FAN_MODE_AUTO_PERFORMANCE: return "Auto Performance";
        default:                           return "N/A";
    }
}
#endif

static void run_cpu_screen(void) {
    int sel     = 2; /* default cursor on Normal */
    bool running = true;

    cat_footer_item footer[] = {
        { .button = CAT_BTN_MENU, .label = "BACK" },
        { .button = CAT_BTN_A,    .label = "APPLY", .is_confirm = true },
    };

    while (running) {
        cat_input_event ev;
        while (cat_poll_input(&ev)) {
            if (!ev.pressed) continue;
            switch (ev.button) {
                case CAT_BTN_UP:   sel = (sel - 1 + CPU_PRESET_COUNT) % CPU_PRESET_COUNT; break;
                case CAT_BTN_DOWN: sel = (sel + 1) % CPU_PRESET_COUNT;                    break;
                case CAT_BTN_A:
                    cat_set_cpu_speed(g_cpu_presets[sel].preset);
                    cat_log("perf: CPU speed set to preset %d", sel);
                    running = false;
                    break;
                case CAT_BTN_MENU: running = false; break;
                default: break;
            }
        }

        cat_clear_screen();
        cat_draw_screen_title("CPU Speed", NULL);
        SDL_Rect content_rect = cat_get_content_rect(true, true, false);
        int y = content_rect.y;

        cat_draw_text(g_hint_font, "Select a preset and press A to apply:", g_pad, y, g_fg);
        y += CAT_DS(20);

        for (int i = 0; i < CPU_PRESET_COUNT; i++) {
            cat_draw_color col = (i == sel) ? g_accent : g_fg;
            char line[128];
            snprintf(line, sizeof(line), "%s %s",
                     (i == sel) ? ">" : " ", g_cpu_presets[i].label);
            cat_draw_text(g_body_font, line, g_pad, y, col);
            y += CAT_DS(22);
        }

        /* Live current speed below the list */
        y += CAT_DS(10);
        char buf[32];
        char cur[64];
        snprintf(cur, sizeof(cur), "Current: %s", fmt_sensor(buf, cat_get_cpu_speed_mhz(), "MHz"));
        cat_draw_text(g_hint_font, cur, g_pad, y, g_fg);

        cat_draw_footer(footer, 2);
        cat_present();
        SDL_Delay(16);
    }
}

/* ──────────────────────────────────────────────────────────────────────────
 * Sub-screen: Fan mode / speed picker (TG5050 only)
 * ────────────────────────────────────────────────────────────────────────── */

#if defined(PLATFORM_TG5050)
static const struct {
    const char *label;
    bool        auto_mode;
    cat_fan_mode mode;
    int         percent;
} g_fan_levels[] = {
    { "Performance (auto)", true,  CAT_FAN_MODE_AUTO_PERFORMANCE, -1  },
    { "Normal (auto)",      true,  CAT_FAN_MODE_AUTO_NORMAL,      -1  },
    { "Quiet (auto)",       true,  CAT_FAN_MODE_AUTO_QUIET,       -1  },
    { "0%",                 false, CAT_FAN_MODE_MANUAL,            0  },
    { "10%",                false, CAT_FAN_MODE_MANUAL,           10  },
    { "20%",                false, CAT_FAN_MODE_MANUAL,           20  },
    { "30%",                false, CAT_FAN_MODE_MANUAL,           30  },
    { "40%",                false, CAT_FAN_MODE_MANUAL,           40  },
    { "50%",                false, CAT_FAN_MODE_MANUAL,           50  },
    { "60%",                false, CAT_FAN_MODE_MANUAL,           60  },
    { "70%",                false, CAT_FAN_MODE_MANUAL,           70  },
    { "80%",                false, CAT_FAN_MODE_MANUAL,           80  },
    { "90%",                false, CAT_FAN_MODE_MANUAL,           90  },
    { "100%",               false, CAT_FAN_MODE_MANUAL,          100  },
};
#define FAN_LEVEL_COUNT ((int)(sizeof(g_fan_levels) / sizeof(g_fan_levels[0])))
#endif

static void run_fan_screen(void) {
    cat_footer_item footer[] = {
        { .button = CAT_BTN_MENU, .label = "BACK" },
        { .button = CAT_BTN_A,    .label = "APPLY", .is_confirm = true },
    };
#if defined(PLATFORM_TG5050)
    cat_list_item items[FAN_LEVEL_COUNT];
    for (int i = 0; i < FAN_LEVEL_COUNT; i++) {
        items[i] = (cat_list_item){ .label = g_fan_levels[i].label };
    }

    cat_list_opts opts = cat_list_default_opts("Fan Speed", items, FAN_LEVEL_COUNT);
    opts.footer       = footer;
    opts.footer_count = 2;

    cat_list_result result;
    if (cat_list(&opts, &result) == CAT_OK && result.selected_index >= 0) {
        int sel = result.selected_index;
        if (g_fan_levels[sel].auto_mode) {
            cat_set_fan_mode(g_fan_levels[sel].mode);
            cat_log("perf: fan mode set to %s", fan_mode_label(g_fan_levels[sel].mode));
        } else {
            cat_set_fan_speed(g_fan_levels[sel].percent);
            cat_log("perf: fan set to %d%%", g_fan_levels[sel].percent);
        }
    }
#else
    bool running = true;
    while (running) {
        cat_input_event ev;
        while (cat_poll_input(&ev)) {
            if (!ev.pressed) continue;
            if (ev.button == CAT_BTN_MENU || ev.button == CAT_BTN_A) running = false;
        }

        cat_clear_screen();
        cat_draw_screen_title("Fan Speed", NULL);
        SDL_Rect content_rect = cat_get_content_rect(true, true, false);
        int y = content_rect.y;
        cat_draw_text(g_hint_font, "Fan control is only available on TG5050.", g_pad, y, g_fg);
        y += CAT_DS(18);
        cat_draw_text(g_hint_font, "This device has no fan hardware.", g_pad, y, g_fg);
        cat_draw_footer(footer, 2);
        cat_present();
        SDL_Delay(16);
    }
#endif
}

/* ──────────────────────────────────────────────────────────────────────────
 * Main screen — live readout + navigation
 * ────────────────────────────────────────────────────────────────────────── */

static const struct {
    const char *label;
    void (*fn)(void);
} g_actions[] = {
    { "Set CPU Speed", run_cpu_screen },
    { "Set Fan Speed", run_fan_screen },
};
#define ACTION_COUNT 2

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    cat_config cfg = {
        .window_title = "Performance Demo",
        .log_path     = cat_resolve_log_path("perf"),
        /* Don't set cpu_speed here — the demo lets the user choose interactively */
    };
    if (cat_init(&cfg) != CAT_OK) {
        fprintf(stderr, "Failed to initialise Catastrophe\n");
        return 1;
    }
    cat_log("perf demo: startup");

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
                case CAT_BTN_UP:   sel = (sel - 1 + ACTION_COUNT) % ACTION_COUNT; break;
                case CAT_BTN_DOWN: sel = (sel + 1) % ACTION_COUNT;                break;
                case CAT_BTN_A:    g_actions[sel].fn();                            break;
                case CAT_BTN_MENU: running = false;                                break;
                default: break;
            }
        }

        /* Read live sensor values each frame */
        int   cpu_mhz  = cat_get_cpu_speed_mhz();
        int   cpu_temp = cat_get_cpu_temp_celsius();
        int   fan_pct  = cat_get_fan_speed();
        cat_fan_mode fan_mode = cat_get_fan_mode();

        cat_clear_screen();
        cat_draw_screen_title("Performance", NULL);
        SDL_Rect content_rect = cat_get_content_rect(true, true, false);
        int y = content_rect.y;

        /* ── Live sensor readout ── */
        char b1[32], b2[32], b3[32];
        y = draw_kv(g_pad, y, "Platform:",   CAT_PLATFORM_NAME);
        y = draw_kv(g_pad, y, "CPU speed:",  fmt_sensor(b1, cpu_mhz,  "MHz"));
        y = draw_kv(g_pad, y, "CPU temp:",   fmt_sensor(b2, cpu_temp, "C"));
#if defined(PLATFORM_TG5050)
        y = draw_kv(g_pad, y, "Fan mode:",   fan_mode_label(fan_mode));
        y = draw_kv(g_pad, y, "Fan speed:",  fmt_sensor(b3, fan_pct,  "%"));
#else
        (void)b3; (void)fan_pct; (void)fan_mode;
        y = draw_kv(g_pad, y, "Fan speed:",  "N/A (no fan)");
#endif

        y += CAT_DS(16);

        /* ── Action menu ── */
        cat_draw_text(g_hint_font, "Actions:", g_pad, y, g_fg);
        y += CAT_DS(20);
        for (int i = 0; i < ACTION_COUNT; i++) {
            cat_draw_color col = (i == sel) ? g_accent : g_fg;
            char line[64];
            snprintf(line, sizeof(line), "%s %s", (i == sel) ? ">" : " ", g_actions[i].label);
            cat_draw_text(g_body_font, line, g_pad, y, col);
            y += CAT_DS(22);
        }

        cat_draw_footer(footer, 2);
        cat_present();
        SDL_Delay(250); /* slower refresh — sensor reads are slow sysfs calls */
    }

    cat_quit();
    return 0;
}
