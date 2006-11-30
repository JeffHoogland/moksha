/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

static void        *_create_data             (E_Config_Dialog *cfd);
static void         _free_data               (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int          _basic_apply_data        (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets    (E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

static int	    _basic_list_sort_cb	     (void *d1, void *d2);

struct _E_Config_Dialog_Data
{
   E_Config_Dialog *cfd;
   
   /* Current data */
   char *imc_current;
   
   Evas_List *imc_basic_list;
   
   struct
     {
	Evas_Object     *imc_basic_list;
     } 
   gui;
};

EAPI E_Config_Dialog *
e_int_config_imc(E_Container *con)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "_config_imc_dialog")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);
   
   v->create_cfdata           = _create_data;
   v->free_cfdata             = _free_data;
   v->advanced.create_widgets = NULL;
   v->advanced.apply_cfdata   = NULL;
   v->basic.create_widgets    = _basic_create_widgets;
   v->basic.apply_cfdata      = _basic_apply_data;
   
   cfd = e_config_dialog_new(con,
			     _("Input Method Configuration"),
			    "E", "_config_imc_dialog",
			     "enlightenment/imc", 0, v, NULL);
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   cfdata->imc_basic_list = e_intl_input_method_list(); 
   	
   /* Sort basic input method list */	
   cfdata->imc_basic_list = evas_list_sort(cfdata->imc_basic_list, 
	 evas_list_count(cfdata->imc_basic_list), 
	 _basic_list_sort_cb);
   
   cfdata->imc_current = strdup(e_config->input_method);
   
   return;
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->cfd = cfd;
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{

   E_FREE(cfdata->imc_current);
   
   while (cfdata->imc_basic_list) { 
	free(cfdata->imc_basic_list->data); 
	cfdata->imc_basic_list = evas_list_remove_list(cfdata->imc_basic_list, cfdata->imc_basic_list);
   }

   free(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{	
   printf("Setting input method to %s\n", cfdata->imc_current); 
   if (cfdata->imc_current)
     {
	if (e_config->input_method) evas_stringshare_del(e_config->input_method);
	e_config->input_method = evas_stringshare_add(cfdata->imc_current);
	e_intl_input_method_set(e_config->input_method);
     }
   
   e_config_save_queue();
   
   return 1;
}

static int
_advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{	  
   if (cfdata->imc_current)
     {
	if (e_config->input_method) evas_stringshare_del(e_config->input_method);
	e_config->input_method = evas_stringshare_add(cfdata->imc_current);
	e_intl_input_method_set(e_config->input_method);
     }
   
   e_config_save_queue();
   
   return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ob;
   int i;
   Evas_List *next;
   
   o = e_widget_list_add(evas, 0, 0);
  
   of = e_widget_frametable_add(evas, _("Input Method Selector"), 1);
  
   /* Language List */ 
   ob = e_widget_ilist_add(evas, 16, 16, &(cfdata->imc_current));
   e_widget_min_size_set(ob, 175, 175);
   cfdata->gui.imc_basic_list = ob;

   evas_event_freeze(evas_object_evas_get(ob));
   edje_freeze();
   e_widget_ilist_freeze(ob);
   
   i = 0;
   for (next = cfdata->imc_basic_list; next; next = next->next) 
     {
	const char * imc;

	imc = next->data;
	
	e_widget_ilist_append(cfdata->gui.imc_basic_list, NULL, imc, NULL, NULL, imc);
	if (cfdata->imc_current && !strncmp(imc, cfdata->imc_current, strlen(cfdata->imc_current)))
	  e_widget_ilist_selected_set(cfdata->gui.imc_basic_list, i);
	
	i++;
     }
   
   e_widget_ilist_go(ob);
   e_widget_ilist_thaw(ob);
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(ob));

   e_widget_frametable_object_append(of, ob, 0, 0, 2, 6, 1, 1, 1, 1);
   e_widget_ilist_selected_set(ob, e_widget_ilist_selected_get(ob));
   
   e_widget_framelist_content_align_set(of, 0.0, 0.0);
   
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   e_dialog_resizable_set(cfd->dia, 1);
   return o;

}

static int
_basic_list_sort_cb(void *d1, void *d2)
{
   if (!d1) return 1;
   if (!d2) return -1;
   
   return (strcmp((const char*)d1, (const char*)d2));
}
