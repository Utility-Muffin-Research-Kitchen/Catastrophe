#!/usr/bin/env python3
"""Build res/fonts/LeafHanSansKR/LeafHanSansKR-Regular.otf.

The Korean UI face is Source Han Sans with Korean glyph forms, cut down to
Latin and Hangul:

  - Input is Adobe's Korean region subset (release 2.005R,
    18_SourceHanSansKR.zip -> SubsetOTF/KR/SourceHanSansKR-Regular.otf). It
    carries every modern Hangul syllable, the Hangul Jamo blocks, Latin and the
    punctuation a Korean UI uses.
  - Han ideographs (Hanja), kana and bopomofo are dropped. A host running a
    Korean UI uses this as its UI face and leaves the Chinese face as the
    `cjk_font`, so a string with Han or kana in it still renders there. Hangul
    is not in the CJK detector, so Korean text stays on this face.
  - Layout features are reduced to ccmp/kern/liga/calt plus the jamo features
    (ljmo/vjmo/tjmo) that compose old Hangul from jamo. Korean forms are the
    defaults in this source, so dropping locl loses nothing Korean.

Source Han Sans is OFL 1.1 with the Reserved Font Name "Source", and a subset is a
Modified Version, so the family is renamed. The copyright, trademark, designer
and license records are kept as the OFL requires.

Usage: scripts/build_leaf_han_sans_kr.py <SourceHanSansKR-Regular.otf>
Requires fontTools.
"""

import os
import sys

from fontTools import subset
from fontTools.ttLib import TTFont

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "res", "fonts", "LeafHanSansKR", "LeafHanSansKR-Regular.otf")

FAMILY = "Leaf Han Sans KR"
PS_NAME = "LeafHanSansKR-Regular"

HAN_AND_KANA = (
    (0x2E80, 0x2EFF),    # CJK Radicals Supplement
    (0x2F00, 0x2FDF),    # Kangxi Radicals
    (0x2FF0, 0x2FFF),    # Ideographic Description
    (0x3005, 0x3007),    # Iteration mark, closing mark, ideographic zero
    (0x3021, 0x3029),    # Hangzhou numerals
    (0x3031, 0x303B),    # Kana repeat marks, more Hangzhou numerals
    (0x3040, 0x30FF),    # Hiragana, Katakana
    (0x3100, 0x312F),    # Bopomofo
    (0x3190, 0x319F),    # Kanbun
    (0x31A0, 0x31FF),    # Bopomofo Extended, CJK Strokes, Katakana Extensions
    (0x3220, 0x3243),    # Parenthesized ideographs
    (0x3280, 0x32B0),    # Circled ideographs
    (0x32C0, 0x32FF),    # Telegraph months, circled katakana
    (0x3300, 0x3357),    # Squared katakana
    (0x3400, 0x4DBF),    # CJK Unified Ideographs Extension A
    (0x4E00, 0x9FFF),    # CJK Unified Ideographs
    (0xF900, 0xFAFF),    # CJK Compatibility Ideographs
    (0xFF66, 0xFF9F),    # Halfwidth katakana
    (0x1B000, 0x1B16F),  # Kana Supplement and Extension
    (0x1F200, 0x1F2FF),  # Enclosed Ideographic Supplement
    (0x20000, 0x3FFFF),  # CJK Extensions B and later, compatibility supplement
)

SYLLABLES = range(0xAC00, 0xD7A4)   # all 11,172 modern Hangul syllables

FEATURES = ["ccmp", "kern", "liga", "calt", "ljmo", "vjmo", "tjmo"]


def is_han_or_kana(cp):
    return any(lo <= cp <= hi for lo, hi in HAN_AND_KANA)


def rename(font):
    name = font["name"]
    # Drop every localized family/style/full name: the Korean ones spell the
    # reserved name in Korean.
    name.names = [r for r in name.names
                  if r.nameID not in (1, 2, 3, 4, 6, 16, 17) or r.langID == 0x409]
    version = name.getDebugName(5).split(";")[0].replace("Version ", "")
    name.setName(FAMILY, 1, 3, 1, 0x409)
    name.setName("Regular", 2, 3, 1, 0x409)
    name.setName(f"{version};{PS_NAME}", 3, 3, 1, 0x409)
    name.setName(f"{FAMILY} Regular", 4, 3, 1, 0x409)
    name.setName(PS_NAME, 6, 3, 1, 0x409)

    cff = font["CFF "].cff
    top = cff[0]
    old = cff.fontNames[0]
    cff.fontNames = [PS_NAME]
    top.FullName = f"{FAMILY} Regular"
    top.FamilyName = FAMILY
    for fd in getattr(top, "FDArray", []) or []:
        if getattr(fd, "FontName", "").startswith(old):
            fd.FontName = PS_NAME + fd.FontName[len(old):]


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    src = sys.argv[1]

    source_cmap = TTFont(src, lazy=True).getBestCmap()
    missing = [cp for cp in SYLLABLES if cp not in source_cmap]
    if missing:
        sys.exit(f"input lacks {len(missing)} Hangul syllables; is it the KR subset?")
    keep = sorted(cp for cp in source_cmap if not is_han_or_kana(cp))

    options = subset.Options()
    options.layout_features = FEATURES
    options.name_IDs = ["*"]
    options.name_languages = ["*"]
    options.notdef_outline = True
    font = TTFont(src)
    subsetter = subset.Subsetter(options)
    subsetter.populate(unicodes=keep)
    subsetter.subset(font)
    rename(font)

    cmap = set(font.getBestCmap())
    lost = [cp for cp in SYLLABLES if cp not in cmap]
    if lost:
        sys.exit(f"{len(lost)} Hangul syllables would be lost")
    stray = [cp for cp in cmap if is_han_or_kana(cp)]
    if stray:
        sys.exit(f"{len(stray)} Han or kana codepoints were kept")

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    font.save(OUT)
    print(f"{OUT}: {os.path.getsize(OUT)} bytes, {font['maxp'].numGlyphs} glyphs, "
          f"{len(cmap)} codepoints")


if __name__ == "__main__":
    main()
