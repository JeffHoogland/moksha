#include <e.h>
#include "e_mod_main.h"
#include "x_clipboard.h"

Eina_Bool  _track(unsigned int mode);
Eina_Bool  _clear(void);
Eina_Bool  _clear_none(void);
Eina_Bool  _clear_both(void);
void       _off(void);
void       _request      (Ecore_X_Window w, const char *target);
void       _request_none (Ecore_X_Window w __UNUSED__, const char *target __UNUSED__);
void       _request_both (Ecore_X_Window w, const char *target);
Eina_Bool  _set          (Ecore_X_Window w, const void *data, int size);
Eina_Bool  _set_clipboard   (Ecore_X_Window w __UNUSED__, const void *data, int size __UNUSED__);
Eina_Bool  _set_primary     (Ecore_X_Window w __UNUSED__, const void *data, int size __UNUSED__);
Eina_Bool  _set_none     (Ecore_X_Window w __UNUSED__, const void *data __UNUSED__, int size __UNUSED__);
Eina_Bool  _set_both     (Ecore_X_Window w, const void *data, int size);
void       _sync(const Eina_Bool state);
Eina_Bool  _track(unsigned int mode);

Ecore_X_Selection_Data_Text *_get_text           (Ecore_X_Event_Selection_Notify *event);
Ecore_X_Selection_Data_Text *_get_text_none      (Ecore_X_Event_Selection_Notify *event __UNUSED__);
Ecore_X_Selection_Data_Text *_get_text_clipboard (Ecore_X_Event_Selection_Notify *event);
Ecore_X_Selection_Data_Text *_get_text_primary   (Ecore_X_Event_Selection_Notify *event);
Ecore_X_Selection_Data_Text *_get_text_both      (Ecore_X_Event_Selection_Notify *event);

const char * const Clip_Mode_Names[] = { "CLIP_SELECTION_NONE",
                                        "CLIP_SELECTION_CLIPBOARD",
                                        "CLIP_SELECTION_PRIMARY",
                                        "CLIP_SELECTION_BOTH" };

Clipboard clipboard = { CLIP_MODE_DEFAULT,
                        CLIP_MODE_NAME_DEFAULT,
                        CLIP_SYNC_DEFAULT,
                        _clear,
                        _off,
                        _request,
                        _set,
                        _sync,
                        _track,
                        _get_text };

extern Mod_Inst *clip_inst;  /* Found in e_mod_main.c */

Eina_Bool (*jmp_table_clear[CLIP_MAX_MODE] )(void) = { _clear_none,
                                                       ecore_x_selection_clipboard_clear,
                                                       ecore_x_selection_primary_clear,
                                                       _clear_both};

void (*jmp_table_request[CLIP_MAX_MODE] )
    (Ecore_X_Window w, const char *target) = { _request_none,
                                               ecore_x_selection_clipboard_request,
                                               ecore_x_selection_primary_request,
                                               _request_both};

Eina_Bool (*jmp_table_set[CLIP_MAX_MODE] )
    (Ecore_X_Window w, const void *data, int size) = { _set_none,
                                                       _set_clipboard,
                                                       _set_primary,
                                                       _set_both};

Ecore_X_Selection_Data_Text *(*jmp_table_get_text[CLIP_MAX_MODE])
    (Ecore_X_Event_Selection_Notify *event) = { _get_text_none,
                                                _get_text_clipboard,
                                                _get_text_primary,
                                                _get_text_both};

Eina_Bool
_clear(void)
{
  return (*jmp_table_clear[clipboard.track_mode]) ();
}

Eina_Bool
_clear_none(void)
{
  return EINA_TRUE;
}

Eina_Bool
_clear_both(void)
{
  return (ecore_x_selection_clipboard_clear() && ecore_x_selection_primary_clear());
}

void
_off(void)
{
  clipboard.track_mode = CLIP_SELECTION_NONE;
}

void
_request(Ecore_X_Window w, const char *target)
{
  (*jmp_table_request[clipboard.track_mode])(w, target);
}

void
_request_none(Ecore_X_Window w __UNUSED__, const char *target __UNUSED__)
{
  return;
}

void
_request_both(Ecore_X_Window w, const char *target)
{
  ecore_x_selection_clipboard_request(w, target);
  ecore_x_selection_primary_request(w, target);
}

Eina_Bool
_set(Ecore_X_Window w, const void *data, int size)
{
  return (*jmp_table_set[clipboard.track_mode])(w, data, size);
}

/* Using xclip to set clipboard contents
 *  as a temporary solution for pasting content to the GTK environment
 *  xclip needs to be installed as dependency
 */
Eina_Bool
_set_clipboard(Ecore_X_Window w __UNUSED__, const void *data, int size __UNUSED__)
{
  e_util_clipboard(w, (const char *) data, ECORE_X_SELECTION_CLIPBOARD);
  return EINA_TRUE;
}

Eina_Bool
_set_primary(Ecore_X_Window w __UNUSED__, const void *data, int size __UNUSED__)
{
  e_util_clipboard(w, (const char *) data, ECORE_X_SELECTION_PRIMARY);
  return EINA_TRUE;
}

Eina_Bool
_set_none(Ecore_X_Window w __UNUSED__, const void *data __UNUSED__, int size __UNUSED__)
{
  return EINA_TRUE;
}

Eina_Bool
_set_both(Ecore_X_Window w, const void *data, int size)
{
  return (_set_clipboard(w, data, size) &&
          _set_primary(w, data, size));
}

void
_sync(const Eina_Bool state)
{
  if (clipboard.track_mode != CLIP_SELECTION_BOTH) {
    ERR("Clipboard sync selected without CLIP_SELECTION_BOTH \n");
  } else
    clipboard.sync_mode = state;
}

Eina_Bool
_track(unsigned int mode)
{
  Eina_Bool ret = EINA_TRUE;
  /* sanity check */
  if (mode >= CLIP_MAX_MODE) {
    ERR("Clipboard tracking mode exceedes CLIP_MAX_MODE \n");
    mode = CLIP_MODE_DEFAULT;
    ret = EINA_FALSE;
  }
  clipboard.track_mode = mode;
  clipboard.track_mode_name = Clip_Mode_Names[mode];
  return ret;
}

Ecore_X_Selection_Data_Text *
_get_text(Ecore_X_Event_Selection_Notify *event) {
  return (*jmp_table_get_text[clipboard.track_mode])(event);
}

Ecore_X_Selection_Data_Text *
_get_text_none(Ecore_X_Event_Selection_Notify *event __UNUSED__){
  return NULL;
}

Ecore_X_Selection_Data_Text *
_get_text_clipboard(Ecore_X_Event_Selection_Notify *event)
{
  Ecore_X_Selection_Data_Text *text_data;

  if ((event->selection == ECORE_X_SELECTION_CLIPBOARD) &&
       (strcmp(event->target, ECORE_X_SELECTION_TARGET_UTF8_STRING) == 0))
  {
    text_data = event->data;

    if ((text_data->data.content == ECORE_X_SELECTION_CONTENT_TEXT) &&
        (text_data->text))
        return text_data;
  }

  return NULL;
}

Ecore_X_Selection_Data_Text *
_get_text_primary(Ecore_X_Event_Selection_Notify *event)
{
  Ecore_X_Selection_Data_Text *text_data;

  if ((event->selection == ECORE_X_SELECTION_PRIMARY) &&
       (strcmp(event->target, ECORE_X_SELECTION_TARGET_UTF8_STRING) == 0))
  {
    text_data = event->data;

    if ((text_data->data.content == ECORE_X_SELECTION_CONTENT_TEXT) &&
        (text_data->text))
      return text_data;
  }

  return NULL;
}

Ecore_X_Selection_Data_Text *
_get_text_both(Ecore_X_Event_Selection_Notify *event)
{
  Ecore_X_Selection_Data_Text *text_data;
  static int count = 0;

  if (((event->selection == ECORE_X_SELECTION_CLIPBOARD) ||
       (event->selection == ECORE_X_SELECTION_PRIMARY)) &&
       (strcmp(event->target, ECORE_X_SELECTION_TARGET_UTF8_STRING) == 0))
  {

    text_data = event->data;

    if ((text_data->data.content == ECORE_X_SELECTION_CONTENT_TEXT) &&
        (text_data->text)) {
      /* If we are syncing clipboard
       *   Ensure both clipboards contain selection. */
      if (clipboard.sync_mode)
      {
        if(count++ < 2)
        {
           if (event->selection == ECORE_X_SELECTION_CLIPBOARD)
             _set_primary(clip_inst->win, text_data->text, strlen(text_data->text) + 1);
           if (event->selection == ECORE_X_SELECTION_PRIMARY)
             _set_clipboard(clip_inst->win, text_data->text, strlen(text_data->text) + 1);
        }
        else count = 0;
      }
      return text_data;
    }
  }

  return NULL;
}

// FIMEME maybe I should return error code here
void
init_clipboard_struct(Config *config) {
  int mode = CLIP_CONFIG_MODE(config);

  if (mode >= CLIP_MAX_MODE) {
    ERR("Module configuration Error: Track Mode %d \n", mode);
    mode = CLIP_MODE_DEFAULT;
    // FIXME reset config here?
  }
  clipboard.track_mode = mode;
  clipboard.track_mode_name = Clip_Mode_Names[mode];
  if (config->sync && (clipboard.track_mode == CLIP_SELECTION_BOTH))
    clipboard.sync_mode = config->sync;
  else
    if (config->sync && (clipboard.track_mode != CLIP_SELECTION_BOTH)) {
      ERR("Module configuration Error: Track Mode %d Sync %d \n", mode, config->sync);
      clipboard.sync_mode = CLIP_SYNC_OFF;
      config->sync = CLIP_SYNC_OFF;
      /* No need to save config will be saved in
       *  _basic_apply_data in e_mod_config.c */
    }
    else
      clipboard.sync_mode = CLIP_SYNC_OFF;
}
