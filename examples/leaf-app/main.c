/*
 * Leaf App — a worked example of a Leaf-consistent app.
 *
 * It's the shape almost every Leaf app takes: a scrollable LIST on the left with
 * a preview on the right, and pressing A drills into a scrollable DETAIL page.
 * It shows the conventions that make an app feel native to Leaf:
 *
 *   - the box model for layout (carve title / content / footer; split columns),
 *     so gaps match the launcher at any font size — see leaf_layout.h;
 *   - theme colour ROLES (never hardcoded colours), so it adapts to all themes;
 *   - the font tiers (EXTRA_LARGE titles, MEDIUM body, SMALL meta);
 *   - the footer hint bar + the standard button grammar
 *     (A = primary / open, B = back, MENU = quit);
 *   - a continuous render loop you drive yourself (vs. the all-in-one cat_list()).
 *
 * Build + run (from the Catastrophe repo root):  make mac && ./build/mac/leaf-app
 */
#define CAT_IMPLEMENTATION
#include "catastrophe.h"
#define CAT_WIDGETS_IMPLEMENTATION
#include "catastrophe_widgets.h"

#include "leaf_layout.h"

/* ── Sample data ────────────────────────────────────────────────────────────
 * Stand-in for whatever your app lists (tracks, files, devices, packages...).
 * Each row has a one-line tagline for the preview and a longer body for the
 * detail page, so the list shows the scroll-and-drill-in pattern end to end. */
typedef struct {
    const char *name;
    const char *tagline;       /* one line, shown in the list preview          */
    const char *kind;
    const char *size;
    const char *description;   /* long; wraps + scrolls on the detail page      */
} demo_item;

static const demo_item ITEMS[] = {
    { "Aurora",  "Northern-lights theme pack", "Theme",  "1.2 MB",
      "A cool-toned theme pack inspired by the aurora borealis. Greens fade to "
      "deep indigo, with a high-contrast selection colour tuned to stay readable "
      "on both the light and dark variants. Pairs well with the default font." },
    { "Cascade", "Waterfall ambient loops",   "Audio",  "8.4 MB",
      "Twelve seamless ambient loops recorded near alpine waterfalls. Each loop "
      "is normalised and gapless so it can run under the launcher without clicks. "
      "Great for testing the audio routing as you plug and unplug headphones." },
    { "Drift",   "Parallax wallpaper set",    "Theme",  "3.1 MB",
      "A set of layered wallpapers that drift on a slow parallax as you move the "
      "cursor. Lightweight enough for the 1 GB device — each layer is a small PNG "
      "composited at draw time rather than a video." },
    { "Ember",   "Warm low-light palette",    "Theme",  "0.9 MB",
      "A warm, low-brightness palette for late-night play. Reds and ambers with a "
      "dimmed background, designed to be easy on the eyes without crushing detail "
      "in dark game art." },
    { "Fathom",  "Deep-sea sound bank",       "Audio",  "11.7 MB",
      "Submarine sonar pings, distant whale song, and slow currents. A moodier "
      "companion to Cascade. Useful as a long, quiet source when you want to hear "
      "exactly when audio cuts out on sleep or jack changes." },
    { "Glint",   "Minimal icon overrides",    "Icons",  "0.4 MB",
      "A restrained icon set that swaps the system glyphs for thinner, lighter "
      "strokes while keeping the same silhouettes, so nothing shifts in the grid. "
      "Demonstrates that cosmetic changes must never move layout geometry." },
    { "Harbor",  "Coastal photo wallpapers",  "Theme",  "5.6 MB",
      "Twenty wide coastal photographs, cropped to the panel's 4:3 and colour-"
      "graded to a consistent muted palette so the selection colour always reads "
      "clearly on top of them." },
    { "Iris",    "Accessibility high-contrast","Theme", "0.7 MB",
      "A maximum-contrast theme built around the WCAG contrast model Leaf uses for "
      "its emphasis colour: headings and status text are clamped to stay legible "
      "against the background on every screen." },
};
#define ITEM_COUNT ((int)(sizeof(ITEMS) / sizeof(ITEMS[0])))

/* ── Screens ────────────────────────────────────────────────────────────────
 * Two screens, one render loop. Input is dispatched by the active screen; this
 * is the Leaf pattern (the launcher and Pak Rat work the same way) rather than
 * nesting blocking widget calls. */
typedef enum { SCREEN_LIST, SCREEN_DETAIL } screen_id;

/* ── List rows ──────────────────────────────────────────────────────────────
 * cat_draw_list_pane calls this once per visible row. Draw the selection pill
 * with theme->highlight and the label with the matching text role, so it tracks
 * every theme. (The pill height comes from the font, centred in the cell.) */
static void draw_item(int idx, int x, int y, int w, int h, bool selected, void *user) {
    (void)user;
    cat_theme *t = cat_get_theme();
    TTF_Font *body = cat_get_font(CAT_FONT_MEDIUM);
    int pill_h = TTF_FontHeight(body) + cat_scale(6);
    int pill_y = y + (h - pill_h) / 2;
    if (selected) {
        cat_draw_pill(x, pill_y, w - cat_scale(4), pill_h, t->highlight);
    }
    cat_draw_text_ellipsized(body, ITEMS[idx].name,
                             x + cat_scale(10), y + (h - TTF_FontHeight(body)) / 2,
                             selected ? t->highlighted_text : t->text,
                             w - cat_scale(20));
}

/* ── Detail page body (scroll-view content) ─────────────────────────────────
 * cat_draw_scroll_view shifts the origin for the scroll offset and clips to the
 * viewport, so we just draw everything at its natural position from (x, y).
 * jw__-style: a heading in the EMPHASIS role, the wrapped description in body
 * text, then SMALL key/value meta rows in the hint role. */
static void draw_detail_body(int x, int y, int w, void *user) {
    const demo_item *it = (const demo_item *)user;
    cat_theme *t = cat_get_theme();
    TTF_Font *body  = cat_get_font(CAT_FONT_MEDIUM);
    TTF_Font *small = cat_get_font(CAT_FONT_SMALL);
    int cy = y;

    cat_draw_text(small, it->tagline, x, cy, t->emphasis);
    cy += TTF_FontHeight(small) + cat_scale(12);

    cat_draw_text_wrapped(body, it->description, x, cy, w, t->text, CAT_ALIGN_LEFT);
    cy += cat_measure_wrapped_text_height(body, it->description, w) + cat_scale(16);

    char line[128];
    snprintf(line, sizeof(line), "Kind:  %s", it->kind);
    cat_draw_text(small, line, x, cy, t->hint);
    cy += TTF_FontHeight(small) + cat_scale(6);
    snprintf(line, sizeof(line), "Size:  %s", it->size);
    cat_draw_text(small, line, x, cy, t->hint);
}

/* Natural height of the detail body, so the scroll view knows when to scroll.
 * Mirrors draw_detail_body's layout exactly. */
static int detail_body_h(const demo_item *it, int w) {
    TTF_Font *body  = cat_get_font(CAT_FONT_MEDIUM);
    TTF_Font *small = cat_get_font(CAT_FONT_SMALL);
    int h = TTF_FontHeight(small) + cat_scale(12);
    h += cat_measure_wrapped_text_height(body, it->description, w) + cat_scale(16);
    h += 2 * (TTF_FontHeight(small) + cat_scale(6));
    return h;
}

/* ── List screen ────────────────────────────────────────────────────────────
 * Title + (list | preview) + footer, all from the box model. */
static void render_list(cat_list_state *list) {
    cat_draw_background();

    SDL_Rect title_rect;
    cat_box content = leaf_carve(&title_rect, /*has_footer=*/true);
    leaf_title("Leaf App", title_rect);

    cat_box list_box, detail_box;
    leaf_split(&content, /*list_pct=*/58, &list_box, &detail_box);

    /* List rows: fit_rows stretches the rows to fill the column exactly (the
     * filled-grid invariant — list length never changes the geometry). It also
     * writes back the visible-row count for the scroll state. */
    int base_h = TTF_FontHeight(cat_get_font(CAT_FONT_MEDIUM)) + cat_scale(12);
    int item_h = 0;
    int vis = list->visible_rows;
    SDL_Rect rows = cat_box_fit_rows(&list_box, base_h, ITEM_COUNT, &vis, &item_h);
    list->visible_rows = vis;
    cat_draw_list_pane(rows.x, rows.y, rows.w, rows.h,
                       ITEM_COUNT, list, item_h, draw_item, NULL);

    /* Preview pane: a faint panel with the selected item's tagline + name.
     * Faint fill is theme-derived (text colour at low alpha) so it works on any
     * theme without a hardcoded colour. */
    SDL_Rect d = cat_box_content(&detail_box);
    cat_theme *t = cat_get_theme();
    cat_draw_color panel = t->text; panel.a = 18;
    cat_draw_rounded_rect(d.x, d.y, d.w, d.h, cat_scale(8), panel);

    const demo_item *sel = &ITEMS[list->cursor];
    int pad = cat_scale(14);
    cat_draw_text_ellipsized(cat_get_font(CAT_FONT_LARGE), sel->name,
                             d.x + pad, d.y + pad, t->text, d.w - pad * 2);
    cat_draw_text_wrapped(cat_get_font(CAT_FONT_SMALL), sel->tagline,
                          d.x + pad,
                          d.y + pad + TTF_FontHeight(cat_get_font(CAT_FONT_LARGE)) + cat_scale(8),
                          d.w - pad * 2, t->hint, CAT_ALIGN_LEFT);

    cat_footer_item footer[] = {
        { .button = CAT_BTN_B, .label = "Quit" },
        { .button = CAT_BTN_A, .label = "Open", .is_confirm = true },
    };
    cat_draw_footer(footer, 2);
}

/* ── Detail screen ──────────────────────────────────────────────────────────
 * Title (item name) + full-width scrollable body + footer. */
static void render_detail(const demo_item *it, cat_scroll_state *scroll) {
    cat_draw_background();

    SDL_Rect title_rect;
    cat_box content = leaf_carve(&title_rect, /*has_footer=*/true);
    leaf_title(it->name, title_rect);

    SDL_Rect c = cat_box_content(&content);
    cat_draw_scroll_view(c.x, c.y, c.w, c.h,
                         detail_body_h(it, c.w), scroll,
                         draw_detail_body, (void *)it);

    cat_footer_item footer[] = {
        { .button = CAT_BTN_B, .label = "Back", .is_confirm = true },
    };
    cat_draw_footer(footer, 1);
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    cat_config cfg = {
        .window_title = "Leaf App",
        .log_path     = cat_resolve_log_path("leaf-app"),
    };
    if (cat_init(&cfg) != CAT_OK) {
        fprintf(stderr, "Failed to initialise Catastrophe\n");
        return 1;
    }
    cat_log("leaf-app: startup");

    screen_id screen = SCREEN_LIST;
    cat_list_state list;
    cat_list_state_init(&list, 0);          /* visible_rows is filled by fit_rows */
    cat_scroll_state scroll;
    cat_scroll_state_init(&scroll);

    bool running = true;
    while (running) {
        cat_input_event ev;
        while (cat_poll_input(&ev)) {
            if (!ev.pressed) {
                continue;
            }
            if (screen == SCREEN_LIST) {
                switch (ev.button) {
                    case CAT_BTN_UP:
                        cat_list_state_move(&list, -1, ITEM_COUNT);
                        break;
                    case CAT_BTN_DOWN:
                        cat_list_state_move(&list, +1, ITEM_COUNT);
                        break;
                    case CAT_BTN_A:                 /* primary action: drill in */
                        cat_scroll_state_init(&scroll);
                        screen = SCREEN_DETAIL;
                        break;
                    case CAT_BTN_B:                 /* back at the top level = quit */
                    case CAT_BTN_MENU:
                        running = false;
                        break;
                    default:
                        break;
                }
            } else {  /* SCREEN_DETAIL */
                int line_h = TTF_FontHeight(cat_get_font(CAT_FONT_SMALL)) + cat_scale(8);
                switch (ev.button) {
                    case CAT_BTN_UP:
                        cat_scroll_state_move(&scroll, -line_h);
                        break;
                    case CAT_BTN_DOWN:
                        cat_scroll_state_move(&scroll, +line_h);
                        break;
                    case CAT_BTN_B:                 /* back to the list */
                        screen = SCREEN_LIST;
                        break;
                    case CAT_BTN_MENU:              /* MENU always quits the app */
                        running = false;
                        break;
                    default:
                        break;
                }
            }
        }

        if (screen == SCREEN_LIST) {
            render_list(&list);
        } else {
            render_detail(&ITEMS[list.cursor], &scroll);
        }
        cat_present();
    }

    cat_quit();
    return 0;
}
