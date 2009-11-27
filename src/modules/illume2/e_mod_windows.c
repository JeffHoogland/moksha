#include "e.h"
#include "e_mod_windows.h"

/* local function prototypes */
static void *_il_config_windows_create(E_Config_Dialog *cfd);
static void _il_config_windows_free(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_il_config_windows_ui(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static void _il_config_windows_change(void *data, Evas_Object *obj, void *event);
static int _il_config_windows_change_timeout(void *data);

/* local variables */
Ecore_Timer *_windows_change_timer = NULL;

/* public functions */
EAPI void 
il_config_windows_show(E_Container *con, const char *params) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "_config_illume_windows_settings")) return;
   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = _il_config_windows_create;
   v->free_cfdata = _il_config_windows_free;
   v->basic.create_widgets = _il_config_windows_ui;
   v->basic_only = 1;
   v->normal_win = 1;
   v->scroll = 1;
   cfd = e_config_dialog_new(con, _("Window Settings"), "E", 
                             "_config_illume_windows_settings", 
                             "enlightenment/windows", 0, v, NULL);
   e_dialog_resizable_set(cfd->dia, 1);
}

/* local function prototypes */
static void *
_il_config_windows_create(E_Config_Dialog *cfd) 
{
   return NULL;
}

static void 
_il_config_windows_free(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{

}

static Evas_Object *
_il_config_windows_ui(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *list, *of, *ow;

   list = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, _("Home"), 0);
   e_widget_list_object_append(list, of, 1, 0, 0.0);

   of = e_widget_framelist_add(evas, _("Indicator"), 0);
   e_widget_list_object_append(list, of, 1, 0, 0.0);

   of = e_widget_framelist_add(evas, _("Keyboard"), 0);
   e_widget_list_object_append(list, of, 1, 0, 0.0);

   of = e_widget_framelist_add(evas, _("Softkey"), 0);
   e_widget_list_object_append(list, of, 1, 0, 0.0);

   return list;
}

static void 
_il_config_windows_change(void *data, Evas_Object *obj, void *event) 
{
   if (_windows_change_timer) ecore_timer_del(_windows_change_timer);
   _windows_change_timer = 
     ecore_timer_add(0.5, _il_config_windows_change_timeout, data);
}

static int 
_il_config_windows_change_timeout(void *data) 
{
   e_config_save_queue();
   _windows_change_timer = NULL;
   return 0;
}
