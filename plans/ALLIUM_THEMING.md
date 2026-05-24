# Allium Theming Plan

Supplements `THEMES_IMPORT.md` (which covers schema, submodule, discovery, override file). This plan covers the active migration: dropping NextUI, wiring the pill to the stylesheet, shipping an Apostrophe-parity default, and the renamed live-preview demo.

## Goals

1. **NextUI is dead in Catastrophe.** Code, doc strings, comments, font paths, asset loaders, and the `cat_theme_load_nextui()` path go away. Allium is the only runtime target.
2. **Apostrophe pixel-perfect default.** Out of the box, Catastrophe looks identical to Apostrophe — same white pill, black-on-pill text, same spacing — but the source of those values is now an Allium-format `stylesheet.json`, not hardcoded NextUI MinUI keys.
3. **Pills are themeable.** Fill color, text color, internal padding, and shape come from `stylesheet.ui` fields. Importing any of the 11 Allium themes produces the colors and feel the theme author intended.
4. **Live theme preview in the demo.** Switching themes, wallpapers, or fonts updates the UI immediately. In-memory only — no disk writes, restart returns to last saved theme.
5. **Allium themes are bundled (via submodule).** All 11 are discoverable at runtime from `themes/Allium-Themes/Themes/` on desktop and `/mnt/SDCARD/Themes/` on device.

## Non-goals

- Status bar sprite replacement. Catastrophe's status pill sprite (`status_assets`) is a generic capped-circle sprite; it stays as a render asset, just renamed to drop "NextUI" from comments.
- RetroArch theme generation (`set_retroarch_theme`). Documented in `THEMES_IMPORT.md` as future work.
- Per-platform integer-scaling logic (`CAT_DS`, `CAT_S`, `device_scale`). Stays as-is — reused for Allium device variants.
- Hardcoded-C default copy of the stylesheet (rejected in grilling). The Apostrophe-parity `stylesheet.json` is the single source of truth.

---

## Phase A — Drop NextUI

Anything that loads, parses, references, or names NextUI gets stripped. Scaling/layout primitives stay.

### A1. Remove the NextUI theme loader

File: `include/catastrophe.h`
- Delete `cat_theme_load_nextui()` (declaration line 764, definition line 1675–~1740). It parses `color1..color7` from `nextval.elf` — has no Allium equivalent.
- Delete the second copy at lines 1774+ if it's the duplicate I saw in grep.
- Delete `is_nextui` field on the init opts struct (line 594).
- Remove any auto-call sites — verify with grep after deletion.

### A2. Strip NextUI from font search paths

File: `include/catastrophe.h` lines 1051–~1080
- Remove `font1.ttf`, `font2.ttf` entries on tg5040/tg5050/my355 platforms.
- Replace with Allium's font convention: `<ALLIUM_BASE_DIR>/.allium/fonts/Nunito.ttf` and fallback to `res/font.ttf`.
- Per-theme `ui_font.path` (loaded by stylesheet) takes precedence over the search list anyway.

### A3. Rename comments and identifiers

- `include/catastrophe_widgets.h:2` — "UI component library for NextUI Paks" → "UI component library for Allium".
- `include/catastrophe.h:2` — "C UI toolkit for NextUI Paks" → "C UI toolkit for Allium".
- `include/catastrophe.h:3046` — comment "pre-rendered NextUI pill sprite" → "pre-rendered pill sprite".
- All comments referencing "NextUI FONT_LARGE", "NextUI FIXED_SCALE", "NextUI PADDING", "NextUI showclock" — strip the "NextUI" qualifier; the constants/behaviors stay.
- `CAT_CLOCK_AUTO`/`SHOW`/`HIDE` comments at lines 521–528 — rewrite without "NextUI clock24h" reference.
- `cat__default_theme` comment at line 1038 "matches Gabagool's NextUI defaults" → delete the legacy struct entirely once stylesheet path is sole source (see Phase B3).

### A4. Strip NextUI from docs

Files: `README.md`, `docs/GETTING_STARTED.md`, `docs/API.md`, `docs/PORTING_FROM_GABAGOOL.md`, `docs/GABAGOOL_PARITY_v2.9.6.md`, `CHANGELOG.md`, `plans/RENAME_PLAN.md`
- Search/replace "NextUI Pak" → "Allium app" (or equivalent — verify per file).
- Remove any "loads theme from NextUI's nextval.elf" sentences.
- Update `GETTING_STARTED.md` to describe the Allium runtime entry point instead of `pak/launch.sh` NextUI style (verify how Allium launches third-party tools — `crates/allium-launcher` source has the answer).

### A5. Status bar assets

File: `include/catastrophe.h:689` — comment "Status bar assets (NextUI spritesheet)" → "Status bar assets (capped-pill sprite)". The sprite itself is generic; only the wording is NextUI-flavored.

---

## Phase B — Apostrophe-parity default theme

### B1. Author the on-disk default theme

New directory: `res/themes/Catastrophe/stylesheet.json`

Values reproduce the current Apostrophe defaults from `catastrophe.h:1040–1049`:

```json
{
  "wallpaper": "",
  "ui": {
    "margin_x": 12,
    "margin_y": 8,
    "list_margin": 4,
    "padding_x": 12,
    "padding_y": 4,
    "ui_font": { "path": "font.ttf", "size": 36 },
    "text_color": "#ffffff",
    "text_stroke_color": "#00000000",
    "background_color": "#000000",
    "highlight_color": "#ffffff",
    "highlight_text_color": "#000000",
    "highlight_text_stroke_color": "#00000000",
    "disabled_color": "#585b70",
    "tab_color": "#ffffff70",
    "tab_selected_color": "#ffffff",
    "stroke_width": 0,
    "pill_radius_ratio": 1.0
  },
  "status_bar": { "show_clock": true, "show_battery_level": false, "show_wifi": false,
                  "text_color": "#ffffff", "font_size": 1.0 },
  "button_hints": {
    "button_a_color": "#9b2257",
    "button_b_color": "#9b2257",
    "button_x_color": "#9b2257",
    "button_y_color": "#9b2257",
    "button_bg_color": "#1e2329",
    "button_text_color": "#ffffff",
    "text_color": "#ffffff"
  },
  "menu": { "background_color": "#000000" }
}
```

> **Apostrophe-pixel-perfect verification:** `highlight=#ffffff` matches `catastrophe.h:1040`. `highlight_text_color=#000000` matches `:1044`. Accent `#9b2257` matches `:1041` (this is the Apostrophe button-hint pink). Background `#000000` matches `:1046`.

This becomes the value returned by `cat_stylesheet_init_default()` — see B3.

### B2. Make this theme discoverable as "Catastrophe"

`cat_stylesheet_available_themes()` already scans the themes dir per `THEMES_IMPORT.md`. Add `res/themes/` to its search list so the shipped default theme appears alongside Allium ones. Order: `$CAT_THEMES_DIR` → `res/themes/` → `themes/Allium-Themes/Themes/` → `/mnt/SDCARD/Themes/`.

### B3. Reroute `cat_stylesheet_init_default()` to load from disk

Currently the C function at `catastrophe.h:~1244` populates a `cat_stylesheet` struct field-by-field with Allium catppuccin defaults (highlight `#7287fd` etc.) — wrong for Apostrophe parity.

Change: `cat_stylesheet_init_default()` calls `cat_stylesheet_load_theme(&ss, "Catastrophe")` and, if that fails (e.g. `res/` not bundled), falls back to a **minimal** hardcoded set whose values match B1 exactly. The fallback is for headless environments only and must be kept in sync with B1 via a unit test (or assert at startup).

Delete the unused `cat__default_theme` struct at `catastrophe.h:1038` once nothing references it.

### B4. Wire NextUI removal closing checks

After Phase A + B, grep should return zero hits for:
```
grep -rn "nextui\|NextUI\|nextval\|color7\|color1[^0-9]" include/ examples/ docs/ README.md CHANGELOG.md
```
Exceptions allowed only inside `plans/` historical docs.

---

## Phase C — Theme the pill

Four bindings + one new field for shape.

### C1. Pill fill (already wired)

`catastrophe.h:1514` does `t->highlight = cat_color_to_sdl(s->ui.highlight_color)`. No code change. Just verify all 8 `cat_draw_pill(...)` call sites pass `theme->highlight` (they do per grep above).

### C2. Pill text + stroke (already wired)

`:1518` maps `highlight_text_color`. Add the stroke mapping if missing (search for `highlight_text_stroke_color` consumer; if absent, plumb through to text rendering on the pill).

### C3. Pill internal padding

Replace `CAT__BUTTON_PADDING` constant uses inside pill drawing with `theme->ui_padding_x / ui_padding_y`. Concretely:
- Add `int ui_padding_x; int ui_padding_y;` to `ap_theme` (catastrophe.h ~line 366 area, alongside existing fields).
- Map in `cat_stylesheet_apply()` (around `:1514`): `t->ui_padding_x = s->ui.padding_x; t->ui_padding_y = s->ui.padding_y;`.
- Update consumers in `catastrophe_widgets.h`:
  - line 583: `int pill_pad = theme->ui_padding_x;` (replaces `CAT_DS(CAT__BUTTON_PADDING)`).
  - line 810: width calc uses `pill_pad` already — no change once `pill_pad` is sourced from theme.
  - Apply `device_scale` where it was previously applied: stylesheet values are stored unscaled; multiply by `device_scale` at the call site, same as Allium does.

> **Apostrophe-pixel-perfect check:** Apostrophe's `CAT__BUTTON_PADDING` (find current value in `catastrophe.h`) must equal B1's `padding_x: 12` after device scaling. If not, adjust B1's `padding_x` (and `padding_y`) to whatever value reproduces today's pixel output. Verify with a screenshot diff against current `examples/demo` output.

### C4. Pill shape (new field)

Add `pill_radius_ratio` to the stylesheet schema (float, 0.0–1.0, default 1.0 = full cap).

- Schema: `cat_stylesheet.ui.pill_radius_ratio` (float). Default 1.0.
- `ap_theme.pill_radius_ratio` (float). Mapped in `cat_stylesheet_apply()`.
- `cat_draw_pill(x, y, w, h, color)` at `catastrophe.h:3046`:
  - If `ratio >= 1.0` → existing sprite blit path (no change).
  - If `ratio <= 0.0` → call `cat_draw_rect(x, y, w, h, color)` (square corners).
  - Otherwise → procedural rounded-rect: compute `r = (int)(ratio * h * 0.5f)`; use existing `cat__fill_circle_quadrant` helper (already at `:3040`) for the four corners plus rect fill for the center. No sprite use — the sprite is fixed-radius.

Default 1.0 keeps Apostrophe pixel-perfect (sprite path = today's behavior).

### C5. Stroke width

`ui.stroke_width` in Allium is **text outline width**, not pill outline. Leave pill outline off (Apostrophe never had one). Plumb `stroke_width` into text rendering only if not already done.

---

## Phase D — Demo rename + live preview

### D1. Rename

- `examples/demo/main.c:2271` — `cat_draw_screen_title("Allium Themes", NULL)` → `"Catastrophe Themes"`.
- `examples/demo/main.c:2911` — menu entry `{ "Allium Themes", demo_allium_themes }` → `{ "Catastrophe Themes", demo_catastrophe_themes }`.
- Rename function `demo_allium_themes` → `demo_catastrophe_themes` (declaration line 22, definition line 2203).

### D2. Live wallpaper picker

Inside `demo_catastrophe_themes()` (post-rename), add a third menu row "Wallpaper" that cycles through wallpaper images discovered in the current theme directory + a built-in set.

- New helper `cat_stylesheet_list_wallpapers(const char *theme_dir, const char ***out, int *count)` — globs `*.png/*.jpg` in the theme dir.
- LEFT/RIGHT on the Wallpaper row swaps `cat__g.stylesheet.wallpaper`, calls `cat_reload_wallpaper_texture()` (new — extract from existing apply-stylesheet path).
- **No disk write.** State lives in the running `cat__g.stylesheet`. Exiting the demo and re-entering shows the last saved theme.

### D3. Live font picker

New menu row "Font" that cycles through fonts discovered in:
- `themes/Allium-Themes/Themes/<current>/fonts/`
- `res/font.ttf`
- System fallbacks from the font search list (Phase A2).

- Helper `cat_stylesheet_list_fonts(const char ***out, int *count)`.
- LEFT/RIGHT updates `cat__g.stylesheet.ui.ui_font.path`, calls `cat_reload_font_atlas()` (new — extract from stylesheet apply path; must free old `TTF_Font*` for each `CAT_FONT_*` size and reopen with the new file).
- **No disk write.**

### D4. Demo stylesheet for new pill features

New file: `res/themes/Catastrophe-Demo/stylesheet.json`. Same as B1 but with knobs cranked to show off C3/C4:

```json
{
  "ui": {
    "highlight_color": "#9b2257",
    "highlight_text_color": "#ffffff",
    "padding_x": 24,
    "padding_y": 10,
    "pill_radius_ratio": 0.25
  }
}
```

Renders fat magenta pills with mostly-square corners — visually obvious that padding + shape are now theme-controlled. Lives alongside the Catastrophe-parity theme so users can swap between the two in the demo and immediately see the difference.

---

## Phase E — Ship Allium themes

### E1. Submodule already added

`.gitmodules` already declares `themes/Allium-Themes` per `git status` in this session. Initialization documented in `THEMES_IMPORT.md`. No new work.

### E2. Build glue

`Makefile` additions per `THEMES_IMPORT.md:131–150` (cJSON compile, submodule init target). If not already in place, add now.

### E3. Default state file

`cat_theme_state_load()` returns `"Catastrophe"` if no state file exists, so first launch shows the parity theme.

### E4. Smoke test

Cycle through all 11 Allium themes in the renamed demo. Acceptance: each loads without crash, pills render in the theme's `highlight_color`, wallpaper (if present) draws, fonts switch.

---

## File-by-file checklist

| File | Change |
|---|---|
| `include/catastrophe.h` | Phase A1–A3, A5; B3; C1–C5 schema/apply; remove `cat__default_theme`. |
| `include/catastrophe_widgets.h` | A3 (header comment); C3 (pill_pad source). |
| `examples/demo/main.c` | D1 (rename); D2/D3 (live wallpaper/font rows). |
| `examples/hello/main.c`, `examples/combo/main.c`, `examples/download/main.c`, `examples/perf/main.c` | A4 if any reference NextUI in comments or theme load calls. |
| `res/themes/Catastrophe/stylesheet.json` | New (B1). |
| `res/themes/Catastrophe-Demo/stylesheet.json` | New (D4). |
| `README.md`, `CHANGELOG.md`, `docs/*.md`, `plans/RENAME_PLAN.md` | A4. |
| `Makefile` | E2 if not already done. |
| `plans/THEMES_IMPORT.md` | Update to reflect: no hardcoded-C default, pill_radius_ratio field added, demo renamed. |

## Acceptance criteria

1. `grep -rn "nextui\|NextUI" include/ examples/ docs/ README.md CHANGELOG.md` returns no results (plans/ allowed).
2. Building and running `examples/demo` with no theme state file produces output pixel-identical to current `main` for the list/grid/button-pill widgets. Diff screenshots if uncertain.
3. The "Catastrophe Themes" menu entry exists; selecting it, then cycling through all 11 Allium themes + Catastrophe + Catastrophe-Demo, never crashes.
4. On the "Catastrophe-Demo" theme, pills are visibly fatter (padding) and squarer (radius_ratio 0.25) than on "Catastrophe" — confirms C3 + C4 are honored.
5. LEFT/RIGHT on the "Wallpaper" and "Font" rows updates the UI within one frame; exiting and re-entering the demo restores the previously-saved theme (no disk write happened).

## Open questions (deferred)

- Should `pill_radius_ratio` also exist as an absolute pixel value (`pill_radius`) for theme authors who want a fixed radius regardless of pill height? Default `ratio` covers 90% of cases; defer.
- Allium also has `button_size` and `button_hint_font_size` multipliers — wire these through when button-hint widgets get themed (separate plan).
