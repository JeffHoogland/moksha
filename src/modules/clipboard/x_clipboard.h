#ifndef E_MOD_CLIPBOARD_H
#define E_MOD_CLIPBOARD_H

#include "e_mod_config.h"

#define CLIP_CONFIG_MODE(x) (x->clip_copy + 2 * (x->clip_select))
#define CLIP_MAX_MODE            CLIP_SELECTION_SYNC
#define CLIP_MODE_DEFAULT        CLIP_SELECTION_CLIPBOARD
#define CLIP_MODE_NAME_DEFAULT   "CLIP_SELECTION_CLIPBOARD"
#define CLIP_SYNC_DEFAULT        EINA_FALSE
#define CLIP_SYNC_ON             EINA_TRUE
#define CLIP_SYNC_OFF            EINA_FALSE

enum
clip_selection_type { CLIP_SELECTION_NONE = 0,
                      CLIP_SELECTION_CLIPBOARD,
                      CLIP_SELECTION_PRIMARY,
                      CLIP_SELECTION_BOTH,
                      CLIP_SELECTION_SYNC };

typedef struct _Clipboard
{
  unsigned int track_mode;
  const char *track_mode_name;
  Eina_Bool sync_mode;

  Eina_Bool (* const clear)(void);
  void (* const off)(void);
  void (* const request)(Ecore_X_Window w, const char *target);
  Eina_Bool (* const set)(Ecore_X_Window w, const void *data, int size);
  void (* const sync)(const Eina_Bool state);
  Eina_Bool (* const track)(unsigned int);
  Ecore_X_Selection_Data_Text * (* const get_text) (Ecore_X_Event_Selection_Notify *event);
} Clipboard;

extern Clipboard clipboard;                   /* Defined in x_clipboard.c */
extern const char * const Clip_Mode_Names[4]; /* Defined in x_clipboard.c */

void       init_clipboard_struct(Config *config);

/* ---- Images ----
 * Images are only tracked on the CLIPBOARD selection. All replies of the
 * owner of the CLIPBOARD arrive in the same property of our window, so only
 * ONE request may be in flight: TARGETS first, text or image after the answer.
 * The flow is:
 *  1. x_clipboard_request_targets()  asks the owner what it can offer,
 *  2. x_clipboard_targets_parse() looks at the answer,
 *  3. x_clipboard_image_fetch() reads the PNG using xclip (which, unlike
 *     ecore_x, copes with big images sent in several chunks (INCR)),
 *  4. the callback gets the PNG data.
 * To put an image back in the clipboard use x_clipboard_set_image(). */
typedef void (*X_Image_Cb)(const void *data, size_t len);
typedef void (*X_Retry_Cb)(void);

Eina_Bool  x_clipboard_images_tracked(void);
void       x_clipboard_request_targets(Ecore_X_Window w);
void       x_clipboard_targets_parse(Ecore_X_Event_Selection_Notify *event,
                                     Eina_Bool *png, Eina_Bool *text);
void       x_clipboard_request_text(Ecore_X_Window w);
void       x_clipboard_image_fetch(size_t max_bytes, X_Image_Cb on_image, X_Retry_Cb on_retry);
Eina_Bool  x_clipboard_set_image(const char *path);
void       x_clipboard_image_shutdown(void);

#endif
