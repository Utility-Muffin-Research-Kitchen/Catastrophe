# Getting Started with Catastrophe

This guide walks you through creating your first Catastrophe pak.

This guide targets the current Catastrophe headers in this repository.

## Prerequisites

### macOS (Development)

```bash
# Install Homebrew dependencies
brew install sdl2 sdl2_ttf sdl2_image

# Optional: libcurl for the Download Manager widget
brew install curl

# Optional: Docker for cross-compiling to device
brew install --cask docker
```

### Linux (Development)

```bash
# Debian/Ubuntu
sudo apt install libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev

# Optional: libcurl for the Download Manager widget
sudo apt install libcurl4-openssl-dev

# Fedora
sudo dnf install SDL2-devel SDL2_ttf-devel SDL2_image-devel

# Arch
sudo pacman -S sdl2 sdl2_ttf sdl2_image

# Optional: Docker for cross-compiling to device
# Follow Docker's official install instructions for your distro
```

### Windows (Development)

Requires [MSYS2](https://www.msys2.org/). Run these commands in an MSYS2 MinGW64 shell:

```bash
# x86_64
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_ttf mingw-w64-x86_64-SDL2_image mingw-w64-x86_64-pkg-config make

# ARM64 (use CLANGARM64 shell instead)
pacman -S mingw-w64-clang-aarch64-gcc mingw-w64-clang-aarch64-SDL2 mingw-w64-clang-aarch64-SDL2_ttf mingw-w64-clang-aarch64-SDL2_image mingw-w64-clang-aarch64-pkg-config make

# Optional: libcurl for the Download Manager widget
pacman -S mingw-w64-x86_64-curl  # or mingw-w64-clang-aarch64-curl

# Optional: Docker for cross-compiling to device
# Install Docker Desktop for Windows
```

### Device Cross-Compilation

You need Docker installed and running. The build system automatically pulls the correct toolchain image for each platform.

The root Makefile currently provides cross-compile targets for `tg5040`,
`tg5050`, and `my355`. MLP1 is a supported runtime platform in the headers, but
MLP1 products consume Catastrophe through their own build systems with
`-DPLATFORM_MLP1`; this repository does not currently expose a standalone
`make mlp1` target.

### Device Download Manager (Bundled curl)

`cat_download_manager` on device builds uses a bundled libcurl toolchain flow.

- Default behavior: `EXAMPLE=download` enables `USE_BUNDLED_CURL=1` automatically.
- Sources are downloaded once and cached in `build/third_party/sources`.
- Built dependencies are stored per-platform under `build/third_party/<platform>/...`.
- Device targets automatically build OpenSSL first, then curl (TLS-enabled).
- curl and OpenSSL are linked statically; the `lib` directory in the pak contains only `cacert.pem` for SSL certificate verification.

You can force-enable it explicitly:

```bash
make tg5040-download USE_BUNDLED_CURL=1
make tg5050-download USE_BUNDLED_CURL=1
make my355-download USE_BUNDLED_CURL=1
```

## Project Structure

A small experiment can be very simple:

```
MyPak/
├── main.c              # Your application code
├── font.ttf            # Optional desktop test font
├── Makefile            # Build configuration
├── launch.sh           # Pak launch script
└── pak.json            # Pak metadata
```

Real multi-platform Catastrophe paks usually look more like this:

```
MyProject/
├── Makefile
├── launch.sh
├── pak.json
├── src/
│   └── main.c
├── ports/
│   ├── tg5040/Makefile
│   ├── tg5050/Makefile
│   ├── my355/Makefile
│   └── mlp1/Makefile      # product-owned when targeting MLP1
└── third_party/
    └── catastrophe/
        ├── include/
        │   ├── catastrophe.h
        │   └── catastrophe_widgets.h
        └── res/
            └── font.ttf
```

That second layout is closer to how real projects are currently structured: application code under `src/`, platform packaging under `ports/`, and Catastrophe vendored under `third_party/catastrophe`.

## Step 1: Include the Headers

Create a `main.c` file. Define the implementation macros in exactly **one** translation unit:

```c
#define CAT_IMPLEMENTATION
#include "catastrophe.h"
#define CAT_WIDGETS_IMPLEMENTATION
#include "catastrophe_widgets.h"
```

If your project has multiple `.c` files, only one should have the `#define` lines. All others just `#include` the headers normally.

If you vendor Catastrophe under `third_party/catastrophe`, add `-Ithird_party/catastrophe/include` to your compiler flags and keep the `#include "catastrophe.h"` form shown above.

## Step 2: Initialise

```c
int main(int argc, char *argv[]) {
    cat_config cfg = {
        .window_title = "My Pak",
        .font_path    = CAT_PLATFORM_IS_DEVICE ? NULL
                        : "third_party/catastrophe/res/font.ttf",
        .log_path     = cat_resolve_log_path("myapp"), // optional helper
    };

    if (cat_init(&cfg) != CAT_OK) {
        return 1;
    }

    // ... your UI code ...

    cat_quit();
    return 0;
}
```

`cat_init()` handles everything: SDL initialisation, window/renderer creation, font loading, theme loading (on device), input setup, and screen size detection.
`cat_resolve_log_path()` maps logs to `LOGS_PATH`, then `SHARED_USERDATA_PATH/logs`, then `HOME/.userdata/logs`.

`font_path` is mainly a desktop concern. On device, leave it as `NULL` and Catastrophe will load the font from the active stylesheet (or the Allium default font location) automatically. For desktop testing, pointing `font_path` at a bundled `.ttf` gives you predictable rendering across machines.

## Step 3: Show a Widget

All widgets are **blocking** — they run their own event loop and return when the user takes an action.

```c
// Create list items
cat_list_item items[] = {
    { .label = "Play Game" },
    { .label = "Settings"  },
    { .label = "About"     },
};

// Create footer hints
cat_footer_item footer[] = {
    { .button = CAT_BTN_B, .label = "Quit" },
    { .button = CAT_BTN_A, .label = "Select", .is_confirm = true },
};

// Configure the list
cat_list_opts opts = cat_list_default_opts("Main Menu", items, 3);
opts.footer       = footer;
opts.footer_count = 2;
// Footer hints are visual only; behavior comes from action bindings.
opts.action_button = CAT_BTN_A;  // optional custom trigger

// Show it (blocks until user acts)
cat_list_result result;
int rc = cat_list(&opts, &result);

if (rc == CAT_OK) {
    printf("Selected: %d\n", result.selected_index);
} else if (rc == CAT_CANCELLED) {
    printf("User pressed back\n");
}
```

## Step 4: Build & Run

### Any platform (auto-detect)

```bash
make native
make run-native

# (no extra setup required — demo aliases run directly)
# and reusable on Linux/Windows via the CAT_* overrides

# Override the desktop preview resolution to match device targets
# Substitute other width/height values as needed.
CAT_WINDOW_WIDTH=1024 CAT_WINDOW_HEIGHT=768 make run-native-demo   # Brick
CAT_WINDOW_WIDTH=1280 CAT_WINDOW_HEIGHT=720 make run-native-demo   # Smart Pro / Smart Pro S
CAT_WINDOW_WIDTH=640 CAT_WINDOW_HEIGHT=480 make run-native-demo    # Miyoo Flip
CAT_WINDOW_WIDTH=960 CAT_WINDOW_HEIGHT=720 make run-native-demo    # MLP1 fallback profile
```

`run-native-demo` resolves to the host-specific demo target. The demo loads themes from `themes/Allium-Themes/Themes/` (initialised as a git submodule) and falls back to the bundled `res/themes/Catastrophe/` default.

### macOS

```bash
make mac
make run-mac
make run-mac-demo      # launches the widget demo with the bundled default theme
make run-mac-download  # launches the download/status bar example

# Same preview override when running the macOS target directly
# Substitute other width/height values as needed.
CAT_WINDOW_WIDTH=1024 CAT_WINDOW_HEIGHT=768 make run-mac-demo
```

The desktop preview uses generated status/control sprites from `res/assets/` by default.
Run `make assets` to regenerate `assets@1x.png` through `assets@4x.png`.
Point `CAT_STATUS_ASSETS_DIR` at another folder with the same filenames if you want to test
alternate sprite assets locally. `run-mac-demo` and `run-mac-download` automatically point
`CAT_STATUS_ASSETS_DIR`, `CAT_THEMES_DIR`, and battery/wifi defaults from the repo unless
you already exported your own override values.

### Linux

```bash
make linux
make run-linux
```

### Windows (MSYS2 MinGW shell)

```bash
make windows
make run-windows
```

### Keyboard Controls

When running on desktop (macOS, Linux, or Windows), keyboard keys are mapped to controller buttons matching [Gabagool's `DefaultInputMapping()`](https://github.com/BrandonKowalski/gabagool):

| Key | Button | | Key | Button |
|-----|--------|-|-----|--------|
| `A` | A (confirm) | | `L` | L1 |
| `B` | B (back) | | `;` | L2 |
| `X` | X | | `R` | R1 |
| `Y` | Y | | `T` | R2 |
| Arrow keys | D-pad | | `Return` | Start |
| `H` | Menu | | `Space` | Select |

> **Note:** This matches the physical layout on the device — press `a` on your keyboard to confirm/select, `b` to go back. `Return` maps to Start (not confirm) and `Space` maps to Select.

### Device

```bash
make tg5040        # or tg5050 or my355
make package       # Creates .pakz archive
make deploy        # Push to connected device via adb
```

For MLP1 product builds, compile the same source with `-DPLATFORM_MLP1` in the
product build system. That selects:

- `CAT_PLATFORM_NAME` = `"mlp1"` and `CAT_PLATFORM_IS_DEVICE` = `1`
- default SD root `/mnt/sdcard`
- launcher assets/themes/fonts under `/mnt/sdcard/umrk-launcher`
- user state under `/mnt/sdcard/.userdata/mlp1`
- device metrics with scale `2`, padding `10`, and `960x720` fullscreen fallback
- MLP1 raw joystick mapping, including `CAT_BTN_STICK` for the stick click

The MLP1 power handler is intentionally disabled in Catastrophe pending a stock
service audit. Use platform services from the host launcher/daemon if the app
needs power, battery, charging, Wi-Fi, or fan state supplied externally.

## Step 5: Create the Launch Script

Every Pak needs a `launch.sh`:

```bash
#!/bin/sh
set -eu

APP_BIN="myapp"
PAK_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PAK_NAME=$(basename "$PAK_DIR")
PAK_NAME=${PAK_NAME%.pak}

cd "$PAK_DIR"

# Point OpenSSL/curl to bundled CA certificates for SSL verification
if [ -f "$PAK_DIR/lib/cacert.pem" ]; then
    export SSL_CERT_FILE="$PAK_DIR/lib/cacert.pem"
fi

SHARED_USERDATA_ROOT=${SHARED_USERDATA_PATH:-"${HOME:-/tmp}/.userdata/shared"}
LOG_ROOT=${LOGS_PATH:-"$SHARED_USERDATA_ROOT/logs"}
mkdir -p "$LOG_ROOT"
LOG_FILE="$LOG_ROOT/$APP_BIN.txt"
: >"$LOG_FILE"

exec >>"$LOG_FILE"
exec 2>&1

echo "=== Launching $PAK_NAME ($APP_BIN) at $(date) ==="
echo "platform=${PLATFORM:-unknown} device=${DEVICE:-unknown}"
echo "args: $*"

exec "./$APP_BIN" "$@"
```

Make it executable: `chmod +x launch.sh`

## Key Concepts

### Return Codes

All widget functions return:
- `CAT_OK` (0) — Action completed successfully
- `CAT_ERROR` (-1) — Something went wrong
- `CAT_CANCELLED` (-2) — User pressed back

### Scaling with `CAT_S()`

Never hard-code pixel sizes. Use the scaling macro:

```c
int margin = CAT_S(20);           // 20px at 1024-width reference
int padding = CAT_S(10);          // 10px at 1024-width reference
```

This automatically adapts to the target screen resolution.

### The Theme

Access current theme colors via `cat_get_theme()`:

```c
cat_theme *t = cat_get_theme();
cat_draw_rounded_rect(x, y, w, h, CAT_S(8), t->highlight);
```

### Fonts

Get sized fonts with `cat_get_font()`:

```c
TTF_Font *font = cat_get_font(CAT_FONT_MEDIUM);
cat_draw_text(font, "Hello!", x, y, cat_get_theme()->text);
```

Font tiers (base × device_scale): Extra Large (24), Large (16), Medium (14), Small (12), Tiny (10), Micro (7). At device_scale=2 (MY355, TG5050, MLP1): 48, 32, 28, 24, 20, 14px.

## Next Steps

- Read the [API Reference](API.md) for the complete function list
- Browse the [Widget Catalog](WIDGETS.md) for all available components
- Check the [Porting from Gabagool](PORTING_FROM_GABAGOOL.md) guide if migrating from Go
- Study the `examples/demo/main.c` for a comprehensive usage example
