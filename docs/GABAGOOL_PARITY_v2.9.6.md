# Gabagool Parity Matrix (`v2.9.6`)

This document tracks current Catastrophe parity against Gabagool `v2.9.6`.

## Sources

- `go doc` output for `github.com/BrandonKowalski/gabagool/v2/pkg/gabagool` and `.../constants`
- Usage scan of a Gabagool-based app project
- Catastrophe public headers in `include/catastrophe.h` and `include/catastrophe_widgets.h`

## Status Legend

- `Implemented`
- `Implemented (behavior differs)`
- `Missing`

## Priority This Cycle

| Area | Status | Notes |
|---|---|---|
| Core directional hold repeat | Implemented | Added digital held repeat in input core for keyboard/D-pad/button-mapped directions; dedupes against hat/axis repeats. |
| Runtime repeat API (`cat_set_input_repeat`) | Implemented | Delay/rate now configurable at runtime, initialized from defaults. |
| List explicit action outcomes | Implemented | Added triggered/secondary/confirmed/tertiary actions. `CAT_ACTION_CUSTOM` kept as alias of `CAT_ACTION_TRIGGERED`. |
| List scroll restoration (`visible_start_index`) | Implemented | Input and output support added to list options/result. |
| Options list action/scroll parity | Implemented | Added `initial_selected_index`, `visible_start_index`, `action_button`, `secondary_action_button` plus result scroll position. |
| Example crash logging | Implemented | SDLReader-style launcher logging + app `log_path` wiring in examples. |

## APIs Used by Catastrophe-based apps

| Gabagool API usage | Catastrophe parity | Status |
|---|---|---|
| `DefaultListOptions` + `List` (`ReorderButton`, `ActionButton`) | `cat_list_default_opts` + `cat_list` (`reorder_button`, `action_button`) | Implemented |
| `ListAction*` outcomes | `cat_list_action` expanded | Implemented |
| `OptionsList` (`ConfirmButton`) | `cat_options_list` (`confirm_button`) | Implemented |
| `OptionsList` (`ActionButton`, `SecondaryActionButton`, index restoration) | Added in this cycle | Implemented |
| `OptionsList` immediate return on standard-option change | `cat_options_list` (`return_on_option_change`, `CAT_ACTION_OPTION_CHANGED`) | Implemented |
| `Keyboard` | `cat_keyboard` | Implemented |
| `ProcessMessage` (progress, interrupt, dynamic message, message lines) | `cat_process_message` | Implemented (behavior differs) |
| `ConfirmationMessage` | `cat_confirmation` | Implemented |
| Detail/info screen secondary action | `cat_detail_screen` (`CAT_DETAIL_SECONDARY_ACTION`) | Implemented |
| `SetLogPath`/`LogPath` option | `cat_set_log_path` / `cat_config.log_path` | Implemented |

## Broader Parity Matrix

| Gabagool capability | Catastrophe equivalent | Status | Notes |
|---|---|---|---|
| Init/close lifecycle | `cat_init`/`cat_quit` | Implemented | |
| Input mapping + face button flip | `cat_poll_input`, `cat_flip_face_buttons` | Implemented | |
| Directional repeat control | `cat_set_input_repeat` | Implemented | |
| Debounce control | `cat_set_input_delay` | Implemented | |
| List widget | `cat_list` | Implemented | Includes reorder, help, images, multi-select. |
| Options list widget | `cat_options_list` | Implemented | Includes standard/keyboard/clickable/color picker and immediate return on standard-option change. |
| Keyboard + URL keyboard | `cat_keyboard`, `cat_url_keyboard` | Implemented | |
| Selection message | `cat_selection` | Implemented | |
| Confirmation message | `cat_confirmation` | Implemented | |
| Process message | `cat_process_message` | Implemented (behavior differs) | Generic Go return values are mapped to C callback/userdata patterns. |
| Idle rendering | `cat_present()` + `cat_request_frame()` | Implemented | Dirty-flag system: thread sleeps via `SDL_WaitEventTimeout` when idle, wakes on input or scheduled redraw. 60fps during animations, near-zero CPU when static. |
| Download manager | `cat_download_manager` | Implemented | |
| Detail/info screen | `cat_detail_screen` | Implemented | Supports primary (A) and secondary (Y) action exits. |
| Color picker | `cat_color_picker` | Implemented | |
| Help overlay | `cat_show_help_overlay` | Implemented | |
| Status bar | `cat_draw_status_bar` + per-widget status options | Implemented | |
| Theme loading | `cat_stylesheet_load_theme` | Implemented | Loads from Allium-format stylesheet.json |
| Logging | `cat_log`, `cat_set_log_path`, `cat_config.log_path` | Implemented | |
| Combo registration/polling | `cat_register_chord`, `cat_register_sequence`, `cat_poll_combo` + `_ex` variants | Implemented | Polling + optional callbacks via `_ex` variants. `cat_combo_event` includes `type` field. |
| Window visibility controls (`ShowWindow`, `HideWindow`) | `cat_show_window`, `cat_hide_window` | Implemented | |
| Input logger/mapping bytes APIs | None | Missing | No direct equivalent for Gabagool input remap byte loader. |
| Infrastructure error typing helpers | None | Missing | Catastrophe uses integer return codes + `cat_get_error()`. |

## Out of Scope

1. Add user-configurable input mapping loader equivalent to `SetInputMappingBytes`.
2. Add optional infrastructure-error style wrappers around current integer return model for migration ergonomics.
