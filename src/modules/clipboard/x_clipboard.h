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

#endif

