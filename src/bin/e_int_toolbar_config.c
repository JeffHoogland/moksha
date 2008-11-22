#include "e.h"

/* local function protos */
static void        *_create_data  (E_Config_Dialog *cfd);
static void         _fill_data    (E_Config_Dialog_Data *cfdata);
static void         _free_data    (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int          _basic_apply  (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create (E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

struct _E_Config_Dialog_Data 
{
   E_Toolbar *tbar;
   int orient;
};

EAPI void 
e_int_toolbar_config(E_Toolbar *tbar) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   E_Container *con;

   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return;
   con = e_container_current_get(e_manager_current_get());
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply;
   v->basic.create_widgets = _basic_create;
   cfd = e_config_dialog_new(con, _("Toolbar Settings"), "E", 
			     "_toolbar_config_dialog", "enlightenment/shelf", 
			     0, v, tbar);
   tbar->cfg_dlg = cfd;
}

/* local functions */
static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->tbar = cfd->data;
   _fill_data(cfdata);
   return cfdata;
}

static void 
_fill_data(E_Config_Dialog_Data *cfdata) 
{
   cfdata->orient = cfdata->tbar->gadcon->orient;
}

static void 
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   cfdata->tbar->cfg_dlg = NULL;
   E_FREE(cfdata);
}

static int 
_basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   E_Toolbar *tbar;

   tbar = cfdata->tbar;
   if (!tbar) return 0;
   e_toolbar_orient(tbar, cfdata->orient);
   e_toolbar_position_calc(tbar);
   if ((tbar->fwin) && (tbar->fwin->cb_resize))
     tbar->fwin->cb_resize(tbar->fwin);
   return 1;
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *ot, *ow;
   E_Radio_Group *rg;

   o = e_widget_list_add(evas, 0, 0);
   ot = e_widget_frametable_add(evas, _("Layout"), 1);
   rg = e_widget_radio_group_new(&(cfdata->orient));
   /*
   ow = e_widget_radio_icon_add(evas, NULL, "enlightenment/shelf_position_left", 
				24, 24, E_GADCON_ORIENT_LEFT, rg);
   e_widget_frametable_object_append(ot, ow, 0, 1, 1, 1, 1, 1, 1, 1);
   */
   ow = e_widget_radio_icon_add(evas, NULL, "enlightenment/shelf_position_top", 
				24, 24, E_GADCON_ORIENT_TOP, rg);
   e_widget_frametable_object_append(ot, ow, 0, 0, 1, 1, 1, 1, 1, 1);
   /*
   ow = e_widget_radio_icon_add(evas, NULL, "enlightenment/shelf_position_right", 
				24, 24, E_GADCON_ORIENT_RIGHT, rg);
   e_widget_frametable_object_append(ot, ow, 2, 1, 1, 1, 1, 1, 1, 1);
   */
   ow = e_widget_radio_icon_add(evas, NULL, "enlightenment/shelf_position_bottom", 
				24, 24, E_GADCON_ORIENT_BOTTOM, rg);
   e_widget_frametable_object_append(ot, ow, 0, 1, 1, 1, 1, 1, 1, 1);
   e_widget_list_object_append(o, ot, 1, 1, 0.5);
   return o;
}

