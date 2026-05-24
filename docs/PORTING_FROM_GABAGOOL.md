# Porting from Gabagool (Go) to Catastrophe (C)

This guide helps you migrate UI code from [Gabagool](https://github.com/LoveRetro/gabagool) to Catastrophe.

This guide targets Catastrophe **v1.1.0** (2026-03-30).

## Overview of Changes

| Aspect | Gabagool (Go) | Catastrophe (C) |
|--------|--------------|----------------|
| Language | Go | C11 |
| Prefix | `gabagool.` | `cat_` / `CAT_` |
| Init | `gabagool.NewApp()` | `cat_init(&cfg)` |
| Cleanup | `defer app.Cleanup()` | `cat_quit()` |
| Generics | `ProcessMessage[T]` | `void*` + function pointer |
| Goroutines | `go func()` | `pthread_create` |
| Error handling | `error` / `ErrCancelled` | `CAT_OK` / `CAT_CANCELLED` |
| Scaling | `app.Scale()` | `CAT_S()` macro |
| Theme | `app.Theme` | `cat_get_theme()` |

## Initialization

### Gabagool
```go
app := gabagool.NewApp()
defer app.Cleanup()
```

### Catastrophe
```c
cat_config cfg = {
    .window_title = "My App",
    .font_path    = "font.ttf",
};
cat_init(&cfg);
// ... at end:
cat_quit();
```

## List

### Gabagool
```go
opts := &gabagool.ListOptions{
    Title:       "Pick One",
    Items:       items,
    MultiSelect: false,
    FooterHints: []gabagool.FooterHint{
        {Button: gabagool.ButtonB, Label: "Back"},
        {Button: gabagool.ButtonA, Label: "Select", IsConfirm: true},
    },
}
result, err := app.List(opts)
if errors.Is(err, gabagool.ErrCancelled) { return }
```

### Catastrophe
```c
cat_footer_item footer[] = {
    { .button = CAT_BTN_B, .label = "Back" },
    { .button = CAT_BTN_A, .label = "Select", .is_confirm = true },
};
cat_list_opts opts = cat_list_default_opts("Pick One", items, count);
opts.footer       = footer;
opts.footer_count = 2;

cat_list_result result;
int rc = cat_list(&opts, &result);
if (rc == CAT_CANCELLED) return;
```

## Options List (Settings)

### Gabagool
```go
opts := &gabagool.OptionsListOptions{
    Title: "Settings",
    Items: []gabagool.OptionsItem{
        {Label: "Volume", Type: gabagool.OptionStandard,
         Options: []string{"Off", "Low", "Mid", "High"},
         Selected: 2},
    },
}
result, _ := app.OptionsList(opts)
```

### Catastrophe
```c
cat_option vol_opts[] = {
    { .label = "Off", .value = "0" },
    { .label = "Low", .value = "25" },
    { .label = "Mid", .value = "50" },
    { .label = "High", .value = "75" },
};
cat_options_item items[] = {
    { .label = "Volume", .type = CAT_OPT_STANDARD,
      .options = vol_opts, .option_count = 4, .selected_option = 2 },
};
cat_options_list_opts opts = {
    .title = "Settings", .items = items, .item_count = 1,
};
cat_options_list_result result;
cat_options_list(&opts, &result);
```

**Key differences**: In Gabagool, options are just strings. In Catastrophe, each option has a `.label` (displayed) and `.value` (returned).
If you want Gabagool-style "A saves settings, Left/Right changes values" behavior, set `.confirm_button = CAT_BTN_A` and rely on Left/Right for `CAT_OPT_STANDARD` changes. For `CAT_OPT_KEYBOARD` and `CAT_OPT_COLOR_PICKER` items, A still opens the sub-editor first, but confirming inside the sub-editor will also confirm and exit the options list. Cancelling the sub-editor returns to the list without confirming.
If your caller needs every successful standard-option change to return immediately, set `.return_on_option_change = true`. Catastrophe will update `selected_option`, then return with `result.action == CAT_ACTION_OPTION_CHANGED`, leaving `CAT_ACTION_TRIGGERED` reserved for `action_button`.

## Detail / Info Screen

If your port expects a secondary detail action, `cat_detail_screen()` returns `CAT_DETAIL_ACTION` on A, `CAT_DETAIL_SECONDARY_ACTION` on Y, and `CAT_DETAIL_BACK` on B. Add a matching Y footer hint if you want that secondary action to be visible in the UI.

## Keyboard

### Gabagool
```go
text, err := app.Keyboard("initial", "Enter name:")
```

### Catastrophe
```c
cat_keyboard_result result;
int rc = cat_keyboard("initial", "Enter name:", CAT_KB_GENERAL, &result);
if (rc == CAT_OK) {
    // result.text contains the string
}
```

The keyboard now matches Gabagool's 5-row layout:
- Row 0: Numbers 1-0 + backspace
- Row 1: QWERTY (centered)
- Row 2: ASDF + enter
- Row 3: Shift + ZXCV + symbol toggle
- Row 4: Space bar

**Button mapping** is Gabagool-compatible: B=backspace, X=space, Y=exit, Select=shift, Start=confirm, L1/R1=cursor, Menu=help.

**URL Keyboard** supports configurable shortcuts:
```c
cat_keyboard_result result;
int rc = cat_url_keyboard("https://", "Enter URL:", NULL, &result);
```

## Process Message (Async Worker)

### Gabagool
```go
result, err := gabagool.ProcessMessage[MyResult](app, &gabagool.ProcessMessageOptions{
    Message:      "Working...",
    ShowProgress: true,
    Task: func(progress *atomic.Value, interrupt *atomic.Bool) (MyResult, error) {
        for i := 0; i <= 100; i++ {
            progress.Store(float64(i) / 100.0)
            time.Sleep(30 * time.Millisecond)
        }
        return MyResult{Data: "done"}, nil
    },
})
```

### Catastrophe
```c
static int my_worker(void *userdata) {
    float *progress = (float *)userdata;
    for (int i = 0; i <= 100; i++) {
        *progress = (float)i / 100.0f;
        SDL_Delay(30);
    }
    return CAT_OK;
}

float progress = 0;
cat_process_opts opts = {
    .message       = "Working...",
    .show_progress = true,
    .progress      = &progress,
};
int rc = cat_process_message(&opts, my_worker, &progress);
```

**Key differences**:
- Go generics → C `void*` userdata pattern
- `atomic.Value` → `float*` pointer (worker writes, UI reads)
- `atomic.Bool` for cancel → `int*` interrupt_signal
- goroutine → pthread (managed internally)

## Confirmation

### Gabagool
```go
ok, err := app.Confirm("Are you sure?")
```

### Catastrophe
```c
cat_footer_item footer[] = {
    { .button = CAT_BTN_B, .label = "No" },
    { .button = CAT_BTN_A, .label = "Yes", .is_confirm = true },
};
cat_message_opts opts = {
    .message = "Are you sure?",
    .footer = footer, .footer_count = 2,
};
cat_confirm_result result;
cat_confirmation(&opts, &result);
if (result.confirmed) { /* yes */ }
```

## Selection

### Gabagool
```go
idx, err := app.Selection("Choose:", []string{"Easy", "Normal", "Hard"})
```

### Catastrophe
```c
cat_selection_option opts[] = {
    { .label = "Easy",   .value = "1" },
    { .label = "Normal", .value = "2" },
    { .label = "Hard",   .value = "3" },
};
cat_selection_result result;
cat_selection("Choose:", opts, 3, footer, 2, &result);
// result.selected_index
```

## Scaling

### Gabagool
```go
margin := app.Scale(20)
```

### Catastrophe
```c
int margin = CAT_S(20);
```

## Theme Colors

### Gabagool
```go
color := app.Theme.Highlight
```

### Catastrophe
```c
cat_color color = cat_get_theme()->highlight;
```

## Button Constants

| Gabagool | Catastrophe |
|----------|-----------|
| `gabagool.ButtonA` | `CAT_BTN_A` |
| `gabagool.ButtonB` | `CAT_BTN_B` |
| `gabagool.ButtonX` | `CAT_BTN_X` |
| `gabagool.ButtonY` | `CAT_BTN_Y` |
| `gabagool.ButtonUp` | `CAT_BTN_UP` |
| `gabagool.ButtonDown` | `CAT_BTN_DOWN` |
| `gabagool.ButtonLeft` | `CAT_BTN_LEFT` |
| `gabagool.ButtonRight` | `CAT_BTN_RIGHT` |
| `gabagool.ButtonL1` | `CAT_BTN_L1` |
| `gabagool.ButtonL2` | `CAT_BTN_L2` |
| `gabagool.ButtonR1` | `CAT_BTN_R1` |
| `gabagool.ButtonR2` | `CAT_BTN_R2` |
| `gabagool.ButtonStart` | `CAT_BTN_START` |
| `gabagool.ButtonSelect` | `CAT_BTN_SELECT` |
| `gabagool.ButtonMenu` | `CAT_BTN_MENU` |
| `gabagool.ButtonPower` | `CAT_BTN_POWER` |

## Font Tiers

| Gabagool | Catastrophe | Base Size | Tier name    |
|----------|-----------|-----------|--------------|
| `gabagool.FontExtraLarge` | `CAT_FONT_EXTRA_LARGE` | 24 × device_scale | — |
| `gabagool.FontLarge` | `CAT_FONT_LARGE` | 16 × device_scale | FONT_LARGE |
| `gabagool.FontMedium` | `CAT_FONT_MEDIUM` | 14 × device_scale | FONT_MEDIUM |
| `gabagool.FontSmall` | `CAT_FONT_SMALL` | 12 × device_scale | FONT_SMALL |
| `gabagool.FontTiny` | `CAT_FONT_TINY` | 10 × device_scale | FONT_TINY |
| `gabagool.FontMicro` | `CAT_FONT_MICRO` | 7 × device_scale | FONT_MICRO |

## Common Patterns

### Error Handling

```go
// Gabagool
result, err := app.List(opts)
if err != nil {
    if errors.Is(err, gabagool.ErrCancelled) {
        return  // user backed out
    }
    log.Fatal(err)
}
```

```c
// Catastrophe
int rc = cat_list(&opts, &result);
if (rc == CAT_CANCELLED) return;   // user backed out
if (rc == CAT_ERROR) exit(1);      // something broke
// CAT_OK — success
```

### Menu Loop

```go
// Gabagool
for {
    result, err := app.List(menuOpts)
    if errors.Is(err, gabagool.ErrCancelled) { break }
    // handle result...
}
```

```c
// Catastrophe
while (1) {
    int rc = cat_list(&opts, &result);
    if (rc != CAT_OK || result.action == CAT_ACTION_BACK) break;
    // handle result...
}
```

## Window Visibility

### Gabagool
```go
app.ShowWindow()
app.HideWindow()
```

### Catastrophe
```c
cat_show_window();
cat_hide_window();
```

Both are no-ops when called before `cat_init()`.

## Combos

Gabagool supports `ChordOptions.OnTrigger`/`OnRelease` callbacks and `SequenceOptions.OnTrigger`.
Catastrophe matches this with two registration styles:

### Polling (classic)

Register with `cat_register_chord` / `cat_register_sequence` and drain `cat_poll_combo()` each frame.

#### Gabagool
```go
app.RegisterChord(&gabagool.ChordOptions{
    ID:      "shoulders",
    Buttons: []gabagool.Button{gabagool.ButtonL1, gabagool.ButtonR1},
    Window:  150 * time.Millisecond,
})
// poll in loop:
for event := range app.ComboEvents() {
    if event.Triggered { fmt.Println("triggered:", event.ID) }
}
```

#### Catastrophe
```c
cat_button shoulders[] = { CAT_BTN_L1, CAT_BTN_R1 };
cat_register_chord("shoulders", shoulders, 2, 150);

/* In your main loop: */
cat_combo_event combo;
while (cat_poll_combo(&combo)) {
    const char *kind = (combo.type == CAT_COMBO_CHORD) ? "chord" : "seq";
    if (combo.triggered)
        printf("Triggered [%s]: %s\n", kind, combo.id);
}
```

### Callbacks (_ex variants)

Register with `cat_register_chord_ex` / `cat_register_sequence_ex` to get callbacks that fire
automatically at trigger/release time — no poll loop required. Polling still works alongside
callbacks; both are additive.

#### Gabagool
```go
app.RegisterChord(&gabagool.ChordOptions{
    ID:      "shoulders",
    Buttons: []gabagool.Button{gabagool.ButtonL1, gabagool.ButtonR1},
    Window:  150 * time.Millisecond,
    OnTrigger: func(id string) { fmt.Println("triggered:", id) },
    OnRelease: func(id string) { fmt.Println("released:", id) },
})
```

#### Catastrophe
```c
void on_trigger(const char *id, cat_combo_type type, void *userdata) {
    printf("triggered: %s\n", id);
}
void on_release(const char *id, cat_combo_type type, void *userdata) {
    printf("released: %s\n", id);
}

cat_button shoulders[] = { CAT_BTN_L1, CAT_BTN_R1 };
cat_register_chord_ex("shoulders", shoulders, 2, 150, on_trigger, on_release, NULL);

/* Sequences: on_trigger only (no release phase) */
cat_button uudd[] = { CAT_BTN_UP, CAT_BTN_UP, CAT_BTN_DOWN, CAT_BTN_DOWN };
cat_register_sequence_ex("uudd", uudd, 4, 500, false, on_trigger, NULL);
```

The `cat_combo_type` argument in the callback is `CAT_COMBO_CHORD` or `CAT_COMBO_SEQUENCE`, letting
a single callback handle both kinds.

## Download Manager

Gabagool's `DownloadManager` maps to Catastrophe's `cat_download_manager`. Both use concurrent downloads with progress reporting.

### Gabagool
```go
downloads := []gabagool.Download{
    {URL: "https://example.com/a.zip", DestPath: "/tmp/a.zip", Label: "a.zip"},
    {URL: "https://example.com/b.zip", DestPath: "/tmp/b.zip", Label: "b.zip"},
}
result, err := app.DownloadManager(downloads, &gabagool.DownloadOptions{
    MaxConcurrent: 3,
})
```

### Catastrophe
```c
cat_download downloads[] = {
    { .url = "https://example.com/a.zip", .dest_path = "/tmp/a.zip", .label = "a.zip" },
    { .url = "https://example.com/b.zip", .dest_path = "/tmp/b.zip", .label = "b.zip" },
};
cat_download_opts opts = { .max_concurrent = 3 };
cat_download_result result;
cat_download_manager(downloads, 2, &opts, &result);
```

**Key differences**:
- Go's HTTP client → libcurl (compile with `-DAP_ENABLE_CURL`, link with `-lcurl`)
- Go's goroutines → pthreads (managed internally by the widget)
- Status is tracked per-job via the `cat_download` struct fields (`status`, `progress`, `speed_bps`)
