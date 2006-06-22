/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* PROTOTYPES - same all the time */
static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static void _load_available_gadgets(void *data);
static void _load_selected_gadgets(void *data);

/* Actual config data we will be playing with whil the dialog is active */
struct _E_Config_Dialog_Data
{
   E_Gadcon *gc;
   char *cname;
   char *ciname;
   Evas_Object *o_add, *o_remove, *o_instances, *o_avail;

   E_Config_Gadcon *cf_gc;
};

/* a nice easy setup function that does the dirty work */
EAPI void
e_int_gadcon_config(E_Gadcon *gc)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   v = E_NEW(E_Config_Dialog_View, 1);
   if (v)
     {
	v->create_cfdata           = _create_data;
	v->free_cfdata             = _free_data;
	v->basic.create_widgets    = _basic_create_widgets;
	v->override_auto_apply = 1;
	
	cfd = e_config_dialog_new(e_container_current_get(e_manager_current_get()),
				  _("Shelf Contents"), NULL, 0, v, gc);
	gc->config_dialog = cfd;
     }
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   Evas_List *l;

   cfdata->cname = NULL;
   cfdata->ciname = NULL;
   cfdata->cf_gc = NULL;

   for (l = e_config->gadcons; l; l = l->next)
     {
	cfdata->cf_gc = l->data;
        if ((!cfdata->cf_gc->name) || (!cfdata->cf_gc->id) ||
	    (!cfdata->gc->name) || (!cfdata->gc->id)) continue;
	if ((!strcmp(cfdata->cf_gc->name, cfdata->gc->name)) &&
	    (!strcmp(cfdata->cf_gc->id, cfdata->gc->id)))
	  {
	     break;
	  }
	cfdata->cf_gc = NULL;
     }
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->gc = cfd->data;
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   cfdata->gc->config_dialog = NULL;
   if (cfdata->cname) free(cfdata->cname);
   if (cfdata->ciname) free(cfdata->ciname);
   free(cfdata);
}

static void
_cb_select_client(void *data)
{
   E_Config_Dialog_Data *cfdata;
   cfdata = data;

   e_widget_disabled_set(cfdata->o_add, 0);
}

static void
_cb_select_client_instance(void *data)
{
   E_Config_Dialog_Data *cfdata;
   cfdata = data;

   e_widget_disabled_set(cfdata->o_remove, 0);
}

static void
_cb_add_instance(void *data, void *data2)
{
   char buf[256];
   int i, ok;
   E_Config_Dialog_Data *cfdata;
   E_Config_Gadcon *cf_gc;
   E_Config_Gadcon_Client *cf_gcc;
   Evas_List *l, *l2;

   cfdata = data;
   if (!cfdata) return;
   
   snprintf(buf, sizeof(buf), "default");
   for (i = 0; ; i++) 
     {
	ok = 1;
	for (l = e_config->gadcons; l; l = l->next) 
	  {
	     cf_gc = l->data;
	     for (l2 = cf_gc->clients; l2; l2 = l2->next)
	       {
		  cf_gcc = l->data;
		  if ((!cf_gcc->name) || (!cf_gcc->id)) continue;
		  if ((!strcmp(cf_gcc->name, cfdata->cname)) && 
		      (!strcmp(cf_gcc->id, buf)))
		    {
		       ok = 0;
		       goto done;
		    }
	       }
	  }
	if (ok) break;
	done:
	snprintf(buf, sizeof(buf), "other-%i", i);
     }

   cf_gcc = E_NEW(E_Config_Gadcon_Client, 1);
   cf_gcc->name = evas_stringshare_add(cfdata->cname);
   cf_gcc->id = evas_stringshare_add(buf);
   cf_gcc->geom.res = 800;
   cf_gcc->geom.size = 80;
   cf_gcc->geom.pos = cf_gcc->geom.res - cf_gcc->geom.size;
   cf_gcc->style = NULL;
   cf_gcc->autoscroll = 0;
   cf_gcc->resizable = 0;
   
   cfdata->cf_gc->clients = evas_list_append(cfdata->cf_gc->clients, cf_gcc);
   
   e_gadcon_unpopulate(cfdata->gc);
   e_gadcon_populate(cfdata->gc);
   e_config_save_queue();
   
   _load_selected_gadgets(cfdata);
   e_widget_ilist_selected_set(cfdata->o_instances,
			       e_widget_ilist_count(cfdata->o_instances) - 1);
   
   e_widget_disabled_set(cfdata->o_add, 1);
}

static void
_cb_remove_instance(void *data, void *data2)
{
   int i;
   E_Config_Dialog_Data *cfdata;
   E_Config_Gadcon_Client *cf_gcc;
   Evas_List *l;

   cfdata = data;
   i = e_widget_ilist_selected_get(cfdata->o_instances);
   l = evas_list_nth_list(cfdata->cf_gc->clients, i);
   cf_gcc = l->data;

   if (cf_gcc->name) evas_stringshare_del(cf_gcc->name);
   if (cf_gcc->id) evas_stringshare_del(cf_gcc->id);
   if (cf_gcc->style) evas_stringshare_del(cf_gcc->style);
   free(cf_gcc);

   cfdata->cf_gc->clients = evas_list_remove_list(cfdata->cf_gc->clients, l);

   _load_selected_gadgets(cfdata);

   e_widget_disabled_set(cfdata->o_remove, 1);
   
   e_gadcon_unpopulate(cfdata->gc);
   e_gadcon_populate(cfdata->gc);
   e_config_save_queue();
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ob, *oi;
   Evas_Coord wmw, wmh;
   Evas_List *l, *l2;

   o = e_widget_list_add(evas, 0, 1);

   of = e_widget_framelist_add(evas, _("Available Gadgets"), 0);

   oi = e_widget_ilist_add(evas, 24, 24, &(cfdata->cname));
   cfdata->o_avail = oi;
   _load_available_gadgets(cfdata);
   e_widget_min_size_get(oi, &wmw, &wmh);
   if (wmw < 200) wmw = 200;
   e_widget_min_size_set(oi, wmw, 250);
   e_widget_framelist_object_append(of, oi);

   ob = e_widget_button_add(evas, _("Add Gadget"), NULL, _cb_add_instance, cfdata, NULL);
   e_widget_disabled_set(ob, 1);
   cfdata->o_add = ob;
   e_widget_framelist_object_append(of, ob);

   e_widget_list_object_append(o, of, 1, 1, 0.5);

   of = e_widget_framelist_add(evas, _("Selected Gadgets"), 0);
   oi = e_widget_ilist_add(evas, 24, 24, &(cfdata->ciname));
   cfdata->o_instances = oi;
   _load_selected_gadgets(cfdata);
   e_widget_min_size_get(oi, &wmw, &wmh);
   if (wmw < 200) wmw = 200;
   e_widget_min_size_set(oi, wmw, 250);
   e_widget_framelist_object_append(of, oi);

   ob = e_widget_button_add(evas, _("Remove Gadget"), NULL, _cb_remove_instance, cfdata, NULL);
   e_widget_disabled_set(ob, 1);
   cfdata->o_remove = ob;
   e_widget_framelist_object_append(of, ob);
   
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   return o;
}

static void 
_load_available_gadgets(void *data) 
{
   E_Config_Dialog_Data *cfdata;
   Evas_Object *oi;
   Evas_List *l;
   Evas *evas;
   
   cfdata = data;
   if (!cfdata) return;
   
   oi = cfdata->o_avail;
   evas = evas_object_evas_get(oi);
   
   e_widget_ilist_clear(oi);
   
   for (l = e_gadcon_provider_list(); l; l = l->next)
     {
	E_Gadcon_Client_Class *cc;
	char *label;
	Evas_Object *icon;

	cc = l->data;
	icon = NULL;
	label = NULL;
	if (cc->func.label) label = cc->func.label();
	if (!label) label = cc->name;
	if (cc->func.icon) icon = cc->func.icon(evas);
	e_widget_ilist_append(oi, icon, label, _cb_select_client, cfdata, cc->name);
     }
   e_widget_ilist_go(oi);   
}

static void 
_load_selected_gadgets(void *data) 
{
   E_Config_Dialog_Data *cfdata;
   Evas_List *l, *l2;
   Evas_Object *oi;
   Evas *evas;
   
   cfdata = data;
   if (!cfdata) return;

   oi = cfdata->o_instances;
   evas = evas_object_evas_get(oi);
   e_widget_ilist_clear(oi);
   
   for (l = cfdata->cf_gc->clients; l; l = l->next)
     {
	E_Config_Gadcon_Client *cf_gcc;
	char *label;
	Evas_Object *icon;

	cf_gcc = l->data;
	for (l2 = e_gadcon_provider_list(); l2; l2 = l2->next)
	  {
	     E_Gadcon_Client_Class *cc;

	     cc = l2->data;
	     label = NULL;
	     icon = NULL;
	     if ((cc->name) && (cf_gcc->name) &&
		 (!strcmp(cc->name, cf_gcc->name))) 
	       {
		  if (cc->func.label) label = cc->func.label();
		  if (!label) label = cc->name;
		  if (cc->func.icon) icon = cc->func.icon(evas);
		  e_widget_ilist_append(oi, icon, label, _cb_select_client_instance,
					cfdata, (char *)cf_gcc->name);
	       }
	  }
     }

   e_widget_ilist_go(oi);   
}
