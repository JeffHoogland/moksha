#include "e.h"

static void *_create_data(E_Config_Dialog *cfd);
static void  _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static void _ilist_fill(E_Config_Dialog_Data *cfdata);
static void _ilist_cb_selected(void *data);
static void _cb_add(void *data, void *data2);
static void _cb_delete(void *data, void *data2);
static void _cb_dialog_yes(void *data);
static void _cb_dialog_destroy(void *data);
static void _cb_config(void *data, void *data2);
static void _cb_contents(void *data, void *data2);

struct _E_Config_Dialog_Data 
{
   Evas_Object *o_list;
   Evas_Object *o_delete;
   Evas_Object *o_config;

   const char *cur_shelf;
};

typedef struct _Shelf_Del_Confirm_Data Shelf_Del_Confirm_Data;
struct _Shelf_Del_Confirm_Data
{
    E_Config_Dialog_Data *cfdata;
    E_Shelf *es;
};

E_Config_Dialog *
e_int_config_shelf(E_Container *con, const char *params __UNUSED__) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "extensions/shelves")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return NULL; 
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;

   cfd = e_config_dialog_new(con, _("Shelf Settings"), "E", 
			     "extensions/shelves",
			     "preferences-desktop-shelf", 0, v, NULL);
   return cfd;
}

static void *
_create_data(E_Config_Dialog *cfd __UNUSED__) 
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata) 
{
   E_FREE(cfdata);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *ol, *ow, *ot, *of;

   ol = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, _("Configured Shelves"), 0);
   cfdata->o_list = e_widget_ilist_add(evas, 24, 24, &(cfdata->cur_shelf));
   e_widget_size_min_set(cfdata->o_list, (140 * e_scale), (80 * e_scale));
   e_widget_framelist_object_append(of, cfdata->o_list);
   e_widget_list_object_append(ol, of, 1, 1, 0.5);

   ot = e_widget_table_add(evas, 0);
   ow = e_widget_button_add(evas, _("Add"), "list-add", _cb_add, cfdata, NULL);
   e_widget_table_object_append(ot, ow, 0, 0, 1, 1, 1, 1, 0, 0);
   cfdata->o_delete = e_widget_button_add(evas, _("Delete"), "list-remove", 
					  _cb_delete, cfdata, NULL);
   e_widget_table_object_append(ot, cfdata->o_delete, 1, 0, 1, 1, 1, 1, 0, 0);
   cfdata->o_config = e_widget_button_add(evas, _("Contents"), "preferences-desktop-shelf", 
					  _cb_contents, cfdata, NULL);
   e_widget_table_object_align_append(ot, cfdata->o_config, 
                                      2, 0, 1, 1, 0, 1, 1, 1, 1.0, 0.5);
   cfdata->o_config = e_widget_button_add(evas, _("Setup"), "configure", 
					  _cb_config, cfdata, NULL);
   e_widget_table_object_align_append(ot, cfdata->o_config, 
                                      3, 0, 1, 1, 0, 1, 1, 1, 1.0, 0.5);
   e_widget_list_object_append(ol, ot, 1, 0, 0.0);

   e_widget_disabled_set(cfdata->o_delete, 1);
   e_widget_disabled_set(cfdata->o_config, 1);

   _ilist_fill(cfdata);

   e_dialog_resizable_set(cfd->dia, 1);

   return ol;
}

/* private functions */
static void 
_ilist_fill(E_Config_Dialog_Data *cfdata) 
{
   Evas *evas;
   Eina_List *l;
   int n = -1;
   char buf[256];

   if (!cfdata) return;
   if (!cfdata->o_list) return;

   evas = evas_object_evas_get(cfdata->o_list);

   if (e_widget_ilist_count(cfdata->o_list) > 0) 
     n = e_widget_ilist_selected_get(cfdata->o_list);

   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->o_list);
   e_widget_ilist_clear(cfdata->o_list);
   e_widget_ilist_go(cfdata->o_list);

   for (l = e_shelf_list(); l; l = l->next) 
     {
	E_Shelf *es;
	Evas_Object *ob;

	if (!(es = l->data)) continue;

        snprintf(buf, sizeof(buf), "Shelf %s", e_shelf_orient_string_get(es));

	ob = e_icon_add(evas);
        switch (es->cfg->orient)
          {
           case E_GADCON_ORIENT_LEFT:
             e_util_icon_theme_set(ob, "preferences-position-left");
             break;
           case E_GADCON_ORIENT_RIGHT:
             e_util_icon_theme_set(ob, "preferences-position-right");
             break;
           case E_GADCON_ORIENT_TOP:
             e_util_icon_theme_set(ob, "preferences-position-top");
             break;
           case E_GADCON_ORIENT_BOTTOM:
             e_util_icon_theme_set(ob, "preferences-position-bottom");
             break;
           case E_GADCON_ORIENT_CORNER_TL:
             e_util_icon_theme_set(ob, "preferences-position-top-left");
             break;
           case E_GADCON_ORIENT_CORNER_TR:
             e_util_icon_theme_set(ob, "preferences-position-top-right");
             break;
           case E_GADCON_ORIENT_CORNER_BL:
             e_util_icon_theme_set(ob, "preferences-position-bottom-left");
             break;
           case E_GADCON_ORIENT_CORNER_BR:
             e_util_icon_theme_set(ob, "preferences-position-bottom-right");
             break;
           case E_GADCON_ORIENT_CORNER_LT:
             e_util_icon_theme_set(ob, "preferences-position-left-top");
             break;
           case E_GADCON_ORIENT_CORNER_RT:
             e_util_icon_theme_set(ob, "preferences-position-right-top");
             break;
           case E_GADCON_ORIENT_CORNER_LB:
             e_util_icon_theme_set(ob, "preferences-position-left-bottom");
             break;
           case E_GADCON_ORIENT_CORNER_RB:
             e_util_icon_theme_set(ob, "preferences-position-right-bottom");
             break;
           default:
             e_util_icon_theme_set(ob, "enlightenment");
             break;
          }
	e_widget_ilist_append(cfdata->o_list, ob, buf, 
			      _ilist_cb_selected, cfdata, buf);
     }

   e_widget_size_min_set(cfdata->o_list, 155, 250);
   e_widget_ilist_go(cfdata->o_list);
   e_widget_ilist_thaw(cfdata->o_list);
   edje_thaw();
   evas_event_thaw(evas);

   if (n > -1) 
     {
	e_widget_disabled_set(cfdata->o_delete, 0);
	e_widget_disabled_set(cfdata->o_config, 0);	
	e_widget_ilist_selected_set(cfdata->o_list, n);
     }
   else
     {
	e_widget_disabled_set(cfdata->o_delete, 1);
	e_widget_disabled_set(cfdata->o_config, 1);
     }
}

static void 
_ilist_cb_selected(void *data) 
{
   E_Config_Dialog_Data *cfdata;

   if (!(cfdata = data)) return;
   e_widget_disabled_set(cfdata->o_delete, 0);
   e_widget_disabled_set(cfdata->o_config, 0);
}

static void 
_cb_add(void *data, void *data2 __UNUSED__) 
{
   E_Config_Dialog_Data *cfdata;
   E_Config_Shelf *cfg;
   E_Container *con;
   E_Zone *zone;

   if (!(cfdata = data)) return;

   con = e_container_current_get(e_manager_current_get());
   zone = e_zone_current_get(con);

   cfg = E_NEW(E_Config_Shelf, 1);
   cfg->name = eina_stringshare_add("shelf");
   cfg->container = con->num;
   cfg->zone = zone->id;
   cfg->popup = 1;
   cfg->layer = 200;
   cfg->orient = E_GADCON_ORIENT_CORNER_BR;
   cfg->fit_along = 1;
   cfg->fit_size = 0;
   cfg->style = eina_stringshare_add("default");
   cfg->size = 40;
   cfg->overlap = 0;
   cfg->autohide = 0;
   e_config->shelves = eina_list_append(e_config->shelves, cfg);
   e_config_save_queue();

   e_shelf_config_update();

   _ilist_fill(cfdata);
}

static void 
_cb_delete(void *data, void *data2 __UNUSED__) 
{
   Shelf_Del_Confirm_Data *d;
   char buf[PATH_MAX];

   d = E_NEW(Shelf_Del_Confirm_Data, 1);
   if (!d) return;
   d->cfdata = data;
   if (!d->cfdata) 
     {
        E_FREE(d);
        return;
     }
   if (!d->cfdata->cur_shelf) 
     {
        E_FREE(d);
        return;
     }
   d->es = eina_list_nth(e_shelf_list(), 
			 e_widget_ilist_selected_get(d->cfdata->o_list));
   if (!d->es) 
     {
        E_FREE(d);
        return;
     }
   e_object_ref(E_OBJECT(d->es));

   if (e_config->cnfmdlg_disabled)
     {
        if (e_object_is_del(E_OBJECT(d->es))) return;
        e_shelf_unsave(d->es);
        e_object_del(E_OBJECT(d->es));
        e_config_save_queue();

        e_object_unref(E_OBJECT(d->es));
        _ilist_fill(d->cfdata);
        E_FREE(d);
        return;
     }

   snprintf(buf, sizeof(buf), _("You requested to delete \"%s\".<br><br>"
				"Are you sure you want to delete this shelf?"),
	    d->cfdata->cur_shelf);

   e_confirm_dialog_show(_("Are you sure you want to delete this shelf?"), 
			 "application-exit", buf, NULL, NULL, 
			 _cb_dialog_yes, NULL, d, NULL, _cb_dialog_destroy, d);
}

static void 
_cb_dialog_yes(void *data) 
{
   Shelf_Del_Confirm_Data *d;

   if (!(d = data)) return;
   if (e_object_is_del(E_OBJECT(d->es))) return;
   e_shelf_unsave(d->es);
   e_object_del(E_OBJECT(d->es));
   e_config_save_queue();
}

static void
_cb_dialog_destroy(void *data)
{
   Shelf_Del_Confirm_Data *d;

   d = data;
   e_object_unref(E_OBJECT(d->es));
   _ilist_fill(d->cfdata);
   E_FREE(d);
}

static void 
_cb_config(void *data, void *data2 __UNUSED__) 
{
   E_Config_Dialog_Data *cfdata;
   E_Shelf *es;

   if (!(cfdata = data)) return;
   es = eina_list_nth(e_shelf_list(), 
		      e_widget_ilist_selected_get(cfdata->o_list));
   if (!es) return;
   if (!es->config_dialog) e_int_shelf_config(es);
}

static void 
_cb_contents(void *data, void *data2 __UNUSED__) 
{
   E_Config_Dialog_Data *cfdata;
   E_Shelf *es;

   if (!(cfdata = data)) return;
   es = eina_list_nth(e_shelf_list(), 
		      e_widget_ilist_selected_get(cfdata->o_list));
   if (!es) return;
   if (!es->config_dialog) e_int_gadcon_config_shelf(es->gadcon);
}
