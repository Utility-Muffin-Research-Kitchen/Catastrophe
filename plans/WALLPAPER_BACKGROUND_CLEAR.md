# Wallpaper Background Fallback Plan

## Background Color Support (Already Exists)

The stylesheet already supports `ui.background_color` (`catastrophe.h:368`, default `#000000`). It's mapped to `ap_theme.background` in `cat__stylesheet_to_ap_theme()` (`:1562`) and used by `cat_clear_screen()` (`:2792`). No schema changes needed.

## The Bug: Stale Wallpaper

### Problem

In `cat_stylesheet_apply()` (`:1594-1607`), wallpaper is loaded only when `s->wallpaper[0]` is non-empty:

```c
if (s->wallpaper[0]) {
    /* resolve path, cat_reload_background(wpath) */
}
```

When the new theme has **no wallpaper** (empty string), this block is skipped entirely. The old `cat__g.bg_texture` from the previous theme persists. `cat_draw_background()` (`:2861`) checks `cat__g.bg_texture` first and blits it — so the old theme's wallpaper shows through.

### Root cause

`cat_reload_background()` (`:1791`) always creates a new texture. There is no code path that *destroys* the existing texture and sets it to NULL, which is the only way `cat_draw_background()` would fall through to the solid `cat_clear_screen()` path.

### Fix (single file, `include/catastrophe.h`)

In `cat_stylesheet_apply()`, add an `else` branch:

```c
/* Load wallpaper if present */
if (s->wallpaper[0]) {
    /* existing resolve-and-load code — unchanged */
} else {
    /* Clear stale wallpaper so cat_draw_background() falls through
       to the theme's solid ui.background_color */
    if (cat__g.bg_texture) {
        SDL_DestroyTexture(cat__g.bg_texture);
        cat__g.bg_texture = NULL;
    }
}
```

That's it. One `else` block, one file. No schema changes.

### Impact

| Scenario | Before | After |
|---|---|---|
| Theme with wallpaper | Works (texture replaced) | Unchanged |
| Theme with no wallpaper, prev had one | **Bug: old wallpaper persists** | Falls through to `ui.background_color` (black if unspecified) |
| Theme with no wallpaper, prev also none | Falls through to `ui.background_color` | Unchanged |
| First launch, no state file, no wallpaper | bg_texture is NULL, solid color | Unchanged |

### Edge cases considered

- **cat_reload_background failure**: If `cat_load_image` fails, the old texture is already destroyed in `cat_reload_background` (`:1812-1814`), so `bg_texture` becomes NULL anyway. No double-free risk.
- **Wallpaper picker in demo**: The demo calls `cat_reload_background()` directly, which handles its own texture swap. Only `cat_stylesheet_apply()` needs the clear-on-empty-wallpaper path.
