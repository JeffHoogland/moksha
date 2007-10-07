#include "e.h"

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static void _cb_config(void *data, void *data2);
static int _cb_bg_change(void *data, int type, void *event);

struct _E_Config_Dialog_Data 
{
   int con_num;
   int zone_num;
   int desk_x;
   int desk_y;
   char *bg;
   char *name;
   
   Evas_Object *preview;
   Ecore_Event_Handler *hdl;
};

EAPI E_Config_Dialog *
e_int_config_desk(E_Container *con, const char *params)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   E_Config_Dialog_Data *cfdata;
   int con_num, zone_num, dx, dy;

   if (!params) return NULL;
   con_num = zone_num = dx = dy = -1;
   if (sscanf(params, "%i %i %i %i", &con_num, &zone_num, &dx, &dy) != 4)
     return NULL;
   
   if (e_config_dialog_find("E", "_config_desk_dialog")) return NULL;

   v = E_NEW(E_Config_Dialog_View, 1);
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->con_num = con_num;
   cfdata->zone_num = zone_num;
   cfdata->desk_x = dx;
   cfdata->desk_y = dy;
   
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply;
   v->basic.create_widgets = _basic_create;
   v->override_auto_apply = 1;
   
   cfd = e_config_dialog_new(con, _("Desk Settings"), "E", "_config_desk_dialog",
			     "enlightenment/desktops", 0, v, cfdata);
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata) 
{
   Evas_List *l;
   const char *bg;
   char name[40];
   int ok = 0;
   
   bg = e_bg_file_get(cfdata->con_num, cfdata->zone_num, cfdata->desk_x, cfdata->desk_y);
   if (!bg)
     bg = e_theme_edje_file_get("base/theme/backgrounds", "e/desktop/background");
   cfdata->bg = strdup(bg);
   
   for (l = e_config->desktop_names; l; l = l->next) 
     {
	E_Config_Desktop_Name *dn;
	
	dn = l->data;
	if (!dn) continue;
	if (dn->container != cfdata->con_num) continue;
	if (dn->zone != cfdata->zone_num) continue;
	if ((dn->desk_x != cfdata->desk_x) || (dn->desk_y != cfdata->desk_y)) 
	  continue;
	
	if (dn->name)
	  cfdata->name = strdup(dn->name);
	ok = 1;
	break;
     }
   if (!ok)
     {     
	snprintf(name, sizeof(name), _(e_config->desktop_default_name), cfdata->desk_x, cfdata->desk_y);
	cfdata->name = strdup(name);
     }
}

static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = cfd->data;
   if (!cfdata) return NULL;
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   if (cfdata->hdl)
     ecore_event_handler_del(cfdata->hdl);
   if (cfdata->bg)
     E_FREE(cfdata->bg);
   if (cfdata->name)
     E_FREE(cfdata->name);
   E_FREE(cfdata);
}

static int
_basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   char name[40];
   
   if (!cfdata->name[0]) 
     {
	snprintf(name, sizeof(name), _(e_config->desktop_default_name), cfdata->desk_x, cfdata->desk_y);
	cfdata->name = strdup(name);
     }

   e_desk_name_del(cfdata->con_num, cfdata->zone_num, cfdata->desk_x, cfdata->desk_y);
   e_desk_name_add(cfdata->con_num, cfdata->zone_num, cfdata->desk_x, cfdata->desk_y, cfdata->name);
   e_desk_name_update();
   
   e_bg_del(cfdata->con_num, cfdata->zone_num, cfdata->desk_x, cfdata->desk_y);
   e_bg_add(cfdata->con_num, cfdata->zone_num, cfdata->desk_x, cfdata->desk_y, cfdata->bg);
   e_bg_update();

   e_config_save_queue();
   return 1;
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *of, *ob;
   E_Zone *zone;
   
   zone = e_zone_current_get(cfd->con);
   
   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_frametable_add(evas, _("Desktop Name"), 0);
   ob = e_widget_label_add(evas, _("Name:"));
   e_widget_frametable_object_append(of, ob, 0, 0, 1, 1, 1, 1, 1, 0);
   ob = e_widget_entry_add(evas, &(cfdata->name), NULL, NULL, NULL);
   e_widget_frametable_object_append(of, ob, 1, 0, 2, 1, 1, 1, 1, 0);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   of = e_widget_frametable_add(evas, _("Desktop Wallpaper"), 0);
   ob = e_widget_preview_add(evas, 240, (240 * zone->h) / zone->w);
   cfdata->preview = ob;
   if (cfdata->bg)
     e_widget_preview_edje_set(ob, cfdata->bg, "e/desktop/background");
   e_widget_frametable_object_append(of, ob, 0, 0, 3, 1, 1, 1, 1, 0);
   ob = e_widget_button_add(evas, _("Configure"), "widget/config", 
			    _cb_config, cfdata, NULL);
   e_widget_frametable_object_append(of, ob, 1, 1, 1, 1, 1, 1, 1, 0);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   if (cfdata->hdl)
     ecore_event_handler_del(cfdata->hdl);
   cfdata->hdl = ecore_event_handler_add(E_EVENT_BG_UPDATE, _cb_bg_change, cfdata);
   
   return o;
}

static void 
_cb_config(void *data, void *data2) 
{
   E_Config_Dialog_Data *cfdata;
   char buf[256];
   
   cfdata = data;
   if (!cfdata) return;
   snprintf(buf, sizeof(buf), "%i %i %i %i", 
	    cfdata->con_num, cfdata->zone_num, cfdata->desk_x, cfdata->desk_y);
   e_configure_registry_call("internal/wallpaper_desk", e_container_current_get(e_manager_current_get()), buf);
}

static int
_cb_bg_change(void *data, int type, void *event) 
{
   E_Event_Bg_Update *ev;
   E_Config_Dialog_Data *cfdata;
   const char *file;
   
   if (type != E_EVENT_BG_UPDATE) return 1;
   
   cfdata = data;
   ev = event;
   if (ev->container != cfdata->con_num) return 1;
   if (ev->zone != cfdata->zone_num) return 1;
   if (ev->desk_x != cfdata->desk_x) return 1;
   if (ev->desk_y != cfdata->desk_y) return 1;
   
   file = e_bg_file_get(cfdata->con_num, cfdata->zone_num, cfdata->desk_x, cfdata->desk_y);
   E_FREE(cfdata->bg);
   cfdata->bg = strdup(file);
   e_widget_preview_edje_set(cfdata->preview, cfdata->bg, "e/desktop/background");

   return 1;
}
