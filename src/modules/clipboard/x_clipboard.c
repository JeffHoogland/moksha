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

    if (text_data &&
        (text_data->data.content == ECORE_X_SELECTION_CONTENT_TEXT) &&
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

    if (text_data &&
        (text_data->data.content == ECORE_X_SELECTION_CONTENT_TEXT) &&
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

    if (text_data &&
        (text_data->data.content == ECORE_X_SELECTION_CONTENT_TEXT) &&
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

/* ------------------------------------------------------------------------
 *  Images
 * ------------------------------------------------------------------------ */

static Ecore_Exe           *_img_exe      = NULL;
static unsigned char       *_img_buf      = NULL;
static size_t               _img_len      = 0;
static size_t               _img_cap      = 0;
static size_t               _img_max      = 0;
static Eina_Bool            _img_too_big  = EINA_FALSE;
static Eina_Bool            _img_again    = EINA_FALSE;
static X_Image_Cb           _img_on_image = NULL;
static X_Retry_Cb           _img_on_retry = NULL;
static Ecore_Event_Handler *_img_h_data   = NULL;
static Ecore_Event_Handler *_img_h_del    = NULL;

static Eina_Bool _img_cb_data(void *data, int type, void *event);
static Eina_Bool _img_cb_del(void *data, int type, void *event);

Eina_Bool
x_clipboard_images_tracked(void)
{
  return (clipboard.track_mode == CLIP_SELECTION_CLIPBOARD) ||
         (clipboard.track_mode == CLIP_SELECTION_BOTH);
}

/* Ask the owner of the CLIPBOARD which formats it can give us. The answer
 *  arrives as an ECORE_X_EVENT_SELECTION_NOTIFY with target "TARGETS". */
void
x_clipboard_request_targets(Ecore_X_Window w)
{
  if (!x_clipboard_images_tracked()) return;
  ecore_x_selection_clipboard_request(w, "TARGETS");
}

/* What does the owner of the CLIPBOARD offer? */
void
x_clipboard_targets_parse(Ecore_X_Event_Selection_Notify *event,
                          Eina_Bool *png, Eina_Bool *text)
{
  Ecore_X_Selection_Data_Targets *t;
  int i;

  *png = *text = EINA_FALSE;
  if (!event || !event->data) return;
  t = event->data;
  if ((t->data.content != ECORE_X_SELECTION_CONTENT_TARGETS) || !t->targets)
    return;

  for (i = 0; i < t->num_targets; i++)
    {
      if (!t->targets[i]) continue;
      if (!strcmp(t->targets[i], "image/png"))
        *png = EINA_TRUE;
      else if (!strcmp(t->targets[i], ECORE_X_SELECTION_TARGET_UTF8_STRING) ||
               !strcmp(t->targets[i], "text/plain;charset=utf-8"))
        *text = EINA_TRUE;
    }
}

void
x_clipboard_request_text(Ecore_X_Window w)
{
  ecore_x_selection_clipboard_request(w, ECORE_X_SELECTION_TARGET_UTF8_STRING);
}

static void
_img_reset(void)
{
  free(_img_buf);
  _img_buf = NULL;
  _img_len = _img_cap = 0;
  _img_too_big = EINA_FALSE;
}

/* Reads the image from the CLIPBOARD asynchronously. 'on_image' is called
 *  with the PNG data (valid only during the call). If another selection
 *  change arrives while we are busy 'on_retry' is called when we are done. */
void
x_clipboard_image_fetch(size_t max_bytes, X_Image_Cb on_image, X_Retry_Cb on_retry)
{
  static Eina_Bool warned = EINA_FALSE;

  EINA_SAFETY_ON_NULL_RETURN(on_image);

  if (_img_exe)
    {
      _img_again = EINA_TRUE;
      return;
    }
  if (!ecore_file_app_installed("xclip"))
    {
      if (!warned)
        WRN("xclip is not installed: images can not be stored");
      warned = EINA_TRUE;
      return;
    }
  _img_reset();
  _img_max = max_bytes;
  _img_on_image = on_image;
  _img_on_retry = on_retry;

  if (!_img_h_data)
    _img_h_data = ecore_event_handler_add(ECORE_EXE_EVENT_DATA, _img_cb_data, NULL);
  if (!_img_h_del)
    _img_h_del = ecore_event_handler_add(ECORE_EXE_EVENT_DEL, _img_cb_del, NULL);

  _img_exe = ecore_exe_pipe_run("xclip -selection clipboard -t image/png -o",
                                ECORE_EXE_PIPE_READ, NULL);
  if (!_img_exe)
    ERR("Unable to run xclip");
}

static Eina_Bool
_img_cb_data(void *data __UNUSED__, int type __UNUSED__, void *event)
{
  Ecore_Exe_Event_Data *ev = event;
  size_t need;

  if (!ev || !_img_exe || (ev->exe != _img_exe)) return ECORE_CALLBACK_PASS_ON;
  if (_img_too_big || !ev->data || (ev->size <= 0)) return ECORE_CALLBACK_DONE;

  need = _img_len + (size_t) ev->size;
  if (need > _img_max)
    {
      /* Image bigger than configured limit: give up and forget what we have */
      INF("Clipboard image bigger than %u bytes ignored", (unsigned int) _img_max);
      _img_too_big = EINA_TRUE;
      free(_img_buf);
      _img_buf = NULL;
      _img_len = _img_cap = 0;
      ecore_exe_terminate(_img_exe);
      return ECORE_CALLBACK_DONE;
    }
  if (need > _img_cap)
    {
      size_t cap = _img_cap ? _img_cap * 2 : 64 * 1024;
      unsigned char *tmp;

      if (cap < need) cap = need;
      if (cap > _img_max) cap = _img_max;
      tmp = realloc(_img_buf, cap);
      if (!tmp)
        {
          ERR("Out of memory reading clipboard image");
          _img_too_big = EINA_TRUE;
          ecore_exe_terminate(_img_exe);
          return ECORE_CALLBACK_DONE;
        }
      _img_buf = tmp;
      _img_cap = cap;
    }
  memcpy(_img_buf + _img_len, ev->data, ev->size);
  _img_len = need;
  return ECORE_CALLBACK_DONE;
}

static Eina_Bool
_img_cb_del(void *data __UNUSED__, int type __UNUSED__, void *event)
{
  Ecore_Exe_Event_Del *ev = event;
  X_Retry_Cb retry;

  if (!ev || !_img_exe || (ev->exe != _img_exe)) return ECORE_CALLBACK_PASS_ON;

  _img_exe = NULL;
  if (!_img_too_big && (ev->exit_code == 0) && _img_len && _img_buf && _img_on_image)
    _img_on_image(_img_buf, _img_len);
  _img_reset();

  retry = _img_on_retry;
  if (_img_again)
    {
      _img_again = EINA_FALSE;
      if (retry) retry();
    }
  return ECORE_CALLBACK_DONE;
}

void
x_clipboard_image_shutdown(void)
{
  if (_img_h_data) ecore_event_handler_del(_img_h_data);
  if (_img_h_del)  ecore_event_handler_del(_img_h_del);
  _img_h_data = _img_h_del = NULL;
  if (_img_exe) ecore_exe_terminate(_img_exe);
  _img_exe = NULL;
  _img_on_image = NULL;
  _img_on_retry = NULL;
  _img_again = EINA_FALSE;
  _img_reset();
}

/* Quotes a string for /bin/sh */
static char *
_shell_quote(const char *s)
{
  size_t len = 3; /* two quotes and the terminator */
  const char *p;
  char *ret, *q;

  for (p = s; *p; p++)
    len += (*p == '\'') ? 4 : 1;
  ret = malloc(len);
  if (!ret) return NULL;
  q = ret;
  *q++ = '\'';
  for (p = s; *p; p++)
    {
      if (*p == '\'')
        {
          memcpy(q, "'\\''", 4);
          q += 4;
        }
      else
        *q++ = *p;
    }
  *q++ = '\'';
  *q = 0;
  return ret;
}

/* Makes the PNG in 'path' the content of the CLIPBOARD.
 *  xclip forks into the background and keeps serving the image until some
 *  other program takes the selection over. */
Eina_Bool
x_clipboard_set_image(const char *path)
{
  Ecore_Exe *exe;
  char *quoted, *cmd;
  size_t len;

  EINA_SAFETY_ON_NULL_RETURN_VAL(path, EINA_FALSE);

  if (!ecore_file_app_installed("xclip"))
    {
      ERR("xclip is not installed: can not copy images");
      return EINA_FALSE;
    }
  quoted = _shell_quote(path);
  if (!quoted) return EINA_FALSE;
  len = strlen(quoted) + 64;
  cmd = malloc(len);
  if (!cmd)
    {
      free(quoted);
      return EINA_FALSE;
    }
  snprintf(cmd, len, "xclip -selection clipboard -t image/png -i %s", quoted);
  exe = ecore_exe_run(cmd, NULL);
  free(cmd);
  free(quoted);
  return exe != NULL;
}
