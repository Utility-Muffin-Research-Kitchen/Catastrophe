# Box Model — shared layout primitive

Status: **proposed** (awaiting sign-off). 2026-06-02.

## Why

The launcher's browse pages (Recents, Favorites, Games tab, in-system game
browser, search) each hand-rolled their list/detail geometry with slightly
different math (different `content_h` derivations, list/detail heights, row
stretch). They drifted, so the same "gap to the hints" looked different per
page and every fix was whack-a-mole. The page with a **sub-header** (search +
game browser) diverged most.

The fix is one shared, font-aware **box model**: a tiny set of primitives in
Catastrophe that every page composes from, so all pages are identical by
construction and there is a single place to tune any gap.

`cat_get_content_rect()` is the existing seed (screen − chrome → content rect);
this generalizes it into a reusable type + two ops. It is **additive** — the old
function stays so in-flight code is untouched.

## The one rule: padding, never margins

Every box may pad its **contents inward** on any side. A box never reserves
space **outside** itself. Boxes tile their parent with no gaps; **all** visible
spacing comes from some box's internal padding.

- Off the screen edges → the root box's padding.
- Below the header / above the hints → the content box's top/bottom padding.
- Gutter between two columns → owned by the split op (each child padded toward
  the shared edge); never a sibling margin.
- Art inset inside its pane → that pane's padding.

Consequences: no margin-collapsing, no auto-margins. Centering (cover art,
dialogs) and free-floating overlays (OSD, the inline status icons) are explicit
positioning, *outside* the box flow — intentionally not part of this model.

## Heights are font-driven (hard requirement)

Every chrome strip height is derived from the live font at render time, never a
constant. The complete set of inputs that move heights:

1. **Font size / bump** — scales tab bar, sub-header, hint bar, status pill, rows.
2. **Hint bar on/off** — `cat_get_footer_height()` or `0`.
3. **Sub-header present/absent** — `TTF_FontHeight(large)` or `0`.

Everything else is width/cosmetic only and must NOT affect heights: pill shape /
list style (corner radius, derived from the height), hiding status icons (tab
bar reserved-*width*), clock style (text width).

So the box model queries those three heights live every frame; `visible_rows`
and the stretched row height are derived from the resulting region, so the
layout is correct at any font size, with or without hints, with or without the
sub-header. `visible_rows` (cached at list-init) must be recomputed on a
font-bump change via the existing rebuild path, using the same region math the
renderer uses (so they can never disagree).

## Catastrophe primitive (`cat_`)

```c
typedef struct {
    int x, y, w, h;          /* the box, in pixels */
    int pad_t, pad_r, pad_b, pad_l;  /* internal padding (pre-scaled px) */
} cat_box;

/* The content area inside the padding (what children tile / what you draw into). */
SDL_Rect cat_box_content(const cat_box *b);

/* Carve a fixed-height band off the TOP of `b`, return it as a box, and shrink
   `b` to the remainder. height==0 → returns an empty box, `b` unchanged (this is
   how an absent sub-header / hidden hint bar costs nothing). */
cat_box  cat_box_carve_top(cat_box *b, int height);

/* Carve a fixed-height band off the BOTTOM (e.g. the hint bar). */
cat_box  cat_box_carve_bottom(cat_box *b, int height);

/* Split a box into a left column of `left_w` and a right column, with `gutter`
   between them. The gutter is realized as padding on the facing edges (left box
   gets pad_r += gutter/2, right box gets pad_l += gutter/2) — no sibling margin.
   left_w may be a fraction helper or absolute px (see open question 1). */
void     cat_box_split_cols(const cat_box *b, int left_w, int gutter,
                            cat_box *left, cat_box *right);
```

Notes:
- Pure geometry: no drawing, no global state. Trivially unit-testable on Mac.
- Padding values are caller-supplied, pre-scaled (callers pass `CAT_S(n)` /
  `CAT_DS(n)` as today).
- C only; `cat_`/`CAT_` naming; lives in the header-only toolkit next to
  `cat_get_content_rect`.

## Jawaka composition (`jw_`)

Each browse page builds its tree from the primitive (launcher-specific, stays in
Jawaka):

```
root = { 0,0, screen_w, screen_h, pad = edge }      // root edge padding
tab    = cat_box_carve_top(&root, cat_get_tab_bar_height())   // tabs + status live inside
sub    = cat_box_carve_top(&root, sub_header_h)               // 0 on tab pages
hints  = cat_box_carve_bottom(&root, jw_footer_height())      // 0 when hints off
content = root                                                // what's left
cat_box_split_cols(&content, list_w, gutter, &list, &image)
```

Then: draw the tab bar (with inline status reserved-right) into `tab`, the
sub-header text into `sub`, the rows into `cat_box_content(&list)` (row height =
content_h / visible_rows), the cover into `cat_box_content(&image)`, the footer
into `hints`. List and Image are the **same box height** by construction — the
asymmetry bug is gone.

A single Jawaka helper (e.g. `jw__browse_boxes(state, has_subheader, &list,
&image, &item_h)`) returns the two content boxes + row height so all five pages
share one assembly. The standalone `jw__browse_layout` added today is the
stopgap; it gets replaced by this.

## Open questions (need answers before building)

1. **Padding values:** one global base pad reused for edges + content insets +
   half-gutter, or a few named ones (edge / gutter / art)? Lean: one base pad,
   art-pad its own.
2. **`left_w`:** pass the list width as a fraction (e.g. 58%) computed by the
   caller, or add a `cat_box_split_frac(...)`? Lean: caller passes px (keeps the
   primitive dumb).
3. **Gap to the hints** (the thing we kept chasing): with equal list/image boxes
   and a content-box bottom padding, what's the target — flush, or one base pad?
   Recommend one base pad, tuned once here.

## Rollout

- Land `cat_box` + ops in Catastrophe (additive), verify with the native demo.
- Migrate the five Jawaka pages to `jw__browse_boxes`; delete the per-page math
  and the stopgap `jw__browse_layout`.
- Flag for Helaas (touches the shared repo; purely additive).
