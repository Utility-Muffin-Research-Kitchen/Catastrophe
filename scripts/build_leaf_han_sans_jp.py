#!/usr/bin/env python3
"""Build res/fonts/LeafHanSansJP/LeafHanSansJP-Regular.otf.

The Japanese UI face is Source Han Sans with Japanese glyph forms as the default,
cut down to what a handheld UI can reach:

  - Input is Adobe's language-specific Japanese OTF (release 2.005R,
    07_SourceHanSansJ.zip -> OTF/Japanese/SourceHanSans-Regular.otf). Unlike the
    region subset SourceHanSansJP, it maps every Han codepoint the Chinese face
    covers, so a Chinese game title still renders under a Japanese UI.
  - Hangul is dropped. The CJK detector never routes Korean to this face, and the
    Chinese face does not carry it either.
  - Layout features are reduced to ccmp/kern/liga/calt. Dropping locl removes
    the thousands of alternate glyphs for other regions; Japanese forms are the
    defaults in this source, so nothing Japanese is lost.
  - Every codepoint of the bundled Chinese face is kept, so switching a UI from
    Chinese to Japanese can never introduce tofu.

Source Han Sans is OFL 1.1 with the Reserved Font Name "Source", and a subset is a
Modified Version, so the family is renamed. The copyright, trademark, designer
and license records are kept as the OFL requires.

Usage: scripts/build_leaf_han_sans_jp.py <SourceHanSans-Regular.otf>
Requires fontTools.
"""

import os
import sys

from fontTools import subset
from fontTools.ttLib import TTFont

HERE = os.path.dirname(os.path.abspath(__file__))
FONTS = os.path.join(HERE, "..", "res", "fonts")
CN_FACE = os.path.join(FONTS, "SourceHanSansCN", "SourceHanSansCN-Regular.otf")
OUT = os.path.join(FONTS, "LeafHanSansJP", "LeafHanSansJP-Regular.otf")

FAMILY = "Leaf Han Sans JP"
PS_NAME = "LeafHanSansJP-Regular"

HANGUL = (
    (0x1100, 0x11FF),  # Jamo
    (0x3130, 0x318F),  # Compatibility Jamo
    (0x3200, 0x321E),  # Parenthesized Hangul
    (0x3260, 0x327E),  # Circled Hangul
    (0xA960, 0xA97F),  # Jamo Extended-A
    (0xAC00, 0xD7AF),  # Syllables
    (0xD7B0, 0xD7FF),  # Jamo Extended-B
)

FEATURES = ["ccmp", "kern", "liga", "calt"]


def is_hangul(cp):
    return any(lo <= cp <= hi for lo, hi in HANGUL)


def rename(font):
    name = font["name"]
    # Drop every localized family/style/full name: the Japanese ones spell the
    # reserved name in Japanese.
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
    cn = set(TTFont(CN_FACE, lazy=True).getBestCmap())
    keep = sorted(cp for cp in source_cmap if cp in cn or not is_hangul(cp))

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

    lost = cn - set(font.getBestCmap())
    if lost:
        sys.exit(f"{len(lost)} codepoints of the Chinese face would be lost")

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    font.save(OUT)
    print(f"{OUT}: {os.path.getsize(OUT)} bytes, {font['maxp'].numGlyphs} glyphs")


if __name__ == "__main__":
    main()
