#include "e.h"

/* local config structure */
struct _E_Config_Dialog_Data 
{

};

/* local function prototypes */
static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

E_Config_Dialog *
e_int_config_acpibindings(E_Container *con, const char *params __UNUSED__) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "keyboard_and_mouse/acpi_bindings")) 
     return NULL;

   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply;
   v->basic.create_widgets = _basic_create;

   cfd = e_config_dialog_new(con, _("ACPI Bindings"), "E", 
			     "keyboard_and_mouse/acpi_bindings", 
			     "preferences-desktop-keyboard-shortcuts", 
			     0, v, NULL);

   return cfd;
}

/* local functions */
static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   return cfdata;
}

static void 
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   E_FREE(cfdata);
}

static int 
_basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   return 1;
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *ol, *of, *ow;

   ol = e_widget_list_add(evas, 0, 1);

   of = e_widget_frametable_add(evas, _("ACPI Bindings"), 0);
   ow = e_widget_ilist_add(evas, (24 * e_scale), (24 * e_scale), NULL);
   e_widget_size_min_set(ow, 200, 200);
   e_widget_frametable_object_append(of, ow, 0, 0, 2, 1, 1, 1, 1, 1);
   e_widget_list_object_append(ol, of, 1, 1, 0.5);

   of = e_widget_framelist_add(evas, _("Action"), 0);
   ow = e_widget_ilist_add(evas, (24 * e_scale), (24 * e_scale), NULL);
   e_widget_size_min_set(ow, 200, 200);
   e_widget_framelist_object_append(of, ow);
   e_widget_list_object_append(ol, of, 1, 1, 0.5);

   return ol;
}
