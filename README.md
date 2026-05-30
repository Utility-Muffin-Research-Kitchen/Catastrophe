# Catastrophe (')

A header-only C UI toolkit for building graphical tools on retro gaming handhelds running [Allium](https://github.com/goweiwen/Allium).

Current release: **v1.1.0** (2026-03-30).

Inspired by [Gabagool](https://github.com/BrandonKowalski/gabagool) (Go). Its framework design directly informed the structure of this project, and this C port would not have been feasible without that foundation.

Thanks to Brandon T. Kowalski (https://github.com/BrandonKowalski) for creating Gabagool and publishing such a well-designed and practical reference implementation.

---

## Supported Platforms

> **Note:** The TrimUI Smart Brick is internally referred to as `tg3040`, but it shares the `tg5040` platform with the Smart Pro.

| Platform | Device | Resolution | CPU |
|-----------|--------|------------|-----|
| `tg5040` | TrimUI Smart Pro | 1280×720 | Allwinner A133 Plus – Quad-core Cortex-A53 |
| `tg5040` | TrimUI Smart Brick (`tg3040` hardware) | 1024×768 | Allwinner A133 Plus – Quad-core Cortex-A53 |
| `tg5050` | TrimUI Smart Pro S | 1280×720 | Allwinner A523 – Octa-core Cortex-A55 |
| `my355`  | Miyoo Flip | 640×480 | Rockchip RK3566 – Quad-core Cortex-A55 |
| `mac`    | macOS (dev/testing) | Windowed preview (default 1024×768) | native host CPU |
| `linux`  | Linux (dev/testing) | Windowed preview (default 1024×768) | native host CPU |
| `windows` | Windows (MSYS2/MinGW dev/testing) | Windowed preview (default 1024×768) | native host CPU |

Desktop development/testing is supported on macOS, Linux, and Windows. `make native` auto-selects the current host target; `make windows` expects an MSYS2/MinGW shell.

## Quick Start

### 1. Clone

```bash
git clone https://github.com/Helaas/catastrophe.git
cd catastrophe
```

### 2. Build for Desktop

Desktop development/testing is supported on macOS, Linux, and Windows (MSYS2/MinGW). Install SDL2, SDL2_ttf, and SDL2_image for your host platform, then use the matching target:

```bash
make native
make run-native        # Runs the hello world example

# Preview other device resolutions on desktop
# Substitute other width/height values as needed.
CAT_WINDOW_WIDTH=1024 CAT_WINDOW_HEIGHT=768 make run-mac-demo   # Brick
CAT_WINDOW_WIDTH=1280 CAT_WINDOW_HEIGHT=720 make run-mac-demo   # Smart Pro / Smart Pro S
CAT_WINDOW_WIDTH=640 CAT_WINDOW_HEIGHT=480 make run-mac-demo    # Miyoo Flip

# Override the preview status-bar inputs
CAT_PREVIEW_WIFI_STRENGTH=3 \
CAT_PREVIEW_BATTERY_PERCENT=100 \
CAT_PREVIEW_CHARGING=0 \
make run-mac-demo
```

The status/control sprites used by desktop previews are generated into `res/assets/`.
Run `make assets` to regenerate `assets@1x.png` through `assets@4x.png`.

For platform-specific dependency install commands, see [Getting Started](docs/GETTING_STARTED.md#prerequisites).

### 3. Build for Device

Requires Docker. Each platform has its own toolchain image:

```bash
make tg5040           # Cross-compile for TrimUI Brick/Smart Pro
make tg5050           # Cross-compile for TrimUI Smart Pro S
make my355            # Cross-compile for Miyoo Flip
make all              # All device platforms
```

### 4. Package & Deploy

```bash
make package          # Create .pakz archives (zipped Pak bundles)
make deploy           # Push to connected device via adb
```

## Usage

Catastrophe is **header-only** (stb-style). In exactly **one** `.c` file:

```c
#define CAT_IMPLEMENTATION
#include "catastrophe.h"
#define CAT_WIDGETS_IMPLEMENTATION
#include "catastrophe_widgets.h"
```

All other files just include the headers normally (without the `#define`s).

### Minimal Example

```c
#define CAT_IMPLEMENTATION
#include "catastrophe.h"
#define CAT_WIDGETS_IMPLEMENTATION
#include "catastrophe_widgets.h"

int main(int argc, char *argv[]) {
    cat_config cfg = {
        .window_title = "My App",
        .font_path    = CAT_PLATFORM_IS_DEVICE ? NULL : "font.ttf",
    };
    cat_init(&cfg);

    cat_list_item items[] = {
        { .label = "Option A" },
        { .label = "Option B" },
        { .label = "Option C" },
    };

    cat_footer_item footer[] = {
        { .button = CAT_BTN_B, .label = "Quit" },
        { .button = CAT_BTN_A, .label = "Select", .is_confirm = true },
    };

    cat_list_opts opts = cat_list_default_opts("Menu", items, 3);
    opts.footer       = footer;
    opts.footer_count = 2;

    cat_list_result result;
    cat_list(&opts, &result);

    cat_quit();
    return 0;
}
```

On device, leave `font_path` unset / `NULL` to use the font from the active stylesheet (or the Allium default font path). A bundled `.ttf` is mainly useful for consistent desktop preview/testing.

## Architecture

```
catastrophe.h          — Core: init, lifecycle, input, drawing, theming, fonts, scaling
catastrophe_widgets.h  — Widgets: list, options list, keyboard, confirmation,
                        selection, process message, download manager, queue manager,
                        detail screen, color picker, help overlay, file picker
```

All widgets use a **blocking model**: they run their own event loop and return a result struct when the user completes an action or presses back (`CAT_CANCELLED`).

### Scaling

All pixel values are specified at a **1024px reference width** and automatically scaled to the target screen via the `CAT_S(x)` macro. Screens wider than 1024px use 75% damping to prevent oversized UI.

### Theming

Catastrophe uses [Allium](https://github.com/goweiwen/Allium)'s `stylesheet.json` format for themes. The runtime discovers themes from `$CAT_THEMES_DIR`, `res/themes/`, the bundled `themes/Allium-Themes/Themes/` submodule, and `/mnt/SDCARD/Themes/` on device. The shipped default theme (`res/themes/Catastrophe/`) reproduces the Apostrophe colors pixel-for-pixel. You can override the accent color at init via `cat_config.primary_color_hex`.

### Input

Catastrophe abstracts all input sources into a unified virtual button system (`CAT_BTN_*`). On desktop (macOS, Linux, or Windows) and recognised gamepads it uses the SDL GameController API; on TrimUI devices it reads raw joystick events; and on the Miyoo Flip (my355) it maps hardware-specific keyboard scancodes. Directional buttons auto-repeat with configurable delay/rate. Miyoo Flip builds also use a higher analog deadzone than other targets to reduce accidental horizontal movement from the thumbstick.

The **combo system** adds support for chords (simultaneous button presses like L1+R1) and sequences (ordered presses like Up, Up, Down, Down). Register combos with `cat_register_chord()` / `cat_register_sequence()` and poll for events with `cat_poll_combo()`. See `examples/combo/` and the [API reference](docs/API.md#combos) for details.

## Widgets

| Widget | Function | Description |
|--------|----------|-------------|
| List | `cat_list()` | Scrollable item list with selection, multi-select, reorder, trailing hints, optional hidden scrollbar |
| Options List | `cat_options_list()` | Settings-style list with cycle/keyboard/click/color options |
| Keyboard | `cat_keyboard()` | 5-row QWERTY keyboard (numbers, qwerty, asdf+enter, shift+zxcv+symbol, space) |
| URL Keyboard | `cat_url_keyboard()` | Keyboard with configurable URL shortcuts, symbol alternates, and a `123` / `abc` number-symbol toggle |
| Download Manager | `cat_download_manager()` | Multi-threaded file downloader with per-file progress bars (requires libcurl) |
| Confirmation | `cat_confirmation()` | Modal message dialog |
| Selection | `cat_selection()` | Horizontal pill-style option chooser |
| Process Message | `cat_process_message()` | Async worker with progress bar |
| Queue Viewer | `cat_queue_viewer()` | Live-updating background job queue with filter, detail, cancel/clear, and inline progress |
| Detail Screen | `cat_detail_screen()` | Scrollable multi-section info view |
| Color Picker | `cat_color_picker()` | 5×5 color grid selector |
| File Picker | `cat_file_picker()` | Filesystem browser for selecting files or directories with optional folder creation in dir-capable modes |
| Help Overlay | `cat_show_help_overlay()` | Scrollable text overlay (Menu trigger) |

## Docs

- [Getting Started](docs/GETTING_STARTED.md) — Step-by-step setup guide
- [API Reference](docs/API.md) — Complete function/struct reference
- [Demo Coverage](docs/DEMO_COVERAGE.md) — Demo-to-API coverage matrix
- [Widget Catalog](docs/WIDGETS.md) — Visual guide to every widget
- [Porting from Gabagool](docs/PORTING_FROM_GABAGOOL.md) — Migration guide from Go to C
- [Gabagool Parity v2.9.6](docs/GABAGOOL_PARITY_v2.9.6.md) — Feature parity matrix and backlog

## License

Catastrophe source code is MIT-licensed — see [LICENSE](LICENSE).

The bundled [res/font.ttf](res/font.ttf) is distributed separately under the SIL Open Font License 1.1. See [res/font.LICENSE.txt](res/font.LICENSE.txt).
