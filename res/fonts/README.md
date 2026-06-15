# Bundled fonts

Selectable UI families (Settings > Appearance > Layout > Font). Nunito is the
default. All ten are SIL Open Font License 1.1; the license text lives in
`OFL.txt` (or `LICENSE.txt`) inside each family folder.

| Family | Weight(s) | License | Use |
|---|---|---|---|
| Space Grotesk | Regular / Medium / Bold | OFL 1.1 | UI family |
| Inter | Regular | OFL 1.1 | UI family (also the legacy `font.ttf` slot name) |
| Rounded M+ | Bold | OFL 1.1 | UI family |
| Nunito | Bold | OFL 1.1 | Default UI family |
| Baloo 2 | Bold | OFL 1.1 | UI family |
| Fredoka | Bold | OFL 1.1 | UI family |
| Lexend | Bold | OFL 1.1 | UI family |
| IBM Plex Sans | Bold | OFL 1.1 | UI family |
| Noto Sans | Bold | OFL 1.1 | UI family |
| Source Han Sans CN | Heavy | OFL 1.1 | CJK fallback (`cjk_font` slot) |

## Symbol fallback (`font.ttf`)

The always-bundled base `font.ttf` is **RoundedMplus 1c Nerd Font** - a Nerd
Fonts-patched build of Rounded M+. `cat_get_symbol_font` falls back to it so UI
symbols the themed family lacks (keyboard glyphs like the shift arrow) still
render in every theme. The underlying Rounded M+ is OFL 1.1; the Nerd Fonts
glyph patch is MIT.

## Referencing fonts from a stylesheet

Themes can use any bundled font by pointing at its path relative to `res/fonts/`:

```json
"ui": {
  "ui_font": { "path": "fonts/SpaceGrotesk/SpaceGrotesk-Regular.ttf", "size": 36 }
},
"cjk_font": { "path": "fonts/SourceHanSansCN/SourceHanSansCN-Heavy.otf", "size": 36 }
```

The font loader searches `./res/fonts/<path>` and `../res/fonts/<path>` automatically,
so themes do not need to copy font files into their own directories.
