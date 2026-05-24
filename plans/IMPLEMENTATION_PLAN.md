# Catastrophe — Allium Theming Implementation Plan

## Overview

Catastrophe (fork of Apostrophe) is a C header-only GUI toolkit targeting retro gaming handhelds. Allium is a Rust-based custom firmware with a rich JSON-driven theming system. This plan describes how to integrate Allium's theming into Catastrophe.

---

## Phase 1: Apostrophe → Catastrophe Rename

**Scope:** Every file, identifier, macro, guard, comment, and URL referencing "Apostrophe" or "AP_" (public API prefix).

### Files to rename

| Current | New |
|---------|-----|
| `include/apostrophe.h` | `include/catastrophe.h` |
| `include/apostrophe_widgets.h` | `include/catastrophe_widgets.h` |
| All `#include "apostrophe.h"` | `#include "catastrophe.h"` |
| All `#include "apostrophe_widgets.h"` | `#include "catastrophe_widgets.h"` |

### Identifier renames (public API)

| Current prefix | New prefix | Count |
|----------------|------------|-------|
| `ap_` | `cat_` | ~80+ functions |
| `AP_` (constants/macros/enums) | `CAT_` | ~40+ |
| `AP__` (internal) | `cat__` | ~30+ |

### Include guards

| Current | New |
|---------|-----|
| `APOSTROPHE_H` | `CATASTROPHE_H` |
| `APOSTROPHE_WIDGETS_H` | `CATASTROPHE_WIDGETS_H` |

### Documentation & metadata

- `CHANGELOG.md` — all "Apostrophe" → "Catastrophe"
- `README.md` — rewrite description
- `docs/*.md` — update all references, URLs
- `CONTRIBUTING.md` — update
- `LICENSE` — keep MIT, update copyright holder
- `Makefile` — all comments, echo strings
- Source file top comments — all `/* Apostrophe ... */` → `/* Catastrophe ... */`
- URLs: `github.com/Helaas/apostrophe` → `github.com/<org>/catastrophe`

### Implementation notes

- Use `sed` for bulk rename across all files
- Verify with `rg "Apostrophe|apostrophe|AP_"` before and after
- Rename actual `.c` and `.h` files last (to keep tooling working during rename)
- Update Makefile include paths to use new header names
- Guard macros `AP_IMPLEMENTATION` → `CAT_IMPLEMENTATION` must be updated in the #define before inclusion in every .c file

**Decision (from grill-me):** Full rename — no backward compat needed since this is a fork.

---

## Phase 2: New `cat_color` Type

Allium's `Color` is a `#[repr(transparent)] u32` with RGBA byte layout. Catastrophe's current `ap_color` is `SDL_Color` (4 bytes, but struct not u32).

### New type definition

```c
// RGBA stored as uint32_t matching Allium's layout
// Byte order: [R][G][B][A] (little-endian native)
typedef uint32_t cat_color;

// Helper to convert to SDL_Color for rendering
static inline SDL_Color cat_color_to_sdl(cat_color c) {
    SDL_Color sdl;
    sdl.r = (c >> 0) & 0xFF;
    sdl.g = (c >> 8) & 0xFF;
    sdl.b = (c >> 16) & 0xFF;
    sdl.a = (c >> 24) & 0xFF;
    return sdl;
}

static inline cat_color cat_color_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return (uint32_t)r | ((uint32_t)g << 8) | ((uint32_t)b << 16) | ((uint32_t)a << 24);
}

static inline cat_color cat_color_rgb(uint8_t r, uint8_t g, uint8_t b) {
    return cat_color_rgba(r, g, b, 0xFF);
}
```

### Methods to implement (matching Allium)

| Allium method | C equivalent |
|---------------|-------------|
| `Color::new(r,g,b)` | `cat_color_rgb(r,g,b)` |
| `Color::rgba(r,g,b,a)` | `cat_color_rgba(r,g,b,a)` |
| `Color::r()` | `CAT_COLOR_R(c)` macro |
| `Color::g()` | `CAT_COLOR_G(c)` macro |
| `Color::b()` | `CAT_COLOR_B(c)` macro |
| `Color::a()` | `CAT_COLOR_A(c)` macro |
| `Color::with_r()` | `cat_color_with_r(c, r)` |
| `Color::blend(other, alpha)` | `cat_color_blend(c, other, alpha)` |
| `Color::invert()` | `cat_color_invert(c)` |
| `Color::is_dark()` | `cat_color_is_dark(c)` |
| Hex `#rrggbb` parse | `cat_color_from_hex("##rrggbb")` |
| Hex `#rrggbbaa` parse | `cat_color_from_hex_alpha("##rrggbbaa")` |
| Serialize to `"#rrggbb"` | `cat_color_to_hex(c, buf, 8)` |

### AP_COLOR_ compatibility

Keep `ap_color` as `SDL_Color` typedef during transition. The new theme system uses `cat_color` internally. All draw functions that currently take `ap_color` will gain `cat_color` overloads or be migrated.

**Decision (from grill-me):** New `cat_color` type wrapping u32, matching Allium's RGBA layout.

---

## Phase 3: Add cJSON Library

Allium themes are JSON files with nested objects. Catastrophe's existing `ap__json_find_string()` is a flat key-value extractor that cannot handle nested structures like `{"ui": {"text_color": "#fff"}}`.

### Approach

- Vendor `cJSON.h` and `cJSON.c` (MIT license) into `include/catastrophe/` or as standalone files
- cJSON is ~700 lines, well-tested, single-file, no external deps
- Provides recursive descent parsing, nested object access, array iteration

### Usage in theme loading

```c
cJSON *root = cJSON_Parse(json_string);
cJSON *ui = cJSON_GetObjectItem(root, "ui");
cJSON *text_color = cJSON_GetObjectItem(ui, "text_color");
if (cJSON_IsString(text_color)) {
    theme.ui.text_color = cat_color_from_hex(text_color->valuestring);
}
cJSON_Delete(root);
```

**Decision (from grill-me):** Pull in cJSON rather than extending the ad-hoc parser.

---

## Phase 4: New `cat_stylesheet` Struct (Allium 1:1 Mirror)

Allium's `Stylesheet` is a Rust struct with nested sub-structs. Catastrophe will mirror it exactly in C.

### Struct hierarchy

```c
// ─── cat_color forward (Phase 2) ───
typedef uint32_t cat_color;

// ─── Font ───
typedef struct {
    char  path[1024];    // TTF file path
    int   size;          // font point size
    // runtime: loaded TTF_Font* at this size
} cat_stylesheet_font;

// ─── UI ───
typedef struct {
    int               margin_x;              // default: 12
    int               margin_y;              // default: 8
    int               list_margin;           // default: 4
    int               padding_x;             // default: 12
    int               padding_y;             // default: 4
    cat_stylesheet_font ui_font;
    cat_color         text_color;            // default: #ffffff
    cat_color         text_stroke_color;     // default: #00000000
    cat_color         background_color;      // default: #000000
    cat_color         highlight_color;       // default: #7287fd
    cat_color         highlight_text_color;  // default: #ffffff
    cat_color         highlight_text_stroke_color; // default: #00000000
    cat_color         disabled_color;        // default: #585b70
    float             tab_font_size;         // default: 1.0 (multiplier)
    cat_color         tab_color;             // default: #ffffff70
    cat_color         tab_stroke_color;      // default: #00000000
    cat_color         tab_selected_color;    // default: #ffffff
    cat_color         tab_selected_stroke_color; // default: #00000000
    uint32_t          stroke_width;          // default: 0
} cat_stylesheet_ui;

// ─── Status Bar ───
typedef struct {
    bool              show_battery_level;    // default: false
    bool              show_clock;            // default: true
    bool              show_wifi;             // default: false
    float             font_size;             // default: 1.0 (multiplier)
    cat_color         text_color;            // default: #ffffff
    cat_color         text_stroke_color;     // default: #00000000
} cat_stylesheet_status_bar;

// ─── Button Hints ───
typedef struct {
    float             button_hint_font_size; // default: 0.9
    float             button_size;           // default: 1.0
    float             button_text_font_size; // default: 0.75
    cat_color         button_a_color;        // default: #eb1a1d
    cat_color         button_b_color;        // default: #fece15
    cat_color         button_x_color;        // default: #0749b4
    cat_color         button_y_color;        // default: #008d45
    cat_color         button_bg_color;       // default: #585b70
    cat_color         button_text_color;     // default: #ffffff
    cat_color         text_color;            // default: #ffffff
} cat_stylesheet_button_hints;

// ─── Recents ───
typedef struct {
    bool              use_recents_carousel;  // default: false
} cat_stylesheet_recents;

// ─── Games ───
typedef struct {
    uint32_t          boxart_width;          // default: 250
} cat_stylesheet_games;

// ─── Menu ───
typedef struct {
    cat_color         background_color;      // default: #000000
    cat_stylesheet_font guide_font;
} cat_stylesheet_menu;

// ─── Top-level Stylesheet ───
typedef struct {
    char                     wallpaper[1024]; // empty = none
    cat_stylesheet_ui        ui;
    cat_stylesheet_status_bar status_bar;
    cat_stylesheet_button_hints button_hints;
    cat_stylesheet_recents   recents;
    cat_stylesheet_games     games;
    cat_stylesheet_menu      menu;
    // runtime-loaded fonts (for CJK)
    cat_stylesheet_font      cjk_font;
} cat_stylesheet;
```

### Allium `StylesheetColor` enum equivalent

```c
typedef enum {
    CAT_SC_FOREGROUND,
    CAT_SC_BACKGROUND,
    CAT_SC_HIGHLIGHT,
    CAT_SC_HIGHLIGHT_TEXT,
    CAT_SC_DISABLED,
    CAT_SC_TAB,
    CAT_SC_TAB_SELECTED,
    CAT_SC_BUTTON_A,
    CAT_SC_BUTTON_B,
    CAT_SC_BUTTON_X,
    CAT_SC_BUTTON_Y,
    CAT_SC_BUTTON_BACKGROUND,
    CAT_SC_BUTTON_TEXT,
    CAT_SC_BUTTON_HINT_TEXT,
    CAT_SC_BACKGROUND_HIGHLIGHT_BLEND,
    CAT_SC_STROKE,
    CAT_SC_HIGHLIGHT_TEXT_STROKE,
    CAT_SC_TAB_STROKE,
    CAT_SC_TAB_SELECTED_STROKE,
    CAT_SC_STATUS_BAR,
    CAT_SC_STATUS_BAR_STROKE,
    CAT_SC_MENU_BACKGROUND,
    CAT_SC_COUNT
} cat_stylesheet_color_slot;

cat_color cat_stylesheet_get_color(const cat_stylesheet *s, cat_stylesheet_color_slot slot);
```

### Relation to legacy `ap_theme`

- `ap_theme` continues to exist as a simplified view for backward compat
- `cat_stylesheet` is the new primary theming object
- A helper `cat_stylesheet_to_ap_theme()` fills an `ap_theme` from a `cat_stylesheet`
- Widgets that use `ap_get_theme()` continue to work via the conversion

**Decision (from grill-me):** Full 1:1 mirror of Allium's Stylesheet with nested C structs.

---

## Phase 5: Theme Loading & JSON Parsing

### Platform path strategy

Allium uses SD card paths:
- `/Themes/<Name>/stylesheet.json` — theme definition
- `/Themes/<Name>/stylesheet.override.json` — user overrides
- Active theme: `<BASE_DIR>/state/theme` (text file with theme name)

Catastrophe should use a configurable base path (env var `CAT_THEMES_DIR`, default `./Themes/` on desktop, `/mnt/SDCARD/Themes/` on device).

### Theme loading API

```c
// Initialize with default hardcoded Allium theme values
void cat_stylesheet_init_default(cat_stylesheet *s);

// Load a stylesheet from JSON file at `path`
// Returns CAT_OK on success, CAT_ERROR on failure
int cat_stylesheet_load_file(cat_stylesheet *s, const char *path);

// Load from a theme directory (reads stylesheet.json + optional override)
int cat_stylesheet_load_theme(cat_stylesheet *s, const char *theme_name);

// Apply the stylesheet: set theme colors, load fonts, load wallpaper
int cat_stylesheet_apply(cat_stylesheet *s);

// Get list of available themes (scans themes dir)
int cat_stylesheet_available_themes(const char ***out_names, int *out_count);

// Free theme name list
void cat_stylesheet_free_theme_list(const char **names, int count);
```

### Loading flow (matching Allium)

1. Read `<CAT_THEMES_DIR>/<theme_name>/stylesheet.json`
2. Parse JSON with cJSON into `cat_stylesheet` struct
3. Check for `<CAT_THEMES_DIR>/<theme_name>/stylesheet.override.json` and merge
4. Resolve font paths (absolute → theme-relative → system fonts dir)
5. Load fonts from disk
6. Resolve wallpaper path (theme dir → absolute)
7. If wallpaper path exists, call `cat_reload_background()`
8. Set all theme colors into the rendering engine

### Defaults (hardcoded, matching Allium)

All Allium default values are compiled into `cat_stylesheet_init_default()` as C constants, matching the Rust `impl Default` blocks.

**Decision (from grill-me):** JSON file loading in C via cJSON.

---

## Phase 6: Allium-Themes Git Submodule

### Setup

```bash
git submodule add https://github.com/goweiwen/Allium-Themes.git themes/Allium-Themes
```

### Directory structure

```
themes/
  Allium-Themes/        # git submodule (full Allium theme collection)
    Themes/
      Allium/
      AlliumBoy/
      Blue/
      Brown/
      Bubblegum/
      EVA 01/
      Min/
      Pastel/
      SNES/
      SpruceOS/
      Synthwave/
```

### At build time

- On desktop: themes are located at `./themes/Allium-Themes/Themes/<Name>/`
- On device: themes are expected at `/mnt/SDCARD/Themes/<Name>/` (standard Allium path)
- The `cat_stylesheet_load_theme()` function tries the `CAT_THEMES_DIR` env var first, falls back to platform-appropriate paths
- Desktop preview mode copies the relevant theme's `stylesheet.json` to the build dir or reads from the `CAT_THEMES_DIR`

### Available themes (11 total)

1. Allium (default)
2. AlliumBoy
3. Blue
4. Brown
5. Bubblegum
6. EVA 01
7. Min
8. Pastel
9. SNES
10. SpruceOS
11. Synthwave

**Decision (from grill-me):** Full git submodule for the Allium-Themes repo.

---

## Phase 7: Theme-Switching Demo

### Location

New sub-screen in the demo example: `demo_allium_themes()`, added to the top-level demo menu as "Allium Themes".

### UX design

```
┌─────────────────────────────────┐
│  ALLIUM THEMES                  │
│                                 │
│  > Theme: [Allium    < >]       │
│    Wallpaper: [None   < >]      │
│    Show Clock: [On     < >]     │
│    Show Battery: [Off   < >]    │
│    Show WiFi: [Off    < >]      │
│    UI Font Size: [36    < >]    │
│    Text Color:    [█ picker]    │
│    Background:    [█ picker]    │
│    Highlight:     [█ picker]    │
│    ...                          │
│    [Restore Defaults]           │
│                                 │
│  ┌──────┐ ┌──┐ ┌──────┐ ┌──┐   │
│  │ BACK │ │←→│ │EDIT │ │OK│   │
│  └──────┘ └──┘ └──────┘ └──┘   │
└─────────────────────────────────┘
```

### Implementation

- Uses `ap_options_list` widget (settings-style) for the main theme configuration
- Theme row: Left/Right cycles through available themes (scanned from `CAT_THEMES_DIR`)
- Upon theme change → `cat_stylesheet_load_theme()` → `cat_stylesheet_apply()` → colors/fonts/wallpaper update immediately
- Wallpaper row: select from available wallpapers
- Color rows: use `ap_color_picker` for individual color editing
- "Restore Defaults" clears the override file and reloads
- Help overlay documents available buttons

### What changes live

- All widget colors (list selections, text, backgrounds)
- Status bar appearance (clock, battery, WiFi visibility + colors)
- Footer button hint colors
- Background wallpaper
- Font sizes

### Integration with existing `demo_input_theme`

The existing `demo_input_theme` continues to exist for legacy accent color + input config testing. The new `demo_allium_themes` is a separate, more comprehensive theme demo that's Allium-specific.

**Decision (from grill-me):** Settings-style `ap_options_list` widget with live preview of theme changes.

---

## Phase 8: Migration of Internal Drawing to `cat_stylesheet`

### What needs to change

All rendering code in `catastrophe.h` and `catastrophe_widgets.h` currently references `ap__g.theme` (the legacy `ap_theme` struct). These need to pull colors from the new `cat_stylesheet` instead.

### Strategy

1. Store both `cat_stylesheet` and legacy `ap_theme` inside `cat__state`
2. On `cat_stylesheet_apply()`, update both:
   - The `cat_stylesheet` (primary)
   - The legacy `ap_theme` via `cat_stylesheet_to_ap_theme()` (for backward compat)
3. Widgets continue using `ap_get_theme()` during transition
4. Deprecate `ap_get_theme()` in favor of `cat_get_stylesheet()`

### `cat_stylesheet_to_ap_theme()` mapping

| `ap_theme` field | `cat_stylesheet` source |
|------------------|------------------------|
| `highlight` | `s->ui.highlight_color` |
| `accent` | `s->ui.highlight_color` (mapped) |
| `button_label` | `s->button_hints.button_text_color` |
| `text` | `s->ui.text_color` |
| `highlighted_text` | `s->ui.highlight_text_color` |
| `hint` | `s->ui.disabled_color` |
| `background` | `s->ui.background_color` |
| `font_path` | `s->ui.ui_font.path` |
| `bg_image_path` | `s->wallpaper` |

---

## Phase 9: Build System Integration

### Makefile changes

1. Rename all targets from `apostrophe` to `catastrophe`
2. Add `cJSON.c` to build dependencies
3. Add `themes/` to distribution packages
4. Add `themes-setup` target for git submodule init
5. Add `CAT_THEMES_DIR` env var support in `run-mac-*` targets

### New Makefile targets

```makefile
themes-setup:
    git submodule init
    git submodule update

run-mac-demo-allium: mac-demo
    CAT_THEMES_DIR="$(CURDIR)/themes/Allium-Themes/Themes" \
    CAT_FONTS_DIR="$(CURDIR)/themes/Allium-Themes/Fonts" \
    ./build/mac/demo/demo
```

### Docker/cross-compilation

- The cJSON.c source must be compiled into each example alongside catastrophic_widgets.c
- On-device builds: themes live at `/mnt/SDCARD/Themes/` (standard Allium path)
- Fonts: Allium ships fonts in its themes repo at `Allium-Themes/Fonts/`

---

## Phase 10: Documentation Updates

### Updated files

| File | Change |
|------|--------|
| `docs/API.md` | Update all `ap_` → `cat_`, add new theme API docs |
| `docs/WIDGETS.md` | Update function signatures, add theming guidance |
| `docs/GETTING_STARTED.md` | Update build instructions, submodule setup |
| `docs/DEMO_COVERAGE.md` | Add Allium themes demo to coverage |
| `docs/PORTING_FROM_GABAGOOL.md` | Update references, note new theming |
| `docs/GABAGOOL_PARITY_v2.9.6.md` | Update references |
| New: `docs/THEMING.md` | Comprehensive theming documentation |

### `docs/THEMING.md` contents

- Overview of Allium's theme system
- JSON format reference (all fields, defaults, types)
- How to create a custom theme
- How to add a wallpaper
- Theme file location guides (desktop vs device)
- Override system (stylesheet.override.json)
- Importing from Allium-Themes
- Programmatic theme changes API
- RetroArch theme integration

---

## Architectural Decisions Record

| # | Decision | Choice |
|---|----------|--------|
| 1 | Theme struct architecture | Full 1:1 mirror of Allium's Stylesheet with nested C structs |
| 2 | Theme loading strategy | JSON file loading in C via cJSON |
| 3 | Allium-Themes sourcing | Full git submodule |
| 4 | Theme-switching demo design | Settings-style `ap_options_list` with live preview |
| 5 | Rename strategy | Full rename (Apostrophe/AP_ → Catastrophe/CAT_) |
| 6 | Color type | New `cat_color` (u32-based, matching Allium's RGBA layout) |
| 7 | Wallpaper handling | Included in theme loading; resolved relative to theme dir |
