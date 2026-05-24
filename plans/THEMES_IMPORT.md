# Allium Themes Import Strategy

## Source

**Repository:** https://github.com/goweiwen/Allium-Themes.git  
**Themes available:** 11 (Allium, AlliumBoy, Blue, Brown, Bubblegum, EVA 01, Min, Pastel, SNES, SpruceOS, Synthwave)

## Integration Method: Git Submodule

```bash
git submodule add https://github.com/goweiwen/Allium-Themes.git themes/Allium-Themes
```

All themes live at `themes/Allium-Themes/Themes/<Name>/` with the following structure per theme:

```
themes/Allium-Themes/Themes/<Name>/
  stylesheet.json     # Theme definition (required)
  fonts/              # Optional theme-specific fonts
  assets/             # Optional button/battery/wifi icons
  wallpaper.png       # Optional wallpaper image
```

## Theme JSON Format

Every theme provides a `stylesheet.json` file matching this schema:

```json
{
  "wallpaper": "wallpaper.png",
  "ui": {
    "margin_x": 12,
    "margin_y": 8,
    "list_margin": 4,
    "padding_x": 12,
    "padding_y": 4,
    "ui_font": { "path": "Nunito.ttf", "size": 36 },
    "text_color": "#ffffff",
    "text_stroke_color": "#00000000",
    "background_color": "#000000",
    "highlight_color": "#7287fd",
    "highlight_text_color": "#ffffff",
    "highlight_text_stroke_color": "#00000000",
    "disabled_color": "#585b70",
    "tab_font_size": 1.0,
    "tab_color": "#ffffff70",
    "tab_stroke_color": "#00000000",
    "tab_selected_color": "#ffffff",
    "tab_selected_stroke_color": "#00000000",
    "stroke_width": 0
  },
  "status_bar": {
    "show_battery_level": false,
    "show_clock": true,
    "show_wifi": false,
    "font_size": 1.0,
    "text_color": "#ffffff",
    "text_stroke_color": "#00000000"
  },
  "button_hints": {
    "button_hint_font_size": 0.9,
    "button_size": 1.0,
    "button_text_font_size": 0.75,
    "button_a_color": "#eb1a1d",
    "button_b_color": "#fece15",
    "button_x_color": "#0749b4",
    "button_y_color": "#008d45",
    "button_bg_color": "#585b70",
    "button_text_color": "#ffffff",
    "text_color": "#ffffff"
  },
  "recents": {
    "use_recents_carousel": false
  },
  "games": {
    "boxart_width": 250
  },
  "menu": {
    "background_color": "#000000",
    "guide_font": { "path": "Nunito.ttf", "size": 28 }
  }
}
```

All fields use `serde_json` defaults in Rust — missing fields fall back to hardcoded defaults. Catastrophe's C JSON parser must replicate the same default fallback logic.

## Fonts

Allium expects fonts at `ALLIUM_FONTS_DIR` (typically `<SD_ROOT>/.allium/fonts/`). The Allium-Themes repo may or may not ship fonts directly. If not, the Catastrophe setup needs to:

1. Check `themes/Allium-Themes/Fonts/` for bundled fonts
2. Fall back to `res/font.ttf` (existing Catastrophe font)
3. On device: use Allium's standard font paths

**Key fonts:** Nunito.ttf (UI + guide), NotoSansCJK.otf (CJK fallback)

## Theme Discovery at Runtime

`cat_stylesheet_available_themes()` scans:
1. `$CAT_THEMES_DIR` — env var override
2. `./themes/Allium-Themes/Themes/` — submodule path (desktop)
3. `/mnt/SDCARD/Themes/` — device path (Allium standard)

Each subdirectory containing a `stylesheet.json` is a valid theme.

## Theme State File

Allium stores the active theme name as plain text in a state file:
- Path: `<ALLIUM_BASE_DIR>/state/theme`
- Content: just the theme directory name, e.g. `"Allium"`
- Catastrophe equivalent: `cat_theme_state_save()` / `cat_theme_state_load()`

For desktop, the state file lives at `./.catastrophe/state/theme`.

## Override System

Allium supports per-user overrides via `stylesheet.override.json` in the same theme directory. Catastrophe must:

1. On load: read `stylesheet.json` first, then merge `stylesheet.override.json` on top
2. On save: write only the override file (keep base intact)
3. "Restore defaults": delete `stylesheet.override.json` and reload base

This allows users to customize any aspect of a theme without modifying the original.

## RetroArch Integration (Future)

Allium's `Stylesheet::set_retroarch_theme()` generates a RetroArch RGUI theme config and tinted wallpaper. Not a priority for initial Catastrophe integration but should be documented as a potential future feature.

## Build Integration

### Makefile additions

```makefile
# Theme submodule
THEMES_DIR := themes/Allium-Themes
THEMES_READY := $(THEMES_DIR)/.git

# cJSON dependency
CJSON_SRC := include/cjson/cJSON.c
CJSON_HDR := include/cjson/cJSON.h

themes-setup: $(THEMES_READY)
$(THEMES_READY):
    git submodule init
    git submodule update

# Compile cJSON into each example
mac-%: CJSON_OBJ = $(BUILD_DIR)/mac/$*/cJSON.o
mac-%: $(CJSON_OBJ)
```

### Docker cross-compilation

cJSON is pure C99 with no dependencies — it compiles cleanly on all target platforms. Simply add `$(CJSON_SRC)` to the compile command for each platform target.

## Default Theme Data

Since the git submodule may not always be initialized, Catastrophe should ship the ALLIUM default theme values as compiled-in C defaults in `catastrophe.h`:

```c
// Allium default theme (hardcoded fallback)
static const cat_stylesheet cat__default_stylesheet = {
    .wallpaper = "",
    .ui = {
        .margin_x = 12,
        .margin_y = 8,
        .list_margin = 4,
        .padding_x = 12,
        .padding_y = 4,
        .ui_font = { .path = "font.ttf", .size = 36 },
        .text_color = 0xFFFFFFFF,           // #ffffff
        .text_stroke_color = 0x00000000,    // #00000000
        .background_color = 0x000000FF,     // #000000
        .highlight_color = 0xFD8772FF,      // #7287fd (note: RGBA u32 layout)
        .highlight_text_color = 0xFFFFFFFF,
        .highlight_text_stroke_color = 0x00000000,
        .disabled_color = 0x705B58FF,       // #585b70
        .tab_font_size = 1.0f,
        .tab_color = 0x70FFFFFF,            // #ffffff70
        .tab_stroke_color = 0x00000000,
        .tab_selected_color = 0xFFFFFFFF,
        .tab_selected_stroke_color = 0x00000000,
        .stroke_width = 0,
    },
    // ... etc (all Allium defaults)
};
```
