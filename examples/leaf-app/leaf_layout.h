/*
 * leaf_layout.h — copy-paste box-model helpers for a Leaf-consistent app.
 *
 * Leaf's launcher and first-party apps all share one layout model so they look
 * identical by construction: every screen is a TITLE band, a CONTENT area, and
 * an optional FOOTER hint bar, all carved from the screen with the box model
 * (see plans/BOX_MODEL.md). The chrome heights are derived from the live font,
 * so the layout is correct at any font size and DPI.
 *
 * These few helpers wrap that carve so your app matches Leaf without re-deriving
 * the geometry. They are intentionally tiny and dependency-free — drop this file
 * into your app and #include it. (They mirror the launcher's private
 * jw__browse_boxes / jw__draw_info_title; if Catastrophe ever promotes shared
 * Leaf helpers, swap to those.)
 */
#ifndef LEAF_LAYOUT_H
#define LEAF_LAYOUT_H

#include "catastrophe.h"

/* One base pad, reused for screen edges, the gutter between columns, and content
 * insets. The box model's one rule: spacing is a box's INTERNAL padding, never an
 * outside margin — so every gap on screen traces back to this single number. */
#define LEAF_PAD 12

/* Height of the title band, derived from the title font (never a constant). */
static inline int leaf_title_h(void) {
    return TTF_FontHeight(cat_get_font(CAT_FONT_EXTRA_LARGE)) + cat_scale(8);
}

/* Carve the screen into [title band] / [content] / [footer reservation] and
 * return the content box. The title rect is written to *out_title. Pass
 * has_footer=true whenever you call cat_draw_footer(), so the content stops above
 * the hint bar (its height is font-driven via cat_get_footer_height()). */
static inline cat_box leaf_carve(SDL_Rect *out_title, bool has_footer) {
    int edge = cat_scale(LEAF_PAD);
    cat_box root = { 0, 0, cat_get_screen_width(), cat_get_screen_height(),
                     edge, edge, edge, edge };
    cat_box title = cat_box_carve_top(&root, leaf_title_h());
    if (has_footer) {
        cat_box_carve_bottom(&root, cat_get_footer_height());  /* reserve hint bar */
    }
    if (out_title) {
        *out_title = cat_box_content(&title);
    }
    return root;  /* what's left is the content box (still padded on every side) */
}

/* Split a content box into a list column (list_pct of the width) and a detail
 * column filling the rest, with one pad of gutter between them. Either out may be
 * NULL. Use for the classic list-left / preview-right browse page. */
static inline void leaf_split(const cat_box *content, int list_pct,
                              cat_box *list, cat_box *detail) {
    int list_w = cat_box_content(content).w * list_pct / 100;
    cat_box_split_cols(content, list_w, cat_scale(LEAF_PAD), list, detail);
}

/* Draw a screen title in the Leaf house style: extra-large, theme text colour,
 * left-aligned, ellipsized to fit — matching the launcher's drilled-in pages. */
static inline void leaf_title(const char *text, SDL_Rect at) {
    cat_draw_text_ellipsized(cat_get_font(CAT_FONT_EXTRA_LARGE), text,
                             at.x, at.y, cat_get_theme()->text, at.w);
}

#endif /* LEAF_LAYOUT_H */
