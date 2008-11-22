/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

struct _E_Config_Dialog_Data
{
   int   show_label;
   int	 zone_policy;
   int	 desk_policy;
   int   icon_label;
   struct
     {
	Evas_Object *o_desk_show_all;
	Evas_Object *o_desk_show_active;
     } gui;
};

/* Protos */
static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);

static void _cb_zone_policy_change(void *data, Evas_Object *obj);

void 
_config_ibox_module(Config_Item *ci)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   char buf[4096];
   
   v = E_NEW(E_Config_Dialog_View, 1);

   /* Dialog Methods */
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->advanced.apply_cfdata = NULL;
   v->advanced.create_widgets = NULL;
   
   /* Create The Dialog */
   snprintf(buf, sizeof(buf), "%s/e-module-ibox.edj", e_module_dir_get(ibox_config->module));
   cfd = e_config_dialog_new(e_container_current_get(e_manager_current_get()),
			     _("IBox Settings"),
			     "E", "_e_mod_ibox_config_dialog",
			     buf, 0, v, ci);
   ibox_config->config_dialog = eina_list_append(ibox_config->config_dialog, cfd);
}

static void 
_fill_data(Config_Item *ci, E_Config_Dialog_Data *cfdata)
{
   cfdata->show_label = ci->show_label;
   cfdata->zone_policy = ci->show_zone;
   cfdata->desk_policy = ci->show_desk;
   cfdata->icon_label = ci->icon_label;
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;
   Config_Item *ci;
   
   ci = cfd->data;
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(ci, cfdata);
   return cfdata;
}

static void 
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   ibox_config->config_dialog = eina_list_remove(ibox_config->config_dialog, cfd);
   free(cfdata);
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   E_Radio_Group *rg;
   Evas_Object *o, *of, *ob;

   Eina_List *l, *l2;
   int zone_count;

   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, _("General Settings"), 0);
   ob = e_widget_check_add(evas, _("Show Icon Label"), &(cfdata->show_label));
   e_widget_framelist_object_append(of, ob);
   rg = e_widget_radio_group_new(&(cfdata->icon_label));
   ob = e_widget_radio_add(evas, _("Display Name"), 0, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Display Title"), 1, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Display Class"), 2, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Display Icon Name"), 3, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Display Border Caption"), 4, rg);
   e_widget_framelist_object_append(of, ob);
   
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   of = e_widget_framelist_add(evas, _("Screen"), 0);

   zone_count = 0;
   for (l = e_manager_list(); l; l = l->next)
     {
	E_Manager *man;
	man = l->data;

	for (l2 = man->containers; l2; l2 = l2->next)
	  {
	     E_Container *con;

	     con = l2->data;
	     zone_count += eina_list_count(con->zones);
	  }
     }

   if (zone_count <= 1) cfdata->zone_policy = 1;

   rg = e_widget_radio_group_new((int *)&(cfdata->zone_policy));
   ob = e_widget_radio_add(evas, _("Show windows from all screens"), 0, rg);
   e_widget_on_change_hook_set(ob, _cb_zone_policy_change, cfdata);
   e_widget_framelist_object_append(of, ob);
   if (zone_count <= 1) e_widget_disabled_set(ob, 1);

   ob = e_widget_radio_add(evas, _("Show windows from current screen"), 1, rg);
   e_widget_on_change_hook_set(ob, _cb_zone_policy_change, cfdata);
   e_widget_framelist_object_append(of, ob);

   e_widget_list_object_append(o, of, 1, 1, 0.5);

   of = e_widget_framelist_add(evas, _("Desktop"), 0);

   rg = e_widget_radio_group_new((int *)&(cfdata->desk_policy));
   ob = e_widget_radio_add(evas, _("Show windows from all desktops"), 0, rg);
   e_widget_framelist_object_append(of, ob);
   if (cfdata->zone_policy == 0) e_widget_disabled_set(ob, 1);
   cfdata->gui.o_desk_show_all = ob;

   ob = e_widget_radio_add(evas, _("Show windows from active desktop"), 1, rg);
   e_widget_framelist_object_append(of, ob);
   if (cfdata->zone_policy == 0) e_widget_disabled_set(ob, 1);
   cfdata->gui.o_desk_show_active = ob;

   e_widget_list_object_append(o, of, 1, 1, 0.5);

   return o;
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   Config_Item *ci;
   
   ci = cfd->data;
   ci->show_label = cfdata->show_label;
   ci->icon_label = cfdata->icon_label;
   ci->show_zone = cfdata->zone_policy;
   ci->show_desk = cfdata->desk_policy;

   _ibox_config_update(ci);
   e_config_save_queue();
   return 1;
}


/****** callbacks **********/

static void
_cb_zone_policy_change(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;

   if (cfdata->zone_policy == 0)
     {
	e_widget_disabled_set(cfdata->gui.o_desk_show_all, 1);
	e_widget_disabled_set(cfdata->gui.o_desk_show_active, 1);
     }
   else
     {
	e_widget_disabled_set(cfdata->gui.o_desk_show_all, 0);
	e_widget_disabled_set(cfdata->gui.o_desk_show_active, 0);
     }
}
