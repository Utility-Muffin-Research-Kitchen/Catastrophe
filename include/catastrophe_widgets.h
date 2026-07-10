/*
 * Catastrophe Widgets — UI component library for Allium
 *
 * Define CAT_WIDGETS_IMPLEMENTATION in exactly ONE .c file before including.
 * Requires catastrophe.h to be included first (with CAT_IMPLEMENTATION).
 *
 *   #define CAT_IMPLEMENTATION
 *   #include "catastrophe.h"
 *   #define CAT_WIDGETS_IMPLEMENTATION
 *   #include "catastrophe_widgets.h"
 *
 * Widgets use a blocking model: each runs its own event loop and returns
 * a result when the user makes a choice or presses back (CAT_CANCELLED).
 */

#ifndef CATASTROPHE_WIDGETS_H
#define CATASTROPHE_WIDGETS_H

#ifndef CATASTROPHE_H
#error "catastrophe.h must be included before catastrophe_widgets.h"
#endif

#include <pthread.h>
#include <ctype.h>
#ifndef _WIN32
#include <strings.h>
#endif

/* dirent.h is included by catastrophe.h on Linux; macOS and Windows need it
   here for the file picker widget. */
#if defined(__APPLE__)
#include <dirent.h>
#endif
#ifdef _WIN32
#include <dirent.h>   /* MSYS2/MinGW provides POSIX dirent */
#endif

/* ═══════════════════════════════════════════════════════════════════════════
 * List Widget
 * ═══════════════════════════════════════════════════════════════════════════ */

/* A single item in a list.
 *
 * NOTE: Always use designated initializers (e.g. { .label = "Foo" }) or the
 * CAT_LIST_ITEM / CAT_LIST_ITEM_BG helper macros below when creating items.
 * New fields may be added in future releases; positional initializers
 * (e.g. { "Foo", NULL, NULL, false, NULL }) are fragile across releases
 * and may need updates when fields are added.
 */
typedef struct {
    const char  *label;
    const char  *metadata;    /* Arbitrary string stored with item (e.g. path), not rendered */
    SDL_Texture *image;       /* Optional preview image, NULL = none */
    bool         selected;    /* For multi-select: is this item checked? */
    SDL_Texture *background_image; /* Optional fullscreen preview for the focused item */
    const char  *trailing_text; /* Optional right-aligned visible hint text */
    bool         disabled;    /* Dim row and let caller reject selection with context */
} cat_list_item;

/* Convenience initializers for cat_list_item.
 * CAT_LIST_ITEM        — label + metadata; all other fields zeroed/NULL.
 * CAT_LIST_ITEM_BG     — same, plus a fullscreen background_image.
 * Using these macros (or designated initializers) is strongly preferred
 * over positional initializers to avoid breakage when fields are added.
 *
 * Example:
 *   cat_list_item items[] = {
 *       CAT_LIST_ITEM("Alpha", "/path/alpha"),
 *       CAT_LIST_ITEM_BG("Beta", "/path/beta", bg_tex),
 *   };
 */
#define CAT_LIST_ITEM(lbl, meta) \
    { .label = (lbl), .metadata = (meta) }
#define CAT_LIST_ITEM_BG(lbl, meta, bg) \
    { .label = (lbl), .metadata = (meta), .background_image = (bg) }

typedef struct cat_list_opts cat_list_opts;
typedef void (*cat_list_footer_update_fn)(cat_list_opts *opts, int cursor, void *userdata);

/* Options controlling list behavior */
struct cat_list_opts {
    const char      *title;
    cat_list_item    *items;
    int              item_count;
    cat_footer_item  *footer;
    int              footer_count;
    cat_status_bar_opts *status_bar;
    bool             multi_select;     /* Enable checkbox multi-selection */
    cat_button        reorder_button;   /* CAT_BTN_NONE = no reorder. Set e.g. CAT_BTN_X */
    cat_button        action_button;    /* Custom action button (CAT_BTN_START, etc.) */
    cat_button        secondary_action_button; /* Secondary action (e.g. Y) */
    cat_button        confirm_button;   /* Confirm/accept action (e.g. START) */
    cat_button        tertiary_action_button; /* Tertiary action (e.g. MENU) */
    bool             show_images;      /* Show image column */
    bool             hide_scrollbar;   /* Hide scrollbar (scrolling still works) */
    const char      *help_text;        /* Help overlay text (Menu to show) */
    uint32_t         input_delay;      /* Override input debounce (0 = default) */
    int              initial_index;    /* Starting cursor position */
    int              visible_start_index; /* Initial scroll top index */
    TTF_Font        *item_font;          /* Override list item text (default: CAT_FONT_LARGE) */
    cat_list_footer_update_fn footer_update; /* Optional live footer updater */
    void            *footer_update_userdata;
};

/* Result from closing a list */
typedef struct {
    int             selected_index;    /* Index of selected/confirmed item, or -1 */
    cat_list_action  action;            /* What caused the list to close */
    cat_list_item   *items;             /* Items array (potentially reordered) */
    int             item_count;
    int             visible_start_index; /* Final scroll top index for restoration */
} cat_list_result;

/* Create default list options with sensible values */
cat_list_opts    cat_list_default_opts(const char *title, cat_list_item *items, int count);

/* Show a blocking list. Returns CAT_OK on selection, CAT_CANCELLED on back. */
int             cat_list(cat_list_opts *opts, cat_list_result *result);

/* ═══════════════════════════════════════════════════════════════════════════
 * Options List Widget (settings-style list with per-item options)
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef enum {
    CAT_OPT_STANDARD = 0,   /* Left/Right to cycle through values */
    CAT_OPT_KEYBOARD,       /* A opens keyboard input */
    CAT_OPT_CLICKABLE,      /* A triggers navigation/action */
    CAT_OPT_COLOR_PICKER    /* A opens color picker */
} cat_option_type;

typedef struct {
    const char *label;
    const char *value;     /* Display value, also returned as result */
} cat_option;

typedef struct {
    const char    *label;
    cat_option_type type;
    cat_option     *options;        /* Array of available options */
    int            option_count;
    int            selected_option;/* Currently selected option index */
} cat_options_item;

typedef struct {
    const char        *title;
    cat_options_item   *items;
    int                item_count;
    cat_footer_item    *footer;
    int                footer_count;
    cat_status_bar_opts *status_bar;
    int                initial_selected_index;  /* Initial focused row */
    int                visible_start_index;     /* Initial scroll top index */
    cat_button          action_button;           /* Primary trigger button */
    cat_button          secondary_action_button; /* Secondary trigger button */
    cat_button          confirm_button;  /* Button that confirms/exits (e.g. START) */
    const char        *help_text;
    uint32_t           input_delay;
    bool               return_on_option_change; /* Return CAT_ACTION_OPTION_CHANGED after cycling a standard option */
    TTF_Font          *label_font;       /* Override option label text (default: CAT_FONT_LARGE) */
    TTF_Font          *value_font;       /* Override option value text (default: CAT_FONT_TINY) */
} cat_options_list_opts;

typedef struct {
    int              focused_index;
    cat_list_action   action;
    cat_options_item *items;
    int              item_count;
    int              visible_start_index;
} cat_options_list_result;

int cat_options_list(cat_options_list_opts *opts, cat_options_list_result *result);

/* ═══════════════════════════════════════════════════════════════════════════
 * Keyboard Widget
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef enum {
    CAT_KB_GENERAL = 0,
    CAT_KB_URL,
    CAT_KB_NUMERIC
} cat_keyboard_layout;

typedef struct {
    char text[1024];
} cat_keyboard_result;

int cat_keyboard(const char *initial_text, const char *help_text,
                cat_keyboard_layout layout, cat_keyboard_result *result);

/* Like cat_keyboard() but paints an optional full-screen `backdrop` texture
   behind the keys (NULL = themed background, i.e. identical to cat_keyboard). */
int cat_keyboard_ex(const char *initial_text, const char *help_text,
                cat_keyboard_layout layout, SDL_Texture *backdrop,
                cat_keyboard_result *result);

/* URL keyboard with customizable shortcut keys */
typedef struct {
    const char **shortcut_keys;   /* e.g. {".com", "https://", ".org"} */
    int          shortcut_count;
} cat_url_keyboard_config;

int cat_url_keyboard(const char *initial_text, const char *help_text,
                    cat_url_keyboard_config *cfg, cat_keyboard_result *result);

/* ═══════════════════════════════════════════════════════════════════════════
 * Confirmation Message
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    const char     *message;
    const char     *image_path;    /* Optional image above message */
    cat_footer_item *footer;
    int             footer_count;
} cat_message_opts;

typedef struct {
    bool confirmed;
} cat_confirm_result;

int cat_confirmation(cat_message_opts *opts, cat_confirm_result *result);

/* ═══════════════════════════════════════════════════════════════════════════
 * Selection Message (horizontal option selector)
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    const char *label;
    const char *value;
} cat_selection_option;

typedef struct {
    int selected_index;
} cat_selection_result;

int cat_selection(const char *message, cat_selection_option *options, int count,
                 cat_footer_item *footer, int footer_count,
                 cat_selection_result *result);

/* ═══════════════════════════════════════════════════════════════════════════
 * Process Message (async task with progress bar)
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef int (*cat_process_fn)(void *userdata);

typedef struct {
    const char     *message;
    bool            show_progress;
    float          *progress;           /* Pointer to float [0.0–1.0], updated by worker */
    int            *interrupt_signal;   /* Worker checks this; UI sets to 1 on cancel */
    cat_button       interrupt_button;   /* Button to cancel (CAT_BTN_NONE = no cancel) */
    char          **dynamic_message;    /* Pointer to string pointer, updated by worker */
    int             message_lines;      /* Number of text lines to show (default 1) */
} cat_process_opts;

int cat_process_message(cat_process_opts *opts, cat_process_fn fn, void *userdata);

/* ═══════════════════════════════════════════════════════════════════════════
 * Detail Screen (scrollable multi-section view)
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef enum {
    CAT_SECTION_INFO = 0,       /* Key-value pairs */
    CAT_SECTION_DESCRIPTION,    /* Wrapped text block */
    CAT_SECTION_IMAGE,          /* Single image */
    CAT_SECTION_TABLE,          /* Table with rows/columns */
} cat_detail_section_type;

/* Key-value pair for info sections */
typedef struct {
    const char *key;
    const char *value;
} cat_detail_info_pair;

typedef struct {
    cat_detail_section_type type;
    const char *title;         /* Section header, NULL = no header */

    /* Type-specific data (use the appropriate one): */
    /* CAT_SECTION_INFO */
    cat_detail_info_pair *info_pairs;
    int                  info_count;

    /* CAT_SECTION_DESCRIPTION */
    const char          *description;

    /* CAT_SECTION_IMAGE */
    const char          *image_path;
    int                  image_w;
    int                  image_h;

    /* CAT_SECTION_TABLE */
    const char         **table_headers;
    const char        ***table_rows;   /* Array of row arrays */
    int                  table_cols;
    int                  table_rows_count;
} cat_detail_section;

typedef enum {
    CAT_DETAIL_BACK = 0,
    CAT_DETAIL_ACTION,
    CAT_DETAIL_SECONDARY_ACTION
} cat_detail_action;

typedef struct {
    const char        *title;
    cat_detail_section *sections;
    int                section_count;
    cat_footer_item    *footer;
    int                footer_count;
    cat_status_bar_opts *status_bar;
    bool               center_title;           /* Center the screen title (default: left-aligned) */
    bool               show_section_separator;  /* Draw accent line under section headers (default: off) */
    const cat_draw_color *key_color;           /* Override info pair key color (default: NULL = theme->hint) */
    const cat_draw_color *section_title_color; /* Override section header color (default: NULL = theme->emphasis) */
    TTF_Font          *body_font;              /* Override body/value text (default: CAT_FONT_TINY) */
    TTF_Font          *section_title_font;     /* Override section headers (default: CAT_FONT_SMALL) */
    TTF_Font          *key_font;               /* Override info-pair key text (default: CAT_FONT_TINY) */
} cat_detail_opts;

typedef struct {
    cat_detail_action action;
} cat_detail_result;

int cat_detail_screen(cat_detail_opts *opts, cat_detail_result *result);

/* ═══════════════════════════════════════════════════════════════════════════
 * Color Picker
 * ═══════════════════════════════════════════════════════════════════════════ */

/* Color context for the picker's live preview strip. Pass NULL to skip. */
typedef struct {
    struct { const char *label; cat_draw_color color; } roles[8];
    int role_count;
    int active_role;   /* Index of the role being edited (-1 = none) */
} cat_color_picker_context;

int cat_color_picker(cat_draw_color initial, cat_draw_color *result);
int cat_color_picker_ctx(cat_draw_color initial, cat_draw_color *result,
                         cat_color_picker_context *context);

/* ═══════════════════════════════════════════════════════════════════════════
 * Help Overlay
 * ═══════════════════════════════════════════════════════════════════════════ */

void cat_show_help_overlay(const char *text);

/* ═══════════════════════════════════════════════════════════════════════════
 * File Picker — filesystem browser for selecting files or directories
 *
 * Presents a scrollable directory listing built on cat_list(). The caller
 * chooses whether files, directories, or both are selectable. Navigation
 * into subdirectories uses A; B goes up one level (or cancels at root).
 * An optional "New Folder" action (X button) opens cat_keyboard() inline in
 * directory-capable modes.
 *
 * On device builds the browser is sandboxed to SDCARD_PATH (env contract)
 * or the platform default SD card root. On desktop it defaults to $HOME
 * (or $USERPROFILE on Windows, CWD as final fallback).
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef enum {
    CAT_FILE_PICKER_FILES = 0,   /* Only files are selectable            */
    CAT_FILE_PICKER_DIRS  = 1,   /* Only directories are selectable      */
    CAT_FILE_PICKER_BOTH  = 2,   /* Files and directories are selectable */
} cat_file_picker_mode;

/* Options controlling file picker behavior.
 *
 * NOTE: Always use designated initializers or cat_file_picker_default_opts()
 * to avoid breakage when fields are added in future releases.
 */
typedef struct {
    const char           *title;           /* Screen title (NULL = show relative path from root) */
    cat_file_picker_mode   mode;            /* What can be selected */
    const char           *initial_path;    /* Starting directory (NULL = root_path) */
    const char           *root_path;       /* Navigation boundary (NULL = auto-detect per platform) */
    const char          **extensions;      /* File extension filter array, e.g. {"zip","7z"} */
    int                   extension_count; /* Number of entries in extensions array */
    bool                  allow_create;    /* Show NEW DIR action (X button) in DIRS/BOTH modes */
    bool                  show_hidden;     /* Show dotfiles/dotdirs (e.g. .env, .gitignore, .config) */
    cat_status_bar_opts   *status_bar;      /* Optional status bar */
} cat_file_picker_opts;

/* Result from closing the file picker */
typedef struct {
    char path[1024];      /* Full absolute path of the selected item */
    bool is_directory;    /* True when the selected item is a directory */
} cat_file_picker_result;

/* Create default file picker options (mode = FILES, no filter, no create) */
cat_file_picker_opts cat_file_picker_default_opts(const char *title);

/* Show a blocking file picker. Returns CAT_OK on selection, CAT_CANCELLED on back from root. */
int                 cat_file_picker(cat_file_picker_opts *opts, cat_file_picker_result *result);

/* ═══════════════════════════════════════════════════════════════════════════
 * Queue Viewer — live-updating job queue display
 *
 * Presents a scrollable, filterable list of background jobs with per-item
 * status, optional progress bars, and a live summary bar. The caller
 * supplies a snapshot callback that copies the current queue state each
 * frame; all threading remains in the caller.
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef enum {
    CAT_QUEUE_PENDING = 0,   /* Waiting to start                 (hint color) */
    CAT_QUEUE_RUNNING = 1,   /* Actively being processed        (accent color) */
    CAT_QUEUE_DONE    = 2,   /* Successfully completed               (soft green) */
    CAT_QUEUE_FAILED  = 3,   /* Ended in error                         (red) */
    CAT_QUEUE_SKIPPED = 4,   /* Intentionally skipped / cancelled (hint color) */
} cat_queue_status;

/* A single job entry displayed in the queue viewer */
typedef struct {
    char            title[256];      /* Large primary label (left-aligned) */
    char            subtitle[128];   /* Small secondary label below title */
    char            status_text[64]; /* Right-aligned status string */
    cat_queue_status status;          /* Drives color-coding and filter */
    float           progress;        /* 0.0–1.0 = draw inline progress bar; < 0 = no bar */
    void           *userdata;        /* Caller-defined opaque context */
} cat_queue_item;

/* Called every frame to fill buf[0..max] with current item state.
 * Must be thread-safe (copy from mutex-protected state).
 * Returns actual count written (≤ max). */
typedef int (*cat_queue_snapshot_fn)(cat_queue_item *buf, int max, void *userdata);

/* Called when user presses A on a terminal (DONE/FAILED/SKIPPED) item.
 * Widget pauses its own loop; callback may push another widget. */
typedef void (*cat_queue_detail_fn)(const cat_queue_item *item, void *userdata);

/* Called when user presses X while any items are still active.
 * Caller owns cancellation semantics; update future snapshots to show
 * cancelled/skipped state after this returns. */
typedef void (*cat_queue_cancel_fn)(void *userdata);

/* Called when user presses X to clear completed items.
 * Widget re-snapshots on the next frame after this returns.
 * Only shown when no PENDING or RUNNING items exist. */
typedef void (*cat_queue_clear_fn)(void *userdata);

/* Optional one-line live summary. The widget supplies a fallback summary when
 * unset or when the callback leaves buf empty. */
typedef void (*cat_queue_summary_fn)(char *buf, size_t buf_size, void *userdata);

typedef struct {
    const char           *title;       /* Screen title, e.g. "DOWNLOADS" */
    cat_queue_snapshot_fn  snapshot;    /* Required: fills items each frame */
    int                   max_items;   /* Snapshot buffer capacity; 0 → 256 */
    void                 *userdata;    /* Passed to all callbacks */
    cat_queue_detail_fn    on_detail;   /* Optional: A button on terminal items */
    cat_queue_cancel_fn    on_cancel;   /* Optional: X button while queue active */
    cat_queue_clear_fn     on_clear;    /* Optional: X button when queue idle */
    cat_status_bar_opts   *status_bar;  /* Optional: top-right status pill */
    bool                  hide_filter; /* Set true to suppress Y=FILTER button */
    const char           *filter_labels[4]; /* Optional labels for filters: [0]=ALL, [1]=PENDING||RUNNING (default "IN PROGRESS"), [2]=DONE||SKIPPED, [3]=FAILED; NULL or "" entries use defaults */
    cat_queue_summary_fn   summary;     /* Optional: one-line summary bar text */
} cat_queue_opts;

/* Runs the queue viewer event loop. Returns CAT_OK when user exits (B). */
int cat_queue_viewer(const cat_queue_opts *opts);

/* ═══════════════════════════════════════════════════════════════════════════
 * Download Manager — multi-threaded file downloader with progress UI
 *
 * Requires libcurl. Link with -lcurl. The widget spawns a thread pool
 * (default 3 concurrent) and displays per-file progress bars with speed.
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef enum {
    CAT_DL_PENDING = 0,     /* Waiting to start */
    CAT_DL_DOWNLOADING,     /* In progress */
    CAT_DL_COMPLETE,        /* Finished successfully */
    CAT_DL_FAILED           /* Error occurred */
} cat_download_status;

/* A single download job */
typedef struct {
    const char          *url;            /* Source URL */
    const char          *dest_path;      /* Destination file path */
    const char          *label;          /* Display label (e.g. filename) */
    cat_download_status   status;         /* Set by download manager */
    float                progress;       /* 0.0–1.0, updated during download */
    double               speed_bps;      /* Bytes per second, updated live */
    int                  http_code;      /* HTTP response code (0 before completion) */
    char                 error[256];     /* Error message if status == CAT_DL_FAILED */
} cat_download;

/* Overall result */
typedef struct {
    int  total;
    int  completed;
    int  failed;
    bool cancelled;   /* True if user cancelled */
} cat_download_result;

/* Options */
typedef struct {
    int   max_concurrent;     /* Max simultaneous downloads (default 3) */
    bool  skip_ssl_verify;    /* Disable SSL cert verification */
    const char **headers;     /* NULL-terminated array of "Header: Value" strings */
    int   header_count;
} cat_download_opts;

/* Run the download manager. Blocks until all downloads finish or user cancels.
 * Returns CAT_OK when all complete, CAT_CANCELLED if user cancelled, CAT_ERROR on error. */
int cat_download_manager(cat_download *downloads, int count,
                        cat_download_opts *opts, cat_download_result *result);

/* ═══════════════════════════════════════════════════════════════════════════
 * Non-Blocking List Pane
 *
 * A stateless draw helper for list UIs that run a continuous render loop
 * (like a launcher main loop) rather than a blocking widget event loop.
 * Unlike cat_list(), this never owns an event loop — the caller drives input
 * and calls cat_draw_list_pane() once per frame.
 *
 * Typical usage:
 *   cat_list_state ls;
 *   cat_list_state_init(&ls, visible_rows);
 *
 *   // on input:
 *   cat_list_state_move(&ls, +1, item_count);
 *
 *   // in render:
 *   cat_draw_list_pane(x, y, w, h, item_count, &ls, item_h, draw_cb, ctx);
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    int cursor;
    int scroll_offset;
    int visible_rows;
} cat_list_state;

typedef void (*cat_list_item_draw_fn)(int idx, int x, int y, int w, int h,
                                       bool selected, void *user);

void cat_list_state_init(cat_list_state *s, int visible_rows);
/* Moves the cursor by delta, wrapping around the ends (top<->bottom). */
void cat_list_state_move(cat_list_state *s, int delta, int count);
void cat_list_state_page(cat_list_state *s, int delta, int count);
void cat_list_state_jump(cat_list_state *s, int target, int count);

/* Jump to the next item whose first character differs from the cursor item's.
 * direction: +1 = forward, -1 = backward. get_label returns item text.
 * Returns 1 if the cursor moved, 0 if it stayed. */
int  cat_list_state_jump_letter(cat_list_state *s,
                                 const char *(*get_label)(int idx, void *user),
                                 void *user, int count, int direction);

/* Draw a scrollable list of items. draw_item is called for each visible row.
 * The pane automatically draws a scrollbar when item_count > visible_rows. */
void cat_draw_list_pane(int x, int y, int w, int h,
                         int item_count, const cat_list_state *state,
                         int item_height, cat_list_item_draw_fn draw_item,
                         void *user);

/* ═══════════════════════════════════════════════════════════════════════════
 * Scroll View
 *
 * A non-interactive vertical scroll container for content taller than its
 * viewport — long text, info / credits lists, and the like. Unlike
 * cat_draw_list_pane there is no cursor or selection, only a scroll offset.
 * Drive it from input with cat_scroll_state_move() (e.g. one line height per
 * Up/Down) and render it with cat_draw_scroll_view().
 *
 * Typical usage:
 *   cat_scroll_state sv;
 *   cat_scroll_state_init(&sv);
 *
 *   // on input:
 *   cat_scroll_state_move(&sv, +line_h);   // Down   (-line_h for Up)
 *
 *   // in render:
 *   cat_draw_scroll_view(x, y, w, h, content_h, &sv, draw_cb, ctx);
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    int offset;   /* current (animated) vertical scroll offset in pixels (>= 0) */
    int target;   /* destination offset input steers toward; offset eases to it */
} cat_scroll_state;

/* Render callback: draw the full content with its top-left at (x, y). The view
 * has already shifted y for the scroll offset and clipped to the viewport, so
 * just draw every item at its natural position. */
typedef void (*cat_scroll_content_fn)(int x, int y, int w, void *user);

void cat_scroll_state_init(cat_scroll_state *s);

/* Adjust the scroll target by delta_px (negative scrolls up); the visible offset
 * eases toward it in cat_draw_scroll_view. Lower-bounded at 0 here; the upper
 * bound is applied by cat_draw_scroll_view once it knows the viewport and content
 * heights, so this is safe to call freely from input. */
void cat_scroll_state_move(cat_scroll_state *s, int delta_px);

/* Draw scrollable content inside (x,y,w,h). content_height is the full natural
 * height of the content. Clamps state->offset to [0, content_height - h] (and
 * stores the clamp), clips drawing to the rect, calls draw() with the content
 * origin already shifted for the scroll offset, then draws a scrollbar when the
 * content overflows. */
void cat_draw_scroll_view(int x, int y, int w, int h, int content_height,
                          cat_scroll_state *state,
                          cat_scroll_content_fn draw, void *user);

/* ═══════════════════════════════════════════════════════════════════════════
 * IMPLEMENTATION
 * ═══════════════════════════════════════════════════════════════════════════ */
#ifdef CAT_WIDGETS_IMPLEMENTATION

/* ─── Internal widget helpers ────────────────────────────────────────────── */


/* ═══════════════════════════════════════════════════════════════════════════
 * LIST WIDGET Implementation
 * ═══════════════════════════════════════════════════════════════════════════ */

/* Duration of the selection pill slide animation in ms (~3 frames at 60fps) */
#define CAT__PILL_ANIM_MS 50.0f     /* ~3 frames at 60fps */
#define CAT__SCROLL_ANIM_MS 80.0f  /* ~5 frames at 60fps — content scroll */

cat_list_opts cat_list_default_opts(const char *title, cat_list_item *items, int count) {
    cat_list_opts opts = {0};
    opts.title = title;
    opts.items = items;
    opts.item_count = count;
    opts.footer = NULL;
    opts.footer_count = 0;
    opts.status_bar = NULL;
    opts.multi_select = false;
    opts.reorder_button = CAT_BTN_NONE;
    opts.action_button = CAT_BTN_NONE;
    opts.secondary_action_button = CAT_BTN_NONE;
    opts.confirm_button = CAT_BTN_NONE;
    opts.tertiary_action_button = CAT_BTN_NONE;
    opts.show_images = false;
    opts.hide_scrollbar = false;
    opts.help_text = NULL;
    opts.input_delay = 0;
    opts.initial_index = 0;
    opts.visible_start_index = 0;
    opts.footer_update = NULL;
    opts.footer_update_userdata = NULL;
    return opts;
}

static inline int cat__alpha_group_for_text(const char *text) {
    int g = 0; /* default: non-alpha */
    if (text && text[0]) {
        char c = (char)tolower((unsigned char)text[0]);
        if (c >= 'a' && c <= 'z') g = (c - 'a') + 1;
    }
    return g;
}

static inline void cat__build_alpha_index(const cat_list_item *items, int item_count,
                                        int alpha_starts[27], int *alpha_count,
                                        int *item_alpha) {
    *alpha_count = 0;
    int prev_group = -1;
    for (int i = 0; i < item_count; i++) {
        int g = cat__alpha_group_for_text(items[i].label);
        if (g != prev_group) {
            if (*alpha_count < 27) {
                alpha_starts[*alpha_count] = i;
                (*alpha_count)++;
            }
            prev_group = g;
        }
        item_alpha[i] = *alpha_count - 1;
    }
}

int cat_list(cat_list_opts *opts, cat_list_result *result) {
    if (!opts || !result) return CAT_ERROR;
    if (!opts->items || opts->item_count <= 0) return CAT_ERROR;

    memset(result, 0, sizeof(*result));
    result->items = opts->items;
    result->item_count = opts->item_count;
    result->selected_index = -1;
    result->visible_start_index = 0;

    /* Save/restore input delay */
    uint32_t saved_delay = 0;
    if (opts->input_delay > 0) {
        saved_delay = opts->input_delay;
        cat_set_input_delay(opts->input_delay);
    }

    cat_theme *theme = cat_get_theme();
    int screen_w = cat_get_screen_width();

    TTF_Font *title_font = cat_get_font(CAT_FONT_EXTRA_LARGE);
    TTF_Font *item_font  = opts->item_font ? opts->item_font : cat_get_font(CAT_FONT_LARGE);
    if (!title_font || !item_font) return CAT_ERROR;

    /* Layout constants — PILL_SIZE × device_scale, no gap */
    int margin     = CAT_DS(cat__g.device_padding);
    int pill_h     = CAT_DS(CAT__PILL_SIZE);
    int pill_pad   = CAT_DS(theme->ui_padding_x);
    int item_gap   = 0;
    int image_size = opts->show_images ? CAT_DS(24) : 0;
    int image_pad  = opts->show_images ? CAT_DS(6) : 0;

    /* Content area */
    SDL_Rect content_rect = cat_get_content_rect(opts->title != NULL, opts->footer_count > 0,
                                                opts->status_bar != NULL);
    int content_y = content_rect.y;
    int content_h = content_rect.h;

    /* Calculate visible items */
    int max_visible = content_h / (pill_h + item_gap);
    if (max_visible < 1) max_visible = 1;

    /* State */
    int cursor = opts->initial_index;
    if (cursor < 0) cursor = 0;
    if (cursor >= opts->item_count) cursor = opts->item_count - 1;
    int scroll_top = opts->visible_start_index;
    if (scroll_top < 0) scroll_top = 0;
    if (scroll_top > opts->item_count - max_visible) scroll_top = opts->item_count - max_visible;
    if (scroll_top < 0) scroll_top = 0;
    bool reorder_mode = false;
    bool running = true;
    bool show_help = false;

    /* Alpha-skip index: maps first-letter groups so L1/R1 can jump between them.
     * alpha_starts[i] = index of the first item in letter group i.
     * item_alpha[j]   = which alpha group item j belongs to.               */
    int alpha_starts[27];  /* max 27 groups: non-alpha + a..z */
    int alpha_count = 0;
    int *item_alpha = (int *)calloc((size_t)opts->item_count, sizeof(int));
    if (item_alpha) {
        cat__build_alpha_index(opts->items, opts->item_count,
                             alpha_starts, &alpha_count, item_alpha);
    }

    /* Text scroll state for selected item */
    cat_text_scroll sel_scroll;
    cat_text_scroll_init(&sel_scroll);
    int last_cursor = cursor;

    /* Pill animation state */
    float    pill_anim_y           = 0.0f;
    float    pill_anim_w           = 0.0f;
    float    pill_from_y           = 0.0f;
    float    pill_from_w           = 0.0f;
    uint32_t pill_anim_start       = 0;
    bool     pill_anim_initialized = false;
    int      pill_prev_target_y    = 0;    /* target_y from previous frame (clean grid row) */
    int      pill_prev_target_w    = 0;

    uint32_t last_frame = SDL_GetTicks();

    while (running) {
        uint32_t now = SDL_GetTicks();
        uint32_t dt = now - last_frame;
        last_frame = now;

        /* Input */
        cat_input_event ev;
        while (cat_poll_input(&ev)) {
            if (!ev.pressed) continue;
            if (show_help) {
                show_help = false;
                continue;
            }

            switch (ev.button) {
                case CAT_BTN_UP:
                    if (cursor == 0) {
                        if (!ev.repeated && !reorder_mode)
                            cursor = opts->item_count - 1;
                    } else {
                        if (reorder_mode) {
                            cat_list_item tmp = opts->items[cursor];
                            opts->items[cursor] = opts->items[cursor - 1];
                            opts->items[cursor - 1] = tmp;
                        }
                        cursor--;
                    }
                    break;

                case CAT_BTN_DOWN:
                    if (cursor >= opts->item_count - 1) {
                        if (!ev.repeated && !reorder_mode)
                            cursor = 0;
                    } else {
                        if (reorder_mode && cursor < opts->item_count - 1) {
                            cat_list_item tmp = opts->items[cursor];
                            opts->items[cursor] = opts->items[cursor + 1];
                            opts->items[cursor + 1] = tmp;
                        }
                        cursor++;
                    }
                    break;

                case CAT_BTN_A:
                    if (opts->multi_select) {
                        opts->items[cursor].selected = !opts->items[cursor].selected;
                    } else {
                        result->selected_index = cursor;
                        result->action = CAT_ACTION_SELECTED;
                        running = false;
                    }
                    break;

                case CAT_BTN_B:
                    if (reorder_mode) {
                        reorder_mode = false;
                        if (item_alpha)
                            cat__build_alpha_index(opts->items, opts->item_count,
                                                 alpha_starts, &alpha_count, item_alpha);
                    } else {
                        result->action = CAT_ACTION_BACK;
                        running = false;
                    }
                    break;

                case CAT_BTN_LEFT:
                    if (!reorder_mode) {
                        cursor -= max_visible;
                        if (cursor < 0) cursor = 0;
                    }
                    break;

                case CAT_BTN_RIGHT:
                    if (!reorder_mode) {
                        cursor += max_visible;
                        if (cursor >= opts->item_count)
                            cursor = opts->item_count - 1;
                    }
                    break;

                case CAT_BTN_L1:
                    if (item_alpha && !reorder_mode) {
                        int gi = item_alpha[cursor] - 1;
                        if (gi >= 0)
                            cursor = alpha_starts[gi];
                    }
                    break;

                case CAT_BTN_R1:
                    if (item_alpha && !reorder_mode) {
                        int gi = item_alpha[cursor] + 1;
                        if (gi < alpha_count)
                            cursor = alpha_starts[gi];
                    }
                    break;

                case CAT_BTN_MENU:
                    if (opts->help_text) {
                        show_help = true;
                        goto done_input; /* stop processing queued events */
                    } else {
                        cat_show_footer_overflow();
                    }
                    break;

                default:
                    /* Check reorder button */
                    if (opts->reorder_button != CAT_BTN_NONE && ev.button == opts->reorder_button) {
                        bool was_reorder = reorder_mode;
                        reorder_mode = !reorder_mode;
                        if (was_reorder && !reorder_mode && item_alpha)
                            cat__build_alpha_index(opts->items, opts->item_count,
                                                 alpha_starts, &alpha_count, item_alpha);
                        break;
                    }
                    /* Check action button */
                    if (opts->action_button != CAT_BTN_NONE && ev.button == opts->action_button) {
                        result->selected_index = cursor;
                        result->action = CAT_ACTION_TRIGGERED;
                        running = false;
                        break;
                    }
                    if (opts->secondary_action_button != CAT_BTN_NONE && ev.button == opts->secondary_action_button) {
                        result->selected_index = cursor;
                        result->action = CAT_ACTION_SECONDARY_TRIGGERED;
                        running = false;
                        break;
                    }
                    if (opts->confirm_button != CAT_BTN_NONE && ev.button == opts->confirm_button) {
                        result->selected_index = cursor;
                        result->action = CAT_ACTION_CONFIRMED;
                        running = false;
                        break;
                    }
                    if (opts->tertiary_action_button != CAT_BTN_NONE && ev.button == opts->tertiary_action_button) {
                        result->selected_index = cursor;
                        result->action = CAT_ACTION_TERTIARY_TRIGGERED;
                        running = false;
                    }
                    break;
            }
        }
        done_input:

        /* Reset text scroll on cursor change; start pill animation */
        if (cursor != last_cursor) {
            cat_text_scroll_reset(&sel_scroll);
            /* Start from the pill's current on-screen position so rapid input
             * doesn't cause a visual jump to the previous target. */
            float prev_t = cat__clampf((float)(now - pill_anim_start) / CAT__PILL_ANIM_MS, 0.0f, 1.0f);
            pill_from_y     = cat__lerpf(pill_from_y, (float)pill_prev_target_y, prev_t);
            pill_from_w     = cat__lerpf(pill_from_w, (float)pill_prev_target_w, prev_t);
            pill_anim_start = now;
            last_cursor     = cursor;
        }

        /* Scroll adjustment */
        if (cursor < scroll_top) scroll_top = cursor;
        if (cursor >= scroll_top + max_visible)
            scroll_top = cursor - max_visible + 1;
        if (scroll_top < 0) scroll_top = 0;

        if (opts->footer_update) {
            opts->footer_update(opts, cursor, opts->footer_update_userdata);
        }

        /* Compute target pill geometry for current cursor */
        int pill_target_y = content_y + (cursor - scroll_top) * (pill_h + item_gap);
        int pill_target_w;
        {
            const char *cur_label = opts->items[cursor].label ? opts->items[cursor].label : "";
            int tw = cat_measure_text(item_font, cur_label);
            pill_target_w = tw + pill_pad * 2;
            if (opts->multi_select) pill_target_w += CAT_S(32);
            if (opts->show_images)  pill_target_w += image_size + image_pad;
            int avail = screen_w - margin * 2;
            if (!opts->hide_scrollbar && opts->item_count > max_visible) avail -= CAT_S(12);
            if (pill_target_w > avail) pill_target_w = avail;
        }
        /* Save for the next frame's cursor-change snap */
        pill_prev_target_y = pill_target_y;
        pill_prev_target_w = pill_target_w;

        /* First-frame: snap pill to starting position with no animation */
        if (!pill_anim_initialized) {
            pill_anim_y           = (float)pill_target_y;
            pill_anim_w           = (float)pill_target_w;
            pill_from_y           = (float)pill_target_y;
            pill_from_w           = (float)pill_target_w;
            pill_anim_start       = now;
            pill_anim_initialized = true;
        }

        /* Advance pill animation */
        {
            float t   = cat__clampf((float)(now - pill_anim_start) / CAT__PILL_ANIM_MS, 0.0f, 1.0f);
            pill_anim_y = cat__lerpf(pill_from_y, (float)pill_target_y, t);
            pill_anim_w = cat__lerpf(pill_from_w, (float)pill_target_w, t);
            if (t < 1.0f) cat_request_frame();
            /* Clamp to content area */
            if (pill_anim_y < (float)content_y)
                pill_anim_y = (float)content_y;
            if (pill_anim_y > (float)(content_y + content_h - pill_h))
                pill_anim_y = (float)(content_y + content_h - pill_h);
        }

        /* Render */
        cat_draw_background();
        if (cursor >= 0 && cursor < opts->item_count &&
            opts->items[cursor].background_image) {
            cat_draw_image(opts->items[cursor].background_image, 0, 0,
                          cat_get_screen_width(), cat_get_screen_height());
        }

        /* Title (clipped if status bar present) */
        if (opts->title) cat_draw_screen_title(opts->title, opts->status_bar);

        /* Status bar */
        if (opts->status_bar) cat_draw_status_bar(opts->status_bar);

        /* List items */
        int available_w = screen_w - margin * 2;
        if (!opts->hide_scrollbar && opts->item_count > max_visible) {
            available_w -= CAT_S(12); /* Space for scrollbar */
        }

        /* Pass 1: draw animated pill background at its interpolated position */
        cat_draw_pill(margin, (int)pill_anim_y, (int)pill_anim_w, pill_h, theme->highlight);

        /* Pass 2: draw all items at fixed grid positions.
         * Highlight the row the pill center is currently over, so the bright text
         * always tracks the pill's physical position — no color snaps on rapid input. */
        int pill_center_y  = (int)pill_anim_y + pill_h / 2;
        int pill_row_idx   = (pill_center_y - content_y) / (pill_h + item_gap);
        if (pill_row_idx < 0) pill_row_idx = 0;
        if (pill_row_idx >= max_visible) pill_row_idx = max_visible - 1;
        int highlight_idx  = scroll_top + pill_row_idx;

        for (int i = 0; i < max_visible && (scroll_top + i) < opts->item_count; i++) {
            int idx = scroll_top + i;
            int item_y = content_y + i * (pill_h + item_gap);
            bool is_selected = (idx == cursor);

            bool is_disabled = opts->items[idx].disabled;
            const char *label = opts->items[idx].label ? opts->items[idx].label : "";
            int text_h = TTF_FontHeight(item_font);

            /* Text area calculation */
            int text_x = margin + pill_pad;
            int text_max_w = available_w - pill_pad * 2;

            if (opts->show_images) {
                text_x += image_size + image_pad;
                text_max_w -= image_size + image_pad;
            }

            if (opts->multi_select) {
                text_x += CAT_S(32);
                text_max_w -= CAT_S(32);
            }

            /* Reserve space for optional right-aligned visible hint text. */
            int trailing_w = 0;
            if (opts->items[idx].trailing_text && opts->items[idx].trailing_text[0]) {
                int hint_w = cat_measure_text(item_font, opts->items[idx].trailing_text);
                int reserve = hint_w + CAT_S(16);
                int min_label_w = CAT_S(96);
                if (hint_w > 0 && (text_max_w - reserve) >= min_label_w) {
                    trailing_w = hint_w;
                    text_max_w -= reserve;
                }
            }

            /* Image — always at fixed item_y */
            if (opts->show_images && opts->items[idx].image) {
                int img_y = item_y + (pill_h - image_size) / 2;
                cat_draw_image(opts->items[idx].image, margin + pill_pad, img_y, image_size, image_size);
            }

            /* Checkbox — always at fixed item_y */
            if (opts->multi_select) {
                int cb_x = margin + pill_pad + (opts->show_images ? image_size + image_pad : 0);
                int cb_y = item_y + (pill_h - CAT_S(20)) / 2;
                int cb_size = CAT_S(20);
                if (idx == highlight_idx) {
                    cat_draw_color cb_color = theme->highlighted_text;
                    if (opts->items[idx].selected) {
                        cat_draw_rect(cb_x, cb_y, cb_size, cb_size, theme->accent);
                        cat_draw_text(item_font, "✓", cb_x + CAT_S(2), cb_y - CAT_S(2), theme->highlighted_text);
                    } else {
                        SDL_SetRenderDrawColor(cat_get_renderer(), cb_color.r, cb_color.g, cb_color.b, cb_color.a);
                        SDL_Rect border = {cb_x, cb_y, cb_size, cb_size};
                        SDL_RenderDrawRect(cat_get_renderer(), &border);
                    }
                } else {
                    cat_draw_color cb_color = theme->text;
                    cb_color.a = 180;
                    if (opts->items[idx].selected) {
                        cat_draw_rect(cb_x, cb_y, cb_size, cb_size, cb_color);
                        cat_draw_text(item_font, "✓", cb_x + CAT_S(2), cb_y - CAT_S(2), theme->text);
                    } else {
                        SDL_SetRenderDrawColor(cat_get_renderer(), cb_color.r, cb_color.g, cb_color.b, cb_color.a);
                        SDL_Rect border = {cb_x, cb_y, cb_size, cb_size};
                        SDL_RenderDrawRect(cat_get_renderer(), &border);
                    }
                }
            }

            /* Text color: follow the pill — highlight the departure item during transit,
             * switch to arrival item only once the pill settles */
            cat_draw_color text_color = (idx == highlight_idx)
                                ? theme->highlighted_text
                                : (is_disabled ? theme->disabled : theme->text);

            /* Text — always at fixed item_y; selected item may scroll */
            if (is_selected) {
                int draw_label_w = cat_measure_text(item_font, label);
                cat_text_scroll_update(&sel_scroll, draw_label_w, text_max_w, dt);
                /* While paused at an end, schedule a single wake when the pause
                   ends instead of pinning 60fps (nothing moves during the pause). */
                if (sel_scroll.active) {
                    if (sel_scroll.pause_timer > 0)
                        cat_request_frame_in((uint32_t)sel_scroll.pause_timer);
                    else
                        cat_request_frame();
                }

                if (draw_label_w > text_max_w) {
                    SDL_Rect clip = {text_x, item_y, text_max_w, pill_h};
                    SDL_RenderSetClipRect(cat_get_renderer(), &clip);
                    cat_draw_text(item_font, label,
                                 text_x - sel_scroll.offset,
                                 item_y + (pill_h - text_h) / 2,
                                 text_color);
                    SDL_RenderSetClipRect(cat_get_renderer(), NULL);
                } else {
                    cat_draw_text(item_font, label,
                                 text_x,
                                 item_y + (pill_h - text_h) / 2,
                                 text_color);
                }

                /* Reorder mode indicator — at fixed item_y */
                if (reorder_mode) {
                    cat_draw_color reorder_color = theme->accent;
                    int indicator_w = CAT_S(4);
                    cat_draw_rect(margin - indicator_w - CAT_S(4), item_y, indicator_w, pill_h, reorder_color);
                }
            } else {
                cat_draw_text_clipped(item_font, label,
                                     text_x,
                                     item_y + (pill_h - text_h) / 2,
                                     text_color, text_max_w);
            }

            if (trailing_w > 0) {
                int trailing_x = margin + available_w - pill_pad - trailing_w;
                int trailing_y = item_y + (pill_h - text_h) / 2;
                cat_draw_color trailing_color = is_disabled ? theme->disabled : theme->hint;
                if (idx == highlight_idx) trailing_color.a = 255;
                cat_draw_text(item_font, opts->items[idx].trailing_text,
                             trailing_x, trailing_y, trailing_color);
            }
        }

        /* Scrollbar */
        if (!opts->hide_scrollbar && opts->item_count > max_visible) {
            int sb_x = screen_w - margin - CAT_S(6);
            cat_draw_scrollbar(sb_x, content_y, content_h, max_visible, opts->item_count, scroll_top);
        }

        /* Footer */
        if (opts->footer && opts->footer_count > 0) {
            cat_draw_footer(opts->footer, opts->footer_count);
        }

        /* Help overlay (on top of everything) */
        if (show_help && opts->help_text) {
            cat_show_help_overlay(opts->help_text);
            show_help = false;
            /* Show footer overflow after help if hidden items exist */
            cat_show_footer_overflow();
        }

        cat_present();

    }

    result->visible_start_index = scroll_top;

    free(item_alpha);

    /* Restore input delay */
    if (saved_delay > 0) {
        cat_set_input_delay(CAT_INPUT_DEBOUNCE);
    }

    return result->action == CAT_ACTION_BACK ? CAT_CANCELLED : CAT_OK;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * OPTIONS LIST Implementation
 * ═══════════════════════════════════════════════════════════════════════════ */

static int cat__options_valid_index(cat_options_item *item) {
    if (!item || item->option_count <= 0 || !item->options) return -1;
    if (item->selected_option < 0) item->selected_option = 0;
    if (item->selected_option >= item->option_count) {
        item->selected_option = item->option_count - 1;
    }
    return item->selected_option;
}

static inline void cat__options_finish_changed(cat_options_list_result *result,
                                              int cursor, bool *running) {
    result->focused_index = cursor;
    result->action = CAT_ACTION_OPTION_CHANGED;
    *running = false;
}

int cat_options_list(cat_options_list_opts *opts, cat_options_list_result *result) {
    if (!opts || !result) return CAT_ERROR;
    if (!opts->items || opts->item_count <= 0) return CAT_ERROR;

    memset(result, 0, sizeof(*result));
    result->items = opts->items;
    result->item_count = opts->item_count;
    result->focused_index = 0;
    result->visible_start_index = 0;

    cat_theme *theme = cat_get_theme();
    int screen_w = cat_get_screen_width();

    TTF_Font *label_font = opts->label_font ? opts->label_font : cat_get_font(CAT_FONT_LARGE);
    TTF_Font *value_font = opts->value_font ? opts->value_font : cat_get_font(CAT_FONT_TINY);
    if (!label_font || !value_font) return CAT_ERROR;

    int margin   = CAT_DS(cat__g.device_padding);
    int pill_h   = CAT_DS(CAT__PILL_SIZE);
    int pill_pad = CAT_DS(theme->ui_padding_x);
    int item_gap = CAT_DS(2);
    int arrow_w  = CAT_DS(12);

    SDL_Rect content_rect = cat_get_content_rect(opts->title != NULL, opts->footer_count > 0,
                                                opts->status_bar != NULL);
    int content_y = content_rect.y;
    int content_h = content_rect.h;
    int max_visible = content_h / (pill_h + item_gap);
    if (max_visible < 1) max_visible = 1;

    int cursor = opts->initial_selected_index;
    if (cursor < 0) cursor = 0;
    if (cursor >= opts->item_count) cursor = opts->item_count - 1;

    int scroll_top = opts->visible_start_index;
    if (scroll_top < 0) scroll_top = 0;
    if (scroll_top > opts->item_count - max_visible) scroll_top = opts->item_count - max_visible;
    if (scroll_top < 0) scroll_top = 0;

    /* Pill animation state */
    float    pill_anim_y           = 0.0f;
    float    pill_from_y           = 0.0f;
    uint32_t pill_anim_start       = 0;
    bool     pill_anim_initialized = false;
    int      pill_prev_target_y    = 0;
    int      last_cursor           = cursor;

    bool running = true;

    while (running) {
        uint32_t now = SDL_GetTicks();

        /* Input */
        cat_input_event ev;
        while (cat_poll_input(&ev)) {
            if (!ev.pressed) continue;

            switch (ev.button) {
                case CAT_BTN_UP:
                    if (cursor == 0) {
                        if (!ev.repeated)
                            cursor = opts->item_count - 1;
                    } else {
                        cursor--;
                    }
                    break;

                case CAT_BTN_DOWN:
                    if (cursor >= opts->item_count - 1) {
                        if (!ev.repeated)
                            cursor = 0;
                    } else {
                        cursor++;
                    }
                    break;

                case CAT_BTN_LEFT: {
                    cat_options_item *item = &opts->items[cursor];
                    int sel = cat__options_valid_index(item);
                    if (item->type == CAT_OPT_STANDARD && sel >= 0) {
                        sel--;
                        if (sel < 0) sel = item->option_count - 1;
                        item->selected_option = sel;
                        if (opts->return_on_option_change) {
                            cat__options_finish_changed(result, cursor, &running);
                        }
                    }
                    break;
                }

                case CAT_BTN_RIGHT: {
                    cat_options_item *item = &opts->items[cursor];
                    int sel = cat__options_valid_index(item);
                    if (item->type == CAT_OPT_STANDARD && sel >= 0) {
                        sel++;
                        if (sel >= item->option_count) sel = 0;
                        item->selected_option = sel;
                        if (opts->return_on_option_change) {
                            cat__options_finish_changed(result, cursor, &running);
                        }
                    }
                    break;
                }

                case CAT_BTN_A: {
                    cat_options_item *item = &opts->items[cursor];
                    if (item->type == CAT_OPT_STANDARD &&
                        opts->confirm_button == CAT_BTN_A) {
                        result->focused_index = cursor;
                        result->action = CAT_ACTION_CONFIRMED;
                        running = false;
                        break;
                    }
                    if (item->type == CAT_OPT_CLICKABLE) {
                        result->focused_index = cursor;
                        result->action = CAT_ACTION_SELECTED;
                        running = false;
                    } else if (item->type == CAT_OPT_KEYBOARD) {
                        /* Open keyboard for this item */
                        cat_keyboard_result kb_result;
                        const char *initial = "";
                        int sel = cat__options_valid_index(item);
                        if (sel >= 0 && item->options[sel].value) {
                            initial = item->options[sel].value;
                        }
                        int kb_ret = cat_keyboard(initial,
                                                  "Start: Confirm\nY: Cancel",
                                                  CAT_KB_GENERAL, &kb_result);
                        if (kb_ret == CAT_OK) {
                            /* Update the option value — caller must manage memory */
                            if (sel >= 0) {
                                free((void *)item->options[sel].value);
                                item->options[sel].value = strdup(kb_result.text);
                                free((void *)item->options[sel].label);
                                item->options[sel].label = strdup(kb_result.text);
                            }
                            if (opts->confirm_button == CAT_BTN_A) {
                                result->focused_index = cursor;
                                result->action = CAT_ACTION_CONFIRMED;
                                running = false;
                            }
                        }
                    } else if (item->type == CAT_OPT_COLOR_PICKER) {
                        cat_draw_color initial_color = {255, 255, 255, 255};
                        cat_draw_color picked;
                        int sel = cat__options_valid_index(item);
                        if (sel >= 0 && item->options[sel].value &&
                            item->options[sel].value[0] == '#') {
                            initial_color = cat_hex_to_color(item->options[sel].value);
                        }
                        int cp_ret = cat_color_picker(initial_color, &picked);
                        if (cp_ret == CAT_OK) {
                            char hex[8];
                            snprintf(hex, sizeof(hex), "#%02X%02X%02X",
                                     picked.r, picked.g, picked.b);
                            if (sel >= 0) {
                                free((void *)item->options[sel].value);
                                item->options[sel].value = strdup(hex);
                                free((void *)item->options[sel].label);
                                item->options[sel].label = strdup(hex);
                            }
                            if (opts->confirm_button == CAT_BTN_A) {
                                result->focused_index = cursor;
                                result->action = CAT_ACTION_CONFIRMED;
                                running = false;
                            }
                        }
                    } else if (item->type == CAT_OPT_STANDARD) {
                        /* Cycle forward on A for standard options */
                        int sel = cat__options_valid_index(item);
                        if (sel >= 0) {
                            sel++;
                            if (sel >= item->option_count) sel = 0;
                            item->selected_option = sel;
                            if (opts->return_on_option_change) {
                                cat__options_finish_changed(result, cursor, &running);
                            }
                        }
                    }
                    break;
                }

                case CAT_BTN_B:
                    result->focused_index = cursor;
                    result->action = CAT_ACTION_BACK;
                    running = false;
                    break;

                case CAT_BTN_MENU:
                    cat_show_footer_overflow();
                    break;

                default:
                    if (opts->confirm_button != CAT_BTN_NONE && ev.button == opts->confirm_button) {
                        result->focused_index = cursor;
                        result->action = CAT_ACTION_CONFIRMED;
                        running = false;
                    } else if (opts->action_button != CAT_BTN_NONE && ev.button == opts->action_button) {
                        result->focused_index = cursor;
                        result->action = CAT_ACTION_TRIGGERED;
                        running = false;
                    } else if (opts->secondary_action_button != CAT_BTN_NONE && ev.button == opts->secondary_action_button) {
                        result->focused_index = cursor;
                        result->action = CAT_ACTION_SECONDARY_TRIGGERED;
                        running = false;
                    }
                    break;
            }
        }

        /* Scroll */
        if (cursor < scroll_top) scroll_top = cursor;
        if (cursor >= scroll_top + max_visible)
            scroll_top = cursor - max_visible + 1;

        /* Pill animation */
        int pill_target_y = content_y + (cursor - scroll_top) * (pill_h + item_gap);

        if (cursor != last_cursor) {
            pill_from_y     = (float)pill_prev_target_y;
            pill_anim_start = now;
            last_cursor     = cursor;
        }

        pill_prev_target_y = pill_target_y;

        if (!pill_anim_initialized) {
            pill_anim_y           = (float)pill_target_y;
            pill_from_y           = (float)pill_target_y;
            pill_anim_start       = now;
            pill_anim_initialized = true;
        }

        {
            float t = cat__clampf((float)(now - pill_anim_start) / CAT__PILL_ANIM_MS, 0.0f, 1.0f);
            pill_anim_y = cat__lerpf(pill_from_y, (float)pill_target_y, t);
            if (t < 1.0f) cat_request_frame();
            if (pill_anim_y < (float)content_y)
                pill_anim_y = (float)content_y;
            if (pill_anim_y > (float)(content_y + content_h - pill_h))
                pill_anim_y = (float)(content_y + content_h - pill_h);
        }

        /* Render */
        cat_draw_background();
        if (opts->title) cat_draw_screen_title(opts->title, opts->status_bar);
        if (opts->status_bar) cat_draw_status_bar(opts->status_bar);

        int available_w = screen_w - margin * 2;
        if (opts->item_count > max_visible) {
            available_w -= CAT_S(12); /* Space for scrollbar */
        }

        /* Draw animated pill background */
        cat_draw_pill(margin, (int)pill_anim_y, available_w, pill_h, theme->highlight);

        /* Determine which grid row the pill visually covers (for text color) */
        int pill_center_y = (int)pill_anim_y + pill_h / 2;
        int pill_row_idx  = (pill_center_y - content_y) / (pill_h + item_gap);
        if (pill_row_idx < 0) pill_row_idx = 0;
        if (pill_row_idx >= max_visible) pill_row_idx = max_visible - 1;
        int highlight_idx = scroll_top + pill_row_idx;

        for (int i = 0; i < max_visible && (scroll_top + i) < opts->item_count; i++) {
            int idx = scroll_top + i;
            int item_y = content_y + i * (pill_h + item_gap);
            bool is_selected = (idx == highlight_idx);
            cat_options_item *item = &opts->items[idx];

            const char *label = item->label ? item->label : "";
            const char *value = "";
            int valid_opt = cat__options_valid_index(item);
            if (valid_opt >= 0) {
                value = item->options[valid_opt].label;
                if (!value) value = item->options[valid_opt].value;
                if (!value) value = "";
            }

            int value_w = cat_measure_text(value_font, value);
            bool show_standard_arrows = is_selected &&
                item->type == CAT_OPT_STANDARD &&
                valid_opt >= 0 &&
                item->option_count > 1;
            bool show_click_arrow = (item->type == CAT_OPT_CLICKABLE);
            int right_decor_w = 0;
            if (show_standard_arrows) {
                right_decor_w = arrow_w * 2 + CAT_S(4);
            } else if (show_click_arrow) {
                right_decor_w = CAT_S(16) + CAT_S(4);
            }

            /* Compute right-side reservation (value + decorations) */
            int right_reserve = right_decor_w;
            if (value_w > 0) right_reserve += value_w;

            /* Adaptive label width: give label as much space as the value doesn't need */
            int inner_w = available_w - pill_pad * 2;
            int min_gap = CAT_DS(8);
            int label_w = cat_measure_text(label_font, label);
            int max_label_w;
            int gap_w = right_reserve > 0 ? min_gap : 0;

            if (right_reserve == 0) {
                max_label_w = inner_w;
            } else {
                int budget = inner_w - gap_w;
                if (label_w + right_reserve <= budget) {
                    max_label_w = label_w;           /* both fit naturally */
                } else {
                    int half = budget / 2;
                    if (right_reserve <= half)
                        max_label_w = budget - right_reserve;  /* value is short */
                    else if (label_w <= half)
                        max_label_w = label_w;                 /* label is short */
                    else
                        max_label_w = half;                    /* both long — split */
                }
            }
            if (max_label_w < 0) max_label_w = 0;
            {
                cat_draw_color label_color = is_selected ? theme->highlighted_text : theme->text;
                cat_draw_color value_color = is_selected ? theme->highlighted_text : theme->hint;
                int label_y = item_y + (pill_h - TTF_FontHeight(label_font)) / 2;
                int value_y = item_y + (pill_h - TTF_FontHeight(value_font)) / 2;
                int content_right = margin + available_w - pill_pad;
                int max_value_w = inner_w - max_label_w - gap_w - right_decor_w;
                int draw_value_w = (value_w <= max_value_w) ? value_w
                    : cat_measure_text_ellipsized(value_font, value, max_value_w);

                /* Labels and values share the same width budget in both states. */
                cat_draw_text_ellipsized(label_font, label,
                    margin + pill_pad,
                    label_y,
                    label_color,
                    max_label_w);

                int vfh   = TTF_FontHeight(value_font);
                int tri_h = vfh * 6 / 10;
                int tri_w = vfh * 5 / 10;
                int tri_y = value_y + (vfh - tri_h) / 2;
                int tri_gap = CAT_S(8);
                if (show_standard_arrows) {
                    int right_tri_x = content_right - arrow_w + (arrow_w - tri_w) / 2;
                    int value_right = right_tri_x - tri_gap;
                    int value_x     = value_right - draw_value_w;
                    int left_tri_x  = value_x - tri_gap - tri_w;

                    cat_draw_triangle(left_tri_x, tri_y, tri_w, tri_h, CAT_DIR_LEFT, value_color);
                    if (draw_value_w > 0) {
                        cat_draw_text_ellipsized(value_font, value,
                            value_x, value_y, value_color, max_value_w);
                    }
                    cat_draw_triangle(right_tri_x, tri_y, tri_w, tri_h, CAT_DIR_RIGHT, value_color);
                } else if (show_click_arrow) {
                    int arrow_x = content_right - CAT_S(16);

                    if (draw_value_w > 0) {
                        cat_draw_text_ellipsized(value_font, value,
                            arrow_x - tri_gap - draw_value_w,
                            value_y,
                            value_color,
                            max_value_w);
                    }
                    cat_draw_triangle(arrow_x, tri_y, tri_w, tri_h, CAT_DIR_RIGHT, value_color);
                } else if (draw_value_w > 0) {
                    cat_draw_text_ellipsized(value_font, value,
                        content_right - draw_value_w,
                        value_y,
                        value_color,
                        max_value_w);
                }
            }
        }

        /* Scrollbar */
        if (opts->item_count > max_visible) {
            int sb_x = screen_w - margin - CAT_S(6);
            cat_draw_scrollbar(sb_x, content_y, content_h, max_visible, opts->item_count, scroll_top);
        }

        if (opts->footer && opts->footer_count > 0) {
            cat_draw_footer(opts->footer, opts->footer_count);
        }

        cat_present();

    }

    result->visible_start_index = scroll_top;

    return result->action == CAT_ACTION_BACK ? CAT_CANCELLED : CAT_OK;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * KEYBOARD Implementation — matches Gabagool 5-row layout with special keys
 * ═══════════════════════════════════════════════════════════════════════════ */

/* Special key identifiers used internally for cursor tracking.
 * These are negative to distinguish them from character-key grid indices. */
#define CAT__KB_KEY_BACKSPACE  (-1)
#define CAT__KB_KEY_ENTER      (-2)
#define CAT__KB_KEY_SHIFT      (-3)
#define CAT__KB_KEY_SYMBOL     (-4)
#define CAT__KB_KEY_SPACE      (-5)

#define CAT__KB_SHIFT_LABEL "\xe2\x87\xa7" /* ⇧ */

/* 5-row general layout — 12-column virtual grid.
 * Row 0: 1-0 keys (10) + backspace (2 cols)
 * Row 1: qwertyuiop (10 cols, centered)
 * Row 2: asdfghjkl (9 cols) + enter (1.5 cols)
 * Row 3: shift (2 cols) + zxcvbnm (7 cols) + symbol (2 cols)
 * Row 4: space (8 cols, centered)
 */

/* Lower-case keys per row (NULL-terminated) */
static const char *cat__kb5_row0_lower[] = {"1","2","3","4","5","6","7","8","9","0", NULL};
static const char *cat__kb5_row1_lower[] = {"q","w","e","r","t","y","u","i","o","p", NULL};
static const char *cat__kb5_row2_lower[] = {"a","s","d","f","g","h","j","k","l", NULL};
static const char *cat__kb5_row3_lower[] = {"z","x","c","v","b","n","m", NULL};

/* Upper-case */
static const char *cat__kb5_row0_upper[] = {"1","2","3","4","5","6","7","8","9","0", NULL};
static const char *cat__kb5_row1_upper[] = {"Q","W","E","R","T","Y","U","I","O","P", NULL};
static const char *cat__kb5_row2_upper[] = {"A","S","D","F","G","H","J","K","L", NULL};
static const char *cat__kb5_row3_upper[] = {"Z","X","C","V","B","N","M", NULL};

/* Symbols mode */
static const char *cat__kb5_row0_sym[] = {"!","@","#","$","%","^","&","*","(",")", NULL};
static const char *cat__kb5_row1_sym[] = {"`","~","[","]","\\","|","{","}",";",":", NULL};
static const char *cat__kb5_row2_sym[] = {"'","\"","<",">","?","/","+","=","_", NULL};
static const char *cat__kb5_row3_sym[] = {",",".","-","\xe2\x82\xac","\xc2\xa3","\xc2\xa5","\xc2\xa2", NULL};

/* Numeric layout */
static const char *cat__kb_numeric[] = {
    "1", "2", "3",
    "4", "5", "6",
    "7", "8", "9",
    NULL, "0", NULL,
};
#define CAT__KB_COLS_NUMERIC  3
#define CAT__KB_ROWS_NUMERIC  4

/* Keyboard row metadata for 5-row general layout */
typedef struct {
    const char **chars;       /* pointer to char array for this row */
    int          char_count;  /* number of character keys */
    int          special;     /* special key at end: CAT__KB_KEY_* or 0 = none */
    int          special_pre; /* special key at start: CAT__KB_KEY_* or 0 = none */
} cat__kb_row_info;

static int cat__kb5_count(const char **row) {
    int n = 0;
    while (row[n]) n++;
    return n;
}

static bool cat__kb_utf8_is_continuation_byte(unsigned char c) {
    return (c & 0xC0u) == 0x80u;
}

static int cat__kb_utf8_clamp_boundary(const char *text, int idx) {
    int len;

    if (!text) return 0;
    len = (int)strlen(text);
    if (idx < 0) idx = 0;
    if (idx > len) idx = len;

    while (idx > 0 && idx < len &&
           cat__kb_utf8_is_continuation_byte((unsigned char)text[idx])) {
        idx--;
    }
    return idx;
}

static int cat__kb_utf8_prev_boundary(const char *text, int idx) {
    int len;

    if (!text) return 0;
    len = (int)strlen(text);
    if (idx < 0) idx = 0;
    if (idx > len) idx = len;
    if (idx > 0 &&
        idx < len &&
        cat__kb_utf8_is_continuation_byte((unsigned char)text[idx])) {
        return cat__kb_utf8_clamp_boundary(text, idx);
    }

    idx = cat__kb_utf8_clamp_boundary(text, idx);
    if (idx <= 0) return 0;
    idx--;
    while (idx > 0 && cat__kb_utf8_is_continuation_byte((unsigned char)text[idx])) {
        idx--;
    }
    return idx;
}

static int cat__kb_utf8_next_boundary(const char *text, int idx) {
    int len;

    if (!text) return 0;
    len = (int)strlen(text);
    if (idx < 0) idx = 0;
    if (idx > len) idx = len;
    if (idx >= len) return len;

    if (cat__kb_utf8_is_continuation_byte((unsigned char)text[idx])) {
        while (idx < len &&
               cat__kb_utf8_is_continuation_byte((unsigned char)text[idx])) {
            idx++;
        }
        return idx;
    }

    idx++;
    while (idx < len && cat__kb_utf8_is_continuation_byte((unsigned char)text[idx])) {
        idx++;
    }
    return idx;
}

/* Helper: insert a string at text_cursor in result->text */
static void cat__kb_insert(cat_keyboard_result *result, int *text_cursor, const char *str) {
    int cursor;
    int len;
    int slen;

    if (!result || !text_cursor || !str) return;
    cursor = cat__kb_utf8_clamp_boundary(result->text, *text_cursor);
    len = (int)strlen(result->text);
    slen = (int)strlen(str);
    if (len + slen < (int)sizeof(result->text) - 1) {
        memmove(result->text + cursor + slen,
                result->text + cursor,
                len - cursor + 1);
        memcpy(result->text + cursor, str, slen);
        *text_cursor = cursor + slen;
    }
}

/* Helper: backspace at text_cursor */
static void cat__kb_backspace(cat_keyboard_result *result, int *text_cursor) {
    int cursor;
    int prev;
    int len;

    if (!result || !text_cursor) return;
    cursor = cat__kb_utf8_clamp_boundary(result->text, *text_cursor);
    if (cursor <= 0) {
        *text_cursor = 0;
        return;
    }

    prev = cat__kb_utf8_prev_boundary(result->text, cursor);
    len = (int)strlen(result->text);
    memmove(result->text + prev,
            result->text + cursor,
            len - cursor + 1);
    *text_cursor = prev;
}

static void cat__kb_move_cursor_left(const char *text, int *text_cursor) {
    if (!text_cursor) return;
    *text_cursor = cat__kb_utf8_prev_boundary(text, *text_cursor);
}

static void cat__kb_move_cursor_right(const char *text, int *text_cursor) {
    if (!text_cursor) return;
    *text_cursor = cat__kb_utf8_next_boundary(text, *text_cursor);
}

static void cat__kb_draw_footer(void) {
    cat_footer_item kb_footer[] = {
        { .button = CAT_BTN_Y,     .label = "CANCEL",  .is_confirm = false },
        { .button = CAT_BTN_MENU,  .label = "HELP",    .is_confirm = false },
        { .button = CAT_BTN_START, .label = "CONFIRM", .is_confirm = true  },
    };
    cat_draw_footer(kb_footer, (int)(sizeof(kb_footer) / sizeof(kb_footer[0])));
}

/* Draw keyboard input text field content with horizontal scrolling.
   Keeps the caret visible by adjusting *text_scroll as the cursor moves. */
static void cat__kb_draw_input_text(TTF_Font *font, cat_keyboard_result *result,
                                   int text_cursor, int *text_scroll,
                                   int input_x, int input_y, int input_w, int input_h,
                                   bool caret_visible, cat_draw_color text_color) {
    int ty = input_y + (input_h - TTF_FontHeight(font)) / 2;
    int tx = input_x + CAT_S(16);
    int field_w = input_w - CAT_S(32);
    int safe_cursor;

    if (field_w <= 0) return;
    int caret_w = CAT_S(2);
    safe_cursor = cat__kb_utf8_clamp_boundary(result->text, text_cursor);

    /* Measure cursor pixel position */
    char saved = result->text[safe_cursor];
    result->text[safe_cursor] = '\0';
    int cursor_px = cat_measure_text(font, result->text);
    result->text[safe_cursor] = saved;

    /* Adjust scroll to keep caret visible, centering when it drifts */
    if (cursor_px - *text_scroll > field_w - caret_w)
        *text_scroll = cursor_px - field_w / 2;
    if (*text_scroll > 0 && cursor_px - *text_scroll < field_w / 3)
        *text_scroll = cursor_px - field_w / 2;
    if (*text_scroll < 0)
        *text_scroll = 0;
    int full_w = cat_measure_text(font, result->text);
    int max_scroll = full_w - field_w;
    if (max_scroll < 0) max_scroll = 0;
    if (*text_scroll > max_scroll)
        *text_scroll = max_scroll;

    /* Draw text with scroll offset, clipped to field */
    if (result->text[0]) {
        SDL_Rect clip = {tx, ty, field_w, TTF_FontHeight(font)};
        SDL_RenderSetClipRect(cat_get_renderer(), &clip);
        cat_draw_text(font, result->text, tx - *text_scroll, ty, text_color);
        SDL_RenderSetClipRect(cat_get_renderer(), NULL);
    }

    /* Draw caret */
    if (caret_visible) {
        int cx = tx + cursor_px - *text_scroll;
        cat_draw_rect(cx, ty, caret_w, TTF_FontHeight(font), text_color);
    }
}

/* Built-in keyboard help text matching Gabagool's instructions */
static const char *cat__kb_help_default =
    "D-Pad: Navigate between keys\n"
    "A: Type the selected key\n"
    "B: Backspace\n"
    "X: Space\n"
    "L1 / R1: Move cursor within text\n"
    "Select: Toggle Shift (uppercase/symbols)\n"
    "Y: Cancel without saving\n"
    "Start: Confirm input";

static const char *cat__kb_help_numeric =
    "D-Pad: Navigate between keys\n"
    "A: Type the selected digit\n"
    "B: Backspace\n"
    "L1 / R1: Move cursor within text\n"
    "Y: Cancel without saving\n"
    "Start: Confirm input";

static const char *cat__kb_help_url =
    "D-Pad: Navigate between keys\n"
    "A: Type the selected key\n"
    "B: Backspace\n"
    "X: Toggle shortcut alternates\n"
    "L1 / R1: Move cursor within text\n"
    "Select: Toggle Shift (uppercase)\n"
    "Y: Cancel without saving\n"
    "Start: Confirm input\n"
    "123/abc: Toggle number/symbol grid";

/* Extended keyboard: identical to cat_keyboard() but accepts an optional
   full-screen backdrop texture. When `backdrop` is non-NULL it is blitted to
   fill the screen each frame in place of the themed background — letting a
   caller (e.g. a Cover Flow launcher) show its own snapshot behind the keys.
   A supplied backdrop also suppresses the control-hint footer and maps X to
   cancel/back-out (the caller owns the chrome, and its users are expected to
   know the controls — matching the host's overlay). cat_keyboard()
   below is a thin wrapper that passes backdrop = NULL, so every existing caller
   is byte-for-byte unchanged. */
int cat_keyboard_ex(const char *initial_text, const char *help_text,
                cat_keyboard_layout layout, SDL_Texture *backdrop,
                cat_keyboard_result *result) {
    if (!result) return CAT_ERROR;

    memset(result, 0, sizeof(*result));
    if (initial_text) {
        strncpy(result->text, initial_text, sizeof(result->text) - 1);
    }

    cat_theme *theme = cat_get_theme();
    int screen_w = cat_get_screen_width();
    int screen_h = cat_get_screen_height();

    TTF_Font *text_font    = cat_get_font(CAT_FONT_MEDIUM);
    TTF_Font *key_font     = cat_get_font(CAT_FONT_MEDIUM);   /* Gabagool: MediumFont for keys */
    /* Special keys (⇧ ← ↵) use the glyph-complete symbol font, not the themed
       family — lean fonts (Inter, Nunito, Lexend…) lack those symbols. */
    TTF_Font *special_font = cat_get_symbol_font();
    if (!text_font || !key_font) return CAT_ERROR;

    bool is_numeric = (layout == CAT_KB_NUMERIC);

    /* ── Numeric keyboard (simple 3×4 grid) ── */
    if (is_numeric) {
        int kb_cols = CAT__KB_COLS_NUMERIC;
        int kb_rows = CAT__KB_ROWS_NUMERIC;
        int key_w = CAT_S(80);
        int key_h = CAT_S(52);
        int key_gap = CAT_S(6);
        int key_r = CAT_S(8);
        int grid_w = kb_cols * (key_w + key_gap) - key_gap;
        int grid_x = (screen_w - grid_w) / 2;
        int grid_y = screen_h - (kb_rows * (key_h + key_gap)) - cat_get_footer_height();
        int input_y = CAT_S(40), input_h = CAT_S(60), input_x = CAT_S(40);
        int input_w = screen_w - CAT_S(80);
        int cursor_x = 0, cursor_y = 0;
        int text_cursor = (int)strlen(result->text);
        int text_scroll = 0;
        bool running = true;
        uint32_t caret_blink = SDL_GetTicks();
        bool caret_visible = true;

        while (running) {
            uint32_t now = SDL_GetTicks();
            if (now - caret_blink > 500) { caret_visible = !caret_visible; caret_blink = now; }
            cat_request_frame_in(500);

            cat_input_event ev;
            while (cat_poll_input(&ev)) {
                if (!ev.pressed) continue;
                switch (ev.button) {
                    case CAT_BTN_UP:    cursor_y = (cursor_y - 1 + kb_rows) % kb_rows; break;
                    case CAT_BTN_DOWN:  cursor_y = (cursor_y + 1) % kb_rows; break;
                    case CAT_BTN_LEFT:  cursor_x = (cursor_x - 1 + kb_cols) % kb_cols; break;
                    case CAT_BTN_RIGHT: cursor_x = (cursor_x + 1) % kb_cols; break;
                    case CAT_BTN_A: {
                        int ki = cursor_y * kb_cols + cursor_x;
                        if (ki < kb_rows * kb_cols && cat__kb_numeric[ki])
                            cat__kb_insert(result, &text_cursor, cat__kb_numeric[ki]);
                        break;
                    }
                    case CAT_BTN_B: cat__kb_backspace(result, &text_cursor); break;
                    case CAT_BTN_X: if (backdrop) return CAT_CANCELLED; break;
                    case CAT_BTN_Y: return CAT_CANCELLED;
                    case CAT_BTN_START: return CAT_OK;
                    case CAT_BTN_L1: cat__kb_move_cursor_left(result->text, &text_cursor); break;
                    case CAT_BTN_R1: cat__kb_move_cursor_right(result->text, &text_cursor); break;
                    case CAT_BTN_MENU:
                        cat_show_help_overlay(help_text ? help_text : cat__kb_help_numeric);
                        break;
                    default: break;
                }
            }

            if (backdrop) SDL_RenderCopy(cat_get_renderer(), backdrop, NULL, NULL);
            else          cat_draw_background();
            cat_draw_pill(input_x, input_y, input_w, input_h, theme->highlight);
            cat__kb_draw_input_text(text_font, result, text_cursor, &text_scroll,
                                   input_x, input_y, input_w, input_h,
                                   caret_visible, theme->highlighted_text);
            for (int row = 0; row < kb_rows; row++) {
                for (int col = 0; col < kb_cols; col++) {
                    int ki = row * kb_cols + col;
                    const char *key = cat__kb_numeric[ki];
                    if (!key) continue;
                    int kx = grid_x + col * (key_w + key_gap);
                    int ky = grid_y + row * (key_h + key_gap);
                    bool sel = (row == cursor_y && col == cursor_x);
                    cat_draw_rounded_rect(kx, ky, key_w, key_h, key_r, sel ? theme->highlight : theme->accent);
                    int tw = cat_measure_text(key_font, key);
                    cat_draw_text(key_font, key, kx + (key_w - tw)/2, ky + (key_h - TTF_FontHeight(key_font))/2,
                                 sel ? theme->highlighted_text : theme->hint);
                }
            }
            if (!backdrop) cat__kb_draw_footer();
            cat_present();

        }
        return CAT_CANCELLED;
    }

    /* ═══════════════════════════════════════════════════════════════════════
     * 5-Row General Keyboard (matches Gabagool)
     * ═══════════════════════════════════════════════════════════════════════ */

    /* Keyboard occupies available space minus footer, text input = screen_h/10 */
    int footer_h = cat_get_footer_height();
    int kb_area_h  = screen_h * 85 / 100 - footer_h;
    int input_h    = screen_h / 10;
    int input_y    = (screen_h - kb_area_h - footer_h - input_h) / 2;
    int input_x    = CAT_S(40);
    int input_w    = screen_w - CAT_S(80);

    /* 12-column grid, 5 key rows (footer handled separately) */
    int grid_cols = 12;
    int grid_rows_total = 5;
    int key_spacing = CAT_S(4);
    int key_w = (screen_w - CAT_S(40) - key_spacing * (grid_cols - 1)) / grid_cols;
    int key_h = (kb_area_h - key_spacing * (grid_rows_total - 1)) / grid_rows_total;
    int key_r = CAT_S(6);

    /* Grid top-left */
    int grid_x = (screen_w - (grid_cols * (key_w + key_spacing) - key_spacing)) / 2;
    int grid_y = input_y + input_h + CAT_S(10);

    /* State */
    int cursor_row = 1;  /* Start on qwerty row */
    int cursor_col = 0;
    bool shift = false;
    bool symbols = false;
    int text_cursor = (int)strlen(result->text);
    int text_scroll = 0;
    bool running = true;

    uint32_t caret_blink = SDL_GetTicks();
    bool caret_visible = true;

    /* Current key arrays per row */
    #define CAT__KB5_ROW_COUNT 5

    while (running) {
        uint32_t now = SDL_GetTicks();
        if (now - caret_blink > 500) { caret_visible = !caret_visible; caret_blink = now; }
        cat_request_frame_in(500);

        /* Build current row info from mode */
        const char **r0 = symbols ? cat__kb5_row0_sym : (shift ? cat__kb5_row0_upper : cat__kb5_row0_lower);
        const char **r1 = symbols ? cat__kb5_row1_sym : (shift ? cat__kb5_row1_upper : cat__kb5_row1_lower);
        const char **r2 = symbols ? cat__kb5_row2_sym : (shift ? cat__kb5_row2_upper : cat__kb5_row2_lower);
        const char **r3 = symbols ? cat__kb5_row3_sym : (shift ? cat__kb5_row3_upper : cat__kb5_row3_lower);

        int r0n = cat__kb5_count(r0);  /* 10 */
        int r1n = cat__kb5_count(r1);  /* 10 */
        int r2n = cat__kb5_count(r2);  /* 9 */
        int r3n = cat__kb5_count(r3);  /* 7 */

        /* Effective navigation column limits per row:
         * Row 0: 0..10 (10 chars + backspace at col 10)
         * Row 1: 0..9  (10 chars)
         * Row 2: 0..9  (9 chars + enter at col 9)
         * Row 3: 0..8  (shift at 0, then 7 chars at 1..7, symbol at 8)
         * Row 4: 0 only (space bar)  */
        int row_max_col[CAT__KB5_ROW_COUNT] = {10, 9, 9, 8, 0};

        /* Input */
        cat_input_event iev;
        while (cat_poll_input(&iev)) {
            if (!iev.pressed) continue;

            switch (iev.button) {
                case CAT_BTN_UP:
                    cursor_row = (cursor_row - 1 + CAT__KB5_ROW_COUNT) % CAT__KB5_ROW_COUNT;
                    if (cursor_col > row_max_col[cursor_row])
                        cursor_col = row_max_col[cursor_row];
                    break;
                case CAT_BTN_DOWN:
                    cursor_row = (cursor_row + 1) % CAT__KB5_ROW_COUNT;
                    if (cursor_col > row_max_col[cursor_row])
                        cursor_col = row_max_col[cursor_row];
                    break;
                case CAT_BTN_LEFT:
                    cursor_col--;
                    if (cursor_col < 0) cursor_col = row_max_col[cursor_row];
                    break;
                case CAT_BTN_RIGHT:
                    cursor_col++;
                    if (cursor_col > row_max_col[cursor_row]) cursor_col = 0;
                    break;

                case CAT_BTN_A: {
                    /* Type the selected character or activate special key */
                    if (cursor_row == 0) {
                        if (cursor_col < r0n)
                            cat__kb_insert(result, &text_cursor, r0[cursor_col]);
                        else /* backspace */
                            cat__kb_backspace(result, &text_cursor);
                    } else if (cursor_row == 1) {
                        if (cursor_col < r1n)
                            cat__kb_insert(result, &text_cursor, r1[cursor_col]);
                    } else if (cursor_row == 2) {
                        if (cursor_col < r2n)
                            cat__kb_insert(result, &text_cursor, r2[cursor_col]);
                        else /* enter = confirm */
                            return CAT_OK;
                    } else if (cursor_row == 3) {
                        if (cursor_col == 0) {
                            /* shift */
                            shift = !shift; symbols = false;
                        } else if (cursor_col <= r3n) {
                            cat__kb_insert(result, &text_cursor, r3[cursor_col - 1]);
                        } else {
                            /* symbol */
                            symbols = !symbols; shift = false;
                        }
                    } else if (cursor_row == 4) {
                        /* space bar */
                        cat__kb_insert(result, &text_cursor, " ");
                    }
                    break;
                }

                case CAT_BTN_B:
                    /* Backspace (Gabagool mapping) */
                    cat__kb_backspace(result, &text_cursor);
                    break;

                case CAT_BTN_X:
                    /* Immersive (backdrop) mode: X backs out, matching the
                       host's overlay. Otherwise it's Space (Gabagool mapping). */
                    if (backdrop) return CAT_CANCELLED;
                    cat__kb_insert(result, &text_cursor, " ");
                    break;

                case CAT_BTN_Y:
                    /* Exit without saving (Gabagool mapping) */
                    return CAT_CANCELLED;

                case CAT_BTN_SELECT:
                    /* Toggle shift (Gabagool mapping) */
                    shift = !shift; symbols = false;
                    break;

                case CAT_BTN_START:
                    /* Enter / confirm */
                    return CAT_OK;

                case CAT_BTN_L1:
                    /* Move text cursor left */
                    cat__kb_move_cursor_left(result->text, &text_cursor);
                    break;

                case CAT_BTN_R1:
                    /* Move text cursor right */
                    cat__kb_move_cursor_right(result->text, &text_cursor);
                    break;

                case CAT_BTN_MENU:
                    /* Show keyboard help overlay.
                     *
                     * NOTE: Unlike earlier releases, this uses the caller-provided
                     * `help_text` (when non-NULL) as the Menu help overlay content,
                     * falling back to `cat__kb_help_default` only when `help_text`
                     * is NULL. Callers should therefore pass concise help/usage
                     * strings here, not prompt-like text, since it will be shown
                     * verbatim in the overlay.
                     */
                    cat_show_help_overlay(help_text ? help_text : cat__kb_help_default);
                    break;

                default: break;
            }
        }

        /* ── Render ── */
        if (backdrop) SDL_RenderCopy(cat_get_renderer(), backdrop, NULL, NULL);
        else          cat_draw_background();

        /* Text input field */
        cat_draw_rounded_rect(input_x, input_y, input_w, input_h, CAT_S(8), theme->highlight);
        cat__kb_draw_input_text(text_font, result, text_cursor, &text_scroll,
                               input_x, input_y, input_w, input_h,
                               caret_visible, theme->highlighted_text);

        /* ── Key rendering ── */
        cat_draw_color key_bg_normal  = theme->accent;
        cat_draw_color key_bg_sel     = theme->highlight;
        cat_draw_color key_fg_normal  = theme->hint;
        cat_draw_color key_fg_sel     = theme->highlighted_text;

        /* Helper macro: draw a single key rectangle with text */
        #define CAT__KB_DRAW_KEY(x, y, w, h, label, is_sel, font_to_use) do { \
            cat_draw_color _bg = (is_sel) ? key_bg_sel : key_bg_normal; \
            cat_draw_color _fg = (is_sel) ? key_fg_sel : key_fg_normal; \
            cat_draw_rounded_rect((x), (y), (w), (h), key_r, _bg); \
            if (label) { \
                int _tw = cat_measure_text((font_to_use), (label)); \
                int _th = TTF_FontHeight((font_to_use)); \
                cat_draw_text((font_to_use), (label), \
                    (x) + ((w) - _tw) / 2, (y) + ((h) - _th) / 2, _fg); \
            } \
        } while(0)

        /* Row 0: numbers + backspace */
        {
            int row_y = grid_y;
            int cx = grid_x;
            for (int i = 0; i < r0n; i++) {
                bool sel = (cursor_row == 0 && cursor_col == i);
                CAT__KB_DRAW_KEY(cx, row_y, key_w, key_h, r0[i], sel, key_font);
                cx += key_w + key_spacing;
            }
            /* Backspace — 2× width */
            {
                int bw = key_w * 2 + key_spacing;
                bool sel = (cursor_row == 0 && cursor_col == r0n);
                CAT__KB_DRAW_KEY(cx, row_y, bw, key_h, "\xe2\x86\x90", sel, special_font ? special_font : key_font); /* ← */
            }
        }

        /* Row 1: qwertyuiop (centered) */
        {
            int row_y = grid_y + (key_h + key_spacing);
            int row_w = r1n * (key_w + key_spacing) - key_spacing;
            int cx = (screen_w - row_w) / 2;
            for (int i = 0; i < r1n; i++) {
                bool sel = (cursor_row == 1 && cursor_col == i);
                CAT__KB_DRAW_KEY(cx, row_y, key_w, key_h, r1[i], sel, key_font);
                cx += key_w + key_spacing;
            }
        }

        /* Row 2: asdfghjkl + enter */
        {
            int row_y = grid_y + 2 * (key_h + key_spacing);
            /* 9 keys + enter (1.5× width) */
            int enter_w = key_w * 3 / 2 + key_spacing / 2;
            int row_w = r2n * (key_w + key_spacing) + enter_w;
            int cx = (screen_w - row_w) / 2;
            for (int i = 0; i < r2n; i++) {
                bool sel = (cursor_row == 2 && cursor_col == i);
                CAT__KB_DRAW_KEY(cx, row_y, key_w, key_h, r2[i], sel, key_font);
                cx += key_w + key_spacing;
            }
            /* Enter */
            {
                bool sel = (cursor_row == 2 && cursor_col == r2n);
                CAT__KB_DRAW_KEY(cx, row_y, enter_w, key_h, "\xe2\x86\xb5", sel, special_font ? special_font : key_font); /* ↵ */
            }
        }

        /* Row 3: shift + zxcvbnm + symbol */
        {
            int row_y = grid_y + 3 * (key_h + key_spacing);
            int shift_w = key_w * 2 + key_spacing;
            int sym_w   = key_w * 2 + key_spacing;
            int row_w = shift_w + key_spacing + r3n * (key_w + key_spacing) - key_spacing + key_spacing + sym_w;
            int cx = (screen_w - row_w) / 2;
            /* Shift */
            {
                bool sel = (cursor_row == 3 && cursor_col == 0);
                CAT__KB_DRAW_KEY(cx, row_y, shift_w, key_h, CAT__KB_SHIFT_LABEL, sel, special_font ? special_font : key_font);
                cx += shift_w + key_spacing;
            }
            /* Character keys */
            for (int i = 0; i < r3n; i++) {
                bool sel = (cursor_row == 3 && cursor_col == i + 1);
                CAT__KB_DRAW_KEY(cx, row_y, key_w, key_h, r3[i], sel, key_font);
                cx += key_w + key_spacing;
            }
            /* Symbol toggle */
            {
                bool sel = (cursor_row == 3 && cursor_col == r3n + 1);
                CAT__KB_DRAW_KEY(cx, row_y, sym_w, key_h, symbols ? "ABC" : "#+=", sel, key_font);
            }
        }

        /* Row 4: space bar (8× width, centered) */
        {
            int row_y = grid_y + 4 * (key_h + key_spacing);
            int space_w = key_w * 8 + key_spacing * 7;
            int sx = (screen_w - space_w) / 2;
            bool sel = (cursor_row == 4);
            cat_draw_color bg = sel ? key_bg_sel : key_bg_normal;
            cat_draw_rounded_rect(sx, row_y, space_w, key_h, key_r, bg);
            /* Space indicator: centered line */
            int line_w = space_w / 3;
            int line_h = CAT_S(3);
            int line_x = sx + (space_w - line_w) / 2;
            int line_y = row_y + (key_h - line_h) / 2;
            cat_draw_color line_c = sel ? key_fg_sel : key_fg_normal;
            cat_draw_rect(line_x, line_y, line_w, line_h, line_c);
        }

        #undef CAT__KB_DRAW_KEY

        if (!backdrop) cat__kb_draw_footer();

        cat_present();

    }

    return CAT_CANCELLED;
}

/* Backdrop-free keyboard — the original entry point. Unchanged behavior. */
int cat_keyboard(const char *initial_text, const char *help_text,
                cat_keyboard_layout layout, cat_keyboard_result *result) {
    return cat_keyboard_ex(initial_text, help_text, layout, NULL, result);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * URL KEYBOARD Implementation — Gabagool-matching with shortcut rows
 * ═══════════════════════════════════════════════════════════════════════════ */

/* Default URL shortcuts (normal mode) */
static const char *cat__url_shortcuts_default[] = {
    "https://", "www.", ".com", ".org", ".net",
    ".io", ".dev", ".app", ".edu", ".gov",
};
/* Symbol-alternate shortcuts */
static const char *cat__url_shortcuts_alt[] = {
    "http://", "ftp://", ".co", ".tv", ".me",
    ".gg", ".uk", ".de", ".ca", ".au",
};
/* URL-specific character row */
static const char *cat__url_chars[] = {"/",":","@","-","_",".","~","?","#","&", NULL};

int cat_url_keyboard(const char *initial_text, const char *help_text,
                    cat_url_keyboard_config *cfg, cat_keyboard_result *result) {
    if (!result) return CAT_ERROR;

    memset(result, 0, sizeof(*result));
    if (initial_text)
        strncpy(result->text, initial_text, sizeof(result->text) - 1);

    cat_theme *theme = cat_get_theme();
    int screen_w = cat_get_screen_width();
    int screen_h = cat_get_screen_height();

    TTF_Font *text_font    = cat_get_font(CAT_FONT_MEDIUM);
    TTF_Font *key_font     = cat_get_font(CAT_FONT_MEDIUM);
    TTF_Font *shortcut_font = cat_get_font(CAT_FONT_SMALL);
    TTF_Font *special_font = cat_get_symbol_font();  /* glyph-complete symbols (⇧ ← ↵) */
    if (!text_font || !key_font) return CAT_ERROR;

    /* Use provided shortcuts or defaults */
    const char **shortcuts = cat__url_shortcuts_default;
    const char **shortcuts_alt = cat__url_shortcuts_alt;
    int shortcut_count = 10;
    if (cfg && cfg->shortcut_keys && cfg->shortcut_count > 0) {
        shortcuts = cfg->shortcut_keys;
        shortcut_count = cfg->shortcut_count;
        if (shortcut_count > 10) shortcut_count = 10;
        shortcuts_alt = NULL; /* No alternates for custom shortcuts */
    }

    /* Layout: 5-row if ≤5 shortcuts, 6-row if 6-10 */
    int shortcut_rows = (shortcut_count > 5) ? 2 : 1;
    int shortcuts_per_row = (shortcut_rows == 2) ? (shortcut_count + 1) / 2 : shortcut_count;
    if (shortcuts_per_row > 5) shortcuts_per_row = 5;

    /* Total rows: shortcut_rows + url_chars + qwerty + asdf + zxcv (no space bar in URL mode) */
    int total_rows = shortcut_rows + 4; /* url chars + 3 QWERTY rows */

    /* Keyboard sizing */
    int footer_h = cat_get_footer_height();
    int kb_area_h = screen_h * 85 / 100 - footer_h;
    int input_h = screen_h / 10;
    int input_y = (screen_h - kb_area_h - input_h) / 2;
    int input_x = CAT_S(40);
    int input_w = screen_w - CAT_S(80);
    int key_spacing = CAT_S(4);
    int grid_cols = 12;
    int key_w = (screen_w - CAT_S(40) - key_spacing * (grid_cols - 1)) / grid_cols;
    int key_h = (kb_area_h - key_spacing * (total_rows + 1)) / (total_rows + 1);
    int key_r = CAT_S(6);
    int grid_y = input_y + input_h + CAT_S(10);

    /* State */
    int cursor_row = shortcut_rows; /* start on url chars row */
    int cursor_col = 0;
    bool shift = false;
    bool sym_alt = false; /* toggle for shortcut alternates */
    bool numbers = false; /* toggle number/symbol grid */
    int text_cursor = (int)strlen(result->text);
    int text_scroll = 0;
    bool running = true;
    uint32_t caret_blink = SDL_GetTicks();
    bool caret_visible = true;

    /* Row max columns for navigation */
    /* shortcut rows: shortcuts_per_row-1 (+ backspace on last shortcut row) */
    /* url_chars row: 9 */
    /* qwerty rows: same as general kb rows 1,2,3 but no space */

    while (running) {
        uint32_t now = SDL_GetTicks();
        if (now - caret_blink > 500) { caret_visible = !caret_visible; caret_blink = now; }
        cat_request_frame_in(500);

        /* Current active shortcuts */
        const char **active_shortcuts = (sym_alt && shortcuts_alt) ? shortcuts_alt : shortcuts;

        /* Row max col computation */
        int row_max[8] = {0}; /* up to 8 rows max */
        int ri = 0;
        for (int sr = 0; sr < shortcut_rows; sr++, ri++) {
            int first = sr * shortcuts_per_row;
            int cnt = 0;
            for (int i = first; i < shortcut_count && i < first + shortcuts_per_row; i++) cnt++;
            row_max[ri] = (sr == shortcut_rows - 1) ? cnt : cnt - 1; /* last shortcut row has backspace */
        }
        int url_chars_row = ri; ri++;
        row_max[url_chars_row] = 9;
        int qwerty_row = ri; ri++;
        row_max[qwerty_row] = 9;
        int asdf_row = ri; ri++;
        row_max[asdf_row] = 9; /* 9 + enter */
        int zxcv_row = ri;
        row_max[zxcv_row] = 8; /* shift + 7 + symbol */
        int actual_total_rows = ri + 1;

        /* Input */
        cat_input_event iev;
        while (cat_poll_input(&iev)) {
            if (!iev.pressed) continue;
            switch (iev.button) {
                case CAT_BTN_UP:
                    cursor_row = (cursor_row - 1 + actual_total_rows) % actual_total_rows;
                    if (cursor_col > row_max[cursor_row]) cursor_col = row_max[cursor_row];
                    break;
                case CAT_BTN_DOWN:
                    cursor_row = (cursor_row + 1) % actual_total_rows;
                    if (cursor_col > row_max[cursor_row]) cursor_col = row_max[cursor_row];
                    break;
                case CAT_BTN_LEFT:
                    cursor_col--;
                    if (cursor_col < 0) cursor_col = row_max[cursor_row];
                    break;
                case CAT_BTN_RIGHT:
                    cursor_col++;
                    if (cursor_col > row_max[cursor_row]) cursor_col = 0;
                    break;

                case CAT_BTN_A: {
                    /* Shortcut rows */
                    if (cursor_row < shortcut_rows) {
                        int first = cursor_row * shortcuts_per_row;
                        if (cursor_row == shortcut_rows - 1 && cursor_col == row_max[cursor_row]) {
                            /* Backspace on last shortcut row */
                            cat__kb_backspace(result, &text_cursor);
                        } else {
                            int si = first + cursor_col;
                            if (si < shortcut_count)
                                cat__kb_insert(result, &text_cursor, active_shortcuts[si]);
                        }
                    }
                    /* URL chars row */
                    else if (cursor_row == url_chars_row) {
                        if (numbers) {
                            if (cursor_col < cat__kb5_count(cat__kb5_row0_lower))
                                cat__kb_insert(result, &text_cursor, cat__kb5_row0_lower[cursor_col]);
                        } else {
                            if (cursor_col < cat__kb5_count((const char **)cat__url_chars))
                                cat__kb_insert(result, &text_cursor, cat__url_chars[cursor_col]);
                        }
                    }
                    /* QWERTY row */
                    else if (cursor_row == qwerty_row) {
                        if (numbers) {
                            if (cursor_col < cat__kb5_count(cat__kb5_row0_sym))
                                cat__kb_insert(result, &text_cursor, cat__kb5_row0_sym[cursor_col]);
                        } else {
                            const char **r = shift ? cat__kb5_row1_upper : cat__kb5_row1_lower;
                            if (cursor_col < cat__kb5_count(r))
                                cat__kb_insert(result, &text_cursor, r[cursor_col]);
                        }
                    }
                    /* ASDF row */
                    else if (cursor_row == asdf_row) {
                        if (numbers) {
                            int rn = cat__kb5_count(cat__kb5_row2_sym);
                            if (cursor_col < rn)
                                cat__kb_insert(result, &text_cursor, cat__kb5_row2_sym[cursor_col]);
                            else /* enter */
                                return CAT_OK;
                        } else {
                            const char **r = shift ? cat__kb5_row2_upper : cat__kb5_row2_lower;
                            int rn = cat__kb5_count(r);
                            if (cursor_col < rn)
                                cat__kb_insert(result, &text_cursor, r[cursor_col]);
                            else /* enter */
                                return CAT_OK;
                        }
                    }
                    /* ZXCV row */
                    else if (cursor_row == zxcv_row) {
                        if (cursor_col == 0) {
                            shift = !shift;
                        } else if (cursor_col == row_max[zxcv_row]) {
                            numbers = !numbers;
                        } else {
                            const char **r = numbers ? cat__kb5_row3_sym
                                           : (shift ? cat__kb5_row3_upper : cat__kb5_row3_lower);
                            int rn = cat__kb5_count(r);
                            if (cursor_col - 1 < rn)
                                cat__kb_insert(result, &text_cursor, r[cursor_col - 1]);
                        }
                    }
                    break;
                }

                case CAT_BTN_B:
                    cat__kb_backspace(result, &text_cursor);
                    break;
                case CAT_BTN_X:
                    /* Toggle symbol alternates for shortcuts (not space in URL mode) */
                    sym_alt = !sym_alt;
                    break;
                case CAT_BTN_Y:
                    return CAT_CANCELLED;
                case CAT_BTN_SELECT:
                    shift = !shift;
                    break;
                case CAT_BTN_START:
                    return CAT_OK;
                case CAT_BTN_L1:
                    cat__kb_move_cursor_left(result->text, &text_cursor);
                    break;
                case CAT_BTN_R1:
                    cat__kb_move_cursor_right(result->text, &text_cursor);
                    break;
                case CAT_BTN_MENU:
                    cat_show_help_overlay(help_text ? help_text : cat__kb_help_url);
                    break;
                default: break;
            }
        }

        /* ── Render ── */
        cat_draw_background();

        /* Input field */
        cat_draw_rounded_rect(input_x, input_y, input_w, input_h, CAT_S(8), theme->highlight);
        cat__kb_draw_input_text(text_font, result, text_cursor, &text_scroll,
                               input_x, input_y, input_w, input_h,
                               caret_visible, theme->highlighted_text);

        cat_draw_color key_bg_normal = theme->accent;
        cat_draw_color key_bg_sel    = theme->highlight;
        cat_draw_color key_fg_normal = theme->hint;
        cat_draw_color key_fg_sel    = theme->highlighted_text;

        #define CAT__KB_DRAW_KEY2(x, y, w, h, label, is_sel, fnt) do { \
            cat_draw_color _bg = (is_sel) ? key_bg_sel : key_bg_normal; \
            cat_draw_color _fg = (is_sel) ? key_fg_sel : key_fg_normal; \
            cat_draw_rounded_rect((x), (y), (w), (h), key_r, _bg); \
            if (label) { \
                int _tw = cat_measure_text((fnt), (label)); \
                int _th = TTF_FontHeight((fnt)); \
                cat_draw_text((fnt), (label), (x)+((w)-_tw)/2, (y)+((h)-_th)/2, _fg); \
            } \
        } while(0)

        int ry = grid_y;

        /* Shortcut rows */
        for (int sr = 0; sr < shortcut_rows; sr++) {
            int first = sr * shortcuts_per_row;
            int cnt = 0;
            for (int i = first; i < shortcut_count && i < first + shortcuts_per_row; i++) cnt++;

            /* Each shortcut uses 2 key widths */
            int short_w = key_w * 2 + key_spacing;
            bool has_backspace = (sr == shortcut_rows - 1);
            int bksp_w = has_backspace ? (key_w * 2 + key_spacing) : 0;
            int row_w = cnt * (short_w + key_spacing) - key_spacing + (has_backspace ? key_spacing + bksp_w : 0);
            int cx = (screen_w - row_w) / 2;

            for (int i = 0; i < cnt; i++) {
                int si = first + i;
                bool sel = (cursor_row == sr && cursor_col == i);
                CAT__KB_DRAW_KEY2(cx, ry, short_w, key_h, active_shortcuts[si], sel, shortcut_font ? shortcut_font : key_font);
                cx += short_w + key_spacing;
            }
            if (has_backspace) {
                bool sel = (cursor_row == sr && cursor_col == cnt);
                CAT__KB_DRAW_KEY2(cx, ry, bksp_w, key_h, "\xe2\x86\x90", sel, special_font ? special_font : key_font); /* ← */
            }
            ry += key_h + key_spacing;
        }

        /* URL chars / numbers row */
        {
            const char **row_keys = numbers ? cat__kb5_row0_lower : (const char **)cat__url_chars;
            int ucn = cat__kb5_count(row_keys);
            int row_w = ucn * (key_w + key_spacing) - key_spacing;
            int cx = (screen_w - row_w) / 2;
            for (int i = 0; i < ucn; i++) {
                bool sel = (cursor_row == url_chars_row && cursor_col == i);
                CAT__KB_DRAW_KEY2(cx, ry, key_w, key_h, row_keys[i], sel, key_font);
                cx += key_w + key_spacing;
            }
            ry += key_h + key_spacing;
        }

        /* QWERTY / symbols row */
        {
            const char **r = numbers ? cat__kb5_row0_sym
                           : (shift ? cat__kb5_row1_upper : cat__kb5_row1_lower);
            int rn = cat__kb5_count(r);
            int row_w = rn * (key_w + key_spacing) - key_spacing;
            int cx = (screen_w - row_w) / 2;
            for (int i = 0; i < rn; i++) {
                bool sel = (cursor_row == qwerty_row && cursor_col == i);
                CAT__KB_DRAW_KEY2(cx, ry, key_w, key_h, r[i], sel, key_font);
                cx += key_w + key_spacing;
            }
            ry += key_h + key_spacing;
        }

        /* ASDF / symbols + enter row */
        {
            const char **r = numbers ? cat__kb5_row2_sym
                           : (shift ? cat__kb5_row2_upper : cat__kb5_row2_lower);
            int rn = cat__kb5_count(r);
            int enter_w = key_w * 3 / 2;
            int row_w = rn * (key_w + key_spacing) + enter_w;
            int cx = (screen_w - row_w) / 2;
            for (int i = 0; i < rn; i++) {
                bool sel = (cursor_row == asdf_row && cursor_col == i);
                CAT__KB_DRAW_KEY2(cx, ry, key_w, key_h, r[i], sel, key_font);
                cx += key_w + key_spacing;
            }
            {
                bool sel = (cursor_row == asdf_row && cursor_col == rn);
                CAT__KB_DRAW_KEY2(cx, ry, enter_w, key_h, "\xe2\x86\xb5", sel, special_font ? special_font : key_font); /* ↵ */
            }
            ry += key_h + key_spacing;
        }

        /* ZXCV / symbols row: shift + chars + 123/abc toggle */
        {
            const char **r = numbers ? cat__kb5_row3_sym
                           : (shift ? cat__kb5_row3_upper : cat__kb5_row3_lower);
            int rn = cat__kb5_count(r);
            int shift_w = key_w * 2 + key_spacing;
            int toggle_w = key_w * 2 + key_spacing;
            int row_w = shift_w + key_spacing + rn * (key_w + key_spacing) + toggle_w;
            int cx = (screen_w - row_w) / 2;
            {
                bool sel = (cursor_row == zxcv_row && cursor_col == 0);
                CAT__KB_DRAW_KEY2(cx, ry, shift_w, key_h, CAT__KB_SHIFT_LABEL, sel, special_font ? special_font : key_font);
                cx += shift_w + key_spacing;
            }
            for (int i = 0; i < rn; i++) {
                bool sel = (cursor_row == zxcv_row && cursor_col == i + 1);
                CAT__KB_DRAW_KEY2(cx, ry, key_w, key_h, r[i], sel, key_font);
                cx += key_w + key_spacing;
            }
            {
                bool sel = (cursor_row == zxcv_row && cursor_col == row_max[zxcv_row]);
                CAT__KB_DRAW_KEY2(cx, ry, toggle_w, key_h, numbers ? "abc" : "123", sel, key_font);
            }
        }

        #undef CAT__KB_DRAW_KEY2

        cat__kb_draw_footer();

        cat_present();

    }

    return CAT_CANCELLED;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * CONFIRMATION MESSAGE Implementation
 * ═══════════════════════════════════════════════════════════════════════════ */

int cat_confirmation(cat_message_opts *opts, cat_confirm_result *result) {
    if (!opts || !result) return CAT_ERROR;

    memset(result, 0, sizeof(*result));
    result->confirmed = false;

    cat_theme *theme = cat_get_theme();
    int screen_w = cat_get_screen_width();
    int screen_h = cat_get_screen_height();
    int msg_max_w = screen_w - CAT_S(80);
    if (msg_max_w < 1) msg_max_w = (screen_w > 0) ? screen_w : 1;

    TTF_Font *msg_font = cat_get_font(CAT_FONT_MEDIUM);
    if (!msg_font) return CAT_ERROR;
    int msg_h = opts->message
        ? cat_measure_wrapped_text_height(msg_font, opts->message, msg_max_w)
        : 0;

    /* Load image if specified */
    SDL_Texture *img_tex = NULL;
    int img_w = 0, img_h = 0;
    if (opts->image_path) {
        img_tex = cat_load_image(opts->image_path);
        if (img_tex) {
            SDL_QueryTexture(img_tex, NULL, NULL, &img_w, &img_h);
            /* Scale image to fit */
            int max_img_w = screen_w / 2;
            int max_img_h = screen_h / 3;
            if (img_w > max_img_w || img_h > max_img_h) {
                float scale = fminf((float)max_img_w / img_w, (float)max_img_h / img_h);
                img_w = (int)(img_w * scale);
                img_h = (int)(img_h * scale);
            }
        }
    }

    bool running = true;
    while (running) {
        cat_input_event ev;
        while (cat_poll_input(&ev)) {
            if (!ev.pressed) continue;

            switch (ev.button) {
                case CAT_BTN_A:
                    result->confirmed = true;
                    running = false;
                    break;
                case CAT_BTN_B:
                    result->confirmed = false;
                    running = false;
                    break;
                default:
                    break;
            }
        }

        /* Render */
        cat_draw_background();

        /* Center the content vertically */
        int total_h = 0;
        if (img_tex) total_h += img_h + CAT_S(20);
        total_h += msg_h;

        int base_y = (screen_h - total_h - cat_get_footer_height()) / 2;
        int min_base_y = CAT_DS(cat__g.device_padding + 5);
        if (base_y < min_base_y) base_y = min_base_y;

        /* Image */
        if (img_tex) {
            cat_draw_image(img_tex,
                (screen_w - img_w) / 2, base_y, img_w, img_h);
            base_y += img_h + CAT_S(20);
        }

        /* Message (centered, wrapped) */
        if (opts->message) {
            cat_draw_text_wrapped(msg_font, opts->message,
                CAT_S(40), base_y,
                msg_max_w,
                theme->text, CAT_ALIGN_CENTER);
        }

        /* Footer */
        if (opts->footer && opts->footer_count > 0) {
            cat_draw_footer(opts->footer, opts->footer_count);
        }

        cat_present();

    }

    /* Cleanup */
    if (img_tex) SDL_DestroyTexture(img_tex);

    return result->confirmed ? CAT_OK : CAT_CANCELLED;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * SELECTION MESSAGE Implementation
 * ═══════════════════════════════════════════════════════════════════════════ */

int cat_selection(const char *message, cat_selection_option *options, int count,
                 cat_footer_item *footer, int footer_count,
                 cat_selection_result *result) {
    if (!result || !options || count <= 0) return CAT_ERROR;

    memset(result, 0, sizeof(*result));

    cat_theme *theme = cat_get_theme();
    int screen_w = cat_get_screen_width();
    int screen_h = cat_get_screen_height();

    TTF_Font *msg_font  = cat_get_font(CAT_FONT_MEDIUM);
    TTF_Font *opt_font  = cat_get_font(CAT_FONT_SMALL);
    if (!msg_font || !opt_font) return CAT_ERROR;

    int selected = 0;
    int pill_h = CAT_S(44);
    int pill_pad = CAT_S(16);
    int pill_gap = CAT_S(10);
    bool running = true;

    while (running) {
        cat_input_event ev;
        while (cat_poll_input(&ev)) {
            if (!ev.pressed) continue;

            switch (ev.button) {
                case CAT_BTN_LEFT:
                    if (selected > 0) selected--;
                    break;
                case CAT_BTN_RIGHT:
                    if (selected < count - 1) selected++;
                    break;
                case CAT_BTN_A:
                    result->selected_index = selected;
                    running = false;
                    break;
                case CAT_BTN_B:
                    return CAT_CANCELLED;
                default:
                    break;
            }
        }

        /* Render */
        cat_draw_background();

        /* Message */
        int msg_y = screen_h / 3;
        if (message) {
            int msg_h = cat_measure_wrapped_text_height(
                msg_font, message, screen_w - CAT_S(80));
            cat_draw_text_wrapped(msg_font, message,
                CAT_S(40), msg_y,
                screen_w - CAT_S(80),
                theme->text, CAT_ALIGN_CENTER);
            msg_y += msg_h + CAT_S(30);
        }

        /* Option pills (horizontal, centered) */
        int total_w = 0;
        for (int i = 0; i < count; i++) {
            total_w += cat_measure_text(opt_font, options[i].label) + pill_pad * 2;
            if (i < count - 1) total_w += pill_gap;
        }

        int opt_x = (screen_w - total_w) / 2;
        int opt_y = msg_y + CAT_S(20);

        for (int i = 0; i < count; i++) {
            int tw = cat_measure_text(opt_font, options[i].label);
            int pw = tw + pill_pad * 2;
            bool is_sel = (i == selected);

            cat_draw_color pill_bg = is_sel ? theme->highlight : theme->accent;
            cat_draw_color pill_fg = is_sel ? theme->highlighted_text : theme->hint;

            cat_draw_pill(opt_x, opt_y, pw, pill_h, pill_bg);
            cat_draw_text(opt_font, options[i].label,
                opt_x + pill_pad,
                opt_y + (pill_h - TTF_FontHeight(opt_font)) / 2,
                pill_fg);

            opt_x += pw + pill_gap;
        }

        if (footer && footer_count > 0) {
            cat_draw_footer(footer, footer_count);
        }

        cat_present();

    }

    return CAT_OK;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * PROCESS MESSAGE Implementation
 * ═══════════════════════════════════════════════════════════════════════════ */

/* Worker thread wrapper */
typedef struct {
    cat_process_fn  fn;
    void          *userdata;
    int            result;
    bool           done;
    pthread_mutex_t mutex;
} cat__process_thread_data;

static void *cat__process_worker(void *arg) {
    cat__process_thread_data *d = (cat__process_thread_data *)arg;
    int worker_result = d->fn(d->userdata);
    pthread_mutex_lock(&d->mutex);
    d->result = worker_result;
    d->done = true;
    pthread_mutex_unlock(&d->mutex);
    return NULL;
}

int cat_process_message(cat_process_opts *opts, cat_process_fn fn, void *userdata) {
    if (!opts || !fn) return CAT_ERROR;

    cat_theme *theme = cat_get_theme();
    int screen_w = cat_get_screen_width();
    int screen_h = cat_get_screen_height();

    TTF_Font *msg_font = cat_get_font(CAT_FONT_MEDIUM);
    TTF_Font *dyn_font = cat_get_font(CAT_FONT_SMALL);
    if (!msg_font) return CAT_ERROR;

    /* Start worker thread */
    cat__process_thread_data thread_data = {
        .fn = fn,
        .userdata = userdata,
        .result = 0,
        .done = false,
    };
    if (pthread_mutex_init(&thread_data.mutex, NULL) != 0) {
        return CAT_ERROR;
    }

    pthread_t worker;
    if (pthread_create(&worker, NULL, cat__process_worker, &thread_data) != 0) {
        pthread_mutex_destroy(&thread_data.mutex);
        return CAT_ERROR;
    }

    int bar_w = CAT_S(400);
    int bar_h = CAT_S(16);
    int bar_x = (screen_w - bar_w) / 2;

    bool running = true;
    while (running) {
        /* Check if worker is done */
        bool worker_done = false;
        pthread_mutex_lock(&thread_data.mutex);
        worker_done = thread_data.done;
        pthread_mutex_unlock(&thread_data.mutex);
        if (worker_done) {
            running = false;
            break;
        }

        /* Input: check for interrupt */
        cat_input_event ev;
        while (cat_poll_input(&ev)) {
            if (!ev.pressed) continue;
            if (opts->interrupt_button != CAT_BTN_NONE && ev.button == opts->interrupt_button) {
                if (opts->interrupt_signal) {
                    *(opts->interrupt_signal) = 1;
                }
            }
        }

        /* Render */
        cat_draw_background();

        int center_y = screen_h / 2;

        /* Static message */
        int msg_y = center_y - CAT_S(60);
        int msg_h = 0;
        if (opts->message) {
            cat_draw_text_wrapped(msg_font, opts->message,
                CAT_S(40), msg_y,
                screen_w - CAT_S(80),
                theme->text, CAT_ALIGN_CENTER);
            msg_h = cat_measure_wrapped_text_height(msg_font, opts->message,
                                                    screen_w - CAT_S(80));
        }

        /* Dynamic message — placed below the message's actual bottom plus a gap,
           so the two lines never crowd regardless of font size or wrapping. */
        int dyn_line_h = dyn_font ? TTF_FontHeight(dyn_font) : CAT_S(16);
        int dyn_lines = opts->dynamic_message ? (opts->message_lines > 0 ? opts->message_lines : 1) : 0;
        int dyn_y = msg_y + (msg_h > 0 ? msg_h + CAT_S(14) : CAT_S(40));

        if (opts->dynamic_message && *opts->dynamic_message) {
            cat_draw_text_wrapped(dyn_font, *opts->dynamic_message,
                CAT_S(40), dyn_y,
                screen_w - CAT_S(80),
                theme->hint, CAT_ALIGN_CENTER);
        }

        /* Progress bar — position below dynamic message area when present */
        if (opts->show_progress && opts->progress) {
            float prog = *opts->progress;
            int bar_y = (dyn_lines > 0)
                ? dyn_y + dyn_lines * dyn_line_h + CAT_S(12)
                : center_y + CAT_S(30);

            cat_draw_color bar_bg = theme->accent;
            bar_bg.a = 80;
            cat_draw_progress_bar(bar_x, bar_y, bar_w, bar_h, prog, theme->accent, bar_bg);

            /* Progress percentage */
            char pct[16];
            snprintf(pct, sizeof(pct), "%.0f%%", prog * 100.0f);
            int pct_w = cat_measure_text(dyn_font, pct);
            cat_draw_text(dyn_font, pct,
                (screen_w - pct_w) / 2,
                bar_y + bar_h + CAT_S(8),
                theme->hint);
            cat_request_frame_in(33); /* ~30fps is sufficient for a progress bar */
        }

        /* Spinner (simple dots animation when no progress bar) — sits below the
           dynamic message line when present, with the same gap the progress bar
           uses, so it never crowds the name above it. */
        if (!opts->show_progress || !opts->progress) {
            uint32_t ticks = SDL_GetTicks() / 300;
            int dots = (ticks % 4);
            char spinner[8] = "";
            for (int i = 0; i < dots; i++) strcat(spinner, ".");
            int sw = cat_measure_text(msg_font, spinner);
            int spin_y = (dyn_lines > 0)
                ? dyn_y + dyn_lines * dyn_line_h + CAT_S(16)
                : center_y + CAT_S(20);
            cat_draw_text(msg_font, spinner,
                (screen_w - sw) / 2,
                spin_y,
                theme->hint);
            cat_request_frame_in(300);
        }

        cat_present();

    }

    /* Wait for thread to finish */
    pthread_join(worker, NULL);
    int final_result = 0;
    pthread_mutex_lock(&thread_data.mutex);
    final_result = thread_data.result;
    pthread_mutex_unlock(&thread_data.mutex);
    pthread_mutex_destroy(&thread_data.mutex);

    return final_result;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * DETAIL SCREEN Implementation
 * ═══════════════════════════════════════════════════════════════════════════ */

int cat_detail_screen(cat_detail_opts *opts, cat_detail_result *result) {
    if (!opts || !result) return CAT_ERROR;

    memset(result, 0, sizeof(*result));

    cat_theme *theme = cat_get_theme();
    int screen_w = cat_get_screen_width();

    TTF_Font *title_font   = cat_get_font(CAT_FONT_SMALL);
    TTF_Font *section_font = opts->section_title_font ? opts->section_title_font : cat_get_font(CAT_FONT_SMALL);
    TTF_Font *body_font    = opts->body_font           ? opts->body_font           : cat_get_font(CAT_FONT_TINY);
    TTF_Font *key_font     = opts->key_font            ? opts->key_font            : cat_get_font(CAT_FONT_TINY);
    if (!title_font || !section_font || !body_font || !key_font) return CAT_ERROR;

    int margin = CAT_S(20);
    int section_gap = CAT_S(24);
    int scrollbar_gutter = CAT_S(16);
    int content_x = margin;
    int content_right = screen_w - margin - scrollbar_gutter;
    if (content_right <= content_x) content_right = screen_w - margin;
    int content_w = content_right - content_x;
    if (content_w < 1) content_w = 1;
    int body_line_h = TTF_FontLineSkip(body_font);
    int key_line_h = TTF_FontLineSkip(key_font);
    int info_row_h = cat__max(body_line_h, key_line_h) + CAT_S(4);
    int table_row_h = info_row_h;
    int section_title_h = TTF_FontLineSkip(section_font) + CAT_S(6);

    SDL_Texture **section_images = NULL;
    int *section_image_w = NULL;
    int *section_image_h = NULL;
    if (opts->section_count > 0) {
        size_t section_count = (size_t)opts->section_count;
        section_images = (SDL_Texture **)calloc(section_count, sizeof(*section_images));
        section_image_w = (int *)calloc(section_count, sizeof(*section_image_w));
        section_image_h = (int *)calloc(section_count, sizeof(*section_image_h));
        if (!section_images || !section_image_w || !section_image_h) {
            free(section_images);
            free(section_image_w);
            free(section_image_h);
            return CAT_ERROR;
        }

        for (int s = 0; s < opts->section_count; s++) {
            cat_detail_section *sec = &opts->sections[s];
            if (sec->type != CAT_SECTION_IMAGE) continue;
            if (!sec->image_path || !sec->image_path[0]) continue;

            section_images[s] = cat_load_image(sec->image_path);
            if (section_images[s]) {
                section_image_w[s] = sec->image_w > 0 ? sec->image_w : CAT_S(300);
                section_image_h[s] = sec->image_h > 0 ? sec->image_h : CAT_S(200);
            }
        }
    }

    /* Pre-compute per-section layout: heights, key widths, value widths.
       Used for total_content_h, offscreen culling, and cached render layout. */
    int n_sections = opts->section_count;
    if (n_sections < 0) {
        free(section_images); free(section_image_w); free(section_image_h);
        return CAT_ERROR;
    }
    int *section_total_h  = n_sections ? (int *)calloc((size_t)n_sections, sizeof(int)) : NULL;
    int *section_kw_max   = n_sections ? (int *)calloc((size_t)n_sections, sizeof(int)) : NULL;
    int *section_val_w    = n_sections ? (int *)calloc((size_t)n_sections, sizeof(int)) : NULL;
    int *section_val_x    = n_sections ? (int *)calloc((size_t)n_sections, sizeof(int)) : NULL;
    if (n_sections > 0 && (!section_total_h || !section_kw_max || !section_val_w || !section_val_x)) {
        free(section_total_h); free(section_kw_max); free(section_val_w); free(section_val_x);
        if (section_images) {
            for (int s = 0; s < opts->section_count; s++) {
                if (section_images[s]) SDL_DestroyTexture(section_images[s]);
            }
        }
        free(section_images); free(section_image_w); free(section_image_h);
        return CAT_ERROR;
    }

    int total_content_h = 0;
    for (int s = 0; s < n_sections; s++) {
        cat_detail_section *sec = &opts->sections[s];
        int h = 0;
        if (sec->title) h += section_title_h;

        switch (sec->type) {
            case CAT_SECTION_INFO: {
                int kw_max = 0;
                for (int p = 0; p < sec->info_count; p++) {
                    if (sec->info_pairs[p].key) {
                        int kw = cat_measure_text(key_font, sec->info_pairs[p].key);
                        if (kw > kw_max) kw_max = kw;
                    }
                }
                section_kw_max[s] = kw_max;
                int vx = margin + kw_max + CAT_S(16);
                int max_vx = content_right - CAT_S(120);
                if (vx > max_vx) vx = max_vx;
                if (vx < margin + CAT_S(16)) vx = margin + CAT_S(16);
                int vw = content_right - vx;
                if (vw < 1) vw = 1;
                section_val_x[s] = vx;
                section_val_w[s] = vw;
                for (int p = 0; p < sec->info_count; p++) {
                    int vh = sec->info_pairs[p].value
                        ? cat_measure_wrapped_text_height(body_font, sec->info_pairs[p].value, vw)
                        : 0;
                    h += cat__max(key_line_h, vh) + CAT_S(4);
                }
                break;
            }
            case CAT_SECTION_DESCRIPTION:
                if (sec->description) {
                    h += cat_measure_wrapped_text_height(body_font, sec->description, content_w);
                }
                break;
            case CAT_SECTION_IMAGE:
                if (section_images && section_images[s]) {
                    h += section_image_h[s];
                }
                break;
            case CAT_SECTION_TABLE:
                h += (sec->table_rows_count + 1) * table_row_h;
                break;
        }
        h += section_gap;
        section_total_h[s] = h;
        total_content_h += h;
    }

    SDL_Rect content_rect = cat_get_content_rect(opts->title != NULL, opts->footer_count > 0,
                                                opts->status_bar != NULL);
    int content_y = content_rect.y;
    int content_h = content_rect.h;

    int scroll_target = 0;
    float scroll_current = 0.0f;
    float scroll_from = 0.0f;
    uint32_t scroll_anim_start = 0;
    int max_scroll = total_content_h - content_h;
    if (max_scroll < 0) max_scroll = 0;

    bool running = true;
    while (running) {
        uint32_t now = SDL_GetTicks();

        int prev_scroll_target = scroll_target;

        cat_input_event ev;
        while (cat_poll_input(&ev)) {
            if (!ev.pressed) continue;

            switch (ev.button) {
                case CAT_BTN_UP:
                    scroll_target -= CAT_S(40);
                    if (scroll_target < 0) scroll_target = 0;
                    break;
                case CAT_BTN_DOWN:
                    scroll_target += CAT_S(40);
                    if (scroll_target > max_scroll) scroll_target = max_scroll;
                    break;
                case CAT_BTN_B:
                    result->action = CAT_DETAIL_BACK;
                    running = false;
                    break;
                case CAT_BTN_A:
                    result->action = CAT_DETAIL_ACTION;
                    running = false;
                    break;
                case CAT_BTN_Y:
                    result->action = CAT_DETAIL_SECONDARY_ACTION;
                    running = false;
                    break;
                default:
                    break;
            }
        }

        if (scroll_target != prev_scroll_target) {
            now = SDL_GetTicks();
            scroll_from = scroll_current;
            scroll_anim_start = now;
        }

        /* Animate scroll */
        if (scroll_target != (int)scroll_current || scroll_anim_start != 0) {
            float t = cat__clampf((float)(now - scroll_anim_start) / CAT__SCROLL_ANIM_MS, 0.0f, 1.0f);
            scroll_current = cat__lerpf(scroll_from, (float)scroll_target, t);
            if (t < 1.0f) {
                cat_request_frame();
            } else {
                scroll_anim_start = 0;
            }
        }

        /* Render */
        cat_draw_background();
        if (opts->title) {
            if (opts->center_title)
                cat_draw_screen_title_centered(opts->title, opts->status_bar);
            else
                cat_draw_screen_title(opts->title, opts->status_bar);
        }
        if (opts->status_bar) cat_draw_status_bar(opts->status_bar);

        /* Resolve optional key color override */
        cat_draw_color info_key_color = opts->key_color ? *opts->key_color : theme->hint;

        /* Clip to content area */
        SDL_Rect clip = {content_x, content_y, content_w, content_h};
        SDL_RenderSetClipRect(cat_get_renderer(), &clip);

        int draw_y = content_y - (int)scroll_current;
        int view_top = content_y;
        int view_bottom = content_y + content_h;

        for (int s = 0; s < n_sections; s++) {
            cat_detail_section *sec = &opts->sections[s];
            int sec_h = section_total_h[s];

            /* Offscreen culling: skip sections entirely above or below viewport */
            if (draw_y + sec_h <= view_top || draw_y >= view_bottom) {
                draw_y += sec_h;
                continue;
            }

            /* Section title */
            if (sec->title) {
                cat_draw_color sec_title_c = opts->section_title_color
                    ? *opts->section_title_color : theme->emphasis;
                cat_draw_text(section_font, sec->title, margin, draw_y, sec_title_c);
                if (opts->show_section_separator) {
                    int line_y = draw_y + TTF_FontLineSkip(section_font) + CAT_S(2);
                    SDL_SetRenderDrawColor(cat_get_renderer(),
                        theme->accent.r, theme->accent.g, theme->accent.b, theme->accent.a);
                    SDL_RenderDrawLine(cat_get_renderer(), margin, line_y, content_right, line_y);
                }
                draw_y += section_title_h;
            }

            switch (sec->type) {
                case CAT_SECTION_INFO:
                    {
                        int val_x = section_val_x[s];
                        int val_w = section_val_w[s];

                        for (int p = 0; p < sec->info_count; p++) {
                            if (sec->info_pairs[p].key) {
                                cat_draw_text(key_font, sec->info_pairs[p].key,
                                    margin, draw_y, info_key_color);
                            }
                            int vh = 0;
                            if (sec->info_pairs[p].value) {
                                vh = cat_draw_text_wrapped(body_font, sec->info_pairs[p].value,
                                    val_x, draw_y, val_w, theme->text, CAT_ALIGN_LEFT);
                            }
                            draw_y += cat__max(key_line_h, vh) + CAT_S(4);
                        }
                    }
                    break;

                case CAT_SECTION_DESCRIPTION:
                    if (sec->description) {
                        draw_y += cat_draw_text_wrapped(body_font, sec->description,
                            margin, draw_y,
                            content_w,
                            theme->text, CAT_ALIGN_LEFT);
                    }
                    break;

                case CAT_SECTION_IMAGE: {
                    SDL_Texture *img = (section_images) ? section_images[s] : NULL;
                    if (img) {
                        int iw = section_image_w[s];
                        int ih = section_image_h[s];
                        int ix = content_x + (content_w - iw) / 2;
                        cat_draw_image(img, ix, draw_y, iw, ih);
                        draw_y += ih;
                    }
                    break;
                }

                case CAT_SECTION_TABLE:
                    if (sec->table_headers && sec->table_cols > 0) {
                        int col_w = content_w / sec->table_cols;
                        /* Headers */
                        for (int c = 0; c < sec->table_cols; c++) {
                            if (sec->table_headers[c]) {
                                cat_draw_text(key_font, sec->table_headers[c],
                                    margin + c * col_w, draw_y, theme->accent);
                            }
                        }
                        draw_y += table_row_h;
                        /* Rows */
                        for (int r = 0; r < sec->table_rows_count; r++) {
                            if (sec->table_rows && sec->table_rows[r]) {
                                for (int c = 0; c < sec->table_cols; c++) {
                                    if (sec->table_rows[r][c]) {
                                        cat_draw_text_clipped(body_font, sec->table_rows[r][c],
                                            margin + c * col_w, draw_y,
                                            theme->text, col_w - CAT_S(8));
                                    }
                                }
                            }
                            draw_y += table_row_h;
                        }
                    }
                    break;
            }

            draw_y += section_gap;
        }

        SDL_RenderSetClipRect(cat_get_renderer(), NULL);

        /* Scrollbar */
        if (max_scroll > 0) {
            int sb_x = content_right + CAT_S(6);
            cat_draw_scrollbar(sb_x, content_y, content_h,
                content_h, total_content_h, (int)scroll_current);
        }

        if (opts->footer && opts->footer_count > 0) {
            cat_draw_footer(opts->footer, opts->footer_count);
        }

        cat_present();

    }

    int rc = (result->action == CAT_DETAIL_BACK) ? CAT_CANCELLED : CAT_OK;
    if (section_images) {
        for (int s = 0; s < opts->section_count; s++) {
            if (section_images[s]) SDL_DestroyTexture(section_images[s]);
        }
    }
    free(section_images);
    free(section_image_w);
    free(section_image_h);
    free(section_total_h);
    free(section_kw_max);
    free(section_val_w);
    free(section_val_x);

    return rc;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * COLOR PICKER Implementation
 * ═══════════════════════════════════════════════════════════════════════════ */

/* 25 predefined colors in a 5x5 grid */
static const cat_draw_color cat__picker_colors[25] = {
    {255,  59,  48, 255}, {255, 149,   0, 255}, {255, 204,   0, 255}, { 52, 199,  89, 255}, {  0, 199, 190, 255},
    { 48, 176, 199, 255}, { 50, 173, 230, 255}, {  0, 122, 255, 255}, { 88,  86, 214, 255}, {175,  82, 222, 255},
    {255,  45,  85, 255}, {162, 132,  94, 255}, {142, 142, 147, 255}, {174, 174, 178, 255}, {199, 199, 204, 255},
    {155,  34,  87, 255}, {128,   0,   0, 255}, {  0, 128,   0, 255}, {  0,   0, 128, 255}, {128, 128,   0, 255},
    {  0, 128, 128, 255}, {128,   0, 128, 255}, {255, 255, 255, 255}, {192, 192, 192, 255}, {  0,   0,   0, 255},
};

/* ─── HSL conversion helpers ───────────────────────────────────────────── */

static float cat__hsl_hue2rgb(float p, float q, float t) {
    if (t < 0.0f) t += 1.0f;
    if (t > 1.0f) t -= 1.0f;
    if (t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;
    if (t < 1.0f / 2.0f) return q;
    if (t < 2.0f / 3.0f) return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
    return p;
}

static void cat__hsl_to_rgb(float h, float s, float l,
                             uint8_t *r, uint8_t *g, uint8_t *b) {
    if (s <= 0.0f) {
        uint8_t v = (uint8_t)(l * 255.0f);
        *r = *g = *b = v;
        return;
    }
    float q = (l < 0.5f) ? (l * (1.0f + s)) : (l + s - l * s);
    float p = 2.0f * l - q;
    *r = (uint8_t)(cat__hsl_hue2rgb(p, q, h + 1.0f / 3.0f) * 255.0f);
    *g = (uint8_t)(cat__hsl_hue2rgb(p, q, h) * 255.0f);
    *b = (uint8_t)(cat__hsl_hue2rgb(p, q, h - 1.0f / 3.0f) * 255.0f);
}

static void cat__rgb_to_hsl(uint8_t r, uint8_t g, uint8_t b,
                             float *h, float *s, float *l) {
    float rf = r / 255.0f, gf = g / 255.0f, bf = b / 255.0f;
    float max = rf > gf ? (rf > bf ? rf : bf) : (gf > bf ? gf : bf);
    float min = rf < gf ? (rf < bf ? rf : bf) : (gf < bf ? gf : bf);
    float d = max - min;
    *l = (max + min) / 2.0f;
    if (d < 0.001f) { *h = 0.0f; *s = 0.0f; return; }
    *s = (*l > 0.5f) ? (d / (2.0f - max - min)) : (d / (max + min));
    if (max == rf)      *h = (gf - bf) / d + (gf < bf ? 6.0f : 0.0f);
    else if (max == gf) *h = (bf - rf) / d + 2.0f;
    else                *h = (rf - gf) / d + 4.0f;
    *h /= 6.0f;
}

static float cat__field_y_to_lightness(int y, int field_h) {
    int mid = field_h / 2;
    if (y <= mid) return 1.0f - ((float)y / (float)mid) * 0.5f;
    return 0.5f - ((float)(y - mid) / (float)(field_h - mid)) * 0.5f;
}

/* ─── HSL color picker ─────────────────────────────────────────────────── */

/* Fill the color-picker field texture: X = hue (0..1), Y = lightness (top=white
   .. bottom=black), at the given saturation. Rebuilt live as the user changes
   saturation with L1/R1 so muted / desaturated colors are reachable (the field
   used to be locked at full saturation). */
static void cat__color_field_fill(SDL_Texture *tex, int w, int h, float sat) {
    if (!tex || w <= 1 || h <= 0) return;
    void *pixels;
    int pitch;
    if (SDL_LockTexture(tex, NULL, &pixels, &pitch) != 0) return;
    for (int fy = 0; fy < h; fy++) {
        uint8_t *row = (uint8_t *)pixels + fy * pitch;
        float l = cat__field_y_to_lightness(fy, h);
        for (int fx = 0; fx < w; fx++) {
            float hue = (float)fx / (float)(w - 1);
            uint8_t pr, pg, pb;
            cat__hsl_to_rgb(hue, sat, l, &pr, &pg, &pb);
            row[fx * 3 + 0] = pr;
            row[fx * 3 + 1] = pg;
            row[fx * 3 + 2] = pb;
        }
    }
    SDL_UnlockTexture(tex);
}

int cat_color_picker_ctx(cat_draw_color initial, cat_draw_color *result,
                         cat_color_picker_context *context) {
    if (!result) return CAT_ERROR;
    *result = initial;

    cat_theme *theme = cat_get_theme();
    SDL_Renderer *renderer = cat_get_renderer();
    int screen_w = cat_get_screen_width();
    int screen_h = cat_get_screen_height();
    int pad = CAT_S(8);

    TTF_Font *title_font = cat_get_font(CAT_FONT_MEDIUM);
    int header_h = (title_font ? TTF_FontHeight(title_font) : CAT_S(16)) + pad * 3;
    /* Bottom preview strip height when context is provided. */
    int preview_strip_h = 0;
    if (context && context->role_count > 0) {
        TTF_Font *preview_font = cat_get_font(CAT_FONT_SMALL);
        int preview_row_h = preview_font ? TTF_FontHeight(preview_font) + CAT_S(6) : CAT_S(20);
        preview_strip_h = preview_row_h * 3 + CAT_S(8);
    }

    /* Color field dimensions — full width, room for header and preview */
    int field_x = 0;
    int field_y = header_h;
    int field_w = screen_w;
    int field_h = screen_h - field_y - preview_strip_h;

    /* Convert initial color to HSL for cursor positioning.
       X = hue (0..1), Y = lightness (top=white, mid=saturated, bottom=black). */
    float init_h, init_s, init_l;
    cat__rgb_to_hsl(initial.r, initial.g, initial.b, &init_h, &init_s, &init_l);

    int cursor_x = (int)(init_h * (float)(field_w - 1));
    int cursor_y;
    if (init_l >= 0.5f)
        cursor_y = (int)((1.0f - init_l) * 2.0f * (float)(field_h / 2));
    else
        cursor_y = field_h / 2 + (int)((0.5f - init_l) * 2.0f * (float)(field_h / 2));
    if (cursor_x < 0) cursor_x = 0;
    if (cursor_x >= field_w) cursor_x = field_w - 1;
    if (cursor_y < 0) cursor_y = 0;
    if (cursor_y >= field_h) cursor_y = field_h - 1;

    /* Saturation is user-adjustable (L1/R1); start at the initial color's own
       saturation so the field opens showing the current color. */
    float sat = init_s;

    /* Build the HSL field texture (X=hue, Y=lightness) at the current saturation. */
    SDL_Texture *field_tex = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, field_w, field_h);
    cat__color_field_fill(field_tex, field_w, field_h, sat);

    /* Current picked color (live preview) */
    uint8_t pick_r = initial.r, pick_g = initial.g, pick_b = initial.b;

    int cursor_speed = CAT_S(4);
    if (cursor_speed < 2) cursor_speed = 2;

    bool running = true;
    while (running) {
        /* Drain the event queue — only A/B need event handling. */
        cat_input_event ev;
        while (cat_poll_input(&ev)) {
            if (!ev.pressed) continue;
            if (ev.button == CAT_BTN_A) {
                result->r = pick_r;
                result->g = pick_g;
                result->b = pick_b;
                result->a = 255;
                if (field_tex) SDL_DestroyTexture(field_tex);
                return CAT_OK;
            }
            if (ev.button == CAT_BTN_B) {
                if (field_tex) SDL_DestroyTexture(field_tex);
                return CAT_CANCELLED;
            }
        }

        /* Read held state directly from Catastrophe's internal tracking for
           reliable diagonal movement. D-pad = hue (X) / lightness (Y); L1/R1 =
           fast cursor; L2/R2 = saturation, so muted / desaturated colors are
           reachable. */
        bool fast_mode = cat__g.buttons_held[CAT_BTN_L1] ||
                         cat__g.buttons_held[CAT_BTN_R1];
        int speed = fast_mode ? cursor_speed * 4 : cursor_speed;
        int dx = 0, dy = 0;
        if (cat__g.buttons_held[CAT_BTN_LEFT])  dx -= speed;
        if (cat__g.buttons_held[CAT_BTN_RIGHT]) dx += speed;
        if (cat__g.buttons_held[CAT_BTN_UP])    dy -= speed;
        if (cat__g.buttons_held[CAT_BTN_DOWN])  dy += speed;

        float sat_delta = 0.0f;
        if (cat__g.buttons_held[CAT_BTN_L2]) sat_delta -= 0.02f;
        if (cat__g.buttons_held[CAT_BTN_R2]) sat_delta += 0.02f;

        if (dx != 0 || dy != 0 || sat_delta != 0.0f) {
            cursor_x += dx;
            cursor_y += dy;
            if (cursor_x < 0) cursor_x = 0;
            if (cursor_x >= field_w) cursor_x = field_w - 1;
            if (cursor_y < 0) cursor_y = 0;
            if (cursor_y >= field_h) cursor_y = field_h - 1;

            if (sat_delta != 0.0f) {
                sat += sat_delta;
                if (sat < 0.0f) sat = 0.0f;
                if (sat > 1.0f) sat = 1.0f;
                cat__color_field_fill(field_tex, field_w, field_h, sat);  /* refresh the field */
            }

            float h = (float)cursor_x / (float)(field_w - 1);
            float l = cat__field_y_to_lightness(cursor_y, field_h);
            cat__hsl_to_rgb(h, sat, l, &pick_r, &pick_g, &pick_b);
        }

        /* Draw */
        cat_draw_background();

        /* Header: title on left, hints on right */
        if (title_font) {
            const char *title = "Pick a Color";
            if (context && context->active_role >= 0 &&
                context->active_role < context->role_count &&
                context->roles[context->active_role].label)
                title = context->roles[context->active_role].label;
            cat_draw_text(title_font, title, pad * 2, pad, theme->text);

            char hints[80];
            snprintf(hints, sizeof(hints), "L2: Sat %d%%   L1:Fast   B:Cancel   A:Pick",
                     (int)(sat * 100.0f + 0.5f));
            int hints_w = cat_measure_text(title_font, hints);
            cat_draw_text(title_font, hints, screen_w - hints_w - pad * 2, pad, theme->hint);
        }

        /* Separator */
        int sep_y = header_h - pad;
        cat_draw_rect(pad * 2, sep_y, screen_w - pad * 4, 1, cat_hex_to_color("#ffffff20"));

        /* Bottom contextual preview — horizontal mock UI showing theme
           colors in their actual roles. The active color updates live.
           Footer hints already cover accent, button text, button bg. */
        if (context && context->role_count > 0 && preview_strip_h > 0) {
            cat_draw_color live_color = { pick_r, pick_g, pick_b, 255 };

            /* Resolve role colors, substituting live pick for active. */
            cat_draw_color resolved[8];
            for (int i = 0; i < context->role_count && i < 8; i++)
                resolved[i] = (i == context->active_role) ? live_color
                                                           : context->roles[i].color;

            cat_draw_color col_accent = context->role_count > 0 ? resolved[0] : theme->accent;
            cat_draw_color col_bg     = context->role_count > 1 ? resolved[1] : theme->background;
            cat_draw_color col_text   = context->role_count > 2 ? resolved[2] : theme->text;
            cat_draw_color col_sel    = context->role_count > 3 ? resolved[3] : theme->highlight;
            cat_draw_color col_hint   = context->role_count > 4 ? resolved[4] : theme->hint;
            cat_draw_color col_btn_tx = context->role_count > 5 ? resolved[5] : theme->button_label;
            cat_draw_color col_btn_bg = context->role_count > 6 ? resolved[6] : theme->button_glyph_bg;

            TTF_Font *preview_font = cat_get_font(CAT_FONT_SMALL);
            int preview_y = field_y + field_h;
            int row_h = preview_font ? TTF_FontHeight(preview_font) + CAT_S(4) : CAT_S(16);
            int inset = CAT_S(8);
            int half_w = screen_w / 2;

            /* Background fill */
            cat_draw_rect(0, preview_y, screen_w, preview_strip_h, col_bg);

            /* Separator */
            cat_draw_rect(0, preview_y, screen_w, 1, cat_hex_to_color("#ffffff30"));

            int row_y = preview_y + CAT_S(3);

            /* Two-column layout: left shows text roles, right shows action roles */

            /* Left col row 1: "Text" in text color */
            if (preview_font)
                cat_draw_text(preview_font, "Text", inset, row_y, col_text);
            /* Right col row 1: accent bar */
            cat_draw_rect(half_w, row_y, half_w - inset, row_h - CAT_S(2), col_accent);
            if (preview_font)
                cat_draw_text(preview_font, "Accent", half_w + CAT_S(4), row_y, col_bg);
            row_y += row_h;

            /* Left col row 2: "Selection" on highlight */
            cat_draw_pill(inset / 2, row_y, half_w - inset, row_h - CAT_S(2), col_sel);
            if (preview_font)
                cat_draw_text(preview_font, "Selection", inset, row_y + CAT_S(1), col_text);
            /* Right col row 2: button pill */
            cat_draw_pill(half_w, row_y, half_w - inset, row_h - CAT_S(2), col_btn_bg);
            if (preview_font)
                cat_draw_text(preview_font, "Button", half_w + CAT_S(4), row_y + CAT_S(1), col_btn_tx);
            row_y += row_h;

            /* Left col row 3: "Secondary" in hint color */
            if (preview_font)
                cat_draw_text(preview_font, "Secondary", inset, row_y, col_hint);

            /* Right col row 3: live hex value */
            if (preview_font) {
                char hex[16];
                snprintf(hex, sizeof(hex), "#%02X%02X%02X", pick_r, pick_g, pick_b);
                cat_draw_color hex_color = { pick_r, pick_g, pick_b, 255 };
                int hex_text_w = cat_measure_text(preview_font, hex);
                cat_draw_text(preview_font, hex, screen_w - hex_text_w - inset, row_y, hex_color);
            }
        }

        /* Color field */
        if (field_tex) {
            SDL_Rect dst = { field_x, field_y, field_w, field_h };
            SDL_RenderCopy(renderer, field_tex, NULL, &dst);
        }

        /* Cursor crosshair */
        int cross_x = field_x + cursor_x;
        int cross_y = field_y + cursor_y;
        {
            int arm = CAT_S(20);
            int thick = CAT_S(3);
            if (thick < 2) thick = 2;
            cat_draw_color white = { 255, 255, 255, 255 };
            cat_draw_color black = { 0, 0, 0, 200 };

            /* Black outline */
            cat_draw_rect(cross_x - arm - 1, cross_y - thick / 2 - 1,
                          arm * 2 + thick + 2, thick + 2, black);
            cat_draw_rect(cross_x - thick / 2 - 1, cross_y - arm - 1,
                          thick + 2, arm * 2 + thick + 2, black);
            /* White cross */
            cat_draw_rect(cross_x - arm, cross_y - thick / 2, arm * 2 + thick, thick, white);
            cat_draw_rect(cross_x - thick / 2, cross_y - arm, thick, arm * 2 + thick, white);
        }

        /* Floating preview swatch — follows cursor with black border to
           isolate from surrounding colors (mitigates simultaneous contrast). */
        {
            cat_draw_color sel = { pick_r, pick_g, pick_b, 255 };
            cat_draw_color border_color = { 0, 0, 0, 255 };
            int swatch_size = CAT_S(32);
            int border = CAT_S(3);
            int offset = CAT_S(24);

            /* Position: offset to the bottom-right of cursor. If that would
               go off-screen, flip to the other side. */
            int swatch_x = cross_x + offset;
            int swatch_y = cross_y + offset;
            if (swatch_x + swatch_size + border > field_x + field_w)
                swatch_x = cross_x - offset - swatch_size;
            if (swatch_y + swatch_size + border > field_y + field_h)
                swatch_y = cross_y - offset - swatch_size;

            /* Black border */
            cat_draw_rect(swatch_x - border, swatch_y - border,
                          swatch_size + border * 2, swatch_size + border * 2,
                          border_color);
            /* Color fill */
            cat_draw_rect(swatch_x, swatch_y, swatch_size, swatch_size, sel);
        }

        /* Inline hints in header instead of footer (avoids blocking preview) */

        cat_present();
    }

    if (field_tex) SDL_DestroyTexture(field_tex);
    return CAT_CANCELLED;
}

int cat_color_picker(cat_draw_color initial, cat_draw_color *result) {
    return cat_color_picker_ctx(initial, result, NULL);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * HELP OVERLAY Implementation
 * ═══════════════════════════════════════════════════════════════════════════ */

void cat_show_help_overlay(const char *text) {
    if (!text || !text[0]) return;

    cat_theme *theme = cat_get_theme();
    int screen_w = cat_get_screen_width();
    int screen_h = cat_get_screen_height();

    TTF_Font *font = cat_get_font(CAT_FONT_TINY);
    if (!font) return;

    int margin = CAT_S(40);
    int line_h = TTF_FontLineSkip(font);
    int max_w  = screen_w - margin * 2;

    /* Estimate content height */
    int chars_per_line = max_w / (CAT_S(10));
    if (chars_per_line < 1) chars_per_line = 1;
    int est_lines = ((int)strlen(text) / chars_per_line) + 2;
    int content_h = est_lines * line_h;
    int scroll = 0;
    int max_scroll = content_h - (screen_h - margin * 2);
    if (max_scroll < 0) max_scroll = 0;

    bool running = true;
    while (running) {
        cat_input_event ev;
        while (cat_poll_input(&ev)) {
            if (!ev.pressed) continue;
            switch (ev.button) {
                case CAT_BTN_UP:
                    scroll -= CAT_S(40);
                    if (scroll < 0) scroll = 0;
                    break;
                case CAT_BTN_DOWN:
                    scroll += CAT_S(40);
                    if (scroll > max_scroll) scroll = max_scroll;
                    break;
                default:
                    /* Any other button closes the overlay */
                    running = false;
                    break;
            }
        }

        /* Semi-transparent background */
        cat_draw_color overlay_bg = {0, 0, 0, 200};
        cat_draw_rect(0, 0, screen_w, screen_h, overlay_bg);

        /* Scrollable text */
        SDL_Rect clip = {margin, margin, max_w, screen_h - margin * 2};
        SDL_RenderSetClipRect(cat_get_renderer(), &clip);

        cat_draw_text_wrapped(font, text,
            margin, margin - scroll,
            max_w, theme->text, CAT_ALIGN_LEFT);

        SDL_RenderSetClipRect(cat_get_renderer(), NULL);

        /* Scrollbar */
        if (max_scroll > 0) {
            cat_draw_scrollbar(screen_w - margin + CAT_S(8), margin,
                screen_h - margin * 2,
                screen_h - margin * 2, content_h, scroll);
        }

        /* "Press any button to close" hint */
        TTF_Font *hint_font = cat_get_font(CAT_FONT_MICRO);
        if (hint_font) {
            const char *hint = "Press any button to close";
            int hw = cat_measure_text(hint_font, hint);
            cat_draw_text(hint_font, hint,
                (screen_w - hw) / 2,
                screen_h - margin + CAT_S(8),
                theme->hint);
        }

        cat_present();

    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 * FILE PICKER Implementation
 * ═══════════════════════════════════════════════════════════════════════════ */

enum { CAT__FILE_PICKER_PATH_MAX = 1024 };

static void cat__file_picker_show_message(const char *message) {
    cat_footer_item footer[] = {
        { .button = CAT_BTN_A, .label = "OK", .is_confirm = true },
    };
    cat_message_opts opts = {
        .message = message,
        .footer = footer,
        .footer_count = 1,
    };
    cat_confirm_result result;
    cat_confirmation(&opts, &result);
}

static int cat__file_picker_stricmp(const char *a, const char *b) {
#ifdef _WIN32
    return _stricmp(a, b);
#else
    return strcasecmp(a, b);
#endif
}

static void cat__file_picker_normalize_path(char *path) {
    if (!path || !path[0]) return;

#ifdef _WIN32
    for (char *p = path; *p; p++) {
        if (*p == '\\') *p = '/';
    }
#endif

    size_t len = strlen(path);
    while (len > 1 && path[len - 1] == '/') {
#ifdef _WIN32
        if (len == 3 && path[1] == ':') break;
#endif
        path[--len] = '\0';
    }
}

static size_t cat__file_picker_capped_len(const char *text, size_t max_len) {
    size_t len = 0;
    if (!text) return 0;
    while (len < max_len && text[len]) len++;
    return len;
}

static void cat__file_picker_copy_text(char *dst, size_t dst_size, const char *src) {
    size_t copy_len;
    if (!dst || dst_size == 0) return;
    if (!src) {
        dst[0] = '\0';
        return;
    }
    copy_len = cat__file_picker_capped_len(src, dst_size - 1);
    memcpy(dst, src, copy_len);
    dst[copy_len] = '\0';
}

static void cat__file_picker_append_text(char *dst, size_t dst_size, const char *src) {
    size_t dst_len;
    size_t copy_len;

    if (!dst || dst_size == 0 || !src || !src[0]) return;

    dst_len = cat__file_picker_capped_len(dst, dst_size - 1);
    dst[dst_len] = '\0';

    copy_len = cat__file_picker_capped_len(src, dst_size - dst_len - 1);

    if (copy_len > 0) {
        memcpy(dst + dst_len, src, copy_len);
        dst[dst_len + copy_len] = '\0';
    }
}

static bool cat__file_picker_copy_path(char *dst, size_t dst_size, const char *src) {
    size_t src_len;
    if (!dst || dst_size == 0 || !src || !src[0]) return false;
    src_len = strlen(src);
    if (src_len >= dst_size) {
        dst[0] = '\0';
        return false;
    }
    memcpy(dst, src, src_len + 1);
    cat__file_picker_normalize_path(dst);
    return true;
}

static bool cat__file_picker_resolve_existing_path(const char *path,
                                                  char *out, size_t out_size) {
    if (!out || out_size == 0) return false;
    out[0] = '\0';
    if (!path || !path[0]) return false;

#ifdef _WIN32
    char resolved[CAT__FILE_PICKER_PATH_MAX];
    if (!_fullpath(resolved, path, sizeof(resolved))) return false;
    if (!cat__file_picker_copy_path(out, out_size, resolved)) return false;
#else
    char resolved[CAT__FILE_PICKER_PATH_MAX];
    if (!realpath(path, resolved)) return false;
    if (!cat__file_picker_copy_path(out, out_size, resolved)) return false;
#endif

    return true;
}

static bool cat__file_picker_path_is_dir(const char *path) {
    if (!path || !path[0]) return false;
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static bool cat__file_picker_is_path_inside(const char *path, const char *root) {
    if (!path || !root) return false;

    size_t root_len = strlen(root);
    size_t path_len = strlen(path);
    if (path_len < root_len) return false;

#ifdef _WIN32
    if (_strnicmp(path, root, root_len) != 0) return false;
#else
    if (strncmp(path, root, root_len) != 0) return false;
#endif

    if (path_len == root_len) return true;
    if (root_len > 0 && root[root_len - 1] == '/') return true;
    return path[root_len] == '/';
}

static bool cat__file_picker_join_path(char *out, size_t out_size,
                                      const char *base, const char *name) {
    if (!out || out_size == 0 || !base || !base[0] || !name || !name[0]) return false;
    bool has_sep = base[strlen(base) - 1] == '/';
    int n = snprintf(out, out_size, has_sep ? "%s%s" : "%s/%s", base, name);
    if (n < 0 || (size_t)n >= out_size) {
        out[0] = '\0';
        return false;
    }
    cat__file_picker_normalize_path(out);
    return true;
}

static bool cat__file_picker_parent_path(char *path) {
    if (!path || !path[0]) return false;

    char *last_sep = strrchr(path, '/');
    if (!last_sep) return false;

    if (last_sep == path) {
        path[1] = '\0';
        return true;
    }
#ifdef _WIN32
    if (last_sep == path + 2 && path[1] == ':') {
        path[3] = '\0';
        return true;
    }
#endif
    *last_sep = '\0';
    return true;
}

static bool cat__file_picker_validate_component(const char *name) {
    if (!name || !name[0]) return false;
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) return false;
    if (strchr(name, '/') || strchr(name, '\\')) return false;
#ifdef _WIN32
    for (const unsigned char *p = (const unsigned char *)name; *p; p++) {
        if (*p < 32) return false;
        if (*p == '<' || *p == '>' || *p == ':' || *p == '"' ||
            *p == '|' || *p == '?' || *p == '*')
            return false;
    }
#endif
    return true;
}

static void cat__file_picker_root_label(char *out, size_t out_size,
                                       const char *root, bool is_custom_root) {
    if (!out || out_size == 0) return;
    out[0] = '\0';

    if (!is_custom_root) {
#if CAT_PLATFORM_IS_DEVICE
        snprintf(out, out_size, "SDCARD");
#else
        snprintf(out, out_size, "Home");
#endif
        return;
    }

    const char *base = strrchr(root, '/');
    if (base && base[1]) {
        cat__file_picker_copy_text(out, out_size, base + 1);
        return;
    }
#ifdef _WIN32
    if (root[0] && root[1] == ':' && root[2] == '/' && root[3] == '\0') {
        snprintf(out, out_size, "%c:", root[0]);
        return;
    }
#endif
    cat__file_picker_copy_text(out, out_size, root);
}

static void cat__file_picker_format_title(char *out, size_t out_size,
                                         const char *root_label,
                                         const char *root,
                                         const char *current_path) {
    if (!out || out_size == 0) return;
    out[0] = '\0';

    if (!root_label || !root_label[0] || !root || !current_path) return;
    if (strcmp(current_path, root) == 0) {
        cat__file_picker_copy_text(out, out_size, root_label);
        return;
    }
    if (cat__file_picker_is_path_inside(current_path, root)) {
        cat__file_picker_copy_text(out, out_size, root_label);
        cat__file_picker_append_text(out, out_size, current_path + strlen(root));
        return;
    }
    cat__file_picker_copy_text(out, out_size, current_path);
}

/* Resolve the default root path per platform */
static const char *cat__file_picker_resolve_root(void) {
#if CAT_PLATFORM_IS_DEVICE
    const char *sdcard = cat__env_nonempty("SDCARD_PATH");
    return sdcard ? sdcard : cat__default_sdcard_path();
#elif defined(_WIN32)
    const char *home = getenv("USERPROFILE");
    return (home && home[0]) ? home : ".";
#else
    const char *home = getenv("HOME");
    return (home && home[0]) ? home : ".";
#endif
}

/* Case-insensitive extension match */
static bool cat__file_picker_ext_match(const char *filename,
                                       const char **extensions, int ext_count) {
    if (!extensions || ext_count <= 0) return true;
    const char *dot = strrchr(filename, '.');
    if (!dot || !dot[1]) return false;
    dot++;
    for (int i = 0; i < ext_count; i++) {
        if (cat__file_picker_stricmp(dot, extensions[i]) == 0) return true;
    }
    return false;
}

/* Internal entry for sorting */
typedef struct {
    char *name;
    char *full_path;
    bool  is_dir;
} cat__file_picker_entry;

/* qsort comparator: dirs first, then case-insensitive alpha */
static int cat__file_picker_cmp(const void *a, const void *b) {
    const cat__file_picker_entry *ea = (const cat__file_picker_entry *)a;
    const cat__file_picker_entry *eb = (const cat__file_picker_entry *)b;
    if (ea->is_dir != eb->is_dir) return ea->is_dir ? -1 : 1;
    return cat__file_picker_stricmp(ea->name, eb->name);
}

static void cat__file_picker_free_entries(cat__file_picker_entry *entries, int entry_count) {
    if (!entries) return;
    for (int i = 0; i < entry_count; i++) {
        free(entries[i].name);
        free(entries[i].full_path);
    }
    free(entries);
}

static void cat__file_picker_free_iteration_allocs(cat_list_item *items,
                                                  bool *is_dir_map,
                                                  char **trailing_pool,
                                                  int list_count) {
    if (trailing_pool) {
        for (int i = 0; i < list_count; i++) {
            free(trailing_pool[i]);
        }
        free(trailing_pool);
    }
    free(items);
    free(is_dir_map);
}

cat_file_picker_opts cat_file_picker_default_opts(const char *title) {
    cat_file_picker_opts opts;
    memset(&opts, 0, sizeof(opts));
    opts.title = title;
    opts.mode = CAT_FILE_PICKER_FILES;
    return opts;
}

typedef struct {
    cat_footer_item *footer;
    int             footer_index;
    bool           *is_dir_map;
    int             entry_count;
} cat__file_picker_footer_context;

static void cat__file_picker_footer_update(cat_list_opts *opts, int cursor, void *userdata) {
    cat__file_picker_footer_context *ctx = (cat__file_picker_footer_context *)userdata;

    (void)opts;

    if (!ctx || !ctx->footer || ctx->footer_index < 0) return;

    if (cursor >= 0 && cursor < ctx->entry_count && ctx->is_dir_map[cursor]) {
        ctx->footer[ctx->footer_index].label = "ENTER";
    } else {
        ctx->footer[ctx->footer_index].label = "OPEN";
    }
}

int cat_file_picker(cat_file_picker_opts *opts, cat_file_picker_result *result) {
    if (!opts || !result) return CAT_ERROR;

    char base_root[CAT__FILE_PICKER_PATH_MAX];
    char requested_root[CAT__FILE_PICKER_PATH_MAX];
    char effective_root[CAT__FILE_PICKER_PATH_MAX];
    char current_path[CAT__FILE_PICKER_PATH_MAX];
    char root_label[128];
    bool using_custom_root = false;

    const char *default_root = cat__file_picker_resolve_root();
    if (!cat__file_picker_resolve_existing_path(default_root, base_root, sizeof(base_root))) {
        if (!cat__file_picker_copy_path(base_root, sizeof(base_root), default_root))
            return CAT_ERROR;
    }

    if (!cat__file_picker_copy_path(effective_root, sizeof(effective_root), base_root))
        return CAT_ERROR;

    if (opts->root_path && opts->root_path[0] &&
        cat__file_picker_resolve_existing_path(opts->root_path, requested_root, sizeof(requested_root)) &&
        cat__file_picker_path_is_dir(requested_root)) {
#if CAT_PLATFORM_IS_DEVICE
        if (cat__file_picker_is_path_inside(requested_root, base_root)) {
            if (!cat__file_picker_copy_path(effective_root, sizeof(effective_root), requested_root))
                return CAT_ERROR;
            using_custom_root = true;
        }
#else
        if (!cat__file_picker_copy_path(effective_root, sizeof(effective_root), requested_root))
            return CAT_ERROR;
        using_custom_root = true;
#endif
    }

    if (opts->initial_path && opts->initial_path[0] &&
        cat__file_picker_resolve_existing_path(opts->initial_path, current_path, sizeof(current_path)) &&
        cat__file_picker_is_path_inside(current_path, effective_root)) {
        struct stat st;
        if (stat(current_path, &st) != 0 || !S_ISDIR(st.st_mode)) {
            if (!cat__file_picker_copy_path(current_path, sizeof(current_path), effective_root))
                return CAT_ERROR;
        }
    } else {
        if (!cat__file_picker_copy_path(current_path, sizeof(current_path), effective_root))
            return CAT_ERROR;
    }

    cat__file_picker_root_label(root_label, sizeof(root_label), effective_root, using_custom_root);

    int capacity = 512;
    cat__file_picker_entry *entries = NULL;
    cat_list_item *items = NULL;
    bool *is_dir_map = NULL;
    char **trailing_pool = NULL; /* pool for uppercase extension strings */
    bool allow_create_dirs = opts->allow_create &&
                             (opts->mode == CAT_FILE_PICKER_DIRS ||
                              opts->mode == CAT_FILE_PICKER_BOTH);

    for (;;) {
        DIR *dir = NULL;
        int entry_count = 0;
        int list_count = 0;
        bool is_empty = false;
        int ret = -99; /* sentinel: keep looping */

        /* ── Scan directory ─────────────────────────────────────────── */
        dir = opendir(current_path);
        if (!dir) {
            char errmsg[512];
            errmsg[0] = '\0';
            cat__file_picker_append_text(errmsg, sizeof(errmsg), "Cannot open directory '");
            cat__file_picker_append_text(errmsg, sizeof(errmsg), current_path);
            cat__file_picker_append_text(errmsg, sizeof(errmsg), "':\n");
            cat__file_picker_append_text(errmsg, sizeof(errmsg), strerror(errno));
            cat__file_picker_show_message(errmsg);

            /* Try to go up */
            if (strcmp(current_path, effective_root) == 0) return CAT_ERROR;
            if (!cat__file_picker_parent_path(current_path) ||
                !cat__file_picker_is_path_inside(current_path, effective_root)) {
                if (!cat__file_picker_copy_path(current_path, sizeof(current_path), effective_root))
                    return CAT_ERROR;
            }
            continue;
        }

        entries = (cat__file_picker_entry *)malloc(sizeof(cat__file_picker_entry) * (size_t)capacity);
        if (!entries) {
            ret = CAT_ERROR;
            goto cleanup_iteration;
        }

        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            /* Skip . and .. */
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
                continue;
            /* Skip hidden unless requested */
            if (!opts->show_hidden && ent->d_name[0] == '.')
                continue;

            /* Build full path for stat */
            char full[CAT__FILE_PICKER_PATH_MAX];
            if (!cat__file_picker_join_path(full, sizeof(full), current_path, ent->d_name))
                continue;

            struct stat st;
            if (stat(full, &st) != 0) continue;

            bool is_directory = S_ISDIR(st.st_mode);

            /* In DIRS mode, hide files entirely */
            if (opts->mode == CAT_FILE_PICKER_DIRS && !is_directory) continue;

            /* Apply extension filter to files */
            if (!is_directory && !cat__file_picker_ext_match(ent->d_name,
                    opts->extensions, opts->extension_count))
                continue;

            /* Grow array if needed */
            if (entry_count >= capacity) {
                int new_capacity = capacity * 2;
                cat__file_picker_entry *new_entries =
                    (cat__file_picker_entry *)realloc(entries,
                        sizeof(cat__file_picker_entry) * (size_t)new_capacity);
                if (!new_entries) {
                    ret = CAT_ERROR;
                    goto cleanup_iteration;
                }
                entries = new_entries;
                capacity = new_capacity;
            }

            char *name = strdup(ent->d_name);
            char *full_path = strdup(full);
            if (!name || !full_path) {
                free(name);
                free(full_path);
                ret = CAT_ERROR;
                goto cleanup_iteration;
            }

            entries[entry_count].name = name;
            entries[entry_count].full_path = full_path;
            entries[entry_count].is_dir = is_directory;
            entry_count++;
        }
        closedir(dir);
        dir = NULL;

        /* Sort: dirs first, then alpha */
        if (entry_count > 0)
            qsort(entries, (size_t)entry_count, sizeof(cat__file_picker_entry),
                  cat__file_picker_cmp);

        /* ── Build list items ───────────────────────────────────────── */
        list_count = entry_count > 0 ? entry_count : 1;
        items = (cat_list_item *)calloc((size_t)list_count, sizeof(cat_list_item));
        is_dir_map = (bool *)calloc((size_t)list_count, sizeof(bool));
        trailing_pool = (char **)calloc((size_t)list_count, sizeof(char *));
        if (!items || !is_dir_map || !trailing_pool) {
            ret = CAT_ERROR;
            goto cleanup_iteration;
        }
        is_empty = (entry_count == 0);

        if (is_empty) {
            items[0] = (cat_list_item){ .label = "(empty)" };
            is_dir_map[0] = false;
        } else {
            for (int i = 0; i < entry_count; i++) {
                items[i].label = entries[i].name;
                items[i].metadata = entries[i].full_path;
                if (entries[i].is_dir) {
                    items[i].trailing_text = ">";
                } else {
                    /* Uppercase file extension as trailing text */
                    const char *dot = strrchr(entries[i].name, '.');
                    if (dot && dot[1]) {
                        dot++;
                        size_t len = strlen(dot);
                        char *upper = (char *)malloc(len + 1);
                        if (!upper) {
                            ret = CAT_ERROR;
                            goto cleanup_iteration;
                        }
                        for (size_t c = 0; c < len; c++)
                            upper[c] = (char)toupper((unsigned char)dot[c]);
                        upper[len] = '\0';
                        items[i].trailing_text = upper;
                        trailing_pool[i] = upper;
                    }
                }
                is_dir_map[i] = entries[i].is_dir;
            }
        }

        /* ── Build title ────────────────────────────────────────────── */
        char title_buf[1024];
        const char *title_str;
        if (opts->title) {
            title_str = opts->title;
        } else {
            cat__file_picker_format_title(title_buf, sizeof(title_buf),
                                         root_label, effective_root, current_path);
            title_str = title_buf;
        }

        /* ── Build footer ───────────────────────────────────────────── */
        cat_footer_item footer[4];
        int footer_count = 0;

        if (opts->mode == CAT_FILE_PICKER_FILES) {
            footer[footer_count++] = (cat_footer_item){ .button = CAT_BTN_B, .label = "BACK" };
            footer[footer_count++] = (cat_footer_item){
                .button = CAT_BTN_A, .label = "OPEN", .is_confirm = true };
        } else if (opts->mode == CAT_FILE_PICKER_DIRS) {
            footer[footer_count++] = (cat_footer_item){ .button = CAT_BTN_A, .label = "ENTER" };
            if (allow_create_dirs)
                footer[footer_count++] = (cat_footer_item){ .button = CAT_BTN_X, .label = "NEW DIR" };
            footer[footer_count++] = (cat_footer_item){ .button = CAT_BTN_B, .label = "BACK" };
            footer[footer_count++] = (cat_footer_item){
                .button = CAT_BTN_START, .label = "HERE", .is_confirm = true };
        } else { /* BOTH */
            footer[footer_count++] = (cat_footer_item){ .button = CAT_BTN_A, .label = "OPEN" };
            if (allow_create_dirs)
                footer[footer_count++] = (cat_footer_item){ .button = CAT_BTN_X, .label = "NEW DIR" };
            footer[footer_count++] = (cat_footer_item){ .button = CAT_BTN_B, .label = "BACK" };
            footer[footer_count++] = (cat_footer_item){
                .button = CAT_BTN_START, .label = "HERE", .is_confirm = true };
        }

        /* ── Show list ──────────────────────────────────────────────── */
        cat_list_opts lopts = cat_list_default_opts(title_str, items, list_count);
        cat__file_picker_footer_context footer_ctx = {0};
        lopts.footer = footer;
        lopts.footer_count = footer_count;
        lopts.status_bar = opts->status_bar;
        if (allow_create_dirs)
            lopts.action_button = CAT_BTN_X;
        if (opts->mode == CAT_FILE_PICKER_DIRS || opts->mode == CAT_FILE_PICKER_BOTH)
            lopts.confirm_button = CAT_BTN_START;
        if (opts->mode == CAT_FILE_PICKER_BOTH) {
            footer_ctx = (cat__file_picker_footer_context){
                .footer = footer,
                .footer_index = 0,
                .is_dir_map = is_dir_map,
                .entry_count = entry_count,
            };
            lopts.footer_update = cat__file_picker_footer_update;
            lopts.footer_update_userdata = &footer_ctx;
        }

        cat_list_result lresult;
        int rc = cat_list(&lopts, &lresult);

        if (rc == CAT_CANCELLED) {
            /* B pressed — go up or cancel */
            if (strcmp(current_path, effective_root) == 0) {
                ret = CAT_CANCELLED;
            } else {
                if (!cat__file_picker_parent_path(current_path) ||
                    !cat__file_picker_is_path_inside(current_path, effective_root))
                    snprintf(current_path, sizeof(current_path), "%s", effective_root);
            }
        } else if (rc == CAT_OK) {
            switch (lresult.action) {
            case CAT_ACTION_SELECTED: {
                if (is_empty) break; /* "(empty)" placeholder — do nothing */
                int idx = lresult.selected_index;
                if (idx >= 0 && idx < entry_count) {
                    char resolved_path[CAT__FILE_PICKER_PATH_MAX];
                    if (is_dir_map[idx]) {
                        /* Enter directory */
                        if (!cat__file_picker_resolve_existing_path(items[idx].metadata,
                                                                   resolved_path, sizeof(resolved_path)) ||
                            !cat__file_picker_is_path_inside(resolved_path, effective_root)) {
                            cat__file_picker_show_message("Folder is outside the allowed root.");
                        } else {
                            snprintf(current_path, sizeof(current_path), "%s", resolved_path);
                        }
                    } else {
                        /* File selected (only reachable if mode allows files) */
                        if (opts->mode == CAT_FILE_PICKER_FILES ||
                            opts->mode == CAT_FILE_PICKER_BOTH) {
                            if (!cat__file_picker_resolve_existing_path(items[idx].metadata,
                                                                       resolved_path, sizeof(resolved_path)) ||
                                !cat__file_picker_is_path_inside(resolved_path, effective_root)) {
                                cat__file_picker_show_message("File is outside the allowed root.");
                            } else {
                                snprintf(result->path, sizeof(result->path), "%s",
                                         items[idx].metadata);
                                result->is_directory = false;
                                ret = CAT_OK;
                            }
                        }
                    }
                }
                break;
            }
            case CAT_ACTION_TRIGGERED: {
                /* X button — new folder */
                cat_keyboard_result kb_result;
                int kb_ret = cat_keyboard("", "Enter folder name",
                                          CAT_KB_GENERAL, &kb_result);
                if (kb_ret == CAT_OK && kb_result.text[0] != '\0') {
                    if (!cat__file_picker_validate_component(kb_result.text)) {
                        cat__file_picker_show_message("Folder name must be a single path component.");
                        break;
                    }

                    char new_path[CAT__FILE_PICKER_PATH_MAX];
                    if (!cat__file_picker_join_path(new_path, sizeof(new_path),
                                                  current_path, kb_result.text)) {
                        cat__file_picker_show_message("Folder path is too long.");
                        break;
                    }

#ifdef _WIN32
                    char native_path[CAT__FILE_PICKER_PATH_MAX];
                    snprintf(native_path, sizeof(native_path), "%s", new_path);
                    for (char *p = native_path; *p; p++) {
                        if (*p == '/') *p = '\\';
                    }
                    int mk = _mkdir(native_path);
#else
                    int mk = mkdir(new_path, 0755);
#endif
                    if (mk != 0) {
                        char errmsg[512];
                        snprintf(errmsg, sizeof(errmsg),
                                 "Failed to create folder:\n%s", strerror(errno));
                        cat__file_picker_show_message(errmsg);
                    }
                }
                /* Refresh directory listing */
                break;
            }
            case CAT_ACTION_CONFIRMED: {
                /* START button — select current directory */
                if (opts->mode == CAT_FILE_PICKER_DIRS ||
                    opts->mode == CAT_FILE_PICKER_BOTH) {
                    snprintf(result->path, sizeof(result->path), "%s", current_path);
                    result->is_directory = true;
                    ret = CAT_OK;
                }
                break;
            }
            default:
                break;
            }
        } else {
            /* cat_list returned an error */
            ret = CAT_ERROR;
        }

        /* ── Cleanup iteration ──────────────────────────────────────── */
cleanup_iteration:
        if (dir) {
            closedir(dir);
            dir = NULL;
        }
        cat__file_picker_free_entries(entries, entry_count);
        entries = NULL;
        cat__file_picker_free_iteration_allocs(items, is_dir_map, trailing_pool, list_count);
        trailing_pool = NULL;
        items = NULL;
        is_dir_map = NULL;

        if (ret != -99) return ret;
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 * DOWNLOAD MANAGER Implementation
 * ═══════════════════════════════════════════════════════════════════════════ */

#ifdef CAT_ENABLE_CURL
#include <curl/curl.h>

/* Worker thread context */
typedef struct {
    cat_download      *downloads;
    int               count;
    cat_download_opts *opts;
    int               next_index;     /* Next download to start */
    int               active;         /* Currently running downloads */
    int               completed;      /* Finished (success + fail) */
    bool              cancel;         /* Set by UI to abort */
    pthread_mutex_t   mutex;
    pthread_t         threads[16];    /* Joinable worker thread IDs */
    int               thread_count;
} cat__dl_context;

/* Per-download thread data */
typedef struct {
    cat_download    *dl;
    cat__dl_context *ctx;
} cat__dl_thread_data;

typedef struct {
    cat_download_status status;
    float              progress;
    double             speed_bps;
    int                http_code;
    char               error[256];
} cat__dl_snapshot;

/* libcurl progress callback */
static int cat__dl_progress_cb(void *clientp,
                               double dltotal, double dlnow,
                               double ultotal, double ulnow) {
    (void)ultotal; (void)ulnow;
    cat__dl_thread_data *td = (cat__dl_thread_data *)clientp;

    /* Check cancellation */
    pthread_mutex_lock(&td->ctx->mutex);
    bool cancel = td->ctx->cancel;
    if (!cancel) {
        if (dltotal > 0.0) {
            td->dl->progress = (float)(dlnow / dltotal);
        }
        td->dl->speed_bps = dlnow; /* Will be divided by elapsed time later */
    }
    pthread_mutex_unlock(&td->ctx->mutex);
    if (cancel) return 1; /* Abort transfer */
    return 0;
}

/* libcurl write callback — write to file */
static size_t cat__dl_write_cb(void *ptr, size_t size, size_t nmemb, void *userdata) {
    FILE *fp = (FILE *)userdata;
    return fwrite(ptr, size, nmemb, fp);
}

/* Worker thread function */
static void *cat__dl_worker(void *arg) {
    cat__dl_thread_data *td = (cat__dl_thread_data *)arg;
    cat_download *dl = td->dl;

    pthread_mutex_lock(&td->ctx->mutex);
    bool already_cancelled = td->ctx->cancel;
    dl->status = CAT_DL_DOWNLOADING;
    dl->progress = 0.0f;
    dl->speed_bps = 0.0;
    dl->http_code = 0;
    dl->error[0] = '\0';
    pthread_mutex_unlock(&td->ctx->mutex);

    if (already_cancelled) {
        pthread_mutex_lock(&td->ctx->mutex);
        dl->status = CAT_DL_FAILED;
        snprintf(dl->error, sizeof(dl->error), "Cancelled");
        pthread_mutex_unlock(&td->ctx->mutex);
        goto done;
    }

    FILE *fp = fopen(dl->dest_path, "wb");
    if (!fp) {
        char err[256];
        snprintf(err, sizeof(err), "Cannot open: %s", dl->dest_path);
        pthread_mutex_lock(&td->ctx->mutex);
        snprintf(dl->error, sizeof(dl->error), "%s", err);
        dl->status = CAT_DL_FAILED;
        pthread_mutex_unlock(&td->ctx->mutex);
        cat_log("download: %s failed: %s", dl->label ? dl->label : dl->url, err);
        goto done;
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        pthread_mutex_lock(&td->ctx->mutex);
        snprintf(dl->error, sizeof(dl->error), "curl_easy_init failed");
        dl->status = CAT_DL_FAILED;
        pthread_mutex_unlock(&td->ctx->mutex);
        cat_log("download: %s failed: curl_easy_init failed", dl->label ? dl->label : dl->url);
        fclose(fp);
        goto done;
    }

    curl_easy_setopt(curl, CURLOPT_URL, dl->url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cat__dl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, cat__dl_progress_cb);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, td);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 100L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 30L);

    if (td->ctx->opts && td->ctx->opts->skip_ssl_verify) {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    } else {
        const char *ca = getenv("SSL_CERT_FILE");
        if (ca) curl_easy_setopt(curl, CURLOPT_CAINFO, ca);
    }

    /* Custom headers */
    struct curl_slist *hdrs = NULL;
    if (td->ctx->opts && td->ctx->opts->headers) {
        for (int i = 0; i < td->ctx->opts->header_count; i++) {
            hdrs = curl_slist_append(hdrs, td->ctx->opts->headers[i]);
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
    }

    CURLcode res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    /* Get final speed */
    double speed = 0;
    curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD, &speed);
    pthread_mutex_lock(&td->ctx->mutex);
    dl->http_code = (int)http_code;
    dl->speed_bps = speed;
    pthread_mutex_unlock(&td->ctx->mutex);

    if (hdrs) curl_slist_free_all(hdrs);
    curl_easy_cleanup(curl);
    fclose(fp);

    if (res == CURLE_ABORTED_BY_CALLBACK) {
        pthread_mutex_lock(&td->ctx->mutex);
        dl->status = CAT_DL_FAILED;
        snprintf(dl->error, sizeof(dl->error), "Cancelled");
        pthread_mutex_unlock(&td->ctx->mutex);
        remove(dl->dest_path);
        cat_log("download: %s cancelled", dl->label ? dl->label : dl->url);
    } else if (res != CURLE_OK) {
        const char *curl_err = curl_easy_strerror(res);
        pthread_mutex_lock(&td->ctx->mutex);
        dl->status = CAT_DL_FAILED;
        snprintf(dl->error, sizeof(dl->error), "%s", curl_err);
        pthread_mutex_unlock(&td->ctx->mutex);
        remove(dl->dest_path);
        cat_log("download: %s failed: %s", dl->label ? dl->label : dl->url, curl_err);
    } else if (http_code >= 400) {
        char err[256];
        snprintf(err, sizeof(err), "HTTP %d", (int)http_code);
        pthread_mutex_lock(&td->ctx->mutex);
        dl->status = CAT_DL_FAILED;
        snprintf(dl->error, sizeof(dl->error), "%s", err);
        pthread_mutex_unlock(&td->ctx->mutex);
        remove(dl->dest_path);
        cat_log("download: %s failed: %s", dl->label ? dl->label : dl->url, err);
    } else {
        pthread_mutex_lock(&td->ctx->mutex);
        dl->status = CAT_DL_COMPLETE;
        dl->progress = 1.0f;
        dl->error[0] = '\0';
        pthread_mutex_unlock(&td->ctx->mutex);
    }

done:
    pthread_mutex_lock(&td->ctx->mutex);
    td->ctx->active--;
    td->ctx->completed++;
    pthread_mutex_unlock(&td->ctx->mutex);

    free(td);
    return NULL;
}

/* Start the next pending download if capacity allows */
static void cat__dl_dispatch(cat__dl_context *ctx) {
    pthread_mutex_lock(&ctx->mutex);
    if (ctx->cancel) {
        pthread_mutex_unlock(&ctx->mutex);
        return;
    }
    int max_c = (ctx->opts && ctx->opts->max_concurrent > 0)
                 ? ctx->opts->max_concurrent : 3;

    while (ctx->active < max_c && ctx->next_index < ctx->count) {
        cat_download *dl = &ctx->downloads[ctx->next_index];
        ctx->next_index++;

        if (dl->status != CAT_DL_PENDING) continue;

        cat__dl_thread_data *td = (cat__dl_thread_data *)calloc(1, sizeof(*td));
        if (!td) break;
        td->dl = dl;
        td->ctx = ctx;

        ctx->active++;

        pthread_t thr;
        if (pthread_create(&thr, NULL, cat__dl_worker, td) != 0) {
            free(td);
            ctx->active--;
            dl->status = CAT_DL_FAILED;
            snprintf(dl->error, sizeof(dl->error), "Thread creation failed");
            ctx->completed++;
        } else if (ctx->thread_count < 16) {
            ctx->threads[ctx->thread_count++] = thr;
        } else {
            pthread_detach(thr); /* fallback: more threads than slots */
        }
    }
    pthread_mutex_unlock(&ctx->mutex);
}

/* Format bytes per second as human-readable speed */
static void cat__dl_format_speed(double bps, char *buf, int bufsize) {
    if (bps >= 1048576.0)
        snprintf(buf, bufsize, "%.1f MB/s", bps / 1048576.0);
    else if (bps >= 1024.0)
        snprintf(buf, bufsize, "%.0f KB/s", bps / 1024.0);
    else
        snprintf(buf, bufsize, "%.0f B/s", bps);
}

#endif /* CAT_ENABLE_CURL */

/* ═══════════════════════════════════════════════════════════════════════════
 * Queue Viewer Implementation
 * ═══════════════════════════════════════════════════════════════════════════ */

int cat_queue_viewer(const cat_queue_opts *opts) {
    if (!opts || !opts->snapshot) return CAT_ERROR;

    int max_items  = (opts->max_items > 0) ? opts->max_items : 256;
    cat_queue_item *items      = (cat_queue_item *)calloc((size_t)max_items, sizeof(cat_queue_item));
    int           *filter_map = (int *)calloc((size_t)max_items, sizeof(int));
    int           *filter_alpha = (int *)calloc((size_t)max_items, sizeof(int));
    if (!items || !filter_map || !filter_alpha) {
        free(items);
        free(filter_map);
        free(filter_alpha);
        return CAT_ERROR;
    }

    /* Filter: 0=All, 1=In Progress, 2=Done, 3=Failed */
    int filter = 0;
    static const char *cat__qv_default_filters[] = { "ALL", "IN PROGRESS", "DONE", "FAILED" };
    const char *cat__qv_filters[4];
    for (int i = 0; i < 4; i++) {
        cat__qv_filters[i] = cat__qv_default_filters[i];
        if (opts->filter_labels[i] && opts->filter_labels[i][0])
            cat__qv_filters[i] = opts->filter_labels[i];
    }

    int cursor     = 0;
    int scroll_top = 0;
    int last_cursor = -1;

    cat_text_scroll sel_scroll;
    cat_text_scroll_init(&sel_scroll);

    /* Pill animation state — same pattern as cat_list */
    float    pill_anim_y           = 0.0f;
    float    pill_anim_w           = 0.0f;
    float    pill_from_y           = 0.0f;
    float    pill_from_w           = 0.0f;
    uint32_t pill_anim_start       = 0;
    bool     pill_anim_initialized = false;
    int      pill_prev_target_y    = 0;
    int      pill_prev_target_w    = 0;

    cat_theme *theme    = cat_get_theme();
    int       screen_w = cat_get_screen_width();
    int       margin   = CAT_DS(cat__g.device_padding);
    int       pill_pad = CAT_DS(theme->ui_padding_x);

    TTF_Font *title_font  = cat_get_font(CAT_FONT_LARGE);
    TTF_Font *status_font = cat_get_font(CAT_FONT_SMALL);  /* right-aligned status text */
    TTF_Font *sub_font    = cat_get_font(CAT_FONT_TINY);   /* subtitle row + summary */
    TTF_Font *compact_title_font = cat_get_font(CAT_FONT_MEDIUM);
    TTF_Font *compact_sub_font   = cat_get_font(CAT_FONT_MICRO);
    if (!title_font || !status_font || !sub_font ||
        !compact_title_font || !compact_sub_font) {
        free(items);
        free(filter_map);
        free(filter_alpha);
        return CAT_ERROR;
    }

    /* Enable footer overflow so H/MENU can show hidden items. Font lookup is
     * the only fallible step after allocation, so do it before mutating the
     * global overflow state. */
    cat_footer_overflow_opts cat__qv_saved_overflow;
    cat_get_footer_overflow_opts(&cat__qv_saved_overflow);
    if (!cat__qv_saved_overflow.enabled)
        cat_set_footer_overflow_opts(NULL);

    int title_fh  = TTF_FontHeight(title_font);
    int status_fh = TTF_FontHeight(status_font);
    int sub_fh    = TTF_FontHeight(sub_font);
    int compact_title_fh = TTF_FontHeight(compact_title_font);
    int compact_sub_fh   = TTF_FontHeight(compact_sub_font);

    cat_draw_color col_done     = {100, 200, 100, 255};
    cat_draw_color col_failed   = {255, 100, 100, 255};
    cat_draw_color col_bar_bg   = { 40,  40,  50, 255};
    cat_draw_color col_bar_done = { 80, 200,  80, 255};
    cat_draw_color col_bar_fail = {200,  80,  80, 255};

    bool     running    = true;
    uint32_t last_frame = SDL_GetTicks();

    while (running) {
        uint32_t now = SDL_GetTicks();
        uint32_t dt  = now - last_frame;
        last_frame   = now;

        /* ── Snapshot ─────────────────────────────────────────────────────── */
        int count = opts->snapshot(items, max_items, opts->userdata);
        if (count < 0) count = 0;
        if (count > max_items) count = max_items;

        /* ── Summary stats (full unfiltered list) ─────────────────────────── */
        int  stat_total   = count;
        int  stat_done    = 0;
        int  stat_failed  = 0;
        bool has_active   = false;
        for (int i = 0; i < count; i++) {
            cat_queue_status s = items[i].status;
            if      (s == CAT_QUEUE_DONE || s == CAT_QUEUE_SKIPPED)  stat_done++;
            else if (s == CAT_QUEUE_FAILED)                          stat_failed++;
            if      (s == CAT_QUEUE_PENDING || s == CAT_QUEUE_RUNNING) has_active = true;
        }

        /* ── Filter map ───────────────────────────────────────────────────── */
        int filtered_count = 0;
        for (int i = 0; i < count; i++) {
            cat_queue_status s = items[i].status;
            bool match = false;
            switch (filter) {
                case 0: match = true; break;
                case 1: match = (s == CAT_QUEUE_PENDING || s == CAT_QUEUE_RUNNING); break;
                case 2: match = (s == CAT_QUEUE_DONE    || s == CAT_QUEUE_SKIPPED); break;
                case 3: match = (s == CAT_QUEUE_FAILED);  break;
            }
            if (match) filter_map[filtered_count++] = i;
        }

        int alpha_starts[27];
        int alpha_count = 0;
        {
            int prev_group = -1;
            for (int i = 0; i < filtered_count; i++) {
                int g = cat__alpha_group_for_text(items[filter_map[i]].title);
                if (g != prev_group) {
                    if (alpha_count < 27) alpha_starts[alpha_count++] = i;
                    prev_group = g;
                }
                filter_alpha[i] = alpha_count - 1;
            }
        }

        /* ── Layout ───────────────────────────────────────────────────────── */
        TTF_Font *layout_title_font  = title_font;
        TTF_Font *layout_status_font = status_font;
        TTF_Font *layout_sub_font    = sub_font;
        int layout_title_fh   = title_fh;
        int layout_status_fh  = status_fh;
        int layout_sub_fh     = sub_fh;
        int layout_title_gap  = CAT_DS(2);
        int layout_bottom_pad = CAT_DS(2);
        int layout_bar_h      = CAT_DS(4);
        int layout_summary_h  = CAT_DS(14);

        /* Content rect covers title→footer; reserve summary space at the bottom.
         * This widget always draws a title (custom or default "QUEUE"). */
        SDL_Rect crect = cat_get_content_rect(true, true,
                                             opts->status_bar != NULL);
        int list_y = crect.y;
        int list_h = crect.h - layout_summary_h;
        if (list_h < 0) list_h = 0;

        int item_h = layout_title_fh + layout_sub_fh + layout_title_gap + layout_bottom_pad;
        int max_visible = (item_h > 0) ? (list_h / item_h) : 1;
        if (max_visible < 1) max_visible = 1;

        int default_leftover_h = list_h - max_visible * item_h;
        if (default_leftover_h < 0) default_leftover_h = 0;
        bool balance_rows = false;

        if (cat__g.device_scale == 3 &&
            filtered_count > 3 &&
            max_visible == 3 &&
            default_leftover_h >= CAT_DS(18)) {
            int compact_title_gap  = CAT_DS(1);
            int compact_bottom_pad = CAT_DS(2);
            int compact_bar_h      = CAT_DS(3);
            int compact_summary_h  = CAT_DS(12);
            int compact_item_h = compact_title_fh + compact_sub_fh
                               + compact_title_gap + compact_bottom_pad;
            int compact_list_h = crect.h - compact_summary_h;
            if (compact_list_h < 0) compact_list_h = 0;

            int compact_max_visible = (compact_item_h > 0)
                ? (compact_list_h / compact_item_h) : 1;
            if (compact_max_visible < 1) compact_max_visible = 1;

            if (compact_max_visible >= 4) {
                balance_rows = true;
                layout_title_font  = compact_title_font;
                layout_sub_font    = compact_sub_font;
                layout_title_fh    = compact_title_fh;
                layout_sub_fh      = compact_sub_fh;
                layout_title_gap   = compact_title_gap;
                layout_bottom_pad  = compact_bottom_pad;
                layout_bar_h       = compact_bar_h;
                layout_summary_h   = compact_summary_h;
                item_h             = compact_item_h;
                list_h             = compact_list_h;
                max_visible        = compact_max_visible;
            }
        }

        int layout_bar_r = layout_bar_h / 2;
        if (layout_bar_r < 1) layout_bar_r = 1;

        bool needs_scrollbar = (filtered_count > max_visible);
        int  pill_w          = screen_w - margin * 2;

        /* ── Clamp cursor ─────────────────────────────────────────────────── */
        if (filtered_count == 0) {
            cursor = 0;
        } else {
            if (cursor < 0) cursor = 0;
            if (cursor >= filtered_count) cursor = filtered_count - 1;
        }

        /* ── Input ────────────────────────────────────────────────────────── */
        cat_input_event ev;
        bool callback_refresh = false;
        while (cat_poll_input(&ev)) {
            if (!ev.pressed) continue;
            switch (ev.button) {
                case CAT_BTN_UP:
                    if (filtered_count > 0)
                        cursor = (cursor > 0) ? cursor - 1 : filtered_count - 1;
                    break;
                case CAT_BTN_DOWN:
                    if (filtered_count > 0)
                        cursor = (cursor < filtered_count - 1) ? cursor + 1 : 0;
                    break;
                case CAT_BTN_LEFT:
                    if (filtered_count > 0) {
                        cursor -= max_visible;
                        if (cursor < 0) cursor = 0;
                    }
                    break;
                case CAT_BTN_RIGHT:
                    if (filtered_count > 0) {
                        cursor += max_visible;
                        if (cursor >= filtered_count) cursor = filtered_count - 1;
                    }
                    break;
                case CAT_BTN_L1:
                    if (filtered_count > 0) {
                        int gi = filter_alpha[cursor] - 1;
                        if (gi >= 0)
                            cursor = alpha_starts[gi];
                    }
                    break;
                case CAT_BTN_R1:
                    if (filtered_count > 0) {
                        int gi = filter_alpha[cursor] + 1;
                        if (gi < alpha_count)
                            cursor = alpha_starts[gi];
                    }
                    break;
                case CAT_BTN_Y:
                    if (!opts->hide_filter) {
                        filter = (filter + 1) % 4;
                        cursor = 0; scroll_top = 0; last_cursor = -1;
                        pill_anim_initialized = false;  /* snap pill to new row */
                    }
                    break;
                case CAT_BTN_MENU:
                    cat_show_footer_overflow();
                    break;
                case CAT_BTN_A:
                    if (opts->on_detail && filtered_count > 0) {
                        cat_queue_status s = items[filter_map[cursor]].status;
                        if (s == CAT_QUEUE_DONE || s == CAT_QUEUE_FAILED || s == CAT_QUEUE_SKIPPED) {
                            opts->on_detail(&items[filter_map[cursor]], opts->userdata);
                            callback_refresh = true;
                            goto done_input;
                        }
                    }
                    break;
                case CAT_BTN_X:
                    if (has_active) {
                        if (opts->on_cancel) {
                            opts->on_cancel(opts->userdata);
                            callback_refresh = true;
                            goto done_input;
                        }
                    } else if (opts->on_clear && stat_done > 0) {
                        opts->on_clear(opts->userdata);
                        callback_refresh = true;
                        goto done_input;
                    }
                    break;
                case CAT_BTN_B:
                    running = false;
                    break;
                default: break;
            }
        }
        done_input:

        if (callback_refresh) {
            last_frame = SDL_GetTicks();
            continue;
        }

        /* Re-clamp after input */
        if (filtered_count > 0) {
            if (cursor < 0) cursor = 0;
            if (cursor >= filtered_count) cursor = filtered_count - 1;
        } else {
            cursor = 0;
        }

        /* ── Scroll adjustment ────────────────────────────────────────────── */
        if (cursor < scroll_top) scroll_top = cursor;
        if (filtered_count > 0 && cursor >= scroll_top + max_visible)
            scroll_top = cursor - max_visible + 1;
        if (scroll_top < 0) scroll_top = 0;

        int max_scroll_top = (filtered_count > max_visible)
            ? (filtered_count - max_visible) : 0;
        if (scroll_top > max_scroll_top) scroll_top = max_scroll_top;

        int visible_rows = filtered_count - scroll_top;
        if (visible_rows > max_visible) visible_rows = max_visible;
        if (visible_rows < 0) visible_rows = 0;

        int used_h = visible_rows * item_h;
        int leftover_h = list_h - used_h;
        if (leftover_h < 0) leftover_h = 0;

        int row_gap = 0;
        if (balance_rows && visible_rows > 1) {
            row_gap = leftover_h / (visible_rows - 1);
            if (row_gap > CAT_DS(6)) row_gap = CAT_DS(6);
        }

        int used_h_with_gaps = used_h;
        if (visible_rows > 1) used_h_with_gaps += (visible_rows - 1) * row_gap;

        int body_offset = balance_rows
            ? cat__max(0, (list_h - used_h_with_gaps) / 2)
            : 0;
        int first_row_y = list_y + body_offset;
        int row_pitch   = item_h + row_gap;

        /* ── Pill animation ───────────────────────────────────────────────── */
        int pill_target_y = first_row_y + (cursor - scroll_top) * row_pitch;
        int pill_target_w = needs_scrollbar ? (pill_w - CAT_S(10)) : pill_w;

        if (cursor != last_cursor) {
            if (last_cursor >= 0) {
                float prev_t = cat__clampf(
                    (float)(now - pill_anim_start) / CAT__PILL_ANIM_MS, 0.0f, 1.0f);
                pill_from_y = cat__lerpf(pill_from_y, (float)pill_prev_target_y, prev_t);
                pill_from_w = cat__lerpf(pill_from_w, (float)pill_prev_target_w, prev_t);
            } else {
                pill_from_y = (float)pill_target_y;
                pill_from_w = (float)pill_target_w;
            }
            pill_anim_start = now;
            last_cursor     = cursor;
            cat_text_scroll_reset(&sel_scroll);
        }
        pill_prev_target_y = pill_target_y;
        pill_prev_target_w = pill_target_w;

        if (!pill_anim_initialized) {
            pill_anim_y = pill_from_y = (float)pill_target_y;
            pill_anim_w = pill_from_w = (float)pill_target_w;
            pill_anim_start       = now;
            pill_anim_initialized = true;
        }

        {
            float t = cat__clampf(
                (float)(now - pill_anim_start) / CAT__PILL_ANIM_MS, 0.0f, 1.0f);
            pill_anim_y = cat__lerpf(pill_from_y, (float)pill_target_y, t);
            pill_anim_w = cat__lerpf(pill_from_w, (float)pill_target_w, t);
            if (t < 1.0f) cat_request_frame();
            if (pill_anim_y < (float)first_row_y) pill_anim_y = (float)first_row_y;
            int pill_max_y = first_row_y;
            if (visible_rows > 0)
                pill_max_y = first_row_y + (visible_rows - 1) * row_pitch;
            if (pill_anim_y > (float)pill_max_y)
                pill_anim_y = (float)pill_max_y;
        }

        /* Keep rendering while jobs are in progress */
        if (has_active) cat_request_frame();

        /* ── Render ───────────────────────────────────────────────────────── */
        cat_draw_background();

        /* Title with filter suffix */
        {
            char tbuf[320];
            const char *base = opts->title ? opts->title : "QUEUE";
            if (!opts->hide_filter)
                snprintf(tbuf, sizeof(tbuf), "%s [%s]", base, cat__qv_filters[filter]);
            else
                snprintf(tbuf, sizeof(tbuf), "%s", base);
            cat_draw_screen_title(tbuf, opts->status_bar);
        }
        if (opts->status_bar) cat_draw_status_bar(opts->status_bar);

        /* Determine which row the pill center is over for text color tracking */
        int pill_center_y = (int)pill_anim_y + item_h / 2;
        int pill_row_idx  = (row_pitch > 0)
            ? (pill_center_y - first_row_y) / row_pitch : 0;
        if (pill_row_idx < 0)            pill_row_idx = 0;
        if (visible_rows > 0 && pill_row_idx >= visible_rows) pill_row_idx = visible_rows - 1;
        int highlight_fi = scroll_top + pill_row_idx;

        /* Pill background */
        if (filtered_count > 0)
            cat_draw_pill(margin, (int)pill_anim_y, (int)pill_anim_w, item_h,
                         theme->highlight);

        /* Items */
        int text_x = margin + pill_pad;

        for (int vi = 0; vi < max_visible && (scroll_top + vi) < filtered_count; vi++) {
            int fi = scroll_top + vi;
            int ri = filter_map[fi];
            int iy = first_row_y + vi * row_pitch;

            cat_queue_item   *item = &items[ri];
            cat_queue_status  s    = item->status;
            bool is_selected  = (fi == cursor);
            bool is_highlight = (fi == highlight_fi);

            cat_draw_color status_color;
            switch (s) {
                case CAT_QUEUE_DONE:    status_color = col_done;      break;
                case CAT_QUEUE_FAILED:  status_color = col_failed;    break;
                case CAT_QUEUE_RUNNING: status_color = theme->accent; break;
                default:               status_color = theme->hint;   break;
            }

            cat_draw_color text_color = is_highlight ? theme->highlighted_text : theme->text;
            cat_draw_color sub_color  = is_highlight ? theme->highlighted_text : theme->hint;
            if (is_highlight) status_color = theme->highlighted_text;

            /* Per-item layout: reserve space for status text if present */
            int content_w  = pill_w - pill_pad * 2;
            int status_w   = item->status_text[0]
                ? cat_measure_text(layout_status_font, item->status_text) : 0;
            int text_max_w = content_w
                - (status_w > 0 ? status_w + CAT_DS(4) : 0);
            if (text_max_w < CAT_S(40)) text_max_w = CAT_S(40);

            int title_y = iy;
            int sub_y   = iy + layout_title_fh + layout_title_gap;

            /* Status text — right-aligned, vertically centered on title row */
            if (item->status_text[0]) {
                int sx = margin + pill_w - pill_pad - status_w;
                int sy = title_y + (layout_title_fh - layout_status_fh) / 2;
                cat_draw_text(layout_status_font, item->status_text, sx, sy, status_color);
            }

            /* Title — scroll horizontally when selected and overflowing */
            {
                int tw = cat_measure_text(layout_title_font, item->title);
                if (is_selected && tw > text_max_w) {
                    cat_text_scroll_update(&sel_scroll, tw, text_max_w, dt);
                    /* Schedule one wake at pause end rather than pinning 60fps. */
                    if (sel_scroll.active) {
                        if (sel_scroll.pause_timer > 0)
                            cat_request_frame_in((uint32_t)sel_scroll.pause_timer);
                        else
                            cat_request_frame();
                    }
                    SDL_Rect clip = { text_x, title_y, text_max_w, layout_title_fh };
                    SDL_RenderSetClipRect(cat_get_renderer(), &clip);
                    cat_draw_text(layout_title_font, item->title,
                                 text_x - sel_scroll.offset, title_y, text_color);
                    SDL_RenderSetClipRect(cat_get_renderer(), NULL);
                } else {
                    cat_draw_text_ellipsized(layout_title_font, item->title,
                                            text_x, title_y, text_color, text_max_w);
                }
            }

            /* Subtitle / inline progress bar */
            {
                bool has_subtitle   = item->subtitle[0] != '\0';
                bool has_progress   = item->progress >= 0.0f;
                int  subtitle_gap   = CAT_DS(8);
                int  subtitle_max_w = content_w;
                int  bar_x          = text_x;
                int  bar_w          = 0;
                int  bar_y          = sub_y + (layout_sub_fh - layout_bar_h) / 2;

                if (has_progress) {
                    if (has_subtitle) {
                        int min_subtitle_w = CAT_S(40);
                        int min_bar_w      = CAT_DS(56);
                        int max_bar_w      = CAT_DS(140);
                        int target_bar_w   = content_w / 3;
                        if (target_bar_w < min_bar_w) target_bar_w = min_bar_w;
                        if (target_bar_w > max_bar_w) target_bar_w = max_bar_w;
                        bar_w = target_bar_w;
                        if (content_w - subtitle_gap - bar_w < min_subtitle_w) {
                            bar_w = content_w - subtitle_gap - min_subtitle_w;
                        }
                        if (bar_w > 0) {
                            subtitle_max_w = content_w - subtitle_gap - bar_w;
                            if (subtitle_max_w < min_subtitle_w) subtitle_max_w = min_subtitle_w;
                            bar_x = text_x + subtitle_max_w + subtitle_gap;
                        } else {
                            bar_w = 0;
                        }
                    } else {
                        bar_w = content_w;
                    }
                }

                if (has_subtitle) {
                    cat_draw_text_ellipsized(layout_sub_font, item->subtitle,
                                            text_x, sub_y,
                                            sub_color, subtitle_max_w);
                }

                if (bar_w > 0) {
                    cat_draw_rounded_rect(bar_x, bar_y, bar_w, layout_bar_h, layout_bar_r, col_bar_bg);

                    cat_draw_color fill;
                    if      (s == CAT_QUEUE_DONE)    fill = col_bar_done;
                    else if (s == CAT_QUEUE_FAILED)  fill = col_bar_fail;
                    else if (s == CAT_QUEUE_SKIPPED) fill = theme->hint;
                    else                            fill = theme->accent;

                    float p = item->progress > 1.0f ? 1.0f : item->progress;
                    int fw  = (int)((float)bar_w * p);
                    if (fw > 0) {
                        if (fw < layout_bar_h) fw = layout_bar_h;
                        if (fw > bar_w) fw = bar_w;
                        cat_draw_rounded_rect(bar_x, bar_y, fw, layout_bar_h, layout_bar_r, fill);
                    }
                }
            }
        }

        /* Empty state */
        if (filtered_count == 0) {
            const char *msg = (stat_total == 0) ? "NO ITEMS IN QUEUE."
                                                : "NO ITEMS MATCH THIS FILTER.";
            int mw = cat_measure_text(layout_sub_font, msg);
            cat_draw_text(layout_sub_font, msg,
                         (screen_w - mw) / 2,
                         list_y + (list_h - layout_sub_fh) / 2,
                         theme->hint);
        }

        /* Scrollbar — align with the visible pill stack, outside pill area */
        if (needs_scrollbar) {
            int sb_x          = screen_w - margin - CAT_S(6);
            int sb_y          = first_row_y;
            int sb_h          = used_h_with_gaps;
            if (sb_h > 0) {
                cat_draw_scrollbar(sb_x, sb_y, sb_h,
                                  max_visible, filtered_count, scroll_top);
            }
        }

        /* Summary bar */
        {
            int sep_y = crect.y + crect.h - layout_summary_h;
            SDL_Renderer *rend = cat_get_renderer();
            SDL_BlendMode prev_blend = SDL_BLENDMODE_NONE;
            SDL_GetRenderDrawBlendMode(rend, &prev_blend);
            SDL_SetRenderDrawColor(rend, theme->hint.r, theme->hint.g,
                                   theme->hint.b, 100);
            SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);
            SDL_Rect sep_rect = { margin, sep_y, screen_w - margin * 2, 1 };
            SDL_RenderFillRect(rend, &sep_rect);
            SDL_SetRenderDrawBlendMode(rend, prev_blend);

            char summary[192];
            summary[0] = '\0';
            if (opts->summary)
                opts->summary(summary, sizeof(summary), opts->userdata);
            if (!summary[0]) {
                snprintf(summary, sizeof(summary), "%d/%d COMPLETE, %d FAILED",
                         stat_done, stat_total, stat_failed);
            }
            int summary_w = screen_w - margin * 2;
            int sw = cat_measure_text(layout_sub_font, summary);
            int sx = (screen_w - sw) / 2;
            if (sw > summary_w) sx = margin;
            cat_draw_text_ellipsized(layout_sub_font, summary,
                                     sx,
                                     sep_y + (layout_summary_h - layout_sub_fh) / 2,
                                     theme->hint, summary_w);
        }

        /* Footer */
        {
            cat_footer_item fi[4];
            int nf = 0;

            if (!opts->hide_filter)
                fi[nf++] = (cat_footer_item){ .button = CAT_BTN_Y, .label = "FILTER" };
            if (opts->on_detail && filtered_count > 0) {
                cat_queue_status s = items[filter_map[cursor]].status;
                if (s == CAT_QUEUE_DONE || s == CAT_QUEUE_FAILED || s == CAT_QUEUE_SKIPPED)
                    fi[nf++] = (cat_footer_item){ .button = CAT_BTN_A, .label = "DETAILS" };
            }
            fi[nf++] = (cat_footer_item){ .button = CAT_BTN_B, .label = "BACK" };
            if (has_active) {
                if (opts->on_cancel)
                    fi[nf++] = (cat_footer_item){
                        .button = CAT_BTN_X, .label = "STOP ALL", .is_confirm = true };
            } else if (opts->on_clear && stat_done > 0) {
                fi[nf++] = (cat_footer_item){
                    .button = CAT_BTN_X, .label = "CLEAR DONE", .is_confirm = true };
            }
            cat_draw_footer(fi, nf);
        }

        cat_present();
    }

    cat_set_footer_overflow_opts(&cat__qv_saved_overflow);
    free(items);
    free(filter_map);
    free(filter_alpha);
    return CAT_OK;
}

#ifdef CAT_ENABLE_CURL

int cat_download_manager(cat_download *downloads, int count,
                        cat_download_opts *opts, cat_download_result *result) {
    if (!downloads || count <= 0 || !result) return CAT_ERROR;

    memset(result, 0, sizeof(*result));
    result->total = count;

    /* Initialize all downloads to pending */
    for (int i = 0; i < count; i++) {
        downloads[i].status = CAT_DL_PENDING;
        downloads[i].progress = 0.0f;
        downloads[i].speed_bps = 0;
        downloads[i].http_code = 0;
        downloads[i].error[0] = '\0';
    }

    /* Init curl globally (safe to call multiple times) */
    curl_global_init(CURL_GLOBAL_DEFAULT);

    cat__dl_context ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.downloads = downloads;
    ctx.count = count;
    ctx.opts = opts;
    pthread_mutex_init(&ctx.mutex, NULL);

    cat__dl_snapshot *snapshots = (cat__dl_snapshot *)calloc((size_t)count, sizeof(*snapshots));
    if (!snapshots) {
        pthread_mutex_destroy(&ctx.mutex);
        curl_global_cleanup();
        return CAT_ERROR;
    }

    cat_theme *theme = cat_get_theme();
    int screen_w = cat_get_screen_width();
    int screen_h = cat_get_screen_height();
    TTF_Font *label_font = cat_get_font(CAT_FONT_MEDIUM);
    TTF_Font *speed_font = cat_get_font(CAT_FONT_SMALL);

    bool show_speed = true;
    bool all_done = false;
    int scroll = 0;

    /* Progress bar sizing — 3/4 of screen width, max 900px */
    int bar_w = screen_w * 3 / 4;
    if (bar_w > CAT_S(900)) bar_w = CAT_S(900);
    int bar_h = CAT_S(16);
    int bar_r = CAT_S(8);
    int item_gap = CAT_S(12);
    int label_h = label_font ? TTF_FontHeight(label_font) : CAT_S(30);
    int speed_h = speed_font ? TTF_FontHeight(speed_font) : CAT_S(20);
    int item_h = label_h + CAT_S(4) + bar_h + (show_speed ? CAT_S(2) + speed_h : 0);

    /* Visible area */
    int title_zone = CAT_S(85);
    int footer_zone = cat_get_footer_height();
    int content_y = title_zone;
    int content_h = screen_h - title_zone - footer_zone;
    int max_visible = content_h / (item_h + item_gap);
    if (max_visible < 1) max_visible = 1;

    while (!all_done) {
        /* Dispatch pending downloads */
        cat__dl_dispatch(&ctx);

        /* Check overall status */
        int active_count = 0;
        pthread_mutex_lock(&ctx.mutex);
        int done_count = ctx.completed;
        bool cancelled = ctx.cancel;
        active_count = ctx.active;
        for (int i = 0; i < count; i++) {
            snapshots[i].status = downloads[i].status;
            snapshots[i].progress = downloads[i].progress;
            snapshots[i].speed_bps = downloads[i].speed_bps;
            snapshots[i].http_code = downloads[i].http_code;
            strncpy(snapshots[i].error, downloads[i].error, sizeof(snapshots[i].error) - 1);
            snapshots[i].error[sizeof(snapshots[i].error) - 1] = '\0';
        }
        pthread_mutex_unlock(&ctx.mutex);

        int comp_ok = 0, comp_fail = 0;
        for (int i = 0; i < count; i++) {
            if (snapshots[i].status == CAT_DL_COMPLETE) comp_ok++;
            if (snapshots[i].status == CAT_DL_FAILED) comp_fail++;
        }
        result->completed = comp_ok;
        result->failed = comp_fail;

        if (done_count >= count) all_done = true;
        if (cancelled && active_count <= 0) { all_done = true; result->cancelled = true; }
        if (!all_done) cat_request_frame();

        /* Input */
        cat_input_event ev;
        while (cat_poll_input(&ev)) {
            if (!ev.pressed) continue;
            switch (ev.button) {
                case CAT_BTN_Y:
                    if (!all_done) {
                        pthread_mutex_lock(&ctx.mutex);
                        ctx.cancel = true;
                        for (int ci = 0; ci < count; ci++) {
                            if (downloads[ci].status == CAT_DL_PENDING) {
                                downloads[ci].status = CAT_DL_FAILED;
                                snprintf(downloads[ci].error, sizeof(downloads[ci].error), "Cancelled");
                                ctx.completed++;
                            } else if (downloads[ci].status == CAT_DL_DOWNLOADING) {
                                downloads[ci].status = CAT_DL_FAILED;
                                snprintf(downloads[ci].error, sizeof(downloads[ci].error), "Cancelled");
                            }
                            /* Update snapshots so render shows cancelled state */
                            snapshots[ci].status = downloads[ci].status;
                            snapshots[ci].progress = downloads[ci].progress;
                            strncpy(snapshots[ci].error, downloads[ci].error, sizeof(snapshots[ci].error) - 1);
                            snapshots[ci].error[sizeof(snapshots[ci].error) - 1] = '\0';
                        }
                        comp_fail = count - comp_ok;
                        pthread_mutex_unlock(&ctx.mutex);
                        result->cancelled = true;
                    }
                    break;
                case CAT_BTN_X:
                    show_speed = !show_speed;
                    item_h = label_h + CAT_S(4) + bar_h + (show_speed ? CAT_S(2) + speed_h : 0);
                    max_visible = content_h / (item_h + item_gap);
                    if (max_visible < 1) max_visible = 1;
                    {
                        int max_scroll = count - max_visible;
                        if (max_scroll < 0) max_scroll = 0;
                        if (scroll > max_scroll) scroll = max_scroll;
                    }
                    break;
                case CAT_BTN_A:
                    if (all_done || result->cancelled) goto dm_exit;
                    break;
                case CAT_BTN_UP:
                    if (scroll > 0) scroll--;
                    break;
                case CAT_BTN_DOWN:
                    if (scroll < count - max_visible) scroll++;
                    break;
                default: break;
            }
        }

        /* Auto-scroll to show active downloads */
        if (!all_done) {
            for (int i = count - 1; i >= 0; i--) {
                if (snapshots[i].status == CAT_DL_DOWNLOADING) {
                    if (i >= scroll + max_visible) scroll = i - max_visible + 1;
                    if (i < scroll) scroll = i;
                    break;
                }
            }
        }

        /* ── Render ── */
        cat_draw_background();

        /* Title: "Downloading..." or completion summary */
        {
            char title_buf[128];
            if (result->cancelled) {
                snprintf(title_buf, sizeof(title_buf), "Downloads Cancelled");
            } else if (all_done) {
                snprintf(title_buf, sizeof(title_buf), "Downloads Complete (%d/%d)", comp_ok, count);
            } else {
                snprintf(title_buf, sizeof(title_buf), "Downloading... %d/%d", done_count, count);
            }
            cat_draw_screen_title(title_buf, NULL);
        }

        /* Download items */
        int bar_x = (screen_w - bar_w) / 2;
        for (int vi = 0; vi < max_visible && vi + scroll < count; vi++) {
            int i = vi + scroll;
            cat_download *dl = &downloads[i];
            cat__dl_snapshot *dls = &snapshots[i];
            int iy = content_y + vi * (item_h + item_gap);

            /* Label */
            const char *lbl = dl->label ? dl->label : dl->url;
            cat_draw_color lbl_color = theme->text;
            if (dls->status == CAT_DL_COMPLETE) lbl_color = (cat_draw_color){100, 255, 100, 255};
            if (dls->status == CAT_DL_FAILED) lbl_color = (cat_draw_color){255, 100, 100, 255};

            if (label_font) {
                cat_draw_text_clipped(label_font, lbl, bar_x, iy, lbl_color, bar_w);
            }

            /* Progress bar */
            int bar_y = iy + label_h + CAT_S(4);
            cat_draw_color bar_bg = {40, 40, 50, 255};
            cat_draw_rounded_rect(bar_x, bar_y, bar_w, bar_h, bar_r, bar_bg);

            if (dls->progress > 0.001f) {
                int fill_w = (int)(dls->progress * bar_w);
                if (fill_w < bar_h) fill_w = bar_h; /* Minimum for rounding */
                cat_draw_color fill_c;
                if (dls->status == CAT_DL_COMPLETE) fill_c = (cat_draw_color){80, 200, 80, 255};
                else if (dls->status == CAT_DL_FAILED) fill_c = (cat_draw_color){200, 80, 80, 255};
                else fill_c = theme->highlight;
                cat_draw_rounded_rect(bar_x, bar_y, fill_w, bar_h, bar_r, fill_c);
            }

            /* Speed / status text */
            if (show_speed && speed_font) {
                int sy = bar_y + bar_h + CAT_S(2);
                char speed_buf[64];
                if (dls->status == CAT_DL_DOWNLOADING) {
                    cat__dl_format_speed(dls->speed_bps, speed_buf, sizeof(speed_buf));
                    char pct_buf[80];
                    snprintf(pct_buf, sizeof(pct_buf), "%.0f%% — %s", dls->progress * 100.0f, speed_buf);
                    cat_draw_text(speed_font, pct_buf, bar_x, sy, theme->hint);
                } else if (dls->status == CAT_DL_COMPLETE) {
                    cat_draw_text(speed_font, "Complete", bar_x, sy, (cat_draw_color){100,255,100,255});
                } else if (dls->status == CAT_DL_FAILED) {
                    cat_draw_text_clipped(speed_font, dls->error, bar_x, sy,
                        (cat_draw_color){255,100,100,255}, bar_w);
                } else {
                    cat_draw_text(speed_font, "Pending...", bar_x, sy, theme->hint);
                }
            }
        }

        /* Scrollbar */
        if (count > max_visible) {
            int visible_rows = count - scroll;
            if (visible_rows > max_visible) visible_rows = max_visible;
            if (visible_rows > 0) {
                int used_h = visible_rows * item_h + (visible_rows - 1) * item_gap;
                cat_draw_scrollbar(bar_x + bar_w + CAT_S(12), content_y,
                    used_h, max_visible, count, scroll);
            }
        }

        /* Footer */
        if (all_done || result->cancelled) {
            cat_footer_item dm_footer[] = {
                { .button = CAT_BTN_A, .label = "CLOSE", .is_confirm = true },
            };
            cat_draw_footer(dm_footer, 1);
        } else {
            cat_footer_item dm_footer[] = {
                { .button = CAT_BTN_Y, .label = "CANCEL", .is_confirm = false },
                { .button = CAT_BTN_X, .label = show_speed ? "HIDE SPEED" : "SHOW SPEED", .is_confirm = false },
            };
            cat_draw_footer(dm_footer, 2);
        }

        cat_present();

    }

dm_exit:
    for (int i = 0; i < ctx.thread_count; i++)
        pthread_join(ctx.threads[i], NULL);
    pthread_mutex_destroy(&ctx.mutex);
    curl_global_cleanup();
    free(snapshots);

    if (result->cancelled) return CAT_CANCELLED;
    if (result->failed > 0 && result->completed == 0) return CAT_ERROR;
    return CAT_OK;
}

#endif /* CAT_ENABLE_CURL */

/* ═══════════════════════════════════════════════════════════════════════════
 * Non-Blocking List Pane Implementation
 * ═══════════════════════════════════════════════════════════════════════════ */

void cat_list_state_init(cat_list_state *s, int visible_rows) {
    if (!s) return;
    s->cursor       = 0;
    s->scroll_offset = 0;
    s->visible_rows  = visible_rows > 0 ? visible_rows : 1;
}

static void cat__ls_clamp(cat_list_state *s, int count) {
    if (s->cursor < s->scroll_offset)
        s->scroll_offset = s->cursor;
    else if (s->cursor >= s->scroll_offset + s->visible_rows)
        s->scroll_offset = s->cursor - s->visible_rows + 1;
    if (s->scroll_offset < 0) s->scroll_offset = 0;
    (void)count;
}

void cat_list_state_move(cat_list_state *s, int delta, int count) {
    if (!s || delta == 0 || count <= 0) return;
    /* Wrap around the ends (top<->bottom) rather than clamping, matching the
       tab navigation. Modulo keeps it correct for any delta magnitude. */
    int next = (s->cursor + delta) % count;
    if (next < 0) next += count;
    s->cursor = next;
    cat__ls_clamp(s, count);
}

void cat_list_state_page(cat_list_state *s, int delta, int count) {
    if (!s || delta == 0 || count <= 0) return;
    int next = s->cursor + s->visible_rows * delta;
    if (next < 0)     next = 0;
    if (next >= count) next = count - 1;
    s->cursor = next;
    cat__ls_clamp(s, count);
}

void cat_list_state_jump(cat_list_state *s, int target, int count) {
    if (!s || count <= 0) return;
    if (target < 0)     target = 0;
    if (target >= count) target = count - 1;
    s->cursor = target;
    cat__ls_clamp(s, count);
}

int cat_list_state_jump_letter(cat_list_state *s,
                                const char *(*get_label)(int, void *),
                                void *user, int count, int direction) {
    if (!s || !get_label || count <= 0 || direction == 0) return 0;
    int cur = (s->cursor >= 0 && s->cursor < count) ? s->cursor : 0;

    const char *cur_lbl = get_label(cur, user);
    char cur_first = (char)toupper((unsigned char)(cur_lbl && cur_lbl[0] ? cur_lbl[0] : '\0'));
    int best = -1;

    if (direction > 0) {
        for (int i = cur + 1; i < count && best < 0; i++) {
            const char *lbl = get_label(i, user);
            if (lbl && lbl[0] && (char)toupper((unsigned char)lbl[0]) != cur_first)
                best = i;
        }
        for (int i = 0; i < cur && best < 0; i++) {
            const char *lbl = get_label(i, user);
            if (lbl && lbl[0] && (char)toupper((unsigned char)lbl[0]) != cur_first)
                best = i;
        }
    } else {
        for (int i = cur - 1; i >= 0 && best < 0; i--) {
            const char *lbl = get_label(i, user);
            if (lbl && lbl[0] && (char)toupper((unsigned char)lbl[0]) != cur_first)
                best = i;
        }
        for (int i = count - 1; i > cur && best < 0; i--) {
            const char *lbl = get_label(i, user);
            if (lbl && lbl[0] && (char)toupper((unsigned char)lbl[0]) != cur_first)
                best = i;
        }
    }

    if (best >= 0 && best != s->cursor) {
        cat_list_state_jump(s, best, count);
        return 1;
    }
    return 0;
}

void cat_draw_list_pane(int x, int y, int w, int h,
                         int item_count, const cat_list_state *state,
                         int item_height, cat_list_item_draw_fn draw_item,
                         void *user) {
    if (!state || item_count <= 0 || item_height <= 0 || !draw_item) return;
    int visible = state->visible_rows;
    int end     = state->scroll_offset + visible;
    if (end > item_count) end = item_count;

    for (int i = state->scroll_offset; i < end; i++) {
        int iy = y + (i - state->scroll_offset) * item_height;
        draw_item(i, x, iy, w, item_height, (i == state->cursor), user);
    }

    if (item_count > visible)
        cat_draw_scrollbar(x + w - CAT_S(4), y, h - CAT_S(4),
                           visible, item_count, state->scroll_offset);
}

void cat_scroll_state_init(cat_scroll_state *s) {
    if (s) { s->offset = 0; s->target = 0; }
}

void cat_scroll_state_move(cat_scroll_state *s, int delta_px) {
    if (!s) return;
    /* Steer the target; cat_draw_scroll_view eases the visible offset toward it. */
    s->target += delta_px;
    if (s->target < 0) s->target = 0;
}

void cat_draw_scroll_view(int x, int y, int w, int h, int content_height,
                          cat_scroll_state *state,
                          cat_scroll_content_fn draw, void *user) {
    if (!state || !draw || w <= 0 || h <= 0) return;

    int max_offset = content_height - h;
    if (max_offset < 0) max_offset = 0;
    if (state->target < 0) state->target = 0;
    if (state->target > max_offset) state->target = max_offset;

    /* Ease the visible offset toward the input-driven target: a quarter of the
       remaining distance each frame (1px floor so it always lands), requesting
       frames until it settles. Snappy but smooth — no hard jumps. */
    if (state->offset != state->target) {
        int diff = state->target - state->offset;
        int step = diff / 4;
        if (step == 0) step = (diff > 0) ? 1 : -1;
        state->offset += step;
        cat_request_frame();
    }
    if (state->offset < 0) state->offset = 0;
    if (state->offset > max_offset) state->offset = max_offset;

    SDL_Renderer *renderer = cat_get_renderer();
    SDL_bool had_clip = SDL_RenderIsClipEnabled(renderer);
    SDL_Rect prev_clip;
    if (had_clip) SDL_RenderGetClipRect(renderer, &prev_clip);

    /* When a scrollbar will be shown, hand the content a width that excludes
       the scrollbar gutter so text never sits underneath the bar. */
    bool show_bar = content_height > h;
    int content_w = show_bar ? w - CAT_S(12) : w;
    if (content_w < 1) content_w = 1;

    SDL_Rect clip = { x, y, w, h };
    SDL_RenderSetClipRect(renderer, &clip);

    draw(x, y - state->offset, content_w, user);

    SDL_RenderSetClipRect(renderer, had_clip ? &prev_clip : NULL);

    if (show_bar)
        cat_draw_scrollbar(x + w - CAT_S(4), y, h, h, content_height, state->offset);
}

#endif /* CAT_WIDGETS_IMPLEMENTATION */
#endif /* CATASTROPHE_WIDGETS_H */
