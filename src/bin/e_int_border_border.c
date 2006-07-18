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
   E_Border *border;
   char *bordername;
   int remember_border;
};

/* a nice easy setup function that does the dirty work */
EAPI void
e_int_border_border(E_Border *bd)
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
	 
         /* create config diaolg for bd object/data */
         cfd = e_config_dialog_new(bd->zone->container, 
			     _("Window Border Selection"), NULL, 0, v, bd);
         bd->border_border_dialog = cfd;
      }
}

/**--CREATE--**/
static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   if ((cfdata->border->remember) &&
       (cfdata->border->remember->apply & E_REMEMBER_APPLY_BORDER))
     cfdata->remember_border = 1;
   cfdata->bordername = strdup(cfdata->border->client.border.name);
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
   cfdata->border = cfd->data;
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   /* Free the cfdata */
   cfdata->border->border_border_dialog = NULL;
   E_FREE(cfdata->bordername);
   free(cfdata);
}

/**--APPLY--**/
static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   /* FIXME: need to check if the remember stuff will actually work or notx
    * (see e_int_border_remember.c where it checks and warns) */
   if (cfdata->remember_border)
     {
	if (!cfdata->border->remember)
	  {
	     cfdata->border->remember = e_remember_new();
	     if (cfdata->border->remember)
	       {
		  e_remember_use(cfdata->border->remember);
		  e_remember_update(cfdata->border->remember, cfdata->border);
		  cfdata->border->remember->match |= E_REMEMBER_MATCH_NAME;
		  cfdata->border->remember->match |= E_REMEMBER_MATCH_CLASS;
		  cfdata->border->remember->match |= E_REMEMBER_MATCH_ROLE;
		  cfdata->border->remember->match |= E_REMEMBER_MATCH_TYPE;
		  cfdata->border->remember->match |= E_REMEMBER_MATCH_TRANSIENT;
		  cfdata->border->remember->apply |= E_REMEMBER_APPLY_BORDER;
	       }
	  }
     }
   else
     {
	if (cfdata->border->remember)
	  {
	     cfdata->border->remember->apply &= ~E_REMEMBER_APPLY_BORDER;
	     if (cfdata->border->remember->apply == 0)
	       {
		  e_remember_unuse(cfdata->border->remember);
		  e_remember_del(cfdata->border->remember);
		  cfdata->border->remember = NULL;
	       }
	  }
     }
   if ((!cfdata->border->lock_border) && (!cfdata->border->shaded))
     {  
	if (cfdata->border->bordername) evas_stringshare_del(cfdata->border->bordername);
	cfdata->border->bordername = evas_stringshare_add(cfdata->bordername);
	cfdata->border->client.border.changed = 1;
	cfdata->border->changed = 1;
     }
   e_config_save_queue();
   return 1; /* Apply was OK */
}

/**--GUI--**/
static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   /* generate the core widget layout for a basic dialog */
   Evas_Object *o, *ob, *oi, *oj, *orect;
   Evas_Coord wmw, wmh;
   Evas_List *borders, *l;
   int sel, n;
   
   o = e_widget_list_add(evas, 0, 0);
   
   oi = e_widget_ilist_add(evas, 80, 48, &(cfdata->bordername));

   sel = 0;
   borders = e_theme_border_list();

   orect = evas_object_rectangle_add(evas);
   evas_object_color_set(orect, 0, 0, 0, 128);
   e_widget_ilist_append(oi, orect, "borderless", NULL, NULL, "borderless");
   
   for (n = 1, l = borders; l; l = l->next, n++)
     {
	char buf[4096];
	
	ob = e_livethumb_add(evas);
	e_livethumb_vsize_set(ob, 160, 96);
	oj = edje_object_add(e_livethumb_evas_get(ob));
        snprintf(buf, sizeof(buf), "widgets/border/%s/border", (char *)l->data);
	e_theme_edje_object_set(oj, "base/theme/borders", buf);
	e_livethumb_thumb_set(ob, oj);
	orect = evas_object_rectangle_add(e_livethumb_evas_get(ob));
	evas_object_color_set(orect, 0, 0, 0, 128);
	evas_object_show(orect);
	edje_object_part_swallow(oj, "client", orect);
	e_widget_ilist_append(oi, ob, (char *)l->data, NULL, NULL, l->data);
	if (!strcmp(cfdata->border->client.border.name, (char *)l->data))
	  sel = n;
     }
   e_widget_min_size_get(oi, &wmw, &wmh);
   e_widget_min_size_set(oi, wmw, 250);
   
   e_widget_ilist_go(oi);
   e_widget_ilist_selected_set(oi, sel);
   
   e_widget_list_object_append(o, oi, 1, 1, 0.5);
   
   ob = e_widget_check_add(evas, _("Remember this Border for this window next time it appears"), &(cfdata->remember_border));
   e_widget_list_object_append(o, ob, 0, 0, 1.0);
   return o;
}
