#include "e_mod_main.h"
#include "x_clipboard.h"

#define WIDGET_DISABLED_SET(condition, widget, state) if(condition)  e_widget_disabled_set(ob, state)
struct _E_Config_Dialog_Data
{
  E_Config_Dialog *cfd;
  Evas_Object *obj;

  /* Used to toogle widgets on and off */
  Evas_Object *sync_widget, *autosave_widget , *save_timer_widget, *label_timer_widget;

  /* Store some initial states of clipboard configuration we will need    */
  double init_label_length;  /* Initial label length.                     */
  double init_save_timer;    /* Initial timer settings                    */

  struct {                   /* Structure used to determine whether to    */
    int copy;                /*  toggle Config Dialog Synchronize option  */
    int select;              /*  on and off                               */
    int sync;
  } sync_state;
  struct{
    Eina_Bool state;
    Eina_Bool autostate;
  } save;
  /* Actual options user can change */
  int   clip_copy;      /* Clipboard to use                                */
  int   clip_select;    /* Clipboard to use                                */
  int   sync;           /* Synchronize clipboards flag                     */
  int   persistence;    /* History file persistance                        */
  int   hist_reverse;   /* Order to display History                        */
  double hist_items;    /* Number of history items to store                */
  int   confirm_clear;  /* Display history confirmation dialog on deletion */
  int   autosave;       /* Save History on every selection change          */
  double save_timer;    /* Timer for save history                          */
  double label_length;  /* Number of characters of item to display         */
  int   ignore_ws;      /* Should we ignore White space in label           */
  int   ignore_ws_copy; /* Should we not copy White space only             */
  int   trim_ws;        /* Should we trim White space from selection       */
  int   trim_nl;        /* Should we trim new lines from selection         */
};

static int           _basic_apply_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);
static void         *_create_data(E_Config_Dialog *cfd __UNUSED__);
static int           _basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);
static void          _fill_data(E_Config_Dialog_Data *cfdata);
void                 _free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);
static Evas_Object  *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int           _update_widget(E_Config_Dialog_Data *cfdata);
static Eina_Bool     _sync_state_changed(E_Config_Dialog_Data *cfdata);
extern Mod_Inst     *clip_inst; /* Found in e_mod_main.c */

static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
  E_Config_Dialog_Data *cfdata = E_NEW(E_Config_Dialog_Data, 1);
  _fill_data(cfdata);
  return cfdata;
}

void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
  EINA_SAFETY_ON_NULL_RETURN(clip_cfg);
  clip_cfg->config_dialog = NULL;
  E_FREE(cfdata);
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
  cfdata->sync_state.copy   = clip_cfg->clip_copy;
  cfdata->sync_state.select = clip_cfg->clip_select;
  cfdata->sync_state.sync   = clip_cfg->sync;
  cfdata->init_label_length = clip_cfg->label_length;
  cfdata->init_save_timer = clip_cfg->save_timer;
  cfdata->save.state      = clip_cfg->persistence;
  cfdata->clip_copy       = clip_cfg->clip_copy;
  cfdata->clip_select     = clip_cfg->clip_select;
  cfdata->sync            = clip_cfg->sync;
  cfdata->persistence     = clip_cfg->persistence;
  cfdata->hist_reverse    = clip_cfg->hist_reverse;
  cfdata->hist_items      = clip_cfg->hist_items;
  cfdata->confirm_clear   = clip_cfg->confirm_clear;
  cfdata->autosave        = clip_cfg->autosave;
  cfdata->save.autostate  = clip_cfg->autosave;
  cfdata->save_timer      = clip_cfg->save_timer/60; // Time is in seconds
  cfdata->label_length    = clip_cfg->label_length;
  cfdata->ignore_ws       = clip_cfg->ignore_ws;
  cfdata->ignore_ws_copy  = clip_cfg->ignore_ws_copy;
  cfdata->trim_ws         = clip_cfg->trim_ws;
  cfdata->trim_nl         = clip_cfg->trim_nl;
}

static int
_basic_apply_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
  clip_cfg->clip_copy      = cfdata->clip_copy;
  clip_cfg->clip_select    = cfdata->clip_select;
  clip_cfg->sync           = cfdata->sync;
  clip_cfg->persistence    = cfdata->persistence;
  clip_cfg->hist_reverse   = cfdata->hist_reverse;

  cfdata->save.state = cfdata->persistence;

  /* Do we need to Truncate our history list? */
  if (!EINA_DBL_EQ(clip_cfg->hist_items, cfdata->hist_items))
    truncate_history(cfdata-> hist_items);

  clip_cfg->hist_items     = cfdata->hist_items;
  clip_cfg->confirm_clear  = cfdata->confirm_clear;
  clip_cfg->autosave       = cfdata->autosave;
  if (EINA_DBL_NONZERO(cfdata->save_timer))
    clip_cfg->save_timer   = 60*cfdata->save_timer; // Time in seconds
  else
    clip_cfg->save_timer   = 1;
  /* Do we need to kill or restart save_timer */
  if (cfdata->persistence && !cfdata->autosave)
  {
    if (clip_inst->save_timer)
    {
      clip_save(clip_inst->items, EINA_TRUE); // Save history before start timer */
      clip_inst->save_timer = ecore_timer_loop_add(clip_cfg->save_timer, cb_clipboard_save, NULL);
    }
    else if (!EINA_DBL_EQ(cfdata->save_timer, cfdata->init_save_timer))
    {
      clip_save(clip_inst->items, EINA_TRUE); // Save history before stopping timer */
      ecore_timer_del(clip_inst->save_timer);
      clip_inst->save_timer = ecore_timer_loop_add(clip_cfg->save_timer, cb_clipboard_save, NULL);
    }
  }
  else if (!cfdata->persistence || cfdata->autosave)
    if (clip_inst->save_timer)
    {
      clip_save(clip_inst->items, EINA_TRUE); // Save history before stopping timer */
      ecore_timer_del(clip_inst->save_timer);
      clip_inst->save_timer = NULL;
     }

  /* Has clipboard label name length changed ? */
  if (!EINA_DBL_EQ(cfdata->label_length, cfdata->init_label_length))
  {
    clip_cfg->label_length_changed = EINA_TRUE;
    cfdata->init_label_length = cfdata->label_length;
  }
  clip_cfg->label_length   = cfdata->label_length;

  clip_cfg->ignore_ws      = cfdata->ignore_ws;
  clip_cfg->ignore_ws_copy = cfdata->ignore_ws_copy;
  clip_cfg->trim_ws        = cfdata->trim_ws;
  clip_cfg->trim_nl        = cfdata->trim_nl;

  /* Be sure we set our clipboard 'object'with new configuration */
  init_clipboard_struct(clip_cfg);

  /* and Update our Widget sync state info */
  cfdata->sync_state.copy   = clip_cfg->clip_copy;
  cfdata->sync_state.select = clip_cfg->clip_select;
  cfdata->sync_state.sync   = clip_cfg->sync;

  /* Now save configuration */
  e_config_save_queue();
  return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
  Evas_Object *otb, *o, *ob, *of;

  otb = e_widget_toolbook_add(evas, (24 * e_scale), (24 * e_scale));

  o = e_widget_list_add(evas, 0, 0);
  /* Clipboard Config Section     */
  of = e_widget_framelist_add(evas, _("Selection"), 0);
  ob = e_widget_check_add(evas, _(" Use Copy (Ctrl-C)"), &(cfdata->clip_copy));
  e_widget_framelist_object_append(of, ob);

  ob = e_widget_check_add(evas, _(" Use Primary (Selection)"), &(cfdata->clip_select));
  e_widget_framelist_object_append(of, ob);

  ob = e_widget_check_add(evas, _(" Synchronize Clipboards"), &(cfdata->sync));
  WIDGET_DISABLED_SET(!(cfdata->clip_copy && cfdata->clip_select), ob, EINA_TRUE);
  cfdata->sync_widget = ob;
  e_widget_framelist_object_append(of, ob);
  e_widget_list_object_append(o, of, 1, 0, 1.0);

  /* Content Config Section */
  of = e_widget_framelist_add(evas, _("Content"), 0);
  ob = e_widget_check_add(evas, _(" Ignore Whitespace"), &(cfdata->ignore_ws_copy));
  e_widget_framelist_object_append(of, ob);

  ob = e_widget_check_add(evas, _(" Trim Whitespace"), &(cfdata->trim_ws));
  e_widget_framelist_object_append(of, ob);

  ob = e_widget_check_add(evas, _(" Trim Newlines"), &(cfdata->trim_nl));
  e_widget_framelist_object_append(of, ob);
  e_widget_list_object_append(o, of, 1, 0, 1.0);

  e_widget_toolbook_page_append(otb, NULL, _("Clipboard"), o,
                                 1, 0, 1, 0, 0.0, 0.0);

    /* History Config Section       */
  o = e_widget_list_add(evas, 0, 0);
  of = e_widget_framelist_add(evas, _("Options"), 0);
  ob = e_widget_check_add(evas, _(" Save History"), &(cfdata->persistence));

  e_widget_framelist_object_append(of, ob);
  ob = e_widget_check_add(evas, _(" Confirm before Clearing"), &(cfdata->confirm_clear));
  e_widget_framelist_object_append(of, ob);
  ob = e_widget_check_add(evas, _(" Save every Selection"), &(cfdata->autosave));
  cfdata->autosave_widget = ob;
  WIDGET_DISABLED_SET(!cfdata->save.state, ob, EINA_TRUE);
  e_widget_framelist_object_append(of, ob);
  ob = e_widget_label_add(evas, _(" Timer"));
  cfdata->label_timer_widget = ob;
  WIDGET_DISABLED_SET(!cfdata->save.state || cfdata->autosave, ob, EINA_TRUE);
  e_widget_framelist_object_append(of, ob);
  ob = e_widget_slider_add(evas, 1, 0, "%2.0f", TIMER_MIN, TIMER_MAX, 1.0, 0, &(cfdata->save_timer), NULL, 40);
  cfdata->save_timer_widget = ob;
  WIDGET_DISABLED_SET(!cfdata->save.state || cfdata->autosave, ob, EINA_TRUE);
  e_widget_framelist_object_append(of, ob);
  e_widget_list_object_append(o, of, 1, 1, 0.0);

  /* Label Config Section       */
  of = e_widget_framelist_add(evas, _("Labels"), 0);
  ob = e_widget_check_add(evas, _(" Ignore Whitespace"), &(cfdata->ignore_ws));
  e_widget_framelist_object_append(of, ob);

  ob = e_widget_label_add(evas, _(" Label Length"));
  e_widget_framelist_object_append(of, ob);
  ob = e_widget_slider_add(evas, 1, 0, "%2.0f", LABEL_MIN, LABEL_MAX, 1.0, 0, &(cfdata->label_length), NULL, 40);
  e_widget_framelist_object_append(of, ob);
  e_widget_list_object_append(o, of, 1, 1, 0.0);

  /* Items Config Section */
  of = e_widget_framelist_add(evas, _("Items"), 0);
  ob = e_widget_check_add(evas, _(" Reverse Order"), &(cfdata->hist_reverse));
  e_widget_framelist_object_append(of, ob);

  ob = e_widget_label_add(evas, _(" Items in History"));
  e_widget_framelist_object_append(of, ob);
  ob = e_widget_slider_add(evas, 1, 0, "%2.0f", HIST_MIN, HIST_MAX, 1.0, 0, &(cfdata->hist_items), NULL, 40);
  e_widget_framelist_object_append(of, ob);
  e_widget_list_object_append(o, of, 1, 1, 0.0);

  e_widget_toolbook_page_append(otb, NULL, _("History"), o,
                                 1, 1, 1, 1, 0.5, 0.0);

  e_widget_toolbook_page_show(otb, 0);
  e_dialog_resizable_set(cfd->dia, EINA_TRUE);
  return otb;
}

E_Config_Dialog *
config_clipboard_module(E_Container *con, const char *params __UNUSED__)
{
  E_Config_Dialog *cfd;
  E_Config_Dialog_View *v;

  if(e_config_dialog_find("Clipboard", "extensions/clipboard")) return NULL;

  v = E_NEW(E_Config_Dialog_View, 1);
  if (!v) return NULL;

  v->create_cfdata = _create_data;
  v->free_cfdata = _free_data;
  v->basic.create_widgets = _basic_create_widgets;
  v->basic.apply_cfdata = _basic_apply_data;
  v->basic.check_changed = _basic_check_changed;

  cfd = e_config_dialog_new(con, _("Clipboard Settings"),
            "Clipboard", "extensions/clipboard",
             "edit-paste", 0, v, NULL);
  clip_cfg->config_dialog = cfd;
  return cfd;
}

static int
_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
  if (_sync_state_changed(cfdata) || cfdata->persistence != cfdata->save.state
                                  || cfdata->autosave != cfdata->save.autostate)
    return _update_widget(cfdata);

  if (clip_cfg->clip_copy      != cfdata->clip_copy) return 1;
  if (clip_cfg->clip_select    != cfdata->clip_select) return 1;
  if (clip_cfg->sync           != cfdata->sync) return 1;
  if (clip_cfg->persistence    != cfdata->persistence) return 1;
  if (clip_cfg->hist_reverse   != cfdata->hist_reverse) return 1;
  if (!EINA_DBL_EQ(clip_cfg->hist_items, cfdata->hist_items)) return 1;
  if (clip_cfg->confirm_clear  != cfdata->confirm_clear) return 1;
  if (clip_cfg->autosave       != cfdata->autosave) return 1;
  if (!EINA_DBL_EQ(clip_cfg->save_timer, cfdata->save_timer)) return 1;
  if (!EINA_DBL_EQ(clip_cfg->label_length, cfdata->label_length)) return 1;
  if (clip_cfg->ignore_ws      != cfdata->ignore_ws) return 1;
  if (clip_cfg->ignore_ws_copy != cfdata->ignore_ws_copy) return 1;
  if (clip_cfg->trim_ws        != cfdata->trim_ws) return 1;
  if (clip_cfg->trim_nl        != cfdata->trim_nl) return 1;

  return 0;
}

static Eina_Bool
_sync_state_changed(E_Config_Dialog_Data *cfdata)
{
  if ((cfdata->sync_state.copy   != cfdata->clip_copy) ||
      (cfdata->sync_state.select != cfdata->clip_select) ||
      (cfdata->sync_state.sync   != cfdata->sync)) {
    cfdata->sync_state.copy   = cfdata->clip_copy;
    cfdata->sync_state.select = cfdata->clip_select;
    cfdata->sync_state.sync   = cfdata->sync;
    return EINA_TRUE;
  }
  return EINA_FALSE;
}

static int
_update_widget(E_Config_Dialog_Data *cfdata)
{
  if(cfdata->clip_copy && cfdata->clip_select) {
    e_widget_disabled_set(cfdata->sync_widget, EINA_FALSE);
  } else {
    e_widget_check_checked_set(cfdata->sync_widget, 0);
    e_widget_disabled_set(cfdata->sync_widget, EINA_TRUE);
  }
  if (!cfdata->persistence)
  {
    e_widget_disabled_set(cfdata->autosave_widget, EINA_TRUE);
    cfdata->save.state = cfdata->persistence;
  } else {
    e_widget_disabled_set(cfdata->autosave_widget, EINA_FALSE);
    cfdata->save.state = cfdata->persistence;
  }
  if (!cfdata->autosave && cfdata->save.state)
  {
    e_widget_disabled_set(cfdata->save_timer_widget, EINA_FALSE);
    e_widget_disabled_set(cfdata->label_timer_widget, EINA_FALSE);
  } else if (!cfdata->save.state || cfdata->autosave) {
    e_widget_disabled_set(cfdata->save_timer_widget, EINA_TRUE);
    e_widget_disabled_set(cfdata->label_timer_widget, EINA_TRUE);
  }
  cfdata->save.autostate = cfdata->autosave;
  return 1;
}

Eet_Error
truncate_history(const unsigned int n)
{
  Eet_Error err = EET_ERROR_NONE;

  EINA_SAFETY_ON_NULL_RETURN_VAL(clip_inst, EET_ERROR_BAD_OBJECT);
  clip_inst->update_history = EINA_TRUE;
  if (clip_inst->items) {
    if (eina_list_count(clip_inst->items) > n) {
      Eina_List *last, *discard;
      last = eina_list_nth_list(clip_inst->items, n-1);
      clip_inst->items = eina_list_split_list(clip_inst->items, last, &discard);
      if (discard)
        E_FREE_LIST(discard, free_clip_data);
      err = clip_save(clip_inst->items, EINA_TRUE);
    }
  }
  else
    err = EET_ERROR_EMPTY;
  return err;
}
