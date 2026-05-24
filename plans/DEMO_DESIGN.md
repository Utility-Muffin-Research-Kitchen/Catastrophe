# Theme-Switching Demo Design

## Overview

A new sub-screen in the demo example that showcases Allium theme loading, switching, and live preview using Catastrophe's existing widget system.

---

## Menu Integration

Add to the top-level demo menu (after "Core API Lab"):

```c
{ .label = "Allium Themes" },
```

This launches `demo_allium_themes()` which is the main theme configuration screen.

---

## Screen Layout

The theme screen is a settings-style `ap_options_list` with the following rows:

### Row 1: Theme Selector
```
Theme: [Allium            < >]
```
- Type: `AP_OPT_STANDARD`
- Values: dynamically populated from `cat_stylesheet_available_themes()`
- On change: immediately calls `cat_stylesheet_load_theme(name)` → `cat_stylesheet_apply()` → live update

### Row 2: Wallpaper Selector
```
Wallpaper: [None          < >]
```
- Type: `AP_OPT_STANDARD`
- Values: dynamically populated from theme directory (PNG/JPG/BMP files + "None")
- On change: updates wallpaper via `cat_reload_background()`

### Row 3: Show Clock
```
Show Clock: [On           < >]
```
- Type: `AP_OPT_STANDARD`
- Values: On/Off
- Toggles `stylesheet.status_bar.show_clock`

### Row 4: Show Battery
```
Show Battery: [Off        < >]
```
- Values: On/Off
- Toggles `stylesheet.status_bar.show_battery_level`

### Row 5: Show WiFi
```
Show WiFi: [Off           < >]
```
- Values: On/Off
- Toggles `stylesheet.status_bar.show_wifi`

### Row 6: UI Font Size
```
UI Font Size: [36         < >]
```
- Type: `AP_OPT_STANDARD`
- Values: 10, 12, 14, 16, 18, 20, 24, 28, 32, 36, 40, 48, 56, 64
- Updates `stylesheet.ui.ui_font.size`

### Row 7: Text Color
```
Text Color: [███████████       ]
```
- Type: `AP_OPT_COLOR_PICKER`
- Opens `ap_color_picker` → sets `stylesheet.ui.text_color`

### Row 8: Background Color
```
Background Color: [███████████ ]
```
- Type: `AP_OPT_COLOR_PICKER`
- Opens `ap_color_picker` → sets `stylesheet.ui.background_color`

### Row 9: Highlight Color
```
Highlight Color: [███████████  ]
```
- Type: `AP_OPT_COLOR_PICKER`
- Opens `ap_color_picker` → sets `stylesheet.ui.highlight_color`

### Row 10: Disabled Color
```
Disabled Color: [███████████   ]
```
- Type: `AP_OPT_COLOR_PICKER`
- Opens `ap_color_picker` → sets `stylesheet.ui.disabled_color`

### Row 11: Tab Color
```
Tab Color: [███████████        ]
```
- Type: `AP_OPT_COLOR_PICKER`
- Opens `ap_color_picker` → sets `stylesheet.ui.tab_color`

### Row 12: Tab Selected Color
```
Tab Selected: [███████████     ]
```
- Type: `AP_OPT_COLOR_PICKER`
- Opens `ap_color_picker` → sets `stylesheet.ui.tab_selected_color`

### Row 13: Restore Defaults
```
Restore Defaults
```
- Type: `AP_OPT_CLICKABLE`
- On click: shows confirmation → `cat_stylesheet_restore_defaults()`

---

## Footer Buttons

```
[BACK] [<->] [SUMMARY] [SAVE]
```

| Button | Action | Label |
|--------|--------|-------|
| B | Back | BACK |
| Left/Right | Cycle option | (no footer) |
| X | Show summary | SUMMARY |
| A | Edit (color picker/keyboard) | EDIT |
| START | Save overrides | SAVE |

---

## Implementation

### New file: `examples/demo/allium_themes.c`

Contains:
- `demo_allium_themes()` — main entry point
- `demo_allium_themes_apply()` — applies current theme state
- `demo_allium_themes_scan()` — scans for available themes
- Live callback for footer updates showing current theme name

### Changes to `examples/demo/main.c`

- Add `#include "catastrophe_themes.h"` (or just use inline functions)
- Add `demo_allium_themes` forward declaration
- Add menu item to the main list: `{ .label = "Allium Themes" }`
- Add case in the main loop to call `demo_allium_themes()`

### Changes to `Makefile`

- Ensure cJSON.c is compiled into the demo
- Add `CAT_THEMES_DIR` env var to `run-mac-demo` target
- Add `themes-setup` prerequisite to `run-mac-demo`

---

## Runtime Theme Switching Flow

```
User selects theme in options list
  → cat_stylesheet_load_theme("AlliumBoy")
    → parse themes/Allium-Themes/Themes/AlliumBoy/stylesheet.json
    → check for stylesheet.override.json
    → resolve fonts (theme dir → fonts dir)
    → load TTF fonts
    → resolve wallpaper path
  → cat_stylesheet_apply()
    → copy colors to global cat_stylesheet
    → convert to legacy ap_theme via cat_stylesheet_to_ap_theme()
    → load wallpaper if set
    → reload fonts
  → Next render frame shows new theme
```

No restart required. The theme applies instantly on the next `ap_present()` call.

---

## Desktop Preview Support

On desktop (macOS/Linux/Windows), the themes directory is resolved in order:
1. `$CAT_THEMES_DIR` env var
2. `./themes/Allium-Themes/Themes/` (relative to CWD)
3. `./Themes/` (fallback)

For `make run-mac-demo`, the Makefile sets `CAT_THEMES_DIR` automatically to point at the git submodule.
