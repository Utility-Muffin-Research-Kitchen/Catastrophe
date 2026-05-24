# Apostrophe → Catastrophe Rename Plan

## Summary

Full rename of every occurrence of "Apostrophe" and the `AP_` / `ap_` prefix across the entire codebase. 167+ references in ~20 files.

---

## Step 1: Bulk text replacements

Use `sed` to replace in all files. Order matters — do generic identifiers last.

### Pass 1: Include guards and `#include` directives

| File pattern | Replace |
|-------------|---------|
| `#ifndef APOSTROPHE_H` | `#ifndef CATASTROPHE_H` |
| `#define APOSTROPHE_H` | `#define CATASTROPHE_H` |
| `#ifndef APOSTROPHE_WIDGETS_H` | `#ifndef CATASTROPHE_WIDGETS_H` |
| `#define APOSTROPHE_WIDGETS_H` | `#define CATASTROPHE_WIDGETS_H` |
| `#ifndef APOSTROPHE_H` guard in widgets | same |
| `#error "apostrophe.h must be included` | `#error "catastrophe.h must be included` |
| `#include "apostrophe.h"` | `#include "catastrophe.h"` |
| `#include "apostrophe_widgets.h"` | `#include "catastrophe_widgets.h"` |

### Pass 2: Implementation macros

| Before | After |
|--------|-------|
| `#define AP_IMPLEMENTATION` | `#define CAT_IMPLEMENTATION` |
| `#define AP_WIDGETS_IMPLEMENTATION` | `#define CAT_WIDGETS_IMPLEMENTATION` |

### Pass 3: Public API prefix `ap_` → `cat_`

All functions, types, enums, and macros prefixed with `ap_` or `AP_`:

```bash
# Functions: ap_init → cat_init, ap_quit → cat_quit, etc. (~80 functions)
sed -i '' 's/\bap_init\b/cat_init/g' ...
sed -i '' 's/\bap_quit\b/cat_quit/g' ...
sed -i '' 's/\bap_get_renderer\b/cat_get_renderer/g' ...
# ... etc for each function
```

Full function rename list:
- `ap_init` → `cat_init`
- `ap_quit` → `cat_quit`
- `ap_get_renderer` → `cat_get_renderer`
- `ap_get_window` → `cat_get_window`
- `ap_get_screen_width` → `cat_get_screen_width`
- `ap_get_screen_height` → `cat_get_screen_height`
- `ap_show_window` → `cat_show_window`
- `ap_hide_window` → `cat_hide_window`
- `ap_get_scale_factor` → `cat_get_scale_factor`
- `ap_scale` → `cat_scale`
- `ap_font_size_for_resolution` → `cat_font_size_for_resolution`
- `ap_get_theme` → `cat_get_theme`
- `ap_theme_load_nextui` → `cat_theme_load_nextui`
- `ap_reload_background` → `cat_reload_background`
- `ap_hex_to_color` → `cat_hex_to_color`
- `ap_set_theme_color` → `cat_set_theme_color`
- `ap_get_font` → `cat_get_font`
- `ap_get_font_bump` → `cat_get_font_bump`
- `ap_poll_input` → `cat_poll_input`
- `ap_set_input_delay` → `cat_set_input_delay`
- `ap_set_input_repeat` → `cat_set_input_repeat`
- `ap_flip_face_buttons` → `cat_flip_face_buttons`
- `ap_button_name` → `cat_button_name`
- `ap_register_chord` → `cat_register_chord`
- `ap_register_sequence` → `cat_register_sequence`
- `ap_register_chord_ex` → `cat_register_chord_ex`
- `ap_register_sequence_ex` → `cat_register_sequence_ex`
- `ap_unregister_combo` → `cat_unregister_combo`
- `ap_clear_combos` → `cat_clear_combos`
- `ap_poll_combo` → `cat_poll_combo`
- `ap_clear_screen` → `cat_clear_screen`
- `ap_present` → `cat_present`
- `ap_draw_background` → `cat_draw_background`
- `ap_draw_rounded_rect` → `cat_draw_rounded_rect`
- `ap_draw_pill` → `cat_draw_pill`
- `ap_draw_rect` → `cat_draw_rect`
- `ap_draw_circle` → `cat_draw_circle`
- `ap_draw_text` → `cat_draw_text`
- `ap_draw_text_clipped` → `cat_draw_text_clipped`
- `ap_draw_text_ellipsized` → `cat_draw_text_ellipsized`
- `ap_draw_text_wrapped` → `cat_draw_text_wrapped`
- `ap_measure_text` → `cat_measure_text`
- `ap_measure_text_ellipsized` → `cat_measure_text_ellipsized`
- `ap_draw_image` → `cat_draw_image`
- `ap_load_image` → `cat_load_image`
- `ap_draw_scrollbar` → `cat_draw_scrollbar`
- `ap_draw_progress_bar` → `cat_draw_progress_bar`
- `ap_get_content_rect` → `cat_get_content_rect`
- `ap_draw_screen_title` → `cat_draw_screen_title`
- `ap_draw_screen_title_centered` → `cat_draw_screen_title_centered`
- `ap_measure_wrapped_text_height` → `cat_measure_wrapped_text_height`
- `ap_text_scroll_*` → `cat_text_scroll_*`
- `ap_cache_*` → `cat_cache_*`
- `ap_draw_footer` → `cat_draw_footer`
- `ap_get_footer_height` → `cat_get_footer_height`
- `ap_set_footer_overflow_opts` → `cat_set_footer_overflow_opts`
- `ap_get_footer_overflow_opts` → `cat_get_footer_overflow_opts`
- `ap_show_footer_overflow` → `cat_show_footer_overflow`
- `ap_draw_status_bar` → `cat_draw_status_bar`
- `ap_get_status_bar_height` → `cat_get_status_bar_height`
- `ap_get_status_bar_width` → `cat_get_status_bar_width`
- `ap_log` → `cat_log`
- `ap_set_log_path` → `cat_set_log_path`
- `ap_resolve_log_path` → `cat_resolve_log_path`
- `ap_set_power_handler` → `cat_set_power_handler`
- `ap_set_cpu_speed` → `cat_set_cpu_speed`
- `ap_get_cpu_speed_mhz` → `cat_get_cpu_speed_mhz`
- `ap_get_cpu_temp_celsius` → `cat_get_cpu_temp_celsius`
- `ap_set_fan_mode` → `cat_set_fan_mode`
- `ap_get_fan_mode` → `cat_get_fan_mode`
- `ap_set_fan_speed` → `cat_set_fan_speed`
- `ap_get_fan_speed` → `cat_get_fan_speed`
- `ap_fade_begin_in` → `cat_fade_begin_in`
- `ap_fade_begin_out` → `cat_fade_begin_out`
- `ap_fade_draw` → `cat_fade_draw`
- `ap_get_error` → `cat_get_error`
- `ap_is_cancelled` → `cat_is_cancelled`
- `ap_request_frame` → `cat_request_frame`
- `ap_request_frame_in` → `cat_request_frame_in`

### Pass 4: Enums and constants

| Before | After |
|--------|-------|
| `AP_OK` | `CAT_OK` |
| `AP_ERROR` | `CAT_ERROR` |
| `AP_CANCELLED` | `CAT_CANCELLED` |
| `AP_REFERENCE_WIDTH` | `CAT_REFERENCE_WIDTH` |
| `AP_SCALE_DAMPING` | `CAT_SCALE_DAMPING` |
| `AP_FONT_BUMP_MAX` | `CAT_FONT_BUMP_MAX` |
| `AP_FONT_BUMP_REF_LOGICAL_W` | `CAT_FONT_BUMP_REF_LOGICAL_W` |
| `AP_FONT_BUMP_REF_LOGICAL_H` | `CAT_FONT_BUMP_REF_LOGICAL_H` |
| `AP_INPUT_REPEAT_DELAY` | `CAT_INPUT_REPEAT_DELAY` |
| `AP_INPUT_REPEAT_RATE` | `CAT_INPUT_REPEAT_RATE` |
| `AP_INPUT_DEBOUNCE` | `CAT_INPUT_DEBOUNCE` |
| `AP_AXIS_DEADZONE` | `CAT_AXIS_DEADZONE` |
| `AP_TEXT_SCROLL_SPEED` | `CAT_TEXT_SCROLL_SPEED` |
| `AP_TEXT_SCROLL_PAUSE_MS` | `CAT_TEXT_SCROLL_PAUSE_MS` |
| `AP_TEXTURE_CACHE_SIZE` | `CAT_TEXTURE_CACHE_SIZE` |
| `AP_MAX_COMBOS` | `CAT_MAX_COMBOS` |
| `AP__MAX_FOOTER_ITEMS` | `CAT__MAX_FOOTER_ITEMS` |
| `AP_MAX_LOG_LEN` | `CAT_MAX_LOG_LEN` |
| `AP_BTN_*` | `CAT_BTN_*` |
| `AP_FONT_*` | `CAT_FONT_*` |
| `AP_ALIGN_*` | `CAT_ALIGN_*` |
| `AP_ACTION_*` | `CAT_ACTION_*` |
| `AP_CPU_SPEED_*` | `CAT_CPU_SPEED_*` |
| `AP_FAN_MODE_*` | `CAT_FAN_MODE_*` |
| `AP_CLOCK_*` | `CAT_CLOCK_*` |
| `AP_COMBO_*` | `CAT_COMBO_*` |
| `AP_PLATFORM_*` | `CAT_PLATFORM_*` |
| `AP_OPT_*` | `CAT_OPT_*` |
| `AP_KB_*` | `CAT_KB_*` |
| `AP_SECTION_*` | `CAT_SECTION_*` |
| `AP_DETAIL_*` | `CAT_DETAIL_*` |
| `AP_QUEUE_*` | `CAT_QUEUE_*` |
| `AP_DL_*` | `CAT_DL_*` |
| `AP_FILE_PICKER_*` | `CAT_FILE_PICKER_*` |

## Pass 5: Type names

| Before | After |
|--------|-------|
| `ap_button` | `cat_button` |
| `ap_font_tier` | `cat_font_tier` |
| `ap_text_align` | `cat_text_align` |
| `ap_list_action` | `cat_list_action` |
| `ap_cpu_speed` | `cat_cpu_speed` |
| `ap_fan_mode` | `cat_fan_mode` |
| `ap_color` | `cat_color` (then redefined as `uint32_t` in Phase 2) |
| `ap_theme` | `cat_theme` (legacy, kept for transition) |
| `ap_input_event` | `cat_input_event` |
| `ap_text_scroll` | `cat_text_scroll` |
| `ap_footer_item` | `cat_footer_item` |
| `ap_footer_overflow_opts` | `cat_footer_overflow_opts` |
| `ap_status_bar_opts` | `cat_status_bar_opts` |
| `ap_fade` | `cat_fade` |
| `ap_cache_entry` | `cat_cache_entry` |
| `ap_texture_cache` | `cat_texture_cache` |
| `ap__seq_entry` | `cat__seq_entry` |
| `ap_combo_type` | `cat_combo_type` |
| `ap_combo_callback` | `cat_combo_callback` |
| `ap_combo` | `cat_combo` |
| `ap_combo_event` | `cat_combo_event` |
| `ap_config` | `cat_config` |
| `ap__state` | `cat__state` |
| `ap_list_item` | `cat_list_item` |
| `ap_list_opts` | `cat_list_opts` |
| `ap_list_footer_update_fn` | `cat_list_footer_update_fn` |
| `ap_list_result` | `cat_list_result` |
| `ap_option_type` | `cat_option_type` |
| `ap_option` | `cat_option` |
| `ap_options_item` | `cat_options_item` |
| `ap_options_list_opts` | `cat_options_list_opts` |
| `ap_options_list_result` | `cat_options_list_result` |
| `ap_keyboard_layout` | `cat_keyboard_layout` |
| `ap_keyboard_result` | `cat_keyboard_result` |
| `ap_url_keyboard_config` | `cat_url_keyboard_config` |
| `ap_message_opts` | `cat_message_opts` |
| `ap_confirm_result` | `cat_confirm_result` |
| `ap_selection_option` | `cat_selection_option` |
| `ap_selection_result` | `cat_selection_result` |
| `ap_process_fn` | `cat_process_fn` |
| `ap_process_opts` | `cat_process_opts` |
| `ap_detail_section_type` | `cat_detail_section_type` |
| `ap_detail_info_pair` | `cat_detail_info_pair` |
| `ap_detail_section` | `cat_detail_section` |
| `ap_detail_action` | `cat_detail_action` |
| `ap_detail_opts` | `cat_detail_opts` |
| `ap_detail_result` | `cat_detail_result` |
| `ap_file_picker_mode` | `cat_file_picker_mode` |
| `ap_file_picker_opts` | `cat_file_picker_opts` |
| `ap_file_picker_result` | `cat_file_picker_result` |
| `ap_queue_status` | `cat_queue_status` |
| `ap_queue_item` | `cat_queue_item` |
| `ap_queue_snapshot_fn` | `cat_queue_snapshot_fn` |
| `ap_queue_detail_fn` | `cat_queue_detail_fn` |
| `ap_queue_cancel_fn` | `cat_queue_cancel_fn` |
| `ap_queue_clear_fn` | `cat_queue_clear_fn` |
| `ap_queue_opts` | `cat_queue_opts` |
| `ap_download_status` | `cat_download_status` |
| `ap_download` | `cat_download` |
| `ap_download_result` | `cat_download_result` |
| `ap_download_opts` | `cat_download_opts` |

## Pass 6: Internal macros & helpers

| Before | After |
|--------|-------|
| `AP_S(base)` | `CAT_S(base)` |
| `AP_DS(base)` | `CAT_DS(base)` |
| `AP__PILL_SIZE` | `CAT__PILL_SIZE` |
| `AP__BUTTON_SIZE` | `CAT__BUTTON_SIZE` |
| `AP__BUTTON_MARGIN` | `CAT__BUTTON_MARGIN` |
| `AP__BUTTON_PADDING` | `CAT__BUTTON_PADDING` |
| `ap__lerpf` | `cat__lerpf` |
| `ap__clampf` | `cat__clampf` |
| `ap__g` (global state) | `cat__g` |
| `ap__error_buf` | `cat__error_buf` |
| `ap__set_error` | `cat__set_error` |
| `ap__clamp` | `cat__clamp` |
| All other `ap__` helpers | `cat__` prefix |

### Pass 7: Widget helper macros

| Before | After |
|--------|-------|
| `AP_LIST_ITEM` | `CAT_LIST_ITEM` |
| `AP_LIST_ITEM_BG` | `CAT_LIST_ITEM_BG` |
| `AP__PILL_ANIM_MS` | `CAT__PILL_ANIM_MS` |
| `AP__SCROLL_ANIM_MS` | `CAT__SCROLL_ANIM_MS` |

### Pass 8: Platform defines

| Before | After |
|--------|-------|
| `PLATFORM_TG5040` | keep (platform, not API) |
| `PLATFORM_MAC` | keep |
| etc. | keep |
| `AP_PLATFORM_NAME` | `CAT_PLATFORM_NAME` |
| `AP_PLATFORM_IS_DEVICE` | `CAT_PLATFORM_IS_DEVICE` |
| `AP_ENABLE_CURL` | `CAT_ENABLE_CURL` |

### Pass 9: Environment variables

| Before | After |
|--------|-------|
| `AP_STATUS_ASSETS_DIR` | `CAT_STATUS_ASSETS_DIR` |
| `AP_NEXTVAL_PATH` | `CAT_NEXTVAL_PATH` |
| `AP_BACKGROUND_PATH` | `CAT_BACKGROUND_PATH` |
| `AP_MINUI_SETTINGS_PATH` | `CAT_MINUI_SETTINGS_PATH` |
| `AP_PREVIEW_WIFI_STRENGTH` | `CAT_PREVIEW_WIFI_STRENGTH` |
| `AP_PREVIEW_BATTERY_PERCENT` | `CAT_PREVIEW_BATTERY_PERCENT` |
| `AP_PREVIEW_CHARGING` | `CAT_PREVIEW_CHARGING` |

---

## Step 2: File renames

```bash
mv include/apostrophe.h include/catastrophe.h
mv include/apostrophe_widgets.h include/catastrophe_widgets.h
```

Docker port files: check if they reference apostrophe in paths or names.

---

## Step 3: Source file header comments

Update the top comment block in every .c and .h file.
Example (catastrophe.h):
```
/*
 * Catastrophe — A C UI toolkit for Allium-based retro gaming handhelds
 * ...
 * https://github.com/<org>/catastrophe
 */
```

---

## Step 4: Makefile

- Update all references in comments (Apostrophe → Catastrophe, NextUI → Allium)
- Update `DIST_DIR` and `STAGING_DIR` paths if needed
- Update help text at `make help`
- Update Docker image references if needed
- Update env vars in `run-mac-nextui-%` → `run-mac-allium-%`
- Update PREVIEW_CACHE references (NextUI → Allium)

---

## Step 5: Documentation files

| File | Changes |
|------|---------|
| `README.md` | Rewrite for Catastrophe + Allium focus |
| `CHANGELOG.md` | First entry: "Renamed from Apostrophe to Catastrophe" |
| `CONTRIBUTING.md` | Update org/repo URLs |
| `LICENSE` | Update copyright (keep MIT) |
| `docs/API.md` | All `ap_` → `cat_` |
| `docs/WIDGETS.md` | All `ap_` → `cat_` |
| `docs/DEMO_COVERAGE.md` | Update references |
| `docs/GETTING_STARTED.md` | Update build instructions |
| `docs/GABAGOOL_PARITY_v2.9.6.md` | Update name, note archival |
| `docs/PORTING_FROM_GABAGOOL.md` | Update references |

---

## Execution Order

1. Run bulk `sed` replacements on all files (identifiers, macros, comments)
2. Rename the physical header files
3. Update `#include` directives in all .c files to use new names
4. Update Makefile comments and targets
5. Update docs
6. Verify with `rg "Apostrophe|apostrophe|AP_"` — should have 0 matches
7. Build test: `make mac` to verify compilation
8. Run test: `make run-mac-demo` to verify runtime
