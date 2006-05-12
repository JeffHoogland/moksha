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
typedef struct _SureBox SureBox;

struct _Resolution 
{
   int size_id;
   Ecore_X_Screen_Size size;
   Ecore_X_Screen_Refresh_Rate *rates;
};

struct _SureBox
{
   E_Dialog *dia;
   Ecore_Timer *timer;
   int iterations;
   Ecore_X_Screen_Size orig_size;
   Ecore_X_Screen_Refresh_Rate orig_rate;
   E_Config_Dialog *cfd;
   E_Config_Dialog_Data *cfdata;
};

struct _E_Config_Dialog_Data 
{
   E_Config_Dialog *cfd;
   Resolution *res;
   Ecore_X_Screen_Size orig_size;
   Ecore_X_Screen_Refresh_Rate orig_rate;
   int restore;
   
   SureBox *surebox;
};

static void
_surebox_dialog_cb_delete(E_Win *win)
{
   E_Dialog *dia;
   SureBox *sb;
   E_Config_Dialog *cfd;
   
   dia = win->data;
   sb = dia->data;
   sb->cfdata->surebox = NULL;
   cfd = sb->cfdata->cfd;
   if (sb->timer) ecore_timer_del(sb->timer);
   sb->timer = NULL;
   free(sb);
   e_object_del(E_OBJECT(dia));
   e_object_unref(E_OBJECT(cfd));
}

static void
_surebox_dialog_cb_yes(void *data, E_Dialog *dia)
{
   _surebox_dialog_cb_delete(dia->win);
}

static void
_surebox_dialog_cb_no(void *data, E_Dialog *dia)
{
   SureBox *sb;
   
   sb = data;
   ecore_x_randr_screen_refresh_rate_set(sb->dia->win->container->manager->root,
					 sb->orig_size, sb->orig_rate);
   e_config->display_res_width = sb->orig_size.width;
   e_config->display_res_height = sb->orig_size.height;
   e_config->display_res_hz = sb->orig_rate.rate;
   sb->cfdata->orig_size = sb->orig_size;
   sb->cfdata->orig_rate = sb->orig_rate;
   e_config_save_queue();
   _surebox_dialog_cb_delete(dia->win);
}

static void
_surebox_text_fill(SureBox *sb)
{
   char buf[4096];
   
   if (!sb->dia) return;
   if (sb->iterations > 1)
     {
	snprintf(buf, sizeof(buf),
		 _("Does this look OK? Press <hilight>Yes</hilight> if it does, or No if not.<br>"
		   "If you do not press a button, the old resolution of<br>"
		   "%dx%d at %d Hz will be restored in %d seconds."),
		 sb->orig_size.width, sb->orig_size.height,
		 sb->orig_rate.rate, sb->iterations);
     }
   else
     {
	snprintf(buf, sizeof(buf),
		 _("Does this look OK? Press <hilight>Yes</hilight> if it does, or No if not.<br>"
		   "If you do not press a button, the old resolution of<br>"
		   "%dx%d at %d Hz will be restored <hilight>IMMEDIATELY</hilight>."),
		 sb->orig_size.width, sb->orig_size.height,
		 sb->orig_rate.rate);
     }
   e_dialog_text_set(sb->dia, buf);
}

static int
_surebox_timer_cb(void *data)
{
   SureBox *sb;
   
   sb = data;
   sb->iterations--;
   _surebox_text_fill(sb);
   if (sb->iterations == 0)
     {
	ecore_x_randr_screen_refresh_rate_set(sb->dia->win->container->manager->root,
					      sb->orig_size, sb->orig_rate);
	e_config->display_res_width = sb->orig_size.width;
	e_config->display_res_height = sb->orig_size.height;
	e_config->display_res_hz = sb->orig_rate.rate;
	sb->cfdata->orig_size = sb->orig_size;
	sb->cfdata->orig_rate = sb->orig_rate;
	e_config_save_queue();
	sb->timer = NULL;
	e_object_del(E_OBJECT(sb->dia));
	sb->dia = NULL;
	return 0;
     }
   return 1;
}

static SureBox *
_surebox_new(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   SureBox *sb;
   
   sb = E_NEW(SureBox, 1);
   sb->dia = e_dialog_new(cfd->con);
   sb->timer = ecore_timer_add(1.0, _surebox_timer_cb, sb);
   sb->iterations = 15;
   sb->orig_size = cfdata->orig_size;
   sb->orig_rate = cfdata->orig_rate;
   sb->cfd = cfd;
   sb->cfdata = cfdata;
   cfdata->surebox = sb;
   sb->dia->data = sb;
   e_dialog_title_set(sb->dia, _("Resolution change"));
   _surebox_text_fill(sb);
   e_win_delete_callback_set(sb->dia->win, _surebox_dialog_cb_delete);
   e_dialog_button_add(sb->dia, _("Yes"), NULL, _surebox_dialog_cb_yes, sb);
   e_dialog_button_add(sb->dia, _("No"), NULL, _surebox_dialog_cb_no, sb);
   e_dialog_button_focus_num(sb->dia, 1);
   e_win_borderless_set(sb->dia->win, 1);
   e_win_layer_set(sb->dia->win, 6);
   e_win_centered_set(sb->dia->win, 1);
   e_win_sticky_set(sb->dia->win, 1);
   e_dialog_show(sb->dia);
   e_object_ref(E_OBJECT(cfd));
}


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
   v->override_auto_apply = 1;
   
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
   cfdata->restore = e_config->display_res_restore;
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
   if (cfdata->surebox)
     _surebox_dialog_cb_delete(cfdata->surebox->dia->win);
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
   if (!sel_res) return 0;
   if (!sel_rate) return 0;
   sscanf(sel_res, "%ix%i", &w, &h);
   sscanf(sel_rate, "%i Hz", &r);
      
   e_config->display_res_width = cfdata->orig_size.width;
   e_config->display_res_height = cfdata->orig_size.height;
   e_config->display_res_hz = cfdata->orig_rate.rate;
   
   if ((cfdata->orig_size.width == w) && 
       (cfdata->orig_size.height == h) &&
       (cfdata->orig_rate.rate == r))
     goto saveonly;

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
   
   e_config->display_res_width = size.width;
   e_config->display_res_height = size.height;
   e_config->display_res_hz = rate.rate;
   ecore_x_randr_screen_refresh_rate_set(man->root, size, rate);
   _surebox_new(cfd, cfdata);
   
   cfdata->orig_size = size;
   cfdata->orig_rate = rate;
   
   saveonly:
   e_config->display_res_restore = cfdata->restore;
   e_config_save_queue();
   
   return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *of, *ol, *rl, *ob;
   E_Manager *man;
   Ecore_X_Screen_Size *sizes;
   Ecore_X_Screen_Size size;
   Ecore_X_Randr_Rotation rots, rot;
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
   
   if (!sizes)
     {
	e_util_dialog_show(_("Missing Features"),
			   _("Your X Display Server is missing support for<br>"
			     "The <hilight>XRandr</hilight> (X Resize and Rotate) extension.<br>"
			     "You cannot change screen resolutions without<br>"
			     "The support of this extension. It could also be<br>"
			     "That at the time <hilight>ecore</hilight> was built there<br>"
			     "was no XRandr support detected."));
     }
   else
     {
	char buf[16];
	int *sortindex;
	int sorted = 0, tmp;
	
	sortindex = alloca(s * sizeof(int));
	for (i = 0; i < s; i++) 
	  sortindex[i] = i;
	/* quick & dirty bubblesort */
	while (!sorted)
	  {
	     sorted = 1;
	     for (i = 0; i < (s - 1); i++)
	       {
		  if (sizes[sortindex[i]].width > sizes[sortindex[i + 1]].width)
		    {
		       sorted = 0;
		       tmp = sortindex[i];
		       sortindex[i] = sortindex[i + 1];
		       sortindex[i + 1] = tmp;
		    }
		  else if (sizes[sortindex[i]].width == sizes[sortindex[i + 1]].width)
		    {
		       if (sizes[sortindex[i]].height > sizes[sortindex[i + 1]].height)
			 {
			    sorted = 0;
			    tmp = sortindex[i];
			    sortindex[i] = sortindex[i + 1];
			    sortindex[i + 1] = tmp;
			 }
		    }
	       }
	  }
	
	for (i = 0; i < s; i++) 
	  {
	     Resolution *res;
	     
	     res = E_NEW(Resolution, 1);
	     if (!res) continue;
	     	     
	     res->size = sizes[sortindex[i]];
	     res->size_id = sortindex[i];
	     res->rates = ecore_x_randr_screen_refresh_rates_get(man->root, res->size_id, &r);
	     	     
	     snprintf(buf, sizeof(buf), "%ix%i", 
		      sizes[sortindex[i]].width, sizes[sortindex[i]].height);
	     e_widget_ilist_append(ol, NULL, buf, _ilist_item_change, res, NULL);

	     if ((res->size.width == size.width) &&
		 (res->size.height == size.height)) 
	       { 	     
		  e_widget_ilist_selected_set(ol, i);
		  _load_rates(res);	     		  
	       }
	  }	
     }
   
   ob = e_widget_check_add(evas, _("Restore this resolution on login"), &(cfdata->restore));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   
   e_widget_ilist_go(ol);
   e_widget_ilist_go(rl);
   
   rots = ecore_x_randr_screen_rotations_get(man->root);
   rot = ecore_x_randr_screen_rotation_get(man->root);
   if (!rots)
     {
	printf("no randr support\n");
     }
   if (rot & ECORE_X_RANDR_ROT_0) printf("rot: 0deg\n");
   if (rot & ECORE_X_RANDR_ROT_90) printf("rot: 90deg\n");
   if (rot & ECORE_X_RANDR_ROT_180) printf("rot: 180deg\n");
   if (rot & ECORE_X_RANDR_ROT_270) printf("rot: 270deg\n");
   if (rot & ECORE_X_RANDR_FLIP_X) printf("rot: flip x\n");
   if (rot & ECORE_X_RANDR_FLIP_Y) printf("rot: flip y\n");
   printf("---\n", rot);
   if (rots & ECORE_X_RANDR_ROT_0) printf("support: 0deg\n");
   if (rots & ECORE_X_RANDR_ROT_90) printf("support: 90deg\n");
   if (rots & ECORE_X_RANDR_ROT_180) printf("support: 180deg\n");
   if (rots & ECORE_X_RANDR_ROT_270) printf("support: 270deg\n");
   if (rots & ECORE_X_RANDR_FLIP_X) printf("support: flip x\n");
   if (rots & ECORE_X_RANDR_FLIP_Y) printf("support: flip y\n");
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
	snprintf(buf, sizeof(buf), "%i Hz", rts[k].rate);
	e_widget_ilist_append(rate_list, NULL, buf, NULL, NULL, NULL);
	if (rt.rate == rts[k].rate) 
	  e_widget_ilist_selected_set(rate_list, k);
     }   
   e_widget_ilist_selected_set(rate_list, 0);
}

static void
_ilist_item_change(void *data) 
{
   _load_rates(data);
}
