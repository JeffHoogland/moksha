#include "e_int_config_randr.h"
#include "e_widget_toolbook.h"
#include "e.h"
#include "e_randr.h"

/*
 * BUGS:
 * - ethumb sometimes returns garbage objects leading to a segv
 *
 * TODO:
 * - write 1.2 per monitor configuration
 * - write Smart object, so crtcs representations can be properly freed (events,
 *   etc.)
 *
 * IMPROVABLE:
 *  See comments starting with 'IMPROVABLE'
 */
#ifndef  ECORE_X_RANDR_1_2
#define ECORE_X_RANDR_1_2 ((1 << 16) | 2)
#endif
#ifndef  ECORE_X_RANDR_1_3
#define ECORE_X_RANDR_1_3 ((1 << 16) | 3)
#endif

#ifdef  Ecore_X_Randr_None
#undef  Ecore_X_Randr_None
#define Ecore_X_Randr_None  0
#else
#define Ecore_X_Randr_None  0
#endif
#ifdef  Ecore_X_Randr_Unset
#undef  Ecore_X_Randr_Unset
#define Ecore_X_Randr_Unset -1
#else
#define Ecore_X_Randr_Unset -1
#endif

#define THEME_FILENAME      "/e-module-conf_randr.edj"
#define TOOLBAR_ICONSIZE    16

static void        *create_data(E_Config_Dialog *cfd);
static void         free_cfdata(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int          basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int          basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static Eina_Bool    _deferred_noxrandr_error(void *data);

// Functions for the arrangement subdialog interaction
extern Eina_Bool    dialog_subdialog_arrangement_create_data(E_Config_Dialog_Data *cfdata);
extern Evas_Object *dialog_subdialog_arrangement_basic_create_widgets(Evas *canvas);
extern Eina_Bool    dialog_subdialog_arrangement_basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
extern Eina_Bool    dialog_subdialog_arrangement_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
extern void         dialog_subdialog_arrangement_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
extern void         dialog_subdialog_arrangement_keep_changes(E_Config_Dialog_Data *cfdata);
extern void         dialog_subdialog_arrangement_discard_changes(E_Config_Dialog_Data *cfdata);

// Functions for the policies subdialog interaction
extern Eina_Bool    dialog_subdialog_policies_create_data(E_Config_Dialog_Data *cfdata);
extern Evas_Object *dialog_subdialog_policies_basic_create_widgets(Evas *canvas);
extern Eina_Bool    dialog_subdialog_policies_basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
extern Eina_Bool    dialog_subdialog_policies_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
extern void         dialog_subdialog_policies_keep_changes(E_Config_Dialog_Data *cfdata);
extern void         dialog_subdialog_policies_discard_changes(E_Config_Dialog_Data *cfdata);

// Functions for the resolutions subdialog interaction
extern Eina_Bool    dialog_subdialog_resolutions_create_data(E_Config_Dialog_Data *cfdata);
extern Evas_Object *dialog_subdialog_resolutions_basic_create_widgets(Evas *canvas);
extern Eina_Bool    dialog_subdialog_resolutions_basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
extern Eina_Bool    dialog_subdialog_resolutions_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
extern void         dialog_subdialog_resolutions_update_list(Evas *canvas, Evas_Object *crtc);
extern void         dialog_subdialog_resolutions_keep_changes(E_Config_Dialog_Data *cfdata);
extern void         dialog_subdialog_resolutions_discard_changes(E_Config_Dialog_Data *cfdata);

// Functions for the orientation subdialog interaction
extern Eina_Bool    dialog_subdialog_orientation_create_data(E_Config_Dialog_Data *cfdata);
extern Evas_Object *dialog_subdialog_orientation_basic_create_widgets(Evas *canvas);
extern Eina_Bool    dialog_subdialog_orientation_basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
extern Eina_Bool    dialog_subdialog_orientation_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
extern void         dialog_subdialog_orientation_update_radio_buttons(Evas_Object *crtc);
extern void         dialog_subdialog_orientation_update_edje(Evas_Object *crtc);
extern void         dialog_subdialog_orientation_keep_changes(E_Config_Dialog_Data *cfdata);
extern void         dialog_subdialog_orientation_discard_changes(E_Config_Dialog_Data *cfdata);

/* actual module specifics */
E_Config_Dialog_Data *e_config_runtime_info = NULL;
extern E_Module *conf_randr_module;
char _theme_file_path[PATH_MAX];

E_Config_Randr_Dialog_Output_Dialog_Data *
_dialog_output_dialog_data_new(E_Randr_Crtc_Info *crtc_info, E_Randr_Output_Info *output_info)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *dialog_data;

   if ((!crtc_info && !output_info) || !(dialog_data = E_NEW(E_Config_Randr_Dialog_Output_Dialog_Data, 1))) return NULL;
   if (crtc_info)
     {
        //already enabled screen
        dialog_data->crtc = crtc_info;
     }
   else if (output_info)
     {
        //disabled monitor
        dialog_data->output = output_info;
     }
   return dialog_data;

   free(dialog_data);
   return NULL;
}

static void *
create_data(E_Config_Dialog *cfd)
{
   Eina_List *iter;
   E_Randr_Output_Info *output_info;
   E_Config_Randr_Dialog_Output_Dialog_Data *odd;

   // Prove we got all things to get going
   EINA_SAFETY_ON_TRUE_RETURN_VAL(!e_randr_screen_info || (e_randr_screen_info->randr_version < ECORE_X_RANDR_1_2), NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(!(e_config_runtime_info = E_NEW(E_Config_Dialog_Data, 1)), NULL);

   e_config_runtime_info->cfd = cfd;

   //Compose theme's file path and name
   snprintf(_theme_file_path, sizeof(_theme_file_path), "%s%s", conf_randr_module->dir, THEME_FILENAME);

   e_config_runtime_info->manager = e_manager_current_get();
   EINA_LIST_FOREACH(e_randr_screen_info->rrvd_info.randr_info_12->outputs, iter, output_info)
     {
        //Create basic data struct for every connected output.
        //Data would have to be recreated if a monitor is connected while dialog
        //is open.
        if (output_info->connection_status != ECORE_X_RANDR_CONNECTION_STATUS_CONNECTED)
          continue;
        if ((odd = _dialog_output_dialog_data_new(output_info->crtc, output_info)))
          EINA_SAFETY_ON_FALSE_GOTO((e_config_runtime_info->output_dialog_data_list = eina_list_append(e_config_runtime_info->output_dialog_data_list, odd)), _e_conf_randr_create_data_failed_free_data);
     }

   fprintf(stderr, "CONF_RANDR: Added %d output data structs.\n", eina_list_count(e_config_runtime_info->output_dialog_data_list));
   //FIXME: Properly (stack-like) free data when creation fails
   EINA_SAFETY_ON_FALSE_GOTO(dialog_subdialog_arrangement_create_data(e_config_runtime_info), _e_conf_randr_create_data_failed_free_data);
   EINA_SAFETY_ON_FALSE_GOTO(dialog_subdialog_resolutions_create_data(e_config_runtime_info), _e_conf_randr_create_data_failed_free_data);
   EINA_SAFETY_ON_FALSE_GOTO(dialog_subdialog_policies_create_data(e_config_runtime_info), _e_conf_randr_create_data_failed_free_data);
   EINA_SAFETY_ON_FALSE_GOTO(dialog_subdialog_orientation_create_data(e_config_runtime_info), _e_conf_randr_create_data_failed_free_data);

   return e_config_runtime_info;

_e_conf_randr_create_data_failed_free_data:
   free(e_config_runtime_info);
   return NULL;
}

static void
free_cfdata(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   EINA_SAFETY_ON_TRUE_RETURN(!e_randr_screen_info);
   dialog_subdialog_arrangement_free_data(cfd, cfdata);

   free(cfdata);
}

static Eina_Bool
_e_conf_randr_confirmation_dialog_timer_cb(void *data)
{
   E_Config_Randr_Dialog_Confirmation_Dialog_Data *cdd = (E_Config_Randr_Dialog_Confirmation_Dialog_Data *)data;
   char buf[4096];

   if (!cdd) return ECORE_CALLBACK_CANCEL;

   --cdd->countdown;

   if (cdd->countdown > 0)
     {
        snprintf(buf, sizeof(buf),
                 _("Does this look OK? Click <hilight>Keep</hilight> if it does, or Restore if not.<br>"
                   "If you do not press a button, the previous settings will be restored in %d seconds."), cdd->countdown);
     }
   else
     {
        snprintf(buf, sizeof(buf),
                 _("Does this look OK? Click <hilight>Keep</hilight> if it does, or Restore if not.<br>"
                   "If you do not press a button, the previous settings will be restored <highlight>IMMEDIATELY</highlight>."));
     }

   e_dialog_text_set(cdd->dialog, buf);

   return (cdd->countdown > 0) ? ECORE_CALLBACK_RENEW : ECORE_CALLBACK_CANCEL;
}

static void
_e_conf_randr_confirmation_dialog_delete_cb(E_Win *win)
{
   E_Dialog *dia;
   E_Config_Randr_Dialog_Confirmation_Dialog_Data *cd;
   E_Config_Dialog *cfd;

   dia = win->data;
   cd = dia->data;
   cd->cfdata->gui.confirmation_dialog = NULL;
   cfd = cd->cfdata->cfd;
   if (cd->timer) ecore_timer_del(cd->timer);
   cd->timer = NULL;
   free(cd);
   e_object_del(E_OBJECT(dia));
   e_object_unref(E_OBJECT(cfd));
}

static void
_e_conf_randr_confirmation_dialog_keep_cb(void *data, E_Dialog *dia)
{
   E_Config_Randr_Dialog_Confirmation_Dialog_Data *cdd = (E_Config_Randr_Dialog_Confirmation_Dialog_Data *)data;

   if (!cdd) return;

   dialog_subdialog_arrangement_keep_changes(cdd->cfdata);
   dialog_subdialog_orientation_keep_changes(cdd->cfdata);
   dialog_subdialog_policies_keep_changes(cdd->cfdata);
   dialog_subdialog_resolutions_keep_changes(cdd->cfdata);
   _e_conf_randr_confirmation_dialog_delete_cb(dia->win);
}

static void
_e_conf_randr_confirmation_dialog_discard_cb(void *data, E_Dialog *dia)
{
   E_Config_Randr_Dialog_Confirmation_Dialog_Data *cdd = (E_Config_Randr_Dialog_Confirmation_Dialog_Data *)data;

   if (!cdd) return;

   dialog_subdialog_arrangement_discard_changes(cdd->cfdata);
   dialog_subdialog_orientation_discard_changes(cdd->cfdata);
   dialog_subdialog_policies_discard_changes(cdd->cfdata);
   dialog_subdialog_resolutions_discard_changes(cdd->cfdata);
   _e_conf_randr_confirmation_dialog_delete_cb(dia->win);
}

static void
_e_conf_randr_confirmation_dialog_store_cb(void *data, E_Dialog *dia)
{
   E_Config_Randr_Dialog_Confirmation_Dialog_Data *cdd = (E_Config_Randr_Dialog_Confirmation_Dialog_Data *)data;

   if (!cdd) return;

   _e_conf_randr_confirmation_dialog_keep_cb(data, dia);
   e_randr_store_configuration(e_randr_screen_info);
}

static void
_e_conf_randr_confirmation_dialog_new(E_Config_Dialog *cfd)
{
   E_Config_Randr_Dialog_Confirmation_Dialog_Data *cd = E_NEW(E_Config_Randr_Dialog_Confirmation_Dialog_Data, 1);

   if (!cd) return;

   cd->cfd = cfd;

   if ((cd->dialog = e_dialog_new(cfd->con, "E", "e_randr_confirmation_dialog")))
     {
        e_dialog_title_set(cd->dialog, _("New settings confirmation"));
        cd->cfdata = cfd->cfdata;
        cd->timer = ecore_timer_add(1.0, _e_conf_randr_confirmation_dialog_timer_cb, cd);
        cd->countdown = 15;
        cd->dialog->data = cd;
        e_dialog_icon_set(cd->dialog, "preferences-system-screen-resolution", 48);
        e_win_delete_callback_set(cd->dialog->win, _e_conf_randr_confirmation_dialog_delete_cb);
        e_dialog_button_add(cd->dialog, _("Keep"), NULL, _e_conf_randr_confirmation_dialog_keep_cb, cd);
        e_dialog_button_add(cd->dialog, _("Store Permanently"), NULL, _e_conf_randr_confirmation_dialog_store_cb, cd);
        e_dialog_button_add(cd->dialog, _("Restore"), NULL, _e_conf_randr_confirmation_dialog_discard_cb, cd);
        e_dialog_button_focus_num(cd->dialog, 1);
        e_win_centered_set(cd->dialog->win, 1);
        e_win_borderless_set(cd->dialog->win, 1);
        e_win_layer_set(cd->dialog->win, 6);
        e_win_sticky_set(cd->dialog->win, 1);
        e_dialog_show(cd->dialog);
        e_object_ref(E_OBJECT(cfd));
     }
}

static Evas_Object *
basic_create_widgets(E_Config_Dialog *cfd, Evas *canvas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *table = NULL, *wl = NULL;

   EINA_SAFETY_ON_TRUE_RETURN_VAL (!e_randr_screen_info || (e_randr_screen_info->randr_version < ECORE_X_RANDR_1_2), NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL((!canvas || !cfdata), NULL);

   if (!(cfdata->gui.subdialogs.arrangement.dialog = dialog_subdialog_arrangement_basic_create_widgets(canvas))) goto _dialog_create_subdialog_arrangement_fail;
   if (!(cfdata->gui.subdialogs.policies.dialog = dialog_subdialog_policies_basic_create_widgets(canvas))) goto _dialog_create_subdialog_policies_fail;
   if (!(cfdata->gui.subdialogs.resolutions.dialog = dialog_subdialog_resolutions_basic_create_widgets(canvas))) goto _dialog_create_subdialog_resolutions_fail;
   if (!(cfdata->gui.subdialogs.orientation.dialog = dialog_subdialog_orientation_basic_create_widgets(canvas))) goto _dialog_create_subdialog_orientation_fail;

   EINA_SAFETY_ON_FALSE_GOTO((table = e_widget_table_add(canvas, EINA_FALSE)), _dialog_create_widgets_fail);
   EINA_SAFETY_ON_FALSE_GOTO((wl = e_widget_list_add(canvas, EINA_FALSE, EINA_TRUE)), _dialog_create_widget_list_fail);

   //e_widget_table_object_append(Evas_Object *obj, Evas_Object *sobj, int col, int row, int colspan, int rowspan, int fill_w, int fill_h, int expand_w, int expand_h);
   e_widget_table_object_append(table, cfdata->gui.subdialogs.arrangement.dialog, 1, 1, 1, 1, EVAS_HINT_FILL, EVAS_HINT_FILL, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   /*
      e_widget_table_object_append(table, cfdata->gui.subdialogs.policies.dialog, 1, 2, 1, 1, 0, 0, 0, 0);
      e_widget_table_object_append(table, cfdata->gui.subdialogs.orientation.dialog, 2, 2, 1, 1, 0, 0, 0, 0);
      e_widget_table_object_append(table, cfdata->gui.subdialogs.resolutions.dialog, 3, 2, 1, 1, EVAS_HINT_FILL, EVAS_HINT_FILL, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    */
   //e_widget_list_object_append(Evas_Object *obj, Evas_Object *sobj, int fill, int expand, double align);
   e_widget_list_object_append(wl, cfdata->gui.subdialogs.policies.dialog, 0, 0, 0.0);
   e_widget_list_object_append(wl, cfdata->gui.subdialogs.orientation.dialog, 0, 0, 0.0);
   e_widget_list_object_append(wl, cfdata->gui.subdialogs.resolutions.dialog, EVAS_HINT_FILL, EVAS_HINT_EXPAND, 1.0);
   e_widget_table_object_append(table, wl, 1, 2, 1, 1, EVAS_HINT_FILL, EVAS_HINT_FILL, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   cfdata->gui.widget_list = wl;

   cfdata->gui.dialog = table;

   e_dialog_resizable_set(cfd->dia, EINA_TRUE);

   return cfdata->gui.dialog;

_dialog_create_widget_list_fail:
   evas_object_del(table);
_dialog_create_widgets_fail:
   evas_object_del(cfdata->gui.subdialogs.orientation.dialog);
_dialog_create_subdialog_orientation_fail:
   evas_object_del(cfdata->gui.subdialogs.resolutions.dialog);
_dialog_create_subdialog_resolutions_fail:
   evas_object_del(cfdata->gui.subdialogs.policies.dialog);
_dialog_create_subdialog_policies_fail:
   evas_object_del(cfdata->gui.subdialogs.arrangement.dialog);
_dialog_create_subdialog_arrangement_fail:
   return NULL;
}

static int
basic_apply_data
  (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   Eina_Bool ret = EINA_TRUE;

   fprintf(stderr, "CONF_RANDR: New configuration is beeing applied.\n");
   //this is a special case, where the function is called, before the
   //configuration data is created.
   if (!cfdata) return EINA_FALSE;

   //the order matters except for policies!
   if (dialog_subdialog_policies_basic_check_changed(cfd, cfdata))
     {
        ret &= dialog_subdialog_policies_basic_apply_data(cfd, cfdata);
        if (!ret) return EINA_FALSE;
     }

   if (dialog_subdialog_resolutions_basic_check_changed(cfd, cfdata))
     {
        ret &= dialog_subdialog_resolutions_basic_apply_data(cfd, cfdata);
        if (!ret) return EINA_FALSE;
     }

   if (dialog_subdialog_arrangement_basic_check_changed(cfd, cfdata))
     {
        ret &= dialog_subdialog_arrangement_basic_apply_data(cfd, cfdata);
        if (!ret) return EINA_FALSE;
     }

   if (dialog_subdialog_orientation_basic_check_changed(cfd, cfdata))
     ret &= dialog_subdialog_orientation_basic_apply_data(cfd, cfdata);

   _e_conf_randr_confirmation_dialog_new(cfd);

   return ret;
}

E_Config_Dialog *
e_int_config_randr(E_Container *con, const char *params __UNUSED__){
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (!e_randr_screen_info || (e_randr_screen_info->randr_version < ECORE_X_RANDR_1_2))
     {
        ecore_timer_add(0.5, _deferred_noxrandr_error, NULL);
        fprintf(stderr, "CONF_RANDR: XRandR version >= 1.2 necessary to work.\n");
        return NULL;
     }

   //Dialog already opened?
   if (e_config_dialog_find("E", "screen/screen_setup")) return NULL;

   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = create_data;
   v->free_cfdata = free_cfdata;
   v->basic.apply_cfdata = basic_apply_data;
   v->basic.create_widgets = basic_create_widgets;
   v->basic.check_changed = basic_check_changed;
   //v->override_auto_apply = 0;

   cfd = e_config_dialog_new(con, _("Screen Setup"),
                             "E", "screen/screen_setup",
                             "preferences-system-screen-setup", 0, v, NULL);
   return cfd;
}

static Eina_Bool
_deferred_noxrandr_error(void *data __UNUSED__)
{
   e_util_dialog_show(_("Missing Features"),
                      _("Your X Display Server is missing support for<br>"
                        "the <hilight>XRandR</hilight> (X Resize and Rotate) extension version 1.2 or above.<br>"
                        "You cannot change screen resolutions without<br>"
                        "the support of this extension. It could also be<br>"
                        "that at the time <hilight>ecore</hilight> was built, there<br>"
                        "was no XRandR support detected."));
   return ECORE_CALLBACK_CANCEL;
}

static int
basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   if (!cfdata) return EINA_FALSE;
   else
     return dialog_subdialog_arrangement_basic_check_changed(cfd, cfdata)
            || dialog_subdialog_policies_basic_check_changed(cfd, cfdata)
            || dialog_subdialog_orientation_basic_check_changed(cfd, cfdata)
            || dialog_subdialog_resolutions_basic_check_changed(cfd, cfdata);
}

