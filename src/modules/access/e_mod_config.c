#include "e.h"
#include "e_mod_main.h"

struct _E_Config_Dialog_Data
{
   int dummy;
};

/* local function prototypes */
static void        *_create_data (E_Config_Dialog *cfd);
static void         _fill_data   (E_Config_Dialog_Data *cfdata);
static void         _free_data   (E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata);
static int          _basic_apply (E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);

void 
_config_pager_module(void)
{
   E_Config_Dialog_View *v;
   E_Container *con;

   if (e_config_dialog_find("E", "_e_mod_access_config_dialog"))
     return;
   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return;

   v->create_cfdata =        _create_data;
   v->free_cfdata =          _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata =   _basic_apply;

   con = e_container_current_get(e_manager_current_get());
   e_config_dialog_new(con, _("Access Settings"), "E", 
                       "_e_mod_access_config_dialog",
                       "preferences-desktop-access", 0, v, NULL);
}

/* local function prototypes */
static void *
_create_data(E_Config_Dialog *cfd __UNUSED__) 
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfdata);
   return cfdata;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   cfdata->dummy = 1;
}

static void 
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata) 
{
   E_FREE(cfdata);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata __UNUSED__) 
{
   Evas_Object *ol, *of;

   ol = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, _("General"), 0);
   e_widget_list_object_append(ol, of, 1, 0, 0.5);

   return ol;
}

static int 
_basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata __UNUSED__) 
{
   e_config_save_queue();
   return 1;
}
