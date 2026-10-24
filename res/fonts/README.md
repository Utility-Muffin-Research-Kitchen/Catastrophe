# Bundled fonts

Selectable UI families (Settings > Appearance > Layout > Font). Nunito is the
default. Every font here is SIL Open Font License 1.1; the license text lives in
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
| Source Han Sans CN | Regular | OFL 1.1 | CJK fallback (`cjk_font` slot) |
| Leaf Han Sans JP | Regular | OFL 1.1 | Japanese CJK face (`CAT_CJK_FONT_PATH`) |
| Leaf Han Sans KR | Regular | OFL 1.1 | Hangul face (`CAT_HANGUL_FONT_PATH`), Korean UI face |

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
"cjk_font": { "path": "fonts/SourceHanSansCN/SourceHanSansCN-Regular.otf", "size": 36 }
```

The font loader searches `./res/fonts/<path>` and `../res/fonts/<path>` automatically,
so themes do not need to copy font files into their own directories.

## Japanese glyph forms (`CAT_CJK_FONT_PATH`)

Han characters shared by Chinese and Japanese are drawn differently in each
(compare 直 or 骨), and the stylesheet's `cjk_font` is the Simplified Chinese face.
A host running a Japanese UI sets `CAT_CJK_FONT_PATH` to
`fonts/LeafHanSansJP/LeafHanSansJP-Regular.otf`, which then takes over from the
stylesheet for every CJK string. The path resolves the same way as `cjk_font`,
and it is read on each lookup, so a host that switches language in-process only
has to update the variable.

Leaf Han Sans JP is Source Han Sans (Japanese defaults, release 2.005R) with
Hangul and regional alternates removed. It keeps every codepoint the Chinese face
has, so Chinese titles still render under a Japanese UI. Source Han Sans reserves
the font name "Source", so the modified font is renamed. Rebuild it with
`scripts/build_leaf_han_sans_jp.py`, whose header documents the input file.

## Hangul (`CAT_HANGUL_FONT_PATH`)

None of the UI families has Hangul, and neither CJK face does. Catastrophe sends
any string with Hangul in it to `fonts/LeafHanSansKR/LeafHanSansKR-Regular.otf`,
the way it sends CJK strings to `cjk_font`, so a Korean game title or the
language name "한국어" renders in every UI language. `CAT_HANGUL_FONT_PATH`
overrides the path, which resolves the same way as `cjk_font`. Chinese and
Japanese text never holds Hangul, so it is unaffected. A string that mixes Hangul
with Han goes to this face, and its Han is missing, the rarer of the two cases.

A host running a Korean UI can also use the face as its UI face, so Latin-only
strings match the Korean ones. It leaves `cjk_font` as it is, and Chinese or
Japanese titles still go there.

Leaf Han Sans KR is Source Han Sans KR (release 2.005R) cut down to Latin and
Hangul: every modern syllable and the jamo blocks, with Han ideographs, kana and
bopomofo removed. That keeps it near 2 MB. Source Han Sans reserves the font name
"Source", so the modified font is renamed. Rebuild it with
`scripts/build_leaf_han_sans_kr.py`, whose header documents the input file.
