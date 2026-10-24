/* Build on Linux:
 * cc -Iinclude tests/hangul-fallback.c include/cjson/cJSON.c -o /tmp/cat-hangul \
 *   $(pkg-config --cflags --libs sdl2 SDL2_ttf SDL2_image) -lm -lpthread
 * On macOS, add -lobjc. Run from the repo root.
 *
 * Neither CJK face carries Hangul, so a string with any goes to Leaf Han Sans
 * KR instead. Chinese and Japanese text, which never holds Hangul, keeps the
 * CJK face, Latin text keeps the theme's, and a missing Hangul file leaves the
 * theme's font as before the face existed.
 */
#define CAT_IMPLEMENTATION
#include "catastrophe.h"
#include <assert.h>

int main(void) {
    char base_path[PATH_MAX], fonts_dir[PATH_MAX];
    assert(realpath("res/font.ttf", base_path));
    assert(realpath("res/fonts", fonts_dir));
    setenv("CAT_FONTS_DIR", fonts_dir, 1);
    setenv("CAT_CJK_FONT_PATH", "SourceHanSansCN/SourceHanSansCN-Regular.otf", 1);
    setenv("CAT_HANGUL_FONT_PATH", "LeafHanSansKR/LeafHanSansKR-Regular.otf", 1);
    assert(TTF_Init() == 0);
    cat__g.device_scale = 1;
    assert(cat__load_fonts(base_path) == CAT_OK);

    /* The detector: syllables, compatibility jamo, mixed text; not Han, kana,
       Latin, or a sequence cut off before its continuation bytes. */
    assert(cat__text_has_hangul("한국어"));
    assert(cat__text_has_hangul("Leaf 설정"));
    assert(cat__text_has_hangul("ㄱ"));
    assert(!cat__text_has_hangul("游戏 Game"));
    assert(!cat__text_has_hangul("ゲーム"));
    assert(!cat__text_has_hangul("Settings"));
    assert(!cat__text_has_hangul("\xED\x95"));
    assert(!cat__text_has_hangul(""));

    TTF_Font *base = cat__g.fonts[CAT_FONT_MEDIUM];
    TTF_Font *korean = cat__font_for_text(base, "한국어");
    TTF_Font *chinese = cat__font_for_text(base, "游戏");
    assert(korean && korean != base);
    assert(chinese && chinese != base && chinese != korean);
    assert(cat__font_for_text(base, "Settings") == base);
    /* Hangul wins over Han in one string, and the face renders it. */
    assert(cat__font_for_text(base, "한국어 漢字") == korean);
    assert(TTF_GlyphIsProvided32(korean, 0xD55C));       /* 한 */
    assert(!TTF_GlyphIsProvided32(chinese, 0xD55C));

    /* A reload releases the face; the next Hangul string opens it again. */
    assert(cat_reload_fonts(base_path) == CAT_OK);
    assert(cat__g.hangul.fonts[CAT_FONT_MEDIUM] == NULL && cat__g.hangul.rel[0] == '\0');
    base = cat__g.fonts[CAT_FONT_MEDIUM];
    assert(cat__font_for_text(base, "한국어") != base);

    /* A missing file leaves the theme's font. */
    setenv("CAT_HANGUL_FONT_PATH", "LeafHanSansKR/Missing.otf", 1);
    assert(cat__font_for_text(base, "한국어") == base);

    puts("hangul-fallback: ok");
    return 0;
}
