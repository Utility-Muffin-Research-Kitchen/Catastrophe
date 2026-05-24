# Bundled fonts

| Family | Weight(s) | License | Use |
|---|---|---|---|
| Inter | Regular | OFL 1.1 | Default Latin UI font (legacy `font.ttf`) |
| Space Grotesk | Regular / Medium / Bold | OFL 1.1 | Latin UI font for Jawaka themes |
| Source Han Sans CN | Heavy | OFL 1.1 | CJK fallback font (`cjk_font` slot) |

All fonts are SIL Open Font License 1.1 — free to bundle and redistribute.
License text lives in `OFL.txt` (or `LICENSE.txt`) inside each family folder.

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
