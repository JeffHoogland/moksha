/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* TODO:
 * 
 * Give list some icons.
*/

static void        *_create_data             (E_Config_Dialog *cfd);
static void         _free_data               (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int          _basic_apply_data        (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets    (E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static void         _load_rates              (E_Config_Dialog_Data *cfdata);
static void         _ilist_item_change       (void *data);
static int          _deferred_noxrandr_error (void *data);
static int          _deferred_norates_error  (void *data);
static int	    _sort_resolutions	     (const void *d1, const void *d2);

typedef struct _Resolution Resolution;
typedef struct _SureBox SureBox;

struct _Resolution 
{
   int id;
   Ecore_X_Screen_Size size;
   Eina_List *rates;
};

struct _SureBox
{
   E_Dialog *dia;
   Ecore_Timer *timer;
   int iterations;
   E_Config_Dialog *cfd;
   E_Config_Dialog_Data *cfdata;
};

struct _E_Config_Dialog_Data 
{
   E_Config_Dialog *cfd;
   Eina_List *resolutions;
   Ecore_X_Screen_Size orig_size;
   Ecore_X_Screen_Refresh_Rate orig_rate;
   int restore;
   int can_rotate;
   int can_flip;
   int rotation;
   int flip;
   int flip_x;
   int flip_y;
   int has_rates;

   Evas_Object *rate_list;
   Evas_Object *res_list;
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
   SureBox *sb;
   Ecore_X_Screen_Size c_size;
   Ecore_X_Screen_Refresh_Rate c_rate;
   E_Manager *man;

   sb = data;
   man = e_manager_current_get();
   c_size = ecore_x_randr_current_screen_size_get(man->root);
   c_rate = ecore_x_randr_current_screen_refresh_rate_get(man->root);
   e_config->display_res_width = c_size.width;
   e_config->display_res_height = c_size.height;
   e_config->display_res_hz = c_rate.rate;
   e_config_save_queue();
   sb->cfdata->orig_size = c_size;
   sb->cfdata->orig_rate = c_rate;
   if (dia) _surebox_dialog_cb_delete(dia->win);
}

static void
_surebox_dialog_cb_no(void *data, E_Dialog *dia)
{
   SureBox *sb;
   Eina_List *l;
   
   sb = data;
   ecore_x_randr_screen_refresh_rate_set(sb->dia->win->container->manager->root,
					 sb->cfdata->orig_size, sb->cfdata->orig_rate);
   for (l = sb->cfdata->resolutions; l; l = l->next)
     {
	Resolution *res = l->data;

	if ((res->size.width == sb->cfdata->orig_size.width) 
	  && (res->size.height == sb->cfdata->orig_size.height))
	  {
	     e_widget_ilist_selected_set(sb->cfdata->res_list, res->id);
	     break;
	  }
     }
   _load_rates(sb->cfdata);
   _surebox_dialog_cb_delete(dia->win);
}

static void
_surebox_text_fill(SureBox *sb)
{
   char buf[4096];
   
   if (!sb->dia) return;
   if (sb->iterations > 1)
     {
	if (sb->cfdata->has_rates)
	  snprintf(buf, sizeof(buf),
		   _("Does this look OK? Press <hilight>Yes</hilight> if it does, or No if not.<br>"
		     "If you do not press a button, the old resolution of<br>"
		     "%dx%d at %d Hz will be restored in %d seconds."),
		   sb->cfdata->orig_size.width, sb->cfdata->orig_size.height,
		   sb->cfdata->orig_rate.rate, sb->iterations);
	else
	  snprintf(buf, sizeof(buf),
		   _("Does this look OK? Press <hilight>Yes</hilight> if it does, or No if not.<br>"
		     "If you do not press a button, the old resolution of<br>"
		     "%dx%d will be restored in %d seconds."),
		   sb->cfdata->orig_size.width, sb->cfdata->orig_size.height,
		   sb->iterations);
     }
   else
     {
	if (sb->cfdata->has_rates)
	  snprintf(buf, sizeof(buf),
		   _("Does this look OK? Press <hilight>Yes</hilight> if it does, or No if not.<br>"
		     "If you do not press a button, the old resolution of<br>"
		     "%dx%d at %d Hz will be restored <hilight>IMMEDIATELY</hilight>."),
		   sb->cfdata->orig_size.width, sb->cfdata->orig_size.height,
		   sb->cfdata->orig_rate.rate);
	else
	  snprintf(buf, sizeof(buf),
		   _("Does this look OK? Press <hilight>Yes</hilight> if it does, or No if not.<br>"
		     "If you do not press a button, the old resolution of<br>"
		     "%dx%d will be restored <hilight>IMMEDIATELY</hilight>."),
		   sb->cfdata->orig_size.width, sb->cfdata->orig_size.height);
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
	Eina_List *l;

	ecore_x_randr_screen_refresh_rate_set(sb->dia->win->container->manager->root,
	      sb->cfdata->orig_size, sb->cfdata->orig_rate);
	for (l = sb->cfdata->resolutions; l; l = l->next)
	  {
	     Resolution *res = l->data;

	     if ((res->size.width == sb->cfdata->orig_size.width) 
		   && (res->size.height == sb->cfdata->orig_size.height))
	       {
		  e_widget_ilist_selected_set(sb->cfdata->res_list, res->id);
		  break;
	       }
	  }
	_load_rates(sb->cfdata);
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
   sb->dia = e_dialog_new(cfd->con, "E", "_display_res_sure_dialog");
   sb->timer = ecore_timer_add(1.0, _surebox_timer_cb, sb);
   sb->iterations = 15;
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
   e_win_centered_set(sb->dia->win, 1);
   e_win_borderless_set(sb->dia->win, 1);
   e_win_layer_set(sb->dia->win, 6);
   e_win_sticky_set(sb->dia->win, 1);
   e_dialog_show(sb->dia);
   e_object_ref(E_OBJECT(cfd));
   return sb;
}


EAPI E_Config_Dialog *
e_int_config_display(E_Container *con, const char *params __UNUSED__) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (!ecore_x_randr_query())
     {
        ecore_timer_add(0.5, _deferred_noxrandr_error, NULL);
        fprintf(stderr, "XRandR not present on this display. \n");
        return NULL;
     }

   if (e_config_dialog_find("E", "_config_display_dialog")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->override_auto_apply = 1;

   cfd = e_config_dialog_new(con,
			     _("Screen Resolution Settings"),
			    "E", "_config_display_dialog",
			     "enlightenment/screen_resolution", 0, v, NULL);
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata) 
{
   E_Manager *man;
   int rots;

   man = e_manager_current_get();
   cfdata->orig_size = ecore_x_randr_current_screen_size_get(man->root);   
   cfdata->orig_rate = ecore_x_randr_current_screen_refresh_rate_get(man->root);
   cfdata->restore = e_config->display_res_restore;

   rots = ecore_x_randr_screen_rotations_get(man->root);
   if (rots)
     {
	cfdata->rotation = ecore_x_randr_screen_rotation_get(man->root);
	cfdata->can_flip = rots & (ECORE_X_RANDR_FLIP_X | ECORE_X_RANDR_FLIP_Y);
	cfdata->flip = cfdata->rotation &
	  (ECORE_X_RANDR_FLIP_X | ECORE_X_RANDR_FLIP_Y);

	if (cfdata->rotation & (ECORE_X_RANDR_FLIP_X))
	  cfdata->flip_x = 1;
	if (cfdata->rotation & (ECORE_X_RANDR_FLIP_Y))
	  cfdata->flip_y = 1;

	cfdata->can_rotate = 
	  rots & (ECORE_X_RANDR_ROT_0 | ECORE_X_RANDR_ROT_90 |
		  ECORE_X_RANDR_ROT_180 | ECORE_X_RANDR_ROT_270);
	cfdata->rotation = 
	  cfdata->rotation & 
	  (ECORE_X_RANDR_ROT_0 | ECORE_X_RANDR_ROT_90 |
	   ECORE_X_RANDR_ROT_180 | ECORE_X_RANDR_ROT_270);
     }
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
   Eina_List *l, *ll;

   if (cfdata->surebox)
     _surebox_dialog_cb_delete(cfdata->surebox->dia->win);

   for (l = cfdata->resolutions; l; l = l->next)
     {
	Resolution *r = l->data;

	for (ll = r->rates; ll; ll = ll->next)
	  E_FREE(ll->data);

	r->rates = eina_list_free(r->rates);
	E_FREE(r);
     }
   cfdata->resolutions = eina_list_free(cfdata->resolutions);
   E_FREE(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   const char *sel_res, *sel_rate;
   Ecore_X_Screen_Size size;
   Ecore_X_Screen_Refresh_Rate rate;
   E_Manager *man;

   sel_res = e_widget_ilist_selected_label_get(cfdata->res_list);
   sel_rate = e_widget_ilist_selected_label_get(cfdata->rate_list);
   if (!sel_res) return 0;
   if (!sel_rate && cfdata->has_rates) return 0;

   sscanf(sel_res, "%ix%i", &size.width, &size.height);
   if (cfdata->has_rates) sscanf(sel_rate, "%i Hz", &rate.rate);

   man = e_manager_current_get();

   if (!((cfdata->orig_size.width == size.width) && (cfdata->orig_size.height == size.height) &&
	 (cfdata->orig_rate.rate == rate.rate || !cfdata->has_rates)))
     {
	if (cfdata->has_rates)
	  ecore_x_randr_screen_refresh_rate_set(man->root, size, rate);
	else
	  ecore_x_randr_screen_size_set(man->root, size);

	if (e_config->cnfmdlg_disabled)
	  {
	     SureBox *sb;

	     sb = E_NEW(SureBox, 1);
	     sb->cfd = cfd;
	     sb->cfdata = cfdata;
	     _surebox_dialog_cb_yes (sb, NULL);
	  }
	else
	  _surebox_new(cfd, cfdata);
     }

   if ((cfdata->can_rotate) || (cfdata->can_flip))
     {
	int rot;

	cfdata->flip = cfdata->rotation;	
	if (cfdata->flip_x)
	  cfdata->flip = cfdata->flip | ECORE_X_RANDR_FLIP_X;
	if (cfdata->flip_y)
	  cfdata->flip = cfdata->flip | ECORE_X_RANDR_FLIP_Y;

	rot = ecore_x_randr_screen_rotation_get(man->root);
	ecore_x_randr_screen_rotation_set(man->root,
					  cfdata->rotation | cfdata->flip);
	e_config->display_res_rotation = cfdata->rotation | cfdata->flip;
     }
   else
     e_config->display_res_rotation = 0;

   e_config->display_res_restore = cfdata->restore;
   e_config_save_queue();

   return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *of, *ob, *ot, *o2;
   E_Radio_Group *rg;
   E_Manager *man;
   Ecore_X_Screen_Size *sizes;
   int i, s, sel = 0;

   o = e_widget_list_add(evas, 0, 1);

   o2 = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("Resolution"), 0);   
   ob = e_widget_ilist_add(evas, 24, 24, NULL);
   cfdata->res_list = ob;
   e_widget_min_size_set(ob, 170, 215);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o2, of, 1, 1, 0.5);

   ob = e_widget_check_add(evas, _("Restore on login"), &cfdata->restore);
   e_widget_list_object_append(o2, ob, 1, 0, 0.0);
   e_widget_list_object_append(o, o2, 1, 1, 0.5);

   ot = e_widget_table_add(evas, 0);
   of = e_widget_framelist_add(evas, _("Refresh"), 0);   
   ob = e_widget_ilist_add(evas, 24, 24, NULL);
   cfdata->rate_list = ob;
   e_widget_min_size_set(ob, 100, 80);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 0, 0, 1, 1, 1, 1, 1, 1);

   man = e_manager_current_get();
   sizes = ecore_x_randr_screen_sizes_get(man->root, &s);
   cfdata->has_rates = 0;

   if ((!sizes) || (s == 0))
     ecore_timer_add(0.5, _deferred_noxrandr_error, NULL);
   else
     {
	Eina_List *l;

	cfdata->orig_size = ecore_x_randr_current_screen_size_get(man->root);
	cfdata->orig_rate = ecore_x_randr_current_screen_refresh_rate_get(man->root);

	for (i = 0; i < (s - 1); i++)
	  {
	     Resolution * res;
	     Ecore_X_Screen_Refresh_Rate * rates;
	     int r = 0, j;

	     res = E_NEW(Resolution, 1);
	     if (!res) continue;

	     res->size.width = sizes[i].width;
	     res->size.height = sizes[i].height;
	     rates = ecore_x_randr_screen_refresh_rates_get(man->root, i, &r);
	     for (j = 0; j < r; j++)
	       {
		  Ecore_X_Screen_Refresh_Rate * rt;

		  cfdata->has_rates = 1;
		  rt = E_NEW(Ecore_X_Screen_Refresh_Rate, 1);
		  if (!rt) continue;
		  rt->rate = rates[j].rate;
		  res->rates = eina_list_append(res->rates, rt);
	       }
	     if (rates) E_FREE(rates);
	     cfdata->resolutions = eina_list_append(cfdata->resolutions, res);
	  }

	cfdata->resolutions = eina_list_sort(cfdata->resolutions, 
	      eina_list_count(cfdata->resolutions), _sort_resolutions);
	cfdata->resolutions = eina_list_reverse(cfdata->resolutions);

	evas_event_freeze(evas_object_evas_get(cfdata->res_list));
	edje_freeze();
	e_widget_ilist_freeze(cfdata->res_list);
	i = 0;
	for (l = cfdata->resolutions; l; l = l->next)
	  {
	     char buf[1024];
	     Resolution *res = l->data;

	     res->id = i++;
	     snprintf(buf, sizeof(buf), "%ix%i", res->size.width, res->size.height);
	     e_widget_ilist_append(cfdata->res_list, NULL, buf, _ilist_item_change, cfdata, NULL);

	     if ((res->size.width == cfdata->orig_size.width) &&
		 (res->size.height == cfdata->orig_size.height))
	       sel = res->id;
	  }

	if (!cfdata->has_rates)
	  ecore_timer_add(0.5, _deferred_norates_error, NULL);
     }

   E_FREE(sizes);

   e_widget_ilist_go(cfdata->res_list);
   e_widget_ilist_selected_set(cfdata->res_list, sel);
   _load_rates(cfdata);
   e_widget_ilist_thaw(cfdata->res_list);
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(cfdata->res_list));

   if (cfdata->can_rotate)
     {
	of = e_widget_framelist_add(evas, _("Rotation"), 0);
	rg = e_widget_radio_group_new(&(cfdata->rotation));
	ob = e_widget_radio_icon_add(evas, NULL, "enlightenment/screen_normal", 24, 24, ECORE_X_RANDR_ROT_0, rg);
        e_widget_framelist_object_append(of, ob);
	if (!(cfdata->can_rotate & ECORE_X_RANDR_ROT_0)) e_widget_disabled_set(ob, 1);
	ob = e_widget_radio_icon_add(evas, NULL, "enlightenment/screen_left", 24, 24, ECORE_X_RANDR_ROT_90, rg);
        e_widget_framelist_object_append(of, ob);
	if (!(cfdata->can_rotate & ECORE_X_RANDR_ROT_90)) e_widget_disabled_set(ob, 1);
	ob = e_widget_radio_icon_add(evas, NULL, "enlightenment/screen_around", 24, 24, ECORE_X_RANDR_ROT_180, rg);
        e_widget_framelist_object_append(of, ob);
	if (!(cfdata->can_rotate & ECORE_X_RANDR_ROT_180)) e_widget_disabled_set(ob, 1);
	ob = e_widget_radio_icon_add(evas, NULL, "enlightenment/screen_right", 24, 24, ECORE_X_RANDR_ROT_270, rg);
        e_widget_framelist_object_append(of, ob);
	if (!(cfdata->can_rotate & ECORE_X_RANDR_ROT_270)) e_widget_disabled_set(ob, 1);
	e_widget_table_object_append(ot, of, 0, 1, 1, 1, 1, 0, 1, 0);
     }

   if (cfdata->can_flip)
     {
	of = e_widget_framelist_add(evas, _("Mirroring"), 0);
	ob = e_widget_check_icon_add(evas, NULL, "enlightenment/screen_hflip", 24, 24, &(cfdata->flip_x));
        e_widget_framelist_object_append(of, ob);
	if (!(cfdata->can_rotate & ECORE_X_RANDR_FLIP_X)) e_widget_disabled_set(ob, 1);
	ob = e_widget_check_icon_add(evas, NULL, "enlightenment/screen_vflip", 24, 24, &(cfdata->flip_y));
        e_widget_framelist_object_append(of, ob);
	if (!(cfdata->can_rotate & ECORE_X_RANDR_FLIP_Y)) 
	  e_widget_disabled_set(ob, 1);
	e_widget_table_object_append(ot, of, 0, 2, 1, 1, 1, 0, 1, 0);
     }

   e_widget_list_object_append(o, ot, 1, 1, 0.5);
   e_dialog_resizable_set(cfd->dia, 1);
   return o;
}

static int
_sort_resolutions(const void *d1, const void *d2)
{
   const Resolution *r1 = d1;
   const Resolution *r2 = d2;

   if (r1->size.width > r2->size.width) return 1;
   if (r1->size.width < r2->size.width) return -1;
   if (r1->size.height > r2->size.height) return 1;

   return -1;
}

static void
_load_rates(E_Config_Dialog_Data *cfdata)
{
   int r, k = 0, sel = 0;
   char buf[16];
   Eina_List *l;

   evas_event_freeze(evas_object_evas_get(cfdata->rate_list));
   edje_freeze();
   e_widget_ilist_freeze(cfdata->rate_list);
   e_widget_ilist_clear(cfdata->rate_list);

   r = e_widget_ilist_selected_get(cfdata->res_list);

   for (l = cfdata->resolutions; l; l = l->next) 
     {
	Resolution *res = l->data;

	if (res->id == r)
	  {
	     Eina_List *ll;
	     for (ll = res->rates; ll; ll = ll->next)
	       {
		  Ecore_X_Screen_Refresh_Rate *rt;

		  rt = ll->data;
		  snprintf(buf, sizeof(buf), "%i Hz", rt->rate);
		  e_widget_ilist_append(cfdata->rate_list, NULL, buf, NULL, NULL, NULL);

		  if (rt->rate == cfdata->orig_rate.rate) sel = k;
		  k++;
	       }
	     break;
	  }
     }   
   
   e_widget_ilist_go(cfdata->rate_list);
   e_widget_ilist_selected_set(cfdata->rate_list, k);
   e_widget_ilist_thaw(cfdata->rate_list);
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(cfdata->rate_list));
}

static void
_ilist_item_change(void *data) 
{
   _load_rates(data);
}

static int
_deferred_noxrandr_error(void *data)
{
   e_util_dialog_show(_("Missing Features"),
		      _("Your X Display Server is missing support for<br>"
			"the <hilight>XRandr</hilight> (X Resize and Rotate) extension.<br>"
			"You cannot change screen resolutions without<br>"
			"the support of this extension. It could also be<br>"
			"that at the time <hilight>ecore</hilight> was built, there<br>"
			"was no XRandr support detected."));
   return 0;
}

static int
_deferred_norates_error(void *data)
{
   e_util_dialog_show(_("No Refresh Rates Found"),
		      _("No refresh rates were reported by your X Display Server.<br>"
			"If you are running a nested X Display Server, then<br>"
			"this is to be expected. However, if you are not, then<br>"
			"the current refresh rate will be used when setting<br>"
			"the resolution, which may cause <hilight>damage</hilight> to your screen."));
   return 0;
}
