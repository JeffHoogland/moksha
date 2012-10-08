#include "e.h"
#include "e_mod_main.h"
#include "e_int_config_randr.h"
#include "e_smart_randr.h"
#include "e_smart_monitor.h"

/* local structures */
struct _E_Config_Dialog_Data
{
   Evas_Object *o_scroll;
};

/* local function prototypes */
static void *_create_data(E_Config_Dialog *cfd __UNUSED__);
static void _fill_data(E_Config_Dialog_Data *cfdata);
static void _free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd  __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata);

/* local variables */

E_Config_Dialog *
e_int_config_randr(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_randr_screen_info.randr_version < ECORE_X_RANDR_1_2) 
     return NULL;

   if (e_config_dialog_find("E", "screen/screen_setup")) return NULL;

   if (!(v = E_NEW(E_Config_Dialog_View, 1))) return NULL;

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;
   v->override_auto_apply = EINA_TRUE;

   cfd = e_config_dialog_new(con, _("Screen Setup"), 
                             "E", "screen/screen_setup", 
                             "preferences-system-screen-setup", 0, v, NULL);

   /* NB: These are just arbitrary values I picked. Feel free to change */
   e_win_size_min_set(cfd->dia->win, 180, 230);
   e_dialog_resizable_set(cfd->dia, 1);

   return cfd;
}

/* local functions */
static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   if (!(cfdata = E_NEW(E_Config_Dialog_Data, 1))) return NULL;
   _fill_data(cfdata);
   return cfdata;
}

static void 
_fill_data(E_Config_Dialog_Data *cfdata)
{

}

static void 
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   E_FREE(cfdata);
}

static int 
_basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   return 1;
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o;
   Eina_List *l;
   E_Randr_Crtc_Info *crtc;

   o = e_widget_list_add(evas, 0, 0);

   cfdata->o_scroll = e_smart_randr_add(evas);
   e_smart_randr_virtual_size_set(cfdata->o_scroll, 
                                  E_RANDR_12->max_size.width,
                                  E_RANDR_12->max_size.height);
   evas_object_show(cfdata->o_scroll);

   /* create monitors based on 'CRTCS' */
   EINA_LIST_FOREACH(E_RANDR_12->crtcs, l, crtc)
     {
        Evas_Object *m;

        if (!crtc) continue;

        /* printf("ADD CRTC %d\n", crtc->xid); */

        if (!(m = e_smart_monitor_add(evas))) continue;
        e_smart_monitor_crtc_set(m, crtc);
        e_smart_randr_monitor_add(cfdata->o_scroll, m);
        evas_object_show(m);
     }

   e_widget_list_object_append(o, cfdata->o_scroll, 1, 1, 0.5);

   return o;
}
