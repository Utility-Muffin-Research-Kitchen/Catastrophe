# Copilot Instructions for Catastrophe

## Repository role in UMRK

- Treat this repository as the shared UI layer for the wider UMRK workspace, not as the entire launcher product.
- UMRK’s current direction is a Mac-first launcher stack that can later be ported to handheld targets such as tg5040-class TrimUI devices and my355-class Miyoo devices.
- Catastrophe is the toolkit/basic application layer that launcher-style apps will be written in. The planned launcher repo is `Jawaka`, and future sibling directories under the UMRK workspace may also contain separate repositories for RetroArch builds, custom RetroArch overlays, a DinguxCommander-based file explorer, and additional applications.
- The broader workspace is inspired by SpruceUI, NextUI, and Allium, but this repository should not assume runtime compatibility with any of them.
- When working in this repository, focus on reusable UI/runtime capabilities, desktop preview parity, and platform support in the current Catastrophe build system. Do not assume emulator integration or application-specific logic belongs here unless the code in this repo already introduces it.

## Build and run commands

- `make help` shows the supported example and platform targets.
- `make native` builds all examples for the current host OS; `make run-native` runs the `hello` example.
- Use `make run-native-<example>` to build and run a single example on the current host, for example `make run-native-demo`, `make run-native-download`, or `make run-native-combo`.
- Desktop platform targets build the same example set explicitly: `make mac`, `make linux`, `make windows`. Single-example variants use the pattern targets directly, for example `make mac-demo`, `make linux-demo`, or `make windows-download`. Run one example with `make run-mac-<example>`, `make run-linux-<example>`, or `make run-windows-<example>`.
- Device builds cross-compile through Docker: `make tg5040`, `make tg5050`, `make my355`, or a single example such as `make tg5040-demo`.
- `examples/download` is special on device builds: use `USE_BUNDLED_CURL=1` when needed, for example `make tg5040-download USE_BUNDLED_CURL=1`.
- Packaging and deployment are first-class build steps: `make package` creates `.pakz` archives in `build/dist/`, `make deploy` pushes the built examples to a connected device over `adb`, and `make clean` removes `build/`.
- There are no dedicated lint or automated test targets. The repository uses single-example build/run commands as its main verification path.

## High-level architecture

- Catastrophe is a header-only C toolkit split into `include/catastrophe.h` and `include/catastrophe_widgets.h`.
  - `catastrophe.h` owns the core runtime: initialization, lifecycle, drawing, theming, fonts, scaling, input, logging, and platform helpers.
  - `catastrophe_widgets.h` layers on the higher-level UI flows: lists, options lists, keyboard, confirmation, file picker, download manager, queue viewer, detail screens, and other blocking widgets.
- In the wider UMRK workspace, this repository should be treated as infrastructure for launcher/apps UI rather than the application layer itself. If a future change needs emulator wiring, launcher state management, app catalog behavior, mock SD-card generation, or in-game overlay policy, first check whether it belongs in a sibling repo instead of Catastrophe.
- The examples under `examples/*/main.c` are the real integration surface. Each example defines `CAT_IMPLEMENTATION` and, when needed, `CAT_WIDGETS_IMPLEMENTATION` in its own `main.c`, so the examples compile the library directly instead of linking against a separately built Catastrophe library target.
- The root `Makefile` is the orchestration layer. Desktop targets compile example entry points directly against SDL2/SDL2_ttf/SDL2_image. Device targets dispatch into `ports/<platform>/Makefile` inside Docker toolchain images and compile those same example entry points with platform macros like `PLATFORM_TG5040`, `PLATFORM_TG5050`, and `PLATFORM_MY355`.
- Packaging is built from those example outputs, not from separate manifests. `make package` stages each example into `build/staging/Tools/<platform>/<Name>.pak/` and zips that into `build/dist/*.pakz`. `make deploy` auto-detects the attached device platform over `adb` and pushes each built example into `/mnt/SDCARD/Tools/<platform>/`.
- Theme and asset loading spans multiple layers. Desktop run targets populate `CAT_THEMES_DIR` and `CAT_FONTS_DIR` from the `themes/Allium-Themes` git submodule, the runtime also checks bundled `res/themes/`, and device builds can load from `/mnt/SDCARD/Themes/`. The repository’s default bundled theme is `res/themes/Catastrophe/`.
- `examples/download` is the only path with extra third-party build machinery. Desktop builds enable curl only when `pkg-config` finds `libcurl`. Device builds route through `scripts/build_third_party.sh`, which downloads and caches OpenSSL and curl under `build/third_party/`, then stages only the runtime certificate bundle into the pak `lib/` directory.

## Key conventions

- Define `CAT_IMPLEMENTATION` and `CAT_WIDGETS_IMPLEMENTATION` in exactly one translation unit, and include `catastrophe_widgets.h` only after `catastrophe.h`. Follow the example apps’ pattern rather than trying to introduce a separate compiled-library build.
- Keep the naming split from `CONTRIBUTING.md`: public API is `cat_`, internal helpers are `cat__`, and constants/macros/enums use `CAT_`.
- Match the repository style: 4-space indentation, same-line braces, and uppercase footer/button labels in example UI code.
- UMRK’s default implementation language is C, with selective C++ only where it materially simplifies a subsystem. Keep Catastrophe’s interfaces straightforward for C-first consumers such as Jawaka.
- Treat widgets as blocking control-flow boundaries. They return `CAT_OK` / `CAT_CANCELLED` together with `CAT_ACTION_*` results, and callers often preserve `initial_index` plus `visible_start_index` when reopening a list or options screen so focus and scroll state survive round-trips.
- Footer hints are display-only. Actual behavior is configured with `action_button`, `secondary_action_button`, `tertiary_action_button`, and `confirm_button`; `cat_footer_item.button_text` only changes what is rendered.
- For `cat_list_item`, use designated initializers or `CAT_LIST_ITEM` / `CAT_LIST_ITEM_BG`. Avoid positional initializers because fields are intentionally allowed to grow over time.
- Keep layout math in Catastrophe’s scaling system instead of hardcoding pixels. UI code is written in reference-width units with `CAT_S()` and `CAT_DS()`, and desktop preview behavior often depends on environment overrides like `CAT_WINDOW_WIDTH`, `CAT_WINDOW_HEIGHT`, `CAT_PREVIEW_WIFI_STRENGTH`, and `CAT_PREVIEW_BATTERY_PERCENT`.
- The wider UMRK workspace will be FAT32-oriented on real SD cards. Avoid assuming symlinks or desktop-only filesystem behavior in shared UI/runtime decisions, even if the higher-level mock SD-card tooling will live in Jawaka rather than here.
