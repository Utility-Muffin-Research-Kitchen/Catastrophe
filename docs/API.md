# Catastrophe API Reference

Complete reference for all public functions, types, and macros in `catastrophe.h` and `catastrophe_widgets.h`.

This reference documents the current Catastrophe headers in this repository.

---

## Table of Contents

- [Core (catastrophe.h)](#core)
  - [Macros & Constants](#macros--constants)
  - [Types & Enums](#types--enums)
  - [Lifecycle](#lifecycle)
  - [Screen & Scaling](#screen--scaling)
  - [Theming](#theming)
  - [Fonts](#fonts)
  - [Input](#input)
  - [Drawing Primitives](#drawing-primitives)
  - [Box Model](#box-model)
  - [Screen Fade](#screen-fade)
  - [Footer & Status Bar](#footer--status-bar)
  - [Platform Services](#platform-services)
  - [Text Scrolling](#text-scrolling)
  - [Texture Cache](#texture-cache)
  - [Combos](#combos)
  - [Logging](#logging)
  - [Accessors](#accessors)
  - [Power](#power)
  - [CPU & Fan](#cpu--fan)
  - [Error Handling](#error-handling)
- [Widgets (catastrophe_widgets.h)](#widgets)
  - [List](#list)
  - [Options List](#options-list)
  - [Keyboard](#keyboard)
  - [Confirmation](#confirmation)
  - [Selection](#selection)
  - [Process Message](#process-message)
  - [Detail Screen](#detail-screen)
  - [Queue Viewer](#queue-viewer)
  - [Download Manager](#download-manager)
  - [Color Picker](#color-picker)
  - [Help Overlay](#help-overlay)
  - [File Picker](#file-picker)
  - [List Pane](#list-pane)
  - [Scroll View](#scroll-view)

---

## Core

### Macros & Constants

| Macro | Value | Description |
|-------|-------|-------------|
| `CAT_OK` | `0` | Success return code |
| `CAT_ERROR` | `-1` | Error return code |
| `CAT_CANCELLED` | `-2` | User cancelled (pressed back) |
| `CAT_REFERENCE_WIDTH` | `1024` | Reference width for scaling |
| `CAT_SCALE_DAMPING` | `0.75f` | Damping for screens wider than reference |
| `CAT_DS(base)` | — | Scale a pixel value by integer `device_scale` (2 or 3) |
| `CAT_S(base)` | — | Scale a pixel value from reference to actual screen |
| `CAT_PLATFORM_NAME` | `"tg5040"`, `"mlp1"`, etc. | Compile-time platform identifier |
| `CAT_PLATFORM_IS_DEVICE` | `0` or `1` | Whether building for a real device |
| `CAT_ENABLE_CURL` | build define | Enables `cat_download_manager()` implementation |
| `CAT_INPUT_DEBOUNCE` | `20` | Input debounce delay (ms) |
| `CAT_INPUT_REPEAT_DELAY` | `300` | Initial hold delay (ms) |
| `CAT_INPUT_REPEAT_RATE` | `100` | Repeat rate (ms) |
| `CAT_AXIS_DEADZONE` | `16000` / `20000` on `my355` | Joystick axis dead zone |
| `CAT_TEXT_SCROLL_SPEED` | `1` | Text scroll speed (pixels per tick) |
| `CAT_TEXT_SCROLL_PAUSE_MS` | `1000` | Pause at scroll endpoints (ms) |
| `CAT_TEXTURE_CACHE_SIZE` | `8` | LRU texture cache capacity |
| `CAT_MAX_COMBOS` | `16` | Max registered button combos |
| `CAT_MAX_LOG_LEN` | `2048` | Max log message length |

Device defaults differ by platform. MLP1 uses `/mnt/sdcard` as the default SD
root, `/mnt/sdcard/umrk-launcher` for launcher assets/themes/fonts, and
`/mnt/sdcard/.userdata/mlp1` for user state when environment overrides are not
set.

### Types & Enums

#### `cat_button`

Virtual button identifiers, unified from all input sources.

```c
CAT_BTN_NONE, CAT_BTN_UP, CAT_BTN_DOWN, CAT_BTN_LEFT, CAT_BTN_RIGHT,
CAT_BTN_A, CAT_BTN_B, CAT_BTN_X, CAT_BTN_Y,
CAT_BTN_L1, CAT_BTN_L2, CAT_BTN_R1, CAT_BTN_R2,
CAT_BTN_START, CAT_BTN_SELECT, CAT_BTN_MENU, CAT_BTN_POWER,
CAT_BTN_QUIT, CAT_BTN_STICK
```

`CAT_BTN_STICK` is the analog-stick click. On MLP1 it maps to the Loong gamepad
`BTN_THUMBL` input.

#### `cat_font_tier`

```c
CAT_FONT_EXTRA_LARGE  // 24 × device_scale (title/header)
CAT_FONT_LARGE        // 16 × device_scale (list items)
CAT_FONT_MEDIUM       // 14 × device_scale (single-char button label)
CAT_FONT_SMALL        // 12 × device_scale (hint text)
CAT_FONT_TINY         // 10 × device_scale (multi-char button label)
CAT_FONT_MICRO        //  7 × device_scale (overlay text)
```

Font sizes use integer `device_scale` (2 for MY355/TG5050/TG5040 handheld/MLP1, 3 for TG5040 brick). On screens with more logical pixels than the 320×240 baseline (MY355), an automatic font bump of 0–5 is added to each base size before scaling. See `cat_get_font_bump()`.

#### `cat_text_align`

```c
CAT_ALIGN_LEFT, CAT_ALIGN_CENTER, CAT_ALIGN_RIGHT
```

#### `cat_list_action`

```c
CAT_ACTION_SELECTED  // User pressed confirm
CAT_ACTION_BACK      // User pressed back
CAT_ACTION_TRIGGERED
CAT_ACTION_SECONDARY_TRIGGERED
CAT_ACTION_CONFIRMED
CAT_ACTION_TERTIARY_TRIGGERED
CAT_ACTION_OPTION_CHANGED   // Options list standard value changed
CAT_ACTION_CUSTOM    // Alias of CAT_ACTION_TRIGGERED (backward compatibility)
```

#### `cat_color`, `cat_draw_color`, and `ap_color`

`cat_color` is a packed `uint32_t` in Catastrophe/Allium RGBA layout:

```c
typedef uint32_t cat_color;

#define CAT_COLOR_R(c)  ((uint8_t)((c) >> 0))
#define CAT_COLOR_G(c)  ((uint8_t)((c) >> 8))
#define CAT_COLOR_B(c)  ((uint8_t)((c) >> 16))
#define CAT_COLOR_A(c)  ((uint8_t)((c) >> 24))
```

Use `cat_color_rgb()`, `cat_color_rgba()`, `cat_color_from_hex()`,
`cat_color_to_hex()`, `cat_color_blend()`, and `cat_color_to_sdl()` for packed
stylesheet colors.

`cat_draw_color` is the immediate drawing color. It has the same shape as
`SDL_Color`:

```c
typedef SDL_Color cat_draw_color;
typedef cat_draw_color ap_color;  // Legacy compatibility alias
```

New Catastrophe consumers should prefer `cat_draw_color`; `ap_color` remains as
a compatibility spelling for Apostrophe-era code and some existing widget APIs.

#### `cat_theme` and `ap_theme`

```c
typedef struct cat_theme {
    cat_draw_color highlight;         // Selected item pill background
    cat_draw_color accent;            // Footer outer pill, status bar bg
    cat_draw_color button_label;      // Text inside footer button pills
    cat_draw_color button_glyph_bg;   // Inner A/B/X/Y glyph pill background
    cat_draw_color text;              // Default text color
    cat_draw_color highlighted_text;  // Text on selected items
    cat_draw_color hint;              // Dim/help text
    cat_draw_color background;        // Screen background
    char     font_path[512];
    char     bg_image_path[512];
    int      ui_padding_x;
    int      ui_padding_y;
    float    pill_radius_ratio;
    int      pill_corner_mask;  // CAT_CORNER_* bitmask
} cat_theme;

typedef cat_theme ap_theme;  // Legacy compatibility alias
```

`cat_get_theme()` returns `cat_theme *`. Stylesheet APIs use packed `cat_color`
values and convert them into this draw-time theme with `cat_stylesheet_apply()`.
`ap_theme` remains as a compatibility alias.

#### `cat_config`

```c
typedef struct {
    const char *window_title;      // Window title (macOS only)
    const char *font_path;         // Path to .ttf, NULL = auto
    const char *bg_image_path;     // Background image, NULL = none
    const char *log_path;          // Log file, NULL = stderr only
    const char *primary_color_hex; // Override accent "#RRGGBB"
    bool        disable_background; // Set true to skip bg.png
    cat_cpu_speed cpu_speed;        // Set CPU at init; 0 = CAT_CPU_SPEED_DEFAULT (no-op)
    bool        disable_font_bump; // Set true to disable automatic font bumping
    bool        start_hidden;      // Create hidden; reveal later with cat_show_window()
} cat_config;
```

#### `cat_input_event`

```c
typedef struct {
    cat_button button;
    bool      pressed;   // true = down, false = up
    bool      repeated;  // true = generated by hold-repeat, false = fresh press
} cat_input_event;
```

#### `cat_footer_item`

```c
typedef struct {
    cat_button    button;
    const char  *label;
    bool         is_confirm;  // true = right-aligned group
    const char  *button_text; // optional display-only override for the button pill text
} cat_footer_item;
```

`button_text` only affects what is drawn in the footer. It does not change input handling, which still comes from your widget logic and button bindings.

#### `cat_footer_overflow_opts`

```c
typedef struct {
    bool      enabled;  // true = collapse overflowing footer hints into +N
    cat_button chord_a;  // First button in the hidden-actions chord
    cat_button chord_b;  // Second button in the hidden-actions chord
} cat_footer_overflow_opts;
```

#### `cat_status_bar_opts`

```c
// Clock display mode constants
#define CAT_CLOCK_AUTO  0  // Follow active stylesheet's status_bar.show_clock (default)
#define CAT_CLOCK_SHOW  1  // Always show, regardless of stylesheet
#define CAT_CLOCK_HIDE  2  // Always hide, regardless of stylesheet

typedef struct {
    int  show_clock;          // CAT_CLOCK_AUTO, CAT_CLOCK_SHOW, CAT_CLOCK_HIDE
    bool use_24h;             // 24-hour clock if true; 12-hour if false
    bool show_battery;        // Show battery icon
    bool show_battery_level;  // Show numeric "85%" beside battery
    bool show_wifi;           // Show wifi icon
    bool wifi_supplied;       // Use wifi_strength instead of self-read/preview
    int  wifi_strength;       // 0..3 when wifi_supplied is true
    bool show_volume;         // Show supplied volume icon
    int  volume_percent;      // 0..100, or -1 if unknown
    bool no_ampm;             // 12-hour mode without AM/PM suffix
    bool no_pill;             // Draw inline without pill background
    bool use_y;               // Use y_position instead of default top padding
    int  y_position;
} cat_status_bar_opts;
```

**Clock behaviour:** By default (`show_clock` left at 0 / `CAT_CLOCK_AUTO`), the clock
visibility follows the active stylesheet's `status_bar.show_clock`. Use `CAT_CLOCK_SHOW` to
force the clock visible regardless of the stylesheet, or `CAT_CLOCK_HIDE` to always suppress
it. The `use_24h` field controls 24- vs 12-hour formatting in all modes (the Allium stylesheet
does not encode a 24-hour preference).

**Wifi behaviour:** When `show_wifi` is true, the wifi icon is only shown when the current
device or desktop preview signal strength is greater than 0. When disconnected, the icon is
hidden and the pill shrinks accordingly.

Battery and wifi can be enabled independently. When status sprites are active, the clock is
hidden, and exactly one of those icons is visible, Catastrophe renders a centered square pill
for that single icon so width calculation and drawing stay aligned.

`cat_status_bar_from_env()` can populate this struct from the launcher-style
`CAT_STATUS_*` environment snapshot, and `cat_hints_enabled_from_env()` reads
`CAT_SHOW_HINTS`.

On device, battery and wifi icons are rendered from the status spritesheet at
`$CAT_STATUS_ASSETS_DIR/assets@Nx.png` when available; otherwise pills are drawn procedurally.
On desktop, the native run targets default `CAT_STATUS_ASSETS_DIR` to the generated
`res/assets/` atlas. Set `CAT_STATUS_ASSETS_DIR` to another folder containing `assets@1x.png`
through `assets@4x.png` to override it, or leave it unset outside the run targets for the
procedural fallback.

Desktop preview environment variables:

- `CAT_STATUS_ASSETS_DIR`: directory containing `assets@Nx.png` (defaults to `res/assets/` in native run targets)
- `CAT_PREVIEW_WIFI_STRENGTH`: preview wifi strength (`0`..`3`)
- `CAT_PREVIEW_BATTERY_PERCENT`: preview battery level (`0`..`100`)
- `CAT_PREVIEW_CHARGING`: preview charging state (`0` or `1`)

### Lifecycle

#### `int cat_init(cat_config *cfg)`

Initialise Catastrophe. Creates the SDL window/renderer, loads fonts, detects
screen size, applies the configured or default stylesheet, sets up input, and
starts the power button handler where supported.

On MLP1, `CAT_ENV=DEV` / `ENVIRONMENT=DEV` still forces windowed/dev mode.
Outside dev mode, MLP1 uses native display detection when available and falls
back to `960x720`, overridable via `CAT_WINDOW_WIDTH` / `CAT_WINDOW_HEIGHT`.

On my355 device builds, raw power handling listens for `KEY_POWER` from Linux input devices. A short press triggers suspend, and a long press (>= 1000ms) triggers shutdown orchestration (`/tmp/poweroff`). Suspend first attempts `echo mem > /sys/power/state` and falls back to `echo freeze > /sys/power/state` if needed. After resume, power-key events are ignored for 1000ms to avoid immediate re-suspend from wake events. On MLP1, the power handler is disabled pending a stock-service audit.

Returns `CAT_OK` on success, `CAT_ERROR` on failure.

#### `void cat_quit(void)`

Shut down completely. Frees all resources, destroys SDL context, stops background threads.

#### `void cat_show_window(void)`

Show and raise the SDL window. This pairs with `cat_config.start_hidden` for
warm-standby overlays.

#### `void cat_hide_window(void)`

Hide the SDL window. No-op if the window has not been created.

#### `void cat_activate_window(void)`

Raise and activate the app window on macOS when the process was spawned from a
background parent. No-op on other platforms.

### Screen & Scaling

#### `int cat_get_screen_width(void)` / `int cat_get_screen_height(void)`

Get the current screen dimensions in pixels.

#### `float cat_get_scale_factor(void)`

Get the current scaling factor (screen_width / reference_width, with damping).

#### `int cat_scale(int base)`

Scale a pixel value from 1024-reference space to actual screen space. Equivalent to the `CAT_S()` macro as a function call.

#### `int cat_device_scale(int base)`

Scale a pixel value by the integer device scale. This is equivalent to
`CAT_DS(base)` as a function call and is useful for pixel-perfect control
surfaces such as footer/status pills.

#### `int cat_font_size_for_resolution(int base_size)`

Scale a base font size by `device_scale` (integer multiplier: 2 for MY355/TG5050/TG5040 handheld/MLP1, 3 for TG5040 brick). Returns `base_size * device_scale`.

#### `CAT_S(base)`

Macro. Scales an integer pixel value from reference (1024px) to actual screen:

```c
int margin = CAT_S(20);  // 20px * scale factor
```

### Theming

#### `cat_theme *cat_get_theme(void)`

Get a pointer to the current theme. Modifiable.

#### `const cat_stylesheet *cat_get_stylesheet(void)`

Get the active stylesheet snapshot. The returned pointer is owned by
Catastrophe and remains valid until the next stylesheet apply or quit.

#### `cat_draw_color cat_hex_to_color(const char *hex)`

Parse a `#RRGGBB` hex string and return the corresponding `cat_draw_color`
(with alpha 255). Returns black `{0,0,0,255}` on invalid input.

#### `void cat_set_theme_color(const char *hex)`

Parse a `#RRGGBB` string and apply it as the theme accent color: `cat_set_theme_color("#FF6600");`

#### `void cat_set_tab_text_colors(cat_color inactive, cat_color selected)`

Override tab-bar inactive and selected text colors on the active stylesheet.
This lets host launchers map tab text onto their own palette roles.

#### `void cat_finalize_theme_colors(cat_theme *theme)`

Recompute derived theme colors after direct theme mutation. The selected-row
text color auto-contrasts against the highlight pill, and tab text colors track
the current palette. Passing `NULL` is a no-op.

#### Stylesheet Loading

```c
void   cat_stylesheet_init_default(cat_stylesheet *s);
int    cat_stylesheet_load_file(cat_stylesheet *s, const char *path);
int    cat_stylesheet_load_theme(cat_stylesheet *s, const char *theme_name);
int    cat_stylesheet_apply(cat_stylesheet *s);
int    cat_stylesheet_available_themes(const char ***out_names, int *out_count);
void   cat_stylesheet_free_theme_list(const char **names, int count);
int    cat_stylesheet_list_wallpapers(const char *theme_dir, const char ***out, int *count);
void   cat_stylesheet_free_string_list(const char **names, int count);
const char *cat_get_active_theme_dir(void);
const char *cat_get_active_theme_name(void);
int    cat_theme_state_load(char *out_name, size_t out_size);
int    cat_theme_state_save(const char *theme_name);
```

`cat_stylesheet_load_theme()` searches `$CAT_THEMES_DIR`, bundled `res/themes`,
the checked-out `themes/Allium-Themes/Themes` tree, and platform runtime theme
paths. On MLP1, the platform theme path is
`/mnt/sdcard/umrk-launcher/res/themes` unless `UMRK_LAUNCHER_PATH` overrides
the launcher root. `cat_stylesheet_apply()` stores the active stylesheet,
updates the runtime `cat_theme`, reloads fonts/backgrounds as needed, and makes
`cat_get_stylesheet()` reflect the new state.

Use the matching free helpers for lists returned by
`cat_stylesheet_available_themes()` and `cat_stylesheet_list_wallpapers()`.

#### `int cat_reload_background(const char *bg_path)`

Reload the background image at runtime. Destroys the current background texture and loads a new one from `bg_path`. If `bg_path` is NULL or empty, falls back to `CAT_BACKGROUND_PATH`, then `$SDCARD_PATH/bg.png` on device. Returns `CAT_OK` on success (or if no fallback path is available), `CAT_ERROR` if the image cannot be loaded.

### Fonts

#### `TTF_Font *cat_get_font(cat_font_tier tier)`

Get a pre-loaded, pre-scaled font for the given tier. Returns NULL if not loaded.

#### `int cat_get_font_bump(void)`

Get the automatic font bump computed at init (0–5). The bump is added to each base font size before `device_scale` multiplication. Computed from the logical resolution (`screen / device_scale`) relative to the 320×240 baseline (MY355). Returns 0 on MY355 and TG5040 brick, typically 2 on TG5050. Set `cat_config.disable_font_bump = true` to force 0.

#### `int cat_set_font_bump(int bump)`

Set the font bump at runtime, clamped to `0..CAT_FONT_BUMP_MAX`, and reload the
active font tiers. Returns `CAT_OK` on success or `CAT_ERROR` if reloading the
font fails.

#### `int cat_reload_fonts(const char *font_path)`

Reload all font tiers from `font_path`, or from the normal font search chain
when `font_path` is `NULL`. The MLP1 fallback chain checks launcher fonts under
`/mnt/sdcard/umrk-launcher/res/` before the system DejaVu path.

### Input

#### `bool cat_poll_input(cat_input_event *event)`

Poll for the next input event. Returns `true` if an event was available.

Handles SDL event processing internally: keyboard events, raw joystick
buttons/axes/hats (TrimUI and MLP1), SDL GameController buttons/axes (desktop +
recognised gamepads), platform-specific scancodes (my355), power/quit events,
and directional repeat. MLP1 uses its own Loong gamepad raw button mapping and
maps the stick click to `CAT_BTN_STICK`.

#### `void cat_flip_face_buttons(bool flip)`

Swap A/B and X/Y button mappings. When `flip` is true, hardware A reports as `CAT_BTN_B` and vice versa (likewise X/Y). Useful for platforms where the firmware already swaps face buttons.

#### `const char *cat_button_name(cat_button btn)`

Get the display name string for a virtual button (e.g. `"A"`, `"UP"`, `"START"`). Returns `"Unknown"` for out-of-range values.

#### `void cat_set_input_delay(uint32_t ms)`

Set input debounce delay in milliseconds.

#### `void cat_set_input_repeat(uint32_t delay_ms, uint32_t rate_ms)`

Configure directional hold repeat timing for D-pad/arrow/button-mapped directions.

### Drawing Primitives

#### `cat_dir`

```c
typedef enum {
    CAT_DIR_LEFT = 0,
    CAT_DIR_RIGHT,
    CAT_DIR_UP,
    CAT_DIR_DOWN,
} cat_dir;
```

#### `CAT_CORNER_*`

```c
CAT_CORNER_TL, CAT_CORNER_TR, CAT_CORNER_BL, CAT_CORNER_BR, CAT_CORNER_ALL
```

Corner-selection bitmask used by per-corner rounded rectangles, rounded image
clipping, and themed pill corner masks.

#### `void cat_clear_screen(void)`

Clear the screen to the theme background color (or render bg image if configured).

#### `void cat_present(void)`

Present the rendered frame. Call after all drawing for the frame. When the renderer does not have vsync (e.g. software fallback), this function automatically throttles to ~60 fps via `SDL_Delay` to prevent CPU hot-spinning.

#### `void cat_request_frame(void)`

Request another frame immediately. Use this for active animations or callbacks that need the next frame to render without waiting for input.

#### `void cat_request_frame_in(uint32_t ms)`

Schedule a redraw in the future while the UI is otherwise idle. This is useful for widgets that need periodic refresh without adding their own polling loop. For example, an `cat_list()` footer callback can call `cat_request_frame_in(100)` to keep a live footer label in sync with external state.

#### `void cat_draw_background(void)`

Draw the background image/color (called automatically by `cat_clear_screen`).

#### `int cat_draw_text(TTF_Font *font, const char *text, int x, int y, cat_draw_color color)`

Render text. Returns the rendered width in pixels.

#### `int cat_draw_text_clipped(TTF_Font *font, const char *text, int x, int y, cat_draw_color color, int max_w)`

Render text clipped to a maximum width. Performs a hard pixel clip with no truncation indicator.

#### `int cat_draw_text_ellipsized(TTF_Font *font, const char *text, int x, int y, cat_draw_color color, int max_w)`

Render text truncated with "..." if it exceeds `max_w`. If the text fits, it is rendered normally. Uses a binary search to find the longest prefix that fits alongside the ellipsis, respecting UTF-8 character boundaries. Returns the rendered width in pixels.

#### `int cat_draw_text_wrapped(TTF_Font *font, const char *text, int x, int y, int max_w, cat_draw_color color, cat_text_align align)`

Render multi-line word-wrapped text and return the rendered height.

#### `int cat_measure_text(TTF_Font *font, const char *text)`

Measure text width without rendering.

#### `int cat_measure_text_ellipsized(TTF_Font *font, const char *text, int max_w)`

Measure the width the text would occupy if ellipsized to fit `max_w`, without rendering. Returns the full text width when the text already fits. Returns `0` when `max_w <= 0`. If the ellipsis itself is as wide as or wider than `max_w`, returns `max_w` to match the clipped fallback behavior used by `cat_draw_text_ellipsized()`.

#### `int cat_measure_wrapped_text_height(TTF_Font *font, const char *text, int max_w)`

Measure the total height in pixels that word-wrapped text would occupy at the given `max_w` constraint, without rendering. Useful for pre-calculating layout sizes.

#### `void cat_draw_rect(int x, int y, int w, int h, cat_draw_color color)`

Draw a filled rectangle.

#### `void cat_draw_pill(int x, int y, int w, int h, cat_draw_color color)`

Draw a pill shape (fully rounded rectangle where corner radius = h/2). Uses the pre-rendered pill sprite when status assets are loaded; otherwise falls back to procedural drawing.

#### `void cat_draw_rounded_rect(int x, int y, int w, int h, int radius, cat_draw_color color)`

Draw a filled rounded rectangle with arbitrary corner radius using scanline quarter-circle fill with sub-pixel anti-aliasing (no SDL2_gfx dependency).

#### `void cat_draw_rounded_rect_ex(int x, int y, int w, int h, int radius, unsigned corners, cat_draw_color color)`

Draw a filled rounded rectangle while rounding only the corners in the
`CAT_CORNER_TL`, `CAT_CORNER_TR`, `CAT_CORNER_BL`, `CAT_CORNER_BR` bitmask.
`CAT_CORNER_ALL` rounds all four corners.

#### `void cat_draw_circle(int cx, int cy, int r, cat_draw_color color)`

Draw a filled circle at center (cx, cy) with radius r.

#### `void cat_draw_star(int cx, int cy, int outer_r, cat_draw_color color)`

Draw a filled five-point star.

#### `void cat_draw_triangle(int x, int y, int w, int h, cat_dir dir, cat_draw_color color)`

Draw a filled triangle inside the given rectangle, pointing `CAT_DIR_LEFT`,
`CAT_DIR_RIGHT`, `CAT_DIR_UP`, or `CAT_DIR_DOWN`.

#### `void cat_draw_image(SDL_Texture *tex, int x, int y, int w, int h)`

Draw a loaded SDL texture at the given position/size.

#### `void cat_draw_image_rounded_ex(SDL_Texture *tex, int x, int y, int w, int h, int radius, unsigned corners)`

Draw a texture clipped to a rounded rectangle using the same `CAT_CORNER_*`
mask as `cat_draw_rounded_rect_ex()`. Falls back to a plain image draw when a
rounded render target is unavailable.

#### `SDL_Texture *cat_load_image(const char *path)`

Load an image from disk (PNG, JPG) and return an SDL_Texture. Returns NULL on failure.

#### `void cat_draw_scrollbar(int x, int y, int h, int visible, int total, int offset)`

Draw a vertical scrollbar track and thumb. The thumb size and position are computed from visible/total/offset. Does nothing if total <= visible.

#### `void cat_draw_progress_bar(int x, int y, int w, int h, float progress, cat_draw_color fg, cat_draw_color bg)`

Draw a rounded progress bar. `progress` is clamped to 0.0–1.0.

#### `void cat_draw_tab_bar(const char *const *labels, int count, int active_index)`

Draw a full-width tab bar using the active stylesheet tab colors. When tabs do
not fit, the bar windows around the active tab and draws triangle affordances
for hidden tabs.

#### `int cat_get_tab_bar_height(void)`

Return the tab bar height in pixels.

#### `void cat_set_tab_bar_reserved_right(int px)`

Reserve right-side tab-bar width, usually for an inline status bar.

#### `void cat_draw_textured_parallelogram(SDL_Texture *tex, const SDL_FPoint quad[4], uint8_t alpha)`

Draw a texture into an arbitrary four-corner quadrilateral using
`SDL_RenderGeometry` when available.

#### `SDL_Rect cat_get_content_rect(bool has_title, bool has_footer, bool has_status_bar)`

Calculate the usable content area of the screen, accounting for title bar, footer, and status bar. Returns an `SDL_Rect` with `x`, `y`, `w`, `h` in pixels. Use this to position widget content within the available space.

#### `void cat_draw_screen_title(const char *title, cat_status_bar_opts *status_bar)`

Draw a title at the top-left of the screen. If `status_bar` is non-NULL, the status bar is also drawn at the top-right and the title is clipped to avoid overlapping it.

#### `void cat_draw_screen_title_centered(const char *title, cat_status_bar_opts *status_bar)`

Draw a title centered horizontally in the available space. Behaves like `cat_draw_screen_title` but centers the text instead of left-aligning it. Uses the same progressive font-tier fallback (extra-large → large → medium) and clips to avoid overlapping the status bar.

### Box Model

Small geometry helpers for building margin-free layouts. `cat_box` stores a
rectangle plus internal padding; helpers carve strips and split columns without
doing any drawing.

```c
typedef struct {
    int x, y, w, h;
    int pad_t, pad_r, pad_b, pad_l;
} cat_box;

SDL_Rect cat_box_content(const cat_box *b);
cat_box  cat_box_carve_top(cat_box *b, int height);
cat_box  cat_box_carve_bottom(cat_box *b, int height);
void     cat_box_split_cols(const cat_box *b, int left_w, int gutter,
                            cat_box *left, cat_box *right);
```

All dimensions are final pixels. Callers should pre-scale constants with
`CAT_S()` or `CAT_DS()`.

### Screen Fade

Fade-in/fade-out overlay for scene transitions.

#### `cat_fade`

```c
typedef struct {
    uint32_t start_ms;     // SDL_GetTicks() value when fade began
    int      duration_ms;  // Total duration of the fade
    bool     fade_in;      // true = black->transparent, false = transparent->black
    bool     active;       // false = not animating
} cat_fade;
```

#### `void cat_fade_begin_in(cat_fade *f, int duration_ms)`

Start a fade-in: the screen transitions from fully black to transparent over `duration_ms` milliseconds.

#### `void cat_fade_begin_out(cat_fade *f, int duration_ms)`

Start a fade-out: the screen transitions from transparent to fully black over `duration_ms` milliseconds.

#### `bool cat_fade_draw(cat_fade *f)`

Draw the fade overlay. Call after drawing your scene and before `cat_present()`. Returns `true` while the fade is still animating, `false` when complete.

### Footer & Status Bar

#### `void cat_draw_footer(cat_footer_item *items, int count)`

Draw the footer bar at the bottom of the screen with button hints.

Non-confirm items render in one continuous outer pill on the left; confirm items render in one continuous outer pill on the right. Inside each outer pill, every item shows an inner button pill (letter/symbol) followed by a text label. Sizing: `PILL_SIZE` (30) outer height, `BUTTON_SIZE` (20) inner circles, all scaled by `device_scale`. Font tiers: `CAT_FONT_MEDIUM` (14 base) for single-char button labels, `CAT_FONT_TINY` (10 base) for multi-char labels, `CAT_FONT_SMALL` (12 base) for hint text.

When footer overflow handling is enabled and the hints do not fit on one row, Catastrophe keeps the footer on a single line, preserves the right-aligned confirm group, shows a compact `+N` marker for hidden hints, and exposes a hidden-actions overlay that can be opened by calling `cat_show_footer_overflow()` (commonly bound to the Menu button) or by configuring a chord. Hidden footer items remain normal actions; only their on-screen hints are collapsed.

#### `int cat_get_footer_height(void)`

Get the footer height in pixels (scaled).

#### `void cat_set_footer_overflow_opts(const cat_footer_overflow_opts *opts)`

Set the global footer overflow behaviour. Pass `NULL` to restore the default configuration (`enabled = true`, `chord_a = CAT_BTN_NONE`, `chord_b = CAT_BTN_NONE`). To enable a button chord shortcut, set `chord_a` and `chord_b` to the desired buttons (e.g. `CAT_BTN_L1` and `CAT_BTN_R1`).

#### `void cat_get_footer_overflow_opts(cat_footer_overflow_opts *out)`

Read the current global footer overflow configuration.

#### `void cat_show_footer_overflow(void)`

Programmatically open the hidden-actions overlay when hidden footer items exist. This is useful in screens with custom input loops (i.e. not using `cat_list`) that need to support the Menu button for footer overflow. The call is a no-op when there are no hidden items. Note: hidden item state is computed by `cat_draw_footer()`, so this function must be called after at least one footer draw pass in the current frame.

#### `void cat_draw_status_bar(cat_status_bar_opts *opts)`

Draw a status bar pill at the top-right of the screen. Shows clock, battery, and wifi status. Battery and wifi icons come from the status spritesheet whenever it is loaded. Position uses the screen-edge `PADDING` offset (10px unscaled). When sprites are active and only one battery/wifi icon is visible with the clock hidden, the pill collapses to a centered square icon pill.

#### `int cat_get_status_bar_width(cat_status_bar_opts *opts)`

Calculate the pixel width of the status bar pill, including padding. Use this to clip long title text to avoid overlap. The result matches `cat_draw_status_bar()` for all clock/battery/wifi combinations, including square single-icon sprite pills.

#### `int cat_get_status_bar_height(void)`

Get the height of the status bar in pixels (scaled).

#### `bool cat_status_bar_from_env(cat_status_bar_opts *out)`

Populate `out` from launcher-style `CAT_STATUS_*` environment variables and
return whether any status element should be drawn. Catastrophe still self-reads
live Wi-Fi/battery/clock state unless the caller supplies explicit values such
as `wifi_supplied` / `wifi_strength`.

#### `bool cat_hints_enabled_from_env(void)`

Read `CAT_SHOW_HINTS`. Unset defaults to `true`; `"0"` disables footer/hint
display.

### Platform Services

#### `cat_platform_services`

Install optional platform callbacks for hardware state and actions that
Catastrophe would otherwise probe directly. Apps that already have a launcher
or daemon platform layer should supply these callbacks after `cat_init()`.

```c
typedef struct cat_platform_services {
    void *userdata;
    int (*wifi_strength)(void *userdata);      /* 0..3, negative if unknown */
    int (*battery_percent)(void *userdata);    /* 0..100, negative if unknown */
    int (*charging_state)(void *userdata);     /* 0/1, negative if unknown */
    int (*set_fan_mode)(void *userdata, cat_fan_mode mode);
    cat_fan_mode (*get_fan_mode)(void *userdata);
    int (*set_fan_speed)(void *userdata, int percent);
    int (*get_fan_speed)(void *userdata);
    int (*power_suspend)(void *userdata);
    int (*power_shutdown)(void *userdata);
} cat_platform_services;
```

Callbacks are optional. When a callback is present and returns a valid value,
Catastrophe uses it first. Missing callbacks, negative state values, or
unsupported action results fall back to the existing compile-time guarded
behavior for standalone examples and direct device ports.

#### `void cat_set_platform_services(const cat_platform_services *services)`

Copy the supplied service table into Catastrophe. Pass `NULL` to clear all
callbacks. Installing or clearing services resets the cached Wi-Fi status so the
next status-bar draw consults the current provider.

### Text Scrolling

#### `void cat_text_scroll_init(cat_text_scroll *s)`

Initialise a text scroll state. Resets offset, direction, and pause timer.

#### `void cat_text_scroll_reset(cat_text_scroll *scroll)`

Reset a text scroll state to the beginning.

#### `void cat_text_scroll_update(cat_text_scroll *scroll, int text_w, int visible_w, uint32_t dt_ms)`

Advance the ping-pong scroll animation. Call once per frame, passing the frame delta time in milliseconds.

#### `cat_marquee` and `cat_draw_text_marquee`

```c
typedef enum {
    CAT_MARQUEE_LOOP = 0,
    CAT_MARQUEE_PINGPONG
} cat_marquee_mode;

typedef struct {
    uint32_t         elapsed_ms;
    cat_marquee_mode mode;
} cat_marquee;

bool cat_draw_text_marquee(TTF_Font *font, const char *text, int x, int y,
                           cat_draw_color color, int visible_w,
                           cat_marquee *m, uint32_t dt_ms);
```

Draw text clipped to `visible_w`. If it fits, the function draws normally and
returns `false`. If it overflows, it scrolls after an initial pause and returns
`true` while another frame is needed. `CAT_MARQUEE_LOOP` wraps continuously;
`CAT_MARQUEE_PINGPONG` scrolls to the end, pauses, and scrolls back. The clip
rect intersects any existing clip, so it composes inside `cat_draw_scroll_view()`.

### Texture Cache

#### `SDL_Texture *cat_cache_get(const char *key, int *w, int *h)`

Look up a texture in the LRU cache by key string. Returns NULL on miss.

#### `void cat_cache_put(const char *key, SDL_Texture *tex, int w, int h)`

Store a texture in the LRU cache. Evicts least-recently-used entries when full.

#### `void cat_cache_clear(void)`

Flush the entire texture cache and free all textures.

### Combos

The combo system detects two kinds of multi-button input:

- **Chords** — multiple buttons pressed simultaneously (e.g. L1+R1 together)
- **Sequences** — buttons pressed in a specific order (e.g. Up, Up, Down, Down)

Combos do **not** suppress individual button events — `cat_poll_input()` still returns every press and release as normal.

#### Types

##### `cat_combo_type`

```c
typedef enum { CAT_COMBO_CHORD, CAT_COMBO_SEQUENCE } cat_combo_type;
```

Distinguishes the kind of combo in a `cat_combo_event`. Set to `CAT_COMBO_CHORD` for chords and `CAT_COMBO_SEQUENCE` for sequences.

##### `cat_combo_callback`

```c
typedef void (*cat_combo_callback)(const char *id, cat_combo_type type, void *userdata);
```

Callback signature used by the `_ex` registration variants. Called synchronously at the moment the combo triggers or a chord releases.

##### `cat_combo_event`

```c
typedef struct {
    const char    *id;        /* identifier passed to cat_register_chord/sequence */
    bool           triggered; /* true = fired, false = chord released */
    cat_combo_type  type;      /* CAT_COMBO_CHORD or CAT_COMBO_SEQUENCE */
} cat_combo_event;
```

#### `int cat_register_chord(const char *id, cat_button *buttons, int count, uint32_t window_ms)`

Register a simultaneous button chord. All `count` buttons must be held at the same time, with
the time between the earliest and latest press no greater than `window_ms` (default 100ms if 0).
Returns `CAT_ERROR` when `id` is NULL/empty, `buttons` is NULL, or `count` is outside `1..8`.

When the chord triggers, a combo event is queued with `triggered = true`. When any button in the
chord is released, a second event is queued with `triggered = false`. A chord will not re-trigger
until all buttons have been released and pressed again.

#### `int cat_register_chord_ex(const char *id, cat_button *buttons, int count, uint32_t window_ms, cat_combo_callback on_trigger, cat_combo_callback on_release, void *userdata)`

Like `cat_register_chord`, but also registers optional callbacks. `on_trigger` fires when the chord fires; `on_release` fires when any button is released. Either may be `NULL`. Callbacks fire synchronously before the event is enqueued, so polling still works alongside them — both are additive.

#### `int cat_register_sequence(const char *id, cat_button *buttons, int count, uint32_t timeout_ms, bool strict)`

Register an ordered button sequence. Each button must be pressed within `timeout_ms` of the
previous one (default 500ms if 0). When `strict` is true, any extraneous button press during
the sequence window causes the match to fail.
Returns `CAT_ERROR` when `id` is NULL/empty, `buttons` is NULL, or `count` is outside `1..8`.

Sequences only fire `triggered = true` events (no release event).

#### `int cat_register_sequence_ex(const char *id, cat_button *buttons, int count, uint32_t timeout_ms, bool strict, cat_combo_callback on_trigger, void *userdata)`

Like `cat_register_sequence`, but also registers an optional `on_trigger` callback. There is no `on_release` parameter for sequences — they are one-shot events with no release phase.

#### `void cat_unregister_combo(const char *id)`

Deactivate a previously registered combo by its `id`. The combo slot is marked inactive but not removed.

#### `void cat_clear_combos(void)`

Remove all registered combos and reset internal detection state.

#### `bool cat_poll_combo(cat_combo_event *event)`

Poll the combo event queue. Returns `true` if a combo event is available, filling `event` with:
- `id` — the string identifier passed to `cat_register_chord()` or `cat_register_sequence()`
- `triggered` — `true` when the combo fires, `false` when a chord is released
- `type` — `CAT_COMBO_CHORD` or `CAT_COMBO_SEQUENCE`

#### Limits

| Limit | Value |
|-------|-------|
| Max registered combos | 16 (`CAT_MAX_COMBOS`) |
| Max buttons per combo | 8 |
| Combo event queue size | 16 |
| Sequence detection buffer | 20 recent presses |

#### Example: Polling

```c
/* Register combos */
cat_button shoulders[] = { CAT_BTN_L1, CAT_BTN_R1 };
cat_register_chord("shoulders", shoulders, 2, 150);

cat_button uudd[] = { CAT_BTN_UP, CAT_BTN_UP, CAT_BTN_DOWN, CAT_BTN_DOWN };
cat_register_sequence("uudd", uudd, 4, 500, false);

/* In your main loop: */
cat_combo_event combo;
while (cat_poll_combo(&combo)) {
    const char *kind = (combo.type == CAT_COMBO_CHORD) ? "chord" : "seq";
    if (combo.triggered)
        printf("Triggered [%s]: %s\n", kind, combo.id);
    else
        printf("Released  [%s]: %s\n", kind, combo.id);
}
```

#### Example: Callbacks (_ex variants)

```c
void on_trigger(const char *id, cat_combo_type type, void *userdata) {
    printf("Triggered: %s\n", id);
}
void on_release(const char *id, cat_combo_type type, void *userdata) {
    printf("Released: %s\n", id);
}

/* Chord with trigger + release callbacks */
cat_button shoulders[] = { CAT_BTN_L1, CAT_BTN_R1 };
cat_register_chord_ex("shoulders", shoulders, 2, 150, on_trigger, on_release, NULL);

/* Sequence with trigger callback only (no release phase) */
cat_button uudd[] = { CAT_BTN_UP, CAT_BTN_UP, CAT_BTN_DOWN, CAT_BTN_DOWN };
cat_register_sequence_ex("uudd", uudd, 4, 500, false, on_trigger, NULL);

/* cat_poll_combo() still works — callbacks and polling are both active */
```

See `examples/combo/main.c` for a complete working example with both modes.

### Logging

#### `void cat_log(const char *fmt, ...)`

Printf-style logging. Writes to stderr and optionally to the configured log file.

#### `void cat_set_log_path(const char *path)`

Set the active log file path. Passing `NULL` disables file logging and keeps stderr logging only.

#### `const char *cat_resolve_log_path(const char *app_name)`

Resolve a standard log path for an app binary name:
- `LOGS_PATH/<app_name>.txt`
- `SHARED_USERDATA_PATH/logs/<app_name>.txt`
- `HOME/.userdata/logs/<app_name>.txt`

Returns `NULL` if no suitable base path is available.

### Accessors

#### `SDL_Renderer *cat_get_renderer(void)`

Get the underlying SDL renderer.

#### `SDL_Window *cat_get_window(void)`

Get the underlying SDL window.

#### `void cat_show_window(void)`

Show and raise the SDL window. Primarily useful with `cat_config.start_hidden`.

#### `void cat_hide_window(void)`

Hide the SDL window. No-op if the window has not been created.

#### `void cat_activate_window(void)`

Raise and activate the application window on macOS when started from a
background process. No-op on other platforms.

### Power

#### `void cat_set_power_handler(bool enabled)`

Enable or disable the background power button handler. On supported device
ports, this listens for `KEY_POWER` from Linux input devices: a short press
triggers suspend, and a long press (>= 1s) triggers shutdown. Enabled
automatically by `cat_init()` on supported device builds. On MLP1, the handler
is disabled pending a stock-service audit; provide power callbacks through
`cat_platform_services` if a host daemon should own those actions.

### CPU & Fan

Control CPU frequency and fan speed on the supported handheld hardware. All functions are no-ops (or
return -1) on desktop builds. Fan functions have no effect on MY355 and TG5040, which have no
fan hardware.

#### `cat_cpu_speed` (enum)

```c
typedef enum {
    CAT_CPU_SPEED_DEFAULT     = 0, /* do not change (zero-init default) */
    CAT_CPU_SPEED_MENU,            /* light UI work  */
    CAT_CPU_SPEED_POWERSAVE,       /* battery saving  */
    CAT_CPU_SPEED_NORMAL,          /* standard pak speed — recommended default */
    CAT_CPU_SPEED_PERFORMANCE,     /* maximum speed  */
} cat_cpu_speed;
```

Approximate frequencies per preset and platform:

| Preset | MY355 | TG5040 | TG5050 (big core) |
|--------|-------|--------|-------------------|
| `CAT_CPU_SPEED_MENU` | 600 MHz | 600 MHz | 672 MHz |
| `CAT_CPU_SPEED_POWERSAVE` | 1200 MHz | 1200 MHz | 1200 MHz |
| `CAT_CPU_SPEED_NORMAL` | 1608 MHz | 1608 MHz | 1680 MHz |
| `CAT_CPU_SPEED_PERFORMANCE` | 2000 MHz | 2000 MHz | 2160 MHz |

`CAT_CPU_SPEED_NORMAL` is the recommended default for apps — it matches what the device's `launch.sh`
sets before handing control to a pak binary.

**`cat_config.cpu_speed`** — set this field in `cat_config` to apply a speed preset during
`cat_init()`. Zero (default struct value) leaves the CPU unchanged.

```c
cat_config cfg = {
    .window_title = "My Pak",
    /* Catastrophe loads the default stylesheet automatically */
    .cpu_speed    = CAT_CPU_SPEED_NORMAL,  /* set at init */
};
cat_init(&cfg);
```

#### `int cat_set_cpu_speed(cat_cpu_speed speed)`

Set the CPU to a named preset. Internally writes `userspace` to the cpufreq governor and then
writes the platform-specific frequency to `scaling_setspeed`. Returns `CAT_OK` on success,
`CAT_ERROR` if the sysfs write fails. No-op (returns `CAT_OK`) on desktop builds.

#### `int cat_get_cpu_speed_mhz(void)`

Read the current CPU frequency in MHz. Returns `-1` on error or desktop builds.

#### `int cat_get_cpu_temp_celsius(void)`

Read the CPU temperature in °C from `/sys/devices/virtual/thermal/thermal_zone0/temp`.
Returns `-1` on error or desktop builds.

`my355` is a special case: the shipped launcher boots at `1992 MHz`, but the platform layer
uses `2000 MHz` for the named performance preset.
Catastrophe follows `platform.c` for the preset API.

#### `cat_fan_mode` (enum)

```c
typedef enum {
    CAT_FAN_MODE_UNSUPPORTED = -1,
    CAT_FAN_MODE_MANUAL = 0,
    CAT_FAN_MODE_AUTO_QUIET,
    CAT_FAN_MODE_AUTO_NORMAL,
    CAT_FAN_MODE_AUTO_PERFORMANCE,
} cat_fan_mode;
```

`CAT_FAN_MODE_AUTO_*` mirrors the TG5050 `fancontrol` helper modes. On platforms without fan
hardware, `cat_get_fan_mode()` returns `CAT_FAN_MODE_UNSUPPORTED`.

#### `int cat_set_fan_mode(cat_fan_mode mode)`

Set the TG5050 fan mode. `CAT_FAN_MODE_MANUAL` stops any active `fancontrol` daemon without
changing the current raw fan state. The auto modes launch the `CAT_FAN_HELPER_PATH`
override or `$SYSTEM_PATH/bin/fancontrol` with `quiet`, `normal`, or `performance`.
Returns `CAT_ERROR` if an auto mode is requested and the helper is unavailable. No-op (returns
`CAT_OK`) on non-TG5050 platforms and desktop builds.

#### `cat_fan_mode cat_get_fan_mode(void)`

Read the current TG5050 fan mode. If a `fancontrol` daemon is running, this reports the matching
auto mode. Otherwise, it reports `CAT_FAN_MODE_MANUAL` when the fan sysfs node is readable, or
`CAT_FAN_MODE_UNSUPPORTED` if the mode cannot be determined.

#### `int cat_set_fan_speed(int percent)`

Set the fan to a fixed 0–100 percentage. On TG5050, Catastrophe first stops any active
`fancontrol` daemon, then prefers invoking the system helper with that percentage; if the helper is
missing it falls back to a direct write to `cooling_device0/cur_state`. The direct sysfs fallback
uses standard rounding: `(31 * percent + 50) / 100`. Pass `-1` to leave the current
speed unchanged. Only has effect on TG5050; no-op (returns `CAT_OK`) on all other platforms and
desktop builds.

#### `int cat_get_fan_speed(void)`

Read the current fan speed as a 0–100 percentage. Returns `0` on non-TG5050 platforms,
`-1` if the sysfs read fails.

#### Platform notes

| Platform | CPU sysfs | Fan |
|----------|-----------|-----|
| MY355 (Miyoo Flip) | `cpufreq/policy0/scaling_setspeed` | None |
| TG5040 (Trimui Brick) | `cpu0/cpufreq/scaling_setspeed` | None |
| TG5050 (Trimui Smart Pro S) | `cpu4/cpufreq/scaling_setspeed` (big core) | `cooling_device0/cur_state` (0–31), plus `fancontrol` helper auto curves |
| MLP1 | Host/platform service expected | Host/platform service expected |
| Desktop | No-op | No-op |

#### Example

```c
/* Set CPU at init */
cat_config cfg = { .window_title = "App",
                  .cpu_speed = CAT_CPU_SPEED_NORMAL };
cat_init(&cfg);

/* Change speed at runtime */
cat_set_cpu_speed(CAT_CPU_SPEED_PERFORMANCE);

int mhz  = cat_get_cpu_speed_mhz();   /* e.g. 1680 on TG5050 */
int temp = cat_get_cpu_temp_celsius(); /* e.g. 42   */

/* Fan (TG5050 only) */
cat_set_fan_mode(CAT_FAN_MODE_AUTO_NORMAL);
cat_set_fan_speed(50);          /* 50% */
cat_fan_mode mode = cat_get_fan_mode();
int fan = cat_get_fan_speed();  /* 0–100 */
```

See `examples/perf/main.c` for a live-readout demo with preset picker.

### Error Handling

#### `const char *cat_get_error(void)`

Get the last error message set by Catastrophe. Returns an empty string if no error.

#### `bool cat_is_cancelled(int result)`

Convenience check: returns `true` if `result == CAT_CANCELLED`.

---

## Widgets

All widget functions return `CAT_OK` on successful interaction, `CAT_CANCELLED` when the user presses back, or `CAT_ERROR` on failure.

### List

```c
cat_list_opts cat_list_default_opts(const char *title, cat_list_item *items, int count);
int          cat_list(cat_list_opts *opts, cat_list_result *result);
```

Scrollable list with:
- Single selection (A button)
- Multi-select mode (checkboxes)
- Reorder mode (toggle reorder button + D-Pad)
- Image thumbnails
- Optional hidden scrollbar (`hide_scrollbar`)
- Optional live footer updates (`footer_update`)
- Text overflow scrolling
- Help overlay (Menu)
- Explicit action bindings (`action_button`, `secondary_action_button`, `confirm_button`, `tertiary_action_button`)

Footer hints are visual only. Behavior is driven by the action button fields in `cat_list_opts`.

**`cat_list_item`**:
```c
typedef struct {
    const char  *label;
    const char  *metadata;         // Hidden item data (e.g. path), not rendered
    SDL_Texture *image;            // Optional thumbnail (shown when show_images = true)
    bool         selected;         // For multi-select
    SDL_Texture *background_image; // Optional fullscreen preview for the focused item
    const char  *trailing_text;    // Optional right-aligned visible hint text
} cat_list_item;
```

Use `metadata` for hidden payloads such as paths or IDs. Use `trailing_text` for right-aligned UI text shown in the row. The `CAT_LIST_ITEM` / `CAT_LIST_ITEM_BG` helper macros still initialize only `label` and `metadata`; use designated initializers when you want a visible trailing hint.

**`cat_list_opts`** (action-related fields):
```c
typedef struct cat_list_opts cat_list_opts;
typedef void (*cat_list_footer_update_fn)(cat_list_opts *opts, int cursor, void *userdata);

struct cat_list_opts {
    ...
    cat_button reorder_button;
    cat_button action_button;
    cat_button secondary_action_button;
    cat_button confirm_button;
    cat_button tertiary_action_button;
    bool      hide_scrollbar;      // Hide scrollbar while keeping scrolling behavior unchanged
    int       initial_index;
    int       visible_start_index;
    TTF_Font *item_font;           // Override list item text (default: CAT_FONT_LARGE)
    cat_list_footer_update_fn footer_update; // Optional live footer updater
    void     *footer_update_userdata;
};
```

`item_font` overrides the font used to render list item labels and trailing hints. When `NULL` (zero-init default), the widget uses `cat_get_font(CAT_FONT_LARGE)`. Pass a font obtained from `cat_get_font()` or a custom-loaded `TTF_Font` to override.

`hide_scrollbar` suppresses the scrollbar gutter and thumb without changing list navigation, cursor behavior, or visible-item paging.

`footer_update` runs once per list loop after cursor and scroll position are finalized, but before the footer is drawn. It may inspect `opts->items[cursor]` and update existing footer text such as `label` or `button_text`. Keep the callback cheap and avoid mutating layout-driving state such as `item_count`, `footer_count`, or replacing the footer/items arrays while the list is open. If the footer needs to refresh without input, call `cat_request_frame_in(ms)` or `cat_request_frame()` from inside the callback.

When `trailing_text` is set, `cat_list()` renders it right-aligned using `theme->hint`. The hint is skipped for that row if reserving space for it would leave less than `CAT_S(96)` for the main label.

D-Pad Left/Right skip forward/backward by one page (`max_visible` items) in `cat_list`. L1/R1 jump between alphabetical letter groups (items should be pre-sorted for best results). Both require no configuration but are disabled while reorder mode is active.

The help overlay is triggered by the Menu button. When `help_text` is set, Menu shows the help overlay first; if hidden footer items also exist, the footer overflow overlay follows. When no `help_text` is set, Menu opens the footer overflow directly.

**`cat_list_result`**:
```c
typedef struct {
    int             selected_index;
    cat_list_action  action;
    cat_list_item   *items;
    int             item_count;
    int             visible_start_index;
} cat_list_result;
```

### Options List

```c
int cat_options_list(cat_options_list_opts *opts, cat_options_list_result *result);
```

Settings-style list where each row has a label and a configurable value area:

| Type | Behavior |
|------|----------|
| `CAT_OPT_STANDARD` | Left/Right cycles through predefined values; if `confirm_button == CAT_BTN_A`, A confirms instead of cycling |
| `CAT_OPT_KEYBOARD` | A opens keyboard for text input; if `confirm_button == CAT_BTN_A`, confirming text also confirms the list |
| `CAT_OPT_CLICKABLE` | A triggers a navigation/action callback |
| `CAT_OPT_COLOR_PICKER` | A opens the color picker; if `confirm_button == CAT_BTN_A`, picking a color also confirms the list |

Action buttons are explicit in `cat_options_list_opts` (`action_button`, `secondary_action_button`, `confirm_button`), and footer hints remain visual-only.
When `return_on_option_change` is enabled, a successful Left/Right cycle on a standard item exits immediately with `CAT_ACTION_OPTION_CHANGED` after updating `selected_option`. If `confirm_button != CAT_BTN_A`, pressing A on a standard item also cycles forward and returns `CAT_ACTION_OPTION_CHANGED`. This keeps option-change exits distinct from `action_button`, which still reports `CAT_ACTION_TRIGGERED`.
When `confirm_button` is set to `CAT_BTN_A`, A takes on a "confirm and exit" role across all item types:
- **Standard items**: A confirms immediately (use Left/Right to change values).
- **Keyboard/Color picker items**: A opens the sub-editor; confirming inside it also exits the options list with `CAT_ACTION_CONFIRMED`. Cancelling the sub-editor returns to the list.
- **Clickable items**: Unchanged — A exits with `CAT_ACTION_SELECTED`.
When option storage is malformed (`options == NULL` or out-of-range `selected_option`), Catastrophe safely clamps/ignores the invalid value instead of dereferencing invalid memory.
Long labels and option values are ellipsized as needed to keep the left label and right value area from overlapping on narrow screens. `CAT_OPT_CLICKABLE` rows render their trailing `>` in both focused and unfocused states.

**`cat_options_list_opts`** (action/scroll fields):
```c
typedef struct {
    ...
    int       initial_selected_index;
    int       visible_start_index;
    cat_button action_button;
    cat_button secondary_action_button;
    cat_button confirm_button;
    bool      return_on_option_change;
    TTF_Font *label_font;          // Override option label text (default: CAT_FONT_LARGE)
    TTF_Font *value_font;          // Override option value text (default: CAT_FONT_TINY)
} cat_options_list_opts;
```

`return_on_option_change` makes standard-option changes return immediately with `CAT_ACTION_OPTION_CHANGED` after the value updates. Leave it as `false` (the zero-init default) to keep the existing in-place behavior. `label_font` overrides the font used for option labels; `value_font` overrides the font used for option values. When `NULL` (zero-init default), the widget uses `cat_get_font(CAT_FONT_LARGE)` and `cat_get_font(CAT_FONT_TINY)` respectively. Pass a font obtained from `cat_get_font()` or a custom-loaded `TTF_Font` to override.

**`cat_options_list_result`**:
```c
typedef struct {
    int            focused_index;
    cat_list_action action;
    ...
    int            visible_start_index;
} cat_options_list_result;
```

`action` may be `CAT_ACTION_OPTION_CHANGED` when `return_on_option_change` is enabled, `CAT_ACTION_TRIGGERED` for `action_button`, `CAT_ACTION_SECONDARY_TRIGGERED` for `secondary_action_button`, `CAT_ACTION_SELECTED` for clickable rows, `CAT_ACTION_CONFIRMED` for confirm exits, or `CAT_ACTION_BACK`.

### Keyboard

```c
int cat_keyboard(const char *initial_text, const char *help_text,
                cat_keyboard_layout layout, cat_keyboard_result *result);

int cat_url_keyboard(const char *initial_text, const char *help_text,
                    cat_url_keyboard_config *cfg, cat_keyboard_result *result);
```

5-row on-screen keyboard matching Gabagool's layout:
- **Row 0**: Numbers 1-0 + backspace (⌫, 2× width)
- **Row 1**: QWERTY row (10 keys, centered)
- **Row 2**: ASDF row (9 keys) + enter (↵, 1.5× width)
- **Row 3**: Shift (⇧, 2× width) + ZXCV row (7 keys) + symbol toggle (#+=, 2× width)
- **Row 4**: Space bar (8× width, centered)

**Parameters**:
- `initial_text`: Pre-filled text shown in the input field on open (may be `NULL` for empty).
- `help_text`: Text shown verbatim in the **Menu help overlay**. Pass `NULL` to use the built-in keyboard instructions. Do not pass a prompt string here — it is not displayed as an on-screen label.

**Button mapping** (Gabagool-compatible):
- **B**: Backspace
- **X**: Space (general) / Toggle symbol alternates (URL)
- **Y**: Cancel without saving
- **Select**: Toggle shift
- **Start**: Confirm
- **L1/R1**: Move text cursor left/right
- **Menu**: Help overlay (shows `help_text` or built-in instructions when `help_text` is `NULL`)

**URL Keyboard** adds configurable shortcut rows above the QWERTY keys:
- Default shortcuts: `https://`, `www.`, `.com`, `.org`, `.net`, `.io`, `.dev`, `.app`, `.edu`, `.gov`
- X toggles to symbol alternates: `http://`, `ftp://`, `.co`, `.tv`, `.me`, `.gg`, `.uk`, `.de`, `.ca`, `.au`
- URL special chars row: `/ : @ - _ . ~ ? # &`
- Bottom-row toggle switches between `123` and `abc`, replacing the URL rows with digits and symbol sets when enabled
- No space bar in URL mode

**Layouts**: `CAT_KB_GENERAL`, `CAT_KB_URL`, `CAT_KB_NUMERIC`

**Result**: `cat_keyboard_result.text` (char[1024])

### Confirmation

```c
int cat_confirmation(cat_message_opts *opts, cat_confirm_result *result);
```

Modal dialog with a message (optionally with an image above it). Waits for user to press A (confirm) or B (cancel).

**Result**: `cat_confirm_result.confirmed` (bool)

### Selection

```c
int cat_selection(const char *message, cat_selection_option *options, int count,
                 cat_footer_item *footer, int footer_count,
                 cat_selection_result *result);
```

Horizontal pill-style chooser. User presses Left/Right to cycle options, A to confirm.

**Result**: `cat_selection_result.selected_index`

### Process Message

```c
int cat_process_message(cat_process_opts *opts, cat_process_fn fn, void *userdata);
```

Runs a worker function in a background thread while displaying a message and optional progress bar.

```c
typedef int (*cat_process_fn)(void *userdata);
```

**`cat_process_opts`**:
```c
typedef struct {
    const char     *message;
    bool            show_progress;
    float          *progress;          // Worker updates this [0.0–1.0]
    int            *interrupt_signal;  // UI sets to 1 on cancel
    cat_button       interrupt_button;  // Cancel button (CAT_BTN_NONE = none)
    char          **dynamic_message;   // Worker can update displayed text
    int             message_lines;
} cat_process_opts;
```

### Detail Screen

```c
int cat_detail_screen(cat_detail_opts *opts, cat_detail_result *result);
```

Scrollable multi-section view for displaying information. Supports:

| Section Type | Content |
|-------------|---------|
| `CAT_SECTION_INFO` | Key-value pairs |
| `CAT_SECTION_DESCRIPTION` | Wrapped text block |
| `CAT_SECTION_IMAGE` | Single image |
| `CAT_SECTION_TABLE` | Tabular data |

`CAT_SECTION_IMAGE` textures are loaded once when the detail screen opens and reused for each frame until the screen exits.

**`cat_detail_action`** enum:
```c
typedef enum {
    CAT_DETAIL_BACK = 0,   // User pressed back
    CAT_DETAIL_ACTION,     // User pressed the primary action button (A)
    CAT_DETAIL_SECONDARY_ACTION // User pressed the secondary action button (Y)
} cat_detail_action;
```

`cat_detail_screen()` exits with `CAT_DETAIL_BACK` on B, `CAT_DETAIL_ACTION` on A, and `CAT_DETAIL_SECONDARY_ACTION` on Y. If you want the Y action to be visible to users, add a matching Y footer hint.

**`cat_detail_opts`** (styling fields):
```c
typedef struct {
    ...
    bool        center_title;
    bool        show_section_separator;
    const cat_draw_color *key_color;
    TTF_Font   *body_font;           // Override body/value text (default: CAT_FONT_TINY)
    TTF_Font   *section_title_font;  // Override section headers (default: CAT_FONT_SMALL)
    TTF_Font   *key_font;            // Override info-pair key text (default: CAT_FONT_TINY)
} cat_detail_opts;
```

`body_font` overrides the font used for description text and info-pair values. `section_title_font` overrides section header text. `key_font` overrides info-pair key (left-hand) text. When `NULL` (zero-init default), the widget uses `cat_get_font(CAT_FONT_TINY)` for body and key text, and `cat_get_font(CAT_FONT_SMALL)` for section titles. Pass a font obtained from `cat_get_font()` or a custom-loaded `TTF_Font` to override.

**`cat_detail_result`**:
```c
typedef struct {
    cat_detail_action action;
} cat_detail_result;
```

### Queue Viewer

```c
int cat_queue_viewer(const cat_queue_opts *opts);
```

Live-updating, filterable display for background job queues. The widget polls a caller-supplied snapshot callback each frame — all threading stays in the caller.

**Features**:
- Animated pill selection (same as list widget)
- Horizontal text scroll on long titles when selected
- Per-item inline progress bars on the subtitle row
- Filter cycling: ALL / IN PROGRESS / DONE / FAILED by default, overrideable via `filter_labels[4]` (Y button)
- Summary bar: `"X/Y COMPLETE, Z FAILED"` above footer
- A: Detail callback for terminal items (DONE/FAILED/SKIPPED)
- X: Cancel callback while active, clear-done callback when idle
- Menu / desktop `H`: open hidden footer actions when the footer shows `+N`
- Idle-aware: calls `cat_request_frame()` only while jobs are active

Navigation matches `cat_list()`: `D-Pad Left/Right` skip by one visible page, while `L1/R1` jump to the previous/next first-letter group within the active filter. For best results, keep queue titles pre-sorted.

**`cat_queue_status`**:
```c
typedef enum {
    CAT_QUEUE_PENDING = 0,   // Waiting to start          (hint color)
    CAT_QUEUE_RUNNING = 1,   // Actively being processed  (accent color)
    CAT_QUEUE_DONE    = 2,   // Completed successfully     (soft green, RGBA 100, 200, 100, 255)
    CAT_QUEUE_FAILED  = 3,   // Ended in error             (red)
    CAT_QUEUE_SKIPPED = 4,   // Intentionally skipped or cancelled (hint color)
} cat_queue_status;
```

**`cat_queue_item`** (filled by snapshot callback each frame):
```c
typedef struct {
    char            title[256];      // Large primary label
    char            subtitle[128];   // Small secondary label
    char            status_text[64]; // Right-aligned status string
    cat_queue_status status;          // Color-coding and filter
    float           progress;        // 0.0–1.0 = inline progress bar; < 0 = no bar
    void           *userdata;        // Caller-defined context
} cat_queue_item;
```

**`cat_queue_opts`**:
```c
typedef struct {
    const char           *title;       // Screen title, e.g. "DOWNLOADS"
    cat_queue_snapshot_fn  snapshot;    // Required: fills items each frame
    int                   max_items;   // Buffer capacity; 0 → 256
    void                 *userdata;    // Passed to all callbacks
    cat_queue_detail_fn    on_detail;   // Optional: A button on terminal items
    cat_queue_cancel_fn    on_cancel;   // Optional: X button while queue active
    cat_queue_clear_fn     on_clear;    // Optional: X button when queue idle
    cat_status_bar_opts   *status_bar;  // Optional: top-right status pill
    bool                  hide_filter; // Set true to hide Y=FILTER cycling
    const char           *filter_labels[4]; // Optional filter labels: [0]=ALL, [1]=IN PROGRESS (PENDING+RUNNING), [2]=DONE, [3]=FAILED; NULL or "" entries use defaults
} cat_queue_opts;
```

**Callbacks**:
```c
// Fill buf[0..max] with current state. Must be thread-safe. Return count.
typedef int  (*cat_queue_snapshot_fn)(cat_queue_item *buf, int max, void *userdata);

// Called when A is pressed on a terminal item. May push another widget.
typedef void (*cat_queue_detail_fn)(const cat_queue_item *item, void *userdata);

// Called when X is pressed while active items remain.
// Update your queue state so subsequent snapshots reflect cancellation.
typedef void (*cat_queue_cancel_fn)(void *userdata);

// Called when X is pressed to clear completed items.
typedef void (*cat_queue_clear_fn)(void *userdata);
```

**Example** (minimal):
```c
static int my_snapshot(cat_queue_item *buf, int max, void *ud) {
    pthread_mutex_lock(&queue_mutex);
    int n = copy_queue_to_buf(buf, max);  // your thread-safe copy
    pthread_mutex_unlock(&queue_mutex);
    return n;
}

static void my_cancel(void *ud) {
    /* confirm cancellation, then mark unfinished items as CANCELLED */
}

cat_queue_opts opts = {
    .title         = "DOWNLOADS",
    .snapshot      = my_snapshot,
    .on_cancel     = my_cancel,
    .filter_labels = { "ALL", "ACTIVE", "COMPLETE", "ERRORS" },
};
cat_queue_viewer(&opts);
```

To represent cancelled downloads, keep queue state in your own data model and have future snapshots return `CAT_QUEUE_SKIPPED` with `status_text = "CANCELLED"` for unfinished items after `on_cancel` runs.

### Download Manager

```c
int cat_download_manager(cat_download *downloads, int count,
                        cat_download_opts *opts, cat_download_result *result);
```

Multi-threaded file downloader with per-file progress bars. Requires libcurl
(compile with `-DCAT_ENABLE_CURL` and link with `-lcurl`).

For Catastrophe device example builds, bundled curl is enabled by default for `EXAMPLE=download` via `USE_BUNDLED_CURL=1`.
This builds dependencies into `build/third_party/<platform>/...`, stages runtime libs in `build/<platform>/download/lib`,
and stages `cacert.pem` into the pak `lib` directory. Launchers should export
`SSL_CERT_FILE=$PAK_DIR/lib/cacert.pem` when that file is present.

**Features**:
- Thread pool with configurable concurrency (default 3)
- Per-file progress bars (3/4 screen width, max 900px)
- Live download speed display (toggleable with X)
- Auto-scroll to active downloads
- Cancel all with Y
- Custom HTTP headers and SSL options

**`cat_download`** (per-job):
```c
typedef struct {
    const char          *url;         // Source URL
    const char          *dest_path;   // Destination file path
    const char          *label;       // Display label
    cat_download_status   status;      // CAT_DL_PENDING/DOWNLOADING/COMPLETE/FAILED
    float                progress;    // 0.0–1.0
    double               speed_bps;   // Bytes per second
    int                  http_code;   // HTTP response code
    char                 error[256];  // Error message
} cat_download;
```

**`cat_download_opts`**:
```c
typedef struct {
    int   max_concurrent;     // Max simultaneous downloads (default 3)
    bool  skip_ssl_verify;    // Disable SSL cert verification
    const char **headers;     // "Header: Value" strings
    int   header_count;
} cat_download_opts;
```

**`cat_download_result`**:
```c
typedef struct {
    int  total;
    int  completed;
    int  failed;
    bool cancelled;
} cat_download_result;
```
### Color Picker

```c
int cat_color_picker(cat_draw_color initial, cat_draw_color *result);
int cat_color_picker_ctx(cat_draw_color initial, cat_draw_color *result,
                         cat_color_picker_context *context);
```

5×5 grid of predefined colors. Navigate with D-Pad, confirm with A.

`cat_color_picker_ctx()` adds an optional live preview strip:

```c
typedef struct {
    struct { const char *label; cat_draw_color color; } roles[8];
    int role_count;
    int active_role;   /* -1 = none */
} cat_color_picker_context;
```

Pass `NULL` for the context to use the plain picker.

### Help Overlay

```c
void cat_show_help_overlay(const char *text);
```

Full-screen scrollable text overlay. Typically triggered by Menu in widgets that have `help_text` configured.

### File Picker

#### Types

```c
typedef enum {
    CAT_FILE_PICKER_FILES = 0,   /* Only files are selectable            */
    CAT_FILE_PICKER_DIRS  = 1,   /* Only directories are selectable      */
    CAT_FILE_PICKER_BOTH  = 2,   /* Files and directories are selectable */
} cat_file_picker_mode;
```

```c
typedef struct {
    const char           *title;           /* Screen title (NULL = show relative path from root) */
    cat_file_picker_mode   mode;            /* What can be selected */
    const char           *initial_path;    /* Starting directory (NULL = root_path) */
    const char           *root_path;       /* Navigation boundary (NULL = auto-detect per platform) */
    const char          **extensions;      /* File extension filter array, e.g. {"zip","7z"} */
    int                   extension_count; /* Number of entries in extensions array */
    bool                  allow_create;    /* Show NEW DIR action (X button) in DIRS/BOTH modes */
    bool                  show_hidden;     /* Show dotfiles/dotdirs (e.g. .env, .gitignore, .config) */
    cat_status_bar_opts   *status_bar;      /* Optional status bar */
} cat_file_picker_opts;
```

```c
typedef struct {
    char path[1024];      /* Full absolute path of the selected item */
    bool is_directory;    /* True when the selected item is a directory */
} cat_file_picker_result;
```

#### Functions

| Function | Description |
|----------|-------------|
| `cat_file_picker_default_opts(title)` | Create default options (mode = FILES, no filter, no create) |
| `cat_file_picker(opts, result)` | Show blocking file picker. Returns `CAT_OK` on selection, `CAT_CANCELLED` on back from root |

#### Controls

| Button | Action |
|--------|--------|
| D-Pad Up/Down | Navigate items |
| D-Pad Left/Right | Page up/down |
| L1/R1 | Jump between letter groups |
| A | Enter folder / select file |
| B | Go up one directory (cancel at root) |
| X | Create new folder (DIRS / BOTH modes with `allow_create`) |
| START | Select current directory (DIRS / BOTH modes) |

#### Root Path Resolution

| Platform | Default root |
|----------|-------------|
| Device (`CAT_PLATFORM_IS_DEVICE`) | `SDCARD_PATH` env var, or the platform default SD card root |
| Windows | `USERPROFILE` env var, or `.` |
| macOS / Linux | `HOME` env var, or `.` |

Notes:
- On device, `SDCARD_PATH` is always the hard ceiling even if `root_path` is provided.
- MLP1's platform default is `/mnt/sdcard`; the other device ports default to `/mnt/SDCARD`.
- On desktop, `root_path` overrides the default home root when provided.
- `initial_path` is only used when it resolves to a directory inside the effective root.
- `allow_create` is ignored in `CAT_FILE_PICKER_FILES`.
- In `CAT_FILE_PICKER_BOTH`, the `A` footer hint updates live: `ENTER` on directories, `OPEN` on files.
- Set `show_hidden = true` to list and select dotfiles like `.env` and dot-directories like `.config`.
- New-folder creation only accepts a single path component; separators and `.` / `..` are rejected.
- Dotfiles still use the normal extension-filter logic when `extensions` are configured.

#### Usage Example

```c
cat_file_picker_opts opts = cat_file_picker_default_opts("Select ROM");
opts.mode = CAT_FILE_PICKER_FILES;
opts.extensions = (const char *[]){"zip", "7z"};
opts.extension_count = 2;

cat_file_picker_result result;
int rc = cat_file_picker(&opts, &result);
if (rc == CAT_OK) {
    printf("Selected: %s (dir=%d)\n", result.path, result.is_directory);
}
```

### List Pane

Non-blocking list draw helper for callers that own their own event/render loop.
Unlike `cat_list()`, this API never polls input or blocks; the caller updates
`cat_list_state` and draws once per frame.

```c
typedef struct {
    int cursor;
    int scroll_offset;
    int visible_rows;
} cat_list_state;

typedef void (*cat_list_item_draw_fn)(int idx, int x, int y, int w, int h,
                                      bool selected, void *user);

void cat_list_state_init(cat_list_state *s, int visible_rows);
void cat_list_state_move(cat_list_state *s, int delta, int count);
void cat_list_state_page(cat_list_state *s, int delta, int count);
void cat_list_state_jump(cat_list_state *s, int target, int count);
int  cat_list_state_jump_letter(cat_list_state *s,
                                const char *(*get_label)(int idx, void *user),
                                void *user, int count, int direction);
void cat_draw_list_pane(int x, int y, int w, int h,
                        int item_count, const cat_list_state *state,
                        int item_height, cat_list_item_draw_fn draw_item,
                        void *user);
```

`cat_list_state_move()` wraps at the top/bottom. Page and direct jumps clamp.
`cat_draw_list_pane()` draws only visible rows and adds a scrollbar when needed.

### Scroll View

Non-selectable vertical scroll container for custom content taller than its
viewport.

```c
typedef struct {
    int offset;
} cat_scroll_state;

typedef void (*cat_scroll_content_fn)(int x, int y, int w, void *user);

void cat_scroll_state_init(cat_scroll_state *s);
void cat_scroll_state_move(cat_scroll_state *s, int delta_px);
void cat_draw_scroll_view(int x, int y, int w, int h, int content_height,
                          cat_scroll_state *state,
                          cat_scroll_content_fn draw, void *user);
```

The view clamps `state->offset` to the valid range, clips drawing to the
viewport, passes a content width that excludes the scrollbar gutter, and draws a
scrollbar only when content overflows. It composes with `cat_draw_text_marquee()`
because the marquee helper preserves existing clip rectangles.
