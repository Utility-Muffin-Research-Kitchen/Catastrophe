/* Build on Linux:
 * cc -Iinclude tests/font-bump.c include/cjson/cJSON.c -o /tmp/cat-font-bump \
 *   $(pkg-config --cflags --libs sdl2 SDL2_ttf SDL2_image) -lm -lpthread
 * On macOS, add -lobjc. Run from the repo root, or pass a font path.
 *
 * cat_set_font_bump() reloads from the theme's own font_path, so the loader
 * stores a path into the buffer it was handed. That must leave the path
 * intact; glibc's snprintf used to empty it and the reload failed.
 */
#define CAT_IMPLEMENTATION
#include "catastrophe.h"
#include <assert.h>

int main(int argc, char **argv) {
    const char *font = argc > 1 ? argv[1] : "res/font.ttf";
    char resolved[PATH_MAX];
    assert(realpath(font, resolved));
    assert(TTF_Init() == 0);
    cat__g.device_scale = 1;

    assert(cat__load_fonts(resolved) == CAT_OK);
    assert(strcmp(cat__g.theme.font_path, resolved) == 0);

    assert(cat_set_font_bump(2) == CAT_OK);
    assert(strcmp(cat__g.theme.font_path, resolved) == 0);
    assert(cat_get_font_bump() == 2);
    assert(cat__g.fonts[CAT_FONT_SMALL]);

    /* cat_reload_fonts() with the theme's path takes the same route. */
    assert(cat_reload_fonts(cat__g.theme.font_path) == CAT_OK);
    assert(strcmp(cat__g.theme.font_path, resolved) == 0);

    for (int i = 0; i < CAT_FONT_TIER_COUNT; i++) {
        if (cat__g.fonts[i]) TTF_CloseFont(cat__g.fonts[i]);
        cat__g.fonts[i] = NULL;
    }
    TTF_Quit();
    puts("font bump: reload from the theme's own font path passed");
    return 0;
}
