/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* PROTOTYPES - same all the time */
static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

/* Actual config data we will be playing with whil the dialog is active */
struct _E_Config_Dialog_Data
{
   E_Gadcon *gc;
   char *cname;
   char *ciname;
   int enabled;
   Evas_Object *o_enabled, *o_disabled;
   Evas_Object *o_add, *o_remove, *o_instances;

   Evas_List *cf_gcc;
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
	/* methods */
	v->create_cfdata           = _create_data;
	v->free_cfdata             = _free_data;
	v->basic.apply_cfdata      = _basic_apply_data;
	v->basic.create_widgets    = _basic_create_widgets;
	v->override_auto_apply = 1;
	
	/* create config dialog for bd object/data */
	cfd = e_config_dialog_new(e_container_current_get(e_manager_current_get()),
				  _("Contents Settings"), NULL, 0, v, gc);
	gc->config_dialog = cfd;
     }
}

/**--CREATE--**/
static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   int ok;
   E_Config_Gadcon   *cf_gc;
   E_Config_Gadcon_Client *cf_gcc, *cf_gcc2;
   Evas_List *l;

   cfdata->cname = NULL;
   cfdata->ciname = NULL;
   cfdata->cf_gcc = NULL;

   ok = 0;
   for (l = e_config->gadcons; l; l = l->next)
     {
	cf_gc = l->data;
	if ((!strcmp(cf_gc->name, cfdata->gc->name)) &&
	    (!strcmp(cf_gc->id, cfdata->gc->id)))
	  {
	     ok = 1;
	     break;
	  }
     }
   if (ok)
     {
	for (l = cf_gc->clients; l; l = l->next)
	  {
	     cf_gcc = l->data;
	     if (!cf_gcc->name) continue;

	     cf_gcc2 = E_NEW(E_Config_Gadcon_Client, 1);
	     cf_gcc2->name = evas_stringshare_add(cf_gcc->name);
	     cf_gcc2->id = evas_stringshare_add(cf_gcc->id);
	     cf_gcc2->geom.res = cf_gcc->geom.res;
	     cf_gcc2->geom.size = cf_gcc->geom.size;
	     cf_gcc2->geom.pos = cf_gcc->geom.pos;
	     cf_gcc2->style = !cf_gcc->style ? NULL : evas_stringshare_add(cf_gcc->style);
	     cf_gcc2->autoscroll = cf_gcc->autoscroll;
	     cf_gcc2->resizable = cf_gcc->resizable;

	     cfdata->cf_gcc = evas_list_append(cfdata->cf_gcc, cf_gcc2);
	  }
     }
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   /* Create cfdata - cfdata is a temporary block of config data that this
    * dialog will be dealing with while configuring. it will be applied to
    * the running systems/config in the apply methods
    */
   E_Config_Dialog_Data *cfdata;
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->gc = cfd->data;
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   Evas_List *l;
   /* Free the cfdata */
   cfdata->gc->config_dialog = NULL;

   while (cfdata->cf_gcc)
     {
	E_Config_Gadcon_Client *cf_gcc = cfdata->cf_gcc->data;

	if (cf_gcc->name) evas_stringshare_del(cf_gcc->name);
	if (cf_gcc->id) evas_stringshare_del(cf_gcc->id);
	if (cf_gcc->style) evas_stringshare_del(cf_gcc->style);
	free(cf_gcc);

	cfdata->cf_gcc = evas_list_remove_list(cfdata->cf_gcc, cfdata->cf_gcc);
     }


   if (cfdata->cname) free(cfdata->cname);
   if (cfdata->ciname) free(cfdata->ciname);
   free(cfdata);
}

/**--APPLY--**/
static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   int ok = 0;
   E_Config_Gadcon *cf_gc;
   E_Config_Gadcon_Client *cf_gcc;
   Evas_List *l;

   Evas_List *new_clients = NULL;


   for (l = e_config->gadcons; l; l = l->next)
     {
	cf_gc = l->data;
	if ((!strcmp(cf_gc->name, cfdata->gc->name)) &&
	    (!strcmp(cf_gc->id, cfdata->gc->id)))
	  {
	     ok = 1;
	     break;
	  }
     }
   if (!ok) return 1;

   //FIXME: some how the settings of the gadcon should be updated before
   //saving it.
   while (cf_gc->clients)
     {
	cf_gcc = cf_gc->clients->data;

	if (!cf_gcc->name) 
	  new_clients = evas_list_append(new_clients, cf_gcc);
	else
	  { 
	     if (cf_gcc->name) evas_stringshare_del(cf_gcc->name); 
	     if (cf_gcc->id) evas_stringshare_del(cf_gcc->id); 
	     if (cf_gcc->style) evas_stringshare_del(cf_gcc->style); 
	     free(cf_gcc);
	  } 
	cf_gc->clients = evas_list_remove_list(cf_gc->clients, cf_gc->clients);
     }
   cf_gc->clients = new_clients;

   for (l = cfdata->cf_gcc; l; l = l->next)
     {
	E_Config_Gadcon_Client *cf_gcc2 = l->data;

	cf_gcc = E_NEW(E_Config_Gadcon_Client, 1); 
	cf_gcc->name	   = evas_stringshare_add(cf_gcc2->name);
	cf_gcc->id	   = evas_stringshare_add(cf_gcc2->id);
	cf_gcc->geom.res   = cf_gcc2->geom.res;
	cf_gcc->geom.size  = cf_gcc2->geom.size;
	cf_gcc->geom.pos   = cf_gcc2->geom.pos;
	cf_gcc->style	   = !cf_gcc2->style ? NULL : evas_stringshare_add(cf_gcc2->style);
	cf_gcc->autoscroll = cf_gcc2->autoscroll;
	cf_gcc->resizable  = cf_gcc2->resizable;

	cf_gc->clients = evas_list_append(cf_gc->clients, cf_gcc);
     }

   e_gadcon_unpopulate(cfdata->gc);
   e_gadcon_populate(cfdata->gc);
   e_config_save_queue();
   return 1;
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
   char buf[256], buf2[256];
   int i, ok;
   E_Config_Dialog_Data *cfdata;
   E_Config_Gadcon *cf_gc;
   E_Config_Gadcon_Client *cf_gcc;
   Evas_List *l, *l2;

   cfdata = data;

   snprintf(buf, sizeof(buf), "default");
   //FIXME: I DO NOT LIKE THIS LOOP. IT CAN NOT END. FOR EXAMPLE IBAR ON SHELF 0
   for (i = 0; ; i++)
     {
	ok = 1;
	for (l = e_config->gadcons; l; l = l->next)
	  {
	     cf_gc = l->data;
	     for (l2 = cf_gc->clients; l2; l2 = l2->next)
	       {
		  cf_gcc = l2->data;
		  if ((!cf_gcc->name) || (!cf_gcc->id)) continue;
		  if ((!strcmp(cf_gcc->name, cfdata->cname)) && (!strcmp(cf_gcc->id, buf)))
		    {
		       ok = 0;
		       break;
		    }
	       }
	     if (!ok) break;
	  }
	if (ok) break;
	snprintf(buf, sizeof(buf), "other-%i", ok);
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

   cfdata->cf_gcc = evas_list_append(cfdata->cf_gcc, cf_gcc);

   e_widget_ilist_append(cfdata->o_instances, NULL, cf_gcc->name, _cb_select_client_instance,
			 cfdata, (char *)cf_gcc->name);
   e_widget_ilist_go(cfdata->o_instances);
   e_widget_ilist_selected_set(cfdata->o_instances,
			       e_widget_ilist_count(cfdata->o_instances) - 1);
}

static void
_cb_remove_instance(void *data, void *data2)
{
   int i;
   E_Config_Dialog_Data *cfdata;
   E_Config_Gadcon_Client *cf_gcc;
   Evas_List   *l;

   cfdata = data;
   i = e_widget_ilist_selected_get(cfdata->o_instances);

   l = evas_list_nth_list(cfdata->cf_gcc, i);
   cf_gcc = l->data;

   if (cf_gcc->name) evas_stringshare_del(cf_gcc->name);
   if (cf_gcc->id) evas_stringshare_del(cf_gcc->id);
   if (cf_gcc->style) evas_stringshare_del(cf_gcc->style);
   free(cf_gcc);

   cfdata->cf_gcc = evas_list_remove_list(cfdata->cf_gcc, l);

   e_widget_ilist_clear(cfdata->o_instances);
   for (l = cfdata->cf_gcc; l; l = l->next)
     {
	cf_gcc = l->data;
	e_widget_ilist_append(cfdata->o_instances, NULL, cf_gcc->name, _cb_select_client_instance,
			      cfdata, (char *)cf_gcc->name);
     }
   e_widget_ilist_go(cfdata->o_instances);

   if (i >= e_widget_ilist_count(cfdata->o_instances))
     i = e_widget_ilist_count(cfdata->o_instances) - 1;

   if (!e_widget_ilist_count(cfdata->o_instances)) 
     e_widget_disabled_set(cfdata->o_remove, 1);
   else
     e_widget_ilist_selected_set(cfdata->o_instances, i);
}

/**--GUI--**/
static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   /* generate the core widget layout for a basic dialog */
   Evas_Object *o, *o2, *of, *oft, *ob, *oi;
   Evas_Coord wmw, wmh;
   Evas_List *l;
   E_Config_Gadcon_Client *cf_gcc;
   //int ok;

   /* FIXME: this is just raw config now - it needs UI improvments */
   o = e_widget_list_add(evas, 0, 1);

   of = e_widget_framelist_add(evas, _("Available Items"), 0);

   oi = e_widget_ilist_add(evas, 24, 24, &(cfdata->cname));

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

   e_widget_min_size_get(oi, &wmw, &wmh);
   if (wmw < 200) wmw = 200;
   e_widget_min_size_set(oi, wmw, 250);

   e_widget_framelist_object_append(of, oi);

   e_widget_list_object_append(o, of, 1, 1, 0.5);

   oft = e_widget_frametable_add(evas, _("Selected Items"), 0);

   oi = e_widget_ilist_add(evas, 24, 24, &(cfdata->ciname));

   for (l = cfdata->cf_gcc; l; l = l->next)
     {
	cf_gcc = l->data;
	e_widget_ilist_append(oi, NULL, cf_gcc->name, _cb_select_client_instance,
			      cfdata, (char *)cf_gcc->name);
     }

   e_widget_ilist_go(oi);

   e_widget_min_size_get(oi, &wmw, &wmh);
   if (wmw < 200) wmw = 200;
   if (wmh < 190) wmh = 190;
   e_widget_min_size_set(oi, wmw, wmh);

   e_widget_frametable_object_append(oft, oi, 0, 0, 1, 1, 1, 1, 1, 1);
   cfdata->o_instances = oi;

   ob = e_widget_button_add(evas, _("Add Instance"), NULL, _cb_add_instance, cfdata, NULL);
   e_widget_frametable_object_append(oft, ob, 0, 1, 1, 1, 1, 1, 1, 1);
   e_widget_disabled_set(ob, 1);
   cfdata->o_add = ob;

   ob = e_widget_button_add(evas, _("Remove Instance"), NULL, _cb_remove_instance, cfdata, NULL);
   e_widget_frametable_object_append(oft, ob, 0, 2, 1, 1, 1, 1, 1, 1);
   e_widget_disabled_set(ob, 1);
   cfdata->o_remove = ob;


   e_widget_list_object_append(o, oft, 1, 1, 0.5);

   return o;
}

/************* raster original code ********************/
#if 0
static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   Evas_List *l, *l2;
   E_Config_Gadcon *cf_gc, *cf_gc2;
   E_Config_Gadcon_Client *cf_gcc, *cf_gcc2;
   int i, ok = 0;
   char buf[256];

   cfdata->gc->config_dialog = cfd;
   for (l = e_config->gadcons; l; l = l->next)
     {
	cf_gc = l->data;
	if ((!strcmp(cf_gc->name, cfdata->gc->name)) &&
	    (!strcmp(cf_gc->id, cfdata->gc->id)))
	  {
	     ok = 1;
	     break;
	  }
     }
   if (!ok) return 1;
   for (l = cf_gc->clients; l; l = l->next)
     {
	cf_gcc = l->data;
	if (!cf_gcc->name) continue;
	if (!strcmp(cf_gcc->name, cfdata->cname))
	  {
	     if (!cfdata->enabled)
	       {
		  /* remove from list */
		  cf_gc->clients = evas_list_remove_list(cf_gc->clients, l);
		  if (cf_gcc->name) evas_stringshare_del(cf_gcc->name);
		  if (cf_gcc->id) evas_stringshare_del(cf_gcc->id);
		  if (cf_gcc->style) evas_stringshare_del(cf_gcc->style);
		  free(cf_gcc);
		  goto savedone;
	       }
	     return 1; /* Apply was OK */
	  }
     }
   snprintf(buf, sizeof(buf), "default");
   for (i = 0; ; i++)
     {
	ok = 1;
	for (l = e_config->gadcons; l; l = l->next)
	  {
	     cf_gc2 = l->data;
	     for (l2 = cf_gc2->clients; l2; l2 = l2->next)
	       {
		  cf_gcc2 = l2->data;
		  if ((!cf_gcc2->name) || (!cf_gcc2->id)) continue;
		  if ((!strcmp(cf_gcc2->name, cfdata->cname)) && (!strcmp(cf_gcc2->id, buf)))
		    {
		       ok = 0;
		       break;
		    }
	       }
	     if (!ok) break;
	  }
	if (ok) break;
	snprintf(buf, sizeof(buf), "other-%i", ok);
     }
   cf_gcc = E_NEW(E_Config_Gadcon_Client, 1);
   cf_gcc->name = evas_stringshare_add(cfdata->cname);
   cf_gcc->id = evas_stringshare_add(buf);
   cf_gcc->geom.res = 800;
   cf_gcc->geom.size = 80;
   cf_gcc->geom.pos = cf_gcc->geom.res - cf_gcc->geom.size;
   cf_gcc->autoscroll = 0;
   cf_gcc->resizable = 0;
   cf_gc->clients = evas_list_append(cf_gc->clients, cf_gcc);
   savedone:
   e_gadcon_unpopulate(cfdata->gc);
   e_gadcon_populate(cfdata->gc);
   e_config_save_queue();
   return 1; /* Apply was OK */
}
static void
_cb_select(void *data)
{
   E_Config_Dialog_Data *cfdata;
   Evas_List *l;
   E_Config_Gadcon *cf_gc;
   E_Config_Gadcon_Client *cf_gcc;
   int ok = 0, enabled = 0;
   
   cfdata = data;
   e_widget_disabled_set(cfdata->o_enabled, 0);
   e_widget_disabled_set(cfdata->o_disabled, 0);
   for (l = e_config->gadcons; l; l = l->next)
     {
	cf_gc = l->data;
	if ((!strcmp(cf_gc->name, cfdata->gc->name)) &&
	    (!strcmp(cf_gc->id, cfdata->gc->id)))
	  {
	     ok = 1;
	     break;
	  }
     }
   if (!ok) return;
   for (l = cf_gc->clients; l; l = l->next)
     {
	cf_gcc = l->data;
	if (!cf_gcc->name) continue;
	if (!strcmp(cf_gcc->name, cfdata->cname))
	  {
	     enabled = 1;
	     break;
	  }
     }
   e_widget_radio_toggle_set(cfdata->o_enabled, enabled);
   e_widget_radio_toggle_set(cfdata->o_disabled, 1 - enabled);
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   /* generate the core widget layout for a basic dialog */
   Evas_Object *o, *o2, *of, *ob, *oi, *oj;
   E_Radio_Group *rg;
   Evas_Coord wmw, wmh;
   Evas_List *styles, *l;
   int sel, n;
   
   /* FIXME: this is just raw config now - it needs UI improvments */
   o = e_widget_list_add(evas, 0, 1);

   of = e_widget_framelist_add(evas, _("Available Items"), 0);
   
   oi = e_widget_ilist_add(evas, 24, 24, &(cfdata->cname));

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
	e_widget_ilist_append(oi, icon, label, _cb_select, cfdata, cc->name);
     }
   
   e_widget_ilist_go(oi);
   
   e_widget_min_size_get(oi, &wmw, &wmh);
   if (wmw < 200) wmw = 200;
   e_widget_min_size_set(oi, wmw, 250);
   
   e_widget_framelist_object_append(of, oi);
   
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   of = e_widget_framelist_add(evas, _("Status"), 0);
   
   rg = e_widget_radio_group_new(&(cfdata->enabled));
   ob = e_widget_radio_add(evas, _("Enabled"), 1, rg);
   e_widget_disabled_set(ob, 1);
   e_widget_framelist_object_append(of, ob);
   cfdata->o_enabled = ob;
   ob = e_widget_radio_add(evas, _("Disabled"), 0, rg);
   e_widget_disabled_set(ob, 1);
   e_widget_framelist_object_append(of, ob);
   cfdata->o_disabled = ob;
   
   e_widget_list_object_append(o, of, 0, 0, 0.0);
   
   return o;
}
#endif
