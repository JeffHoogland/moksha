#include "e.h"

/* TODO:
 * 
 * Give list some icons.
 * Support XRandr Rotation
*/

static void        *_create_data             (E_Config_Dialog *cfd);
static void         _free_data               (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int          _basic_apply_data        (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets    (E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static void         _load_rates              (void *data);
static void         _ilist_item_change       (void *data);

Evas_Object *rate_list = NULL;
Evas_Object *res_list = NULL;

typedef struct _Resolution Resolution;
struct _Resolution 
{
   int size_id;
   Ecore_X_Screen_Size size;
   Ecore_X_Screen_Refresh_Rate *rates;
};

struct _E_Config_Dialog_Data 
{
   E_Config_Dialog *cfd;
   Resolution *res;
   Ecore_X_Screen_Size orig_size;
   Ecore_X_Screen_Refresh_Rate orig_rate;
};

EAPI E_Config_Dialog *
e_int_config_display(E_Container *con) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   
   cfd = e_config_dialog_new(con, _("Display Settings"), NULL, 0, v, NULL);
   e_dialog_resizable_set(cfd->dia, 1);
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata) 
{
   E_Manager *man;
   
   man = e_manager_current_get();
   cfdata->orig_size = ecore_x_randr_current_screen_size_get(man->root);   
   cfdata->orig_rate = ecore_x_randr_current_screen_refresh_rate_get(man->root);
}

static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfdata);
   cfdata->cfd = cfd;
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   free(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   char *sel_res, *sel_rate;
   int w, h, r, i, n;
   Ecore_X_Screen_Size *sizes;
   Ecore_X_Screen_Size size;
   Ecore_X_Screen_Refresh_Rate *rates;   
   Ecore_X_Screen_Refresh_Rate rate;
   E_Manager *man;
   
   sel_res = (char *)e_widget_ilist_selected_label_get(res_list);
   sel_rate = (char *)e_widget_ilist_selected_label_get(rate_list);
   sscanf(sel_res, "%dx%d", &w, &h);
   sscanf(sel_rate, "%d Hz", &r);
      
   if ((cfdata->orig_size.width == w) && 
       (cfdata->orig_size.height == h) &&
       (cfdata->orig_rate.rate == r))
     return 1;

   man = e_manager_current_get();
   sizes = ecore_x_randr_screen_sizes_get(man->root, &n);
   for (i = 0; i < n; i++) 
     {
	if ((sizes[i].width == w) && 
	    (sizes[i].height == h))
	  {
	     size = sizes[i];
	     int k, rr;
	     rates = ecore_x_randr_screen_refresh_rates_get(man->root, i, &rr);
	     for (k = 0; k < rr; k++) 
	       {
		  if (rates[k].rate == r) 
		    {
		       rate = rates[k];
		       break;
		    }  
	       }
	     break;
	  }
     }
   
   int ret;   
   ret = ecore_x_randr_screen_refresh_rate_set(man->root, size, rate);
   cfdata->orig_size = size;
   cfdata->orig_rate = rate;
   
   return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *of, *ol, *rl;
   E_Manager *man;
   Ecore_X_Screen_Size *sizes;
   Ecore_X_Screen_Size size;
   int i, r, s;
   
   _fill_data(cfdata);
   
   o = e_widget_list_add(evas, 0, 0);
   
   of = e_widget_framelist_add(evas, _("Resolution"), 0);   
   ol = e_widget_ilist_add(evas, 32, 32, NULL);
   e_widget_min_size_set(ol, 140, 120);   
   e_widget_framelist_object_append(of, ol);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   res_list = ol;
   
   of = e_widget_framelist_add(evas, _("Refresh Rate"), 0);   
   rl = e_widget_ilist_add(evas, 8, 8, NULL);
   e_widget_min_size_set(rl, 140, 90);   
   e_widget_framelist_object_append(of, rl);
   e_widget_list_object_append(o, of, 1, 1, 0.5);   
   
   rate_list = rl;
   
   man = e_manager_current_get();
   sizes = ecore_x_randr_screen_sizes_get(man->root, &s);
   size = ecore_x_randr_current_screen_size_get(man->root);
   
   if (sizes) 
     {	
	char buf[16];

	for (i = 0; i < s; i++) 
	  {
	     Resolution *res;
	     
	     res = E_NEW(Resolution, 1);
	     if (!res) continue;
	     	     
	     res->size = sizes[i];
	     res->size_id = i;
	     res->rates = ecore_x_randr_screen_refresh_rates_get(man->root, res->size_id, &r);
	     	     
	     snprintf(buf, sizeof(buf), "%dx%d", sizes[i].width, sizes[i].height);
	     e_widget_ilist_append(ol, NULL, buf, _ilist_item_change, res, NULL);	     

	     if ((res->size.width == size.width) && (res->size.height == size.height)) 
	       { 	     
		  e_widget_ilist_selected_set(ol, i);
		  _load_rates(res);	     		  
	       }
	  }	
     }
   
   e_widget_ilist_go(ol);
   e_widget_ilist_go(rl);
   return o;
}

static void
_load_rates(void *data) 
{
   int k, r;
   E_Manager *man;
   Resolution *res = data;
   Ecore_X_Screen_Refresh_Rate rt;
   Ecore_X_Screen_Refresh_Rate *rts;
   char buf[16];

   man = e_manager_current_get();   
   rts = ecore_x_randr_screen_refresh_rates_get(man->root, res->size_id, &r);
   rt = ecore_x_randr_current_screen_refresh_rate_get(man->root);

   e_widget_ilist_clear(rate_list);
   
   for (k = 0; k < r; k++) 
     {	

	snprintf(buf, sizeof(buf), "%d Hz", rts[k].rate);
	e_widget_ilist_append(rate_list, NULL, buf, NULL, NULL, NULL);
	if (rt.rate == rts[k].rate) 
	  e_widget_ilist_selected_set(rate_list, k);
     }   
}

static void
_ilist_item_change(void *data) 
{
   _load_rates(data);
}
