#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data 
{
   Evas_Object *obj, *o_frame, *o_clip;
   Evas_Object *prev_bg, *bg, *o_trans;
};

static void _e_wid_del_hook(Evas_Object *obj);
static void _e_wid_done(void *data, Evas_Object *obj, const char *emission, const char *source);
static Evas_Object *_trans_preview_add(Evas *evas, int minw, int minh);
static void         _trans_preview_trans_set(Evas_Object *obj, const char *trans);

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

static void _event_cb_changed(void *data);
static void _trans_cb_changed(void *data);

struct _E_Config_Dialog_Data 
{
   char *transition_start;
   char *transition_desk;
   char *transition_change;
   
   Evas_Object *event_list;
   Evas_Object *trans_list;
   Evas_Object *tp;
};

EAPI E_Config_Dialog *
e_int_config_transitions(E_Container *con) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   if (e_config_dialog_find("E", "_config_transitions_dialog")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return NULL;
   
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   
   cfd = e_config_dialog_new(con, _("Transition Settings"),
			     "E", "_config_transitions_dialog",
			     "enlightenment/transitions", 0, v, NULL);
   if (!cfd) return NULL;
   return cfd;
}

static void 
_fill_data(E_Config_Dialog_Data *cfdata) 
{
   if (e_config->transition_start)
     cfdata->transition_start = strdup(e_config->transition_start);
   if (e_config->transition_desk)
     cfdata->transition_desk = strdup(e_config->transition_desk);
   if (e_config->transition_change)
     cfdata->transition_change = strdup(e_config->transition_change);
}

static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfdata);
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
   if (e_config->transition_start)
     evas_stringshare_del(e_config->transition_start);
   e_config->transition_start = NULL;
   if (cfdata->transition_start) 
     {
	if (e_theme_transition_find(cfdata->transition_start)) 
	  e_config->transition_start = evas_stringshare_add(cfdata->transition_start);
     }

   if (e_config->transition_desk)
     evas_stringshare_del(e_config->transition_desk);
   e_config->transition_desk = NULL;
   if (cfdata->transition_desk) 
     {
	if (e_theme_transition_find(cfdata->transition_desk)) 
	  e_config->transition_desk = evas_stringshare_add(cfdata->transition_desk);
     }

   if (e_config->transition_change)
     evas_stringshare_del(e_config->transition_change);
   e_config->transition_change = NULL;
   if (cfdata->transition_change) 
     {   
	if (e_theme_transition_find(cfdata->transition_change)) 
	  e_config->transition_change = evas_stringshare_add(cfdata->transition_change);
     }

   e_config_save_queue();
   return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   E_Zone *zone;
   Evas_Object *o, *of, *ot, *il, *ob;
   Evas_List *l;
   char *t;

   zone = e_zone_current_get(cfd->con);
   
   o = e_widget_list_add(evas, 1, 0);
   ot = e_widget_table_add(evas, 0);

   of = e_widget_framelist_add(evas, _("Events"), 0);
   il = e_widget_ilist_add(evas, 48, 48, NULL);
   cfdata->event_list = il;
   e_widget_min_size_set(il, 160, 225);
   e_widget_ilist_append(il, NULL, _("Startup"), _event_cb_changed, cfdata, NULL);
   e_widget_ilist_append(il, NULL, _("Desk Change"), _event_cb_changed, cfdata, NULL);
   e_widget_ilist_append(il, NULL, _("Background Change"), _event_cb_changed, cfdata, NULL);
   e_widget_ilist_go(il);
   e_widget_framelist_object_append(of, il);
   e_widget_table_object_append(ot, of, 0, 0, 1, 1, 1, 1, 1, 1);

   of = e_widget_framelist_add(evas, _("Transitions"), 0);
   il = e_widget_ilist_add(evas, 48, 48, NULL);
   cfdata->trans_list = il;
   e_widget_min_size_set(il, 160, 225);

   e_widget_ilist_append(il, NULL, _("None"), _trans_cb_changed, cfdata, NULL);
   l = e_theme_transition_list();
   for (l = e_theme_transition_list(); l; l = l->next) 
     {
	t = l->data;
	if (!t) continue;
	e_widget_ilist_append(il, NULL, t, _trans_cb_changed, cfdata, NULL);
     }
   e_widget_ilist_go(il);
   
   e_widget_framelist_object_append(of, il);
   e_widget_table_object_append(ot, of, 1, 0, 1, 1, 1, 1, 1, 1);

   of = e_widget_framelist_add(evas, _("Preview"), 0);
   ob = _trans_preview_add(evas, 300, ((300 * zone->h) / zone->w));
   cfdata->tp = ob;
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 2, 0, 1, 1, 1, 1, 1, 1);
   
   e_widget_list_object_append(o, ot, 1, 1, 0.5);
   
   return o;
}

static void 
_event_cb_changed(void *data) 
{
   E_Config_Dialog_Data *cfdata;
   const char *list, *trans;
   int sel, i;
   
   cfdata = data;
   sel = e_widget_ilist_selected_get(cfdata->event_list);
   switch (sel) 
     {
      case 0:
	trans = e_config->transition_start;
	break;
      case 1:
	trans = e_config->transition_desk;
	break;
      case 2:
	trans = e_config->transition_change;
	break;
     }

   for (i = 0; i < e_widget_ilist_count(cfdata->trans_list); i++) 
     {
	list = e_widget_ilist_nth_label_get(cfdata->trans_list, i);
	if (!list) continue;
	if (!trans)
	  {
	     if (!strcmp(_("None"), list)) 
	       {
		  e_widget_ilist_selected_set(cfdata->trans_list, i);
		  return;
	       }
	  }
	else
	  {
	     if (!strcmp(trans, list)) 
	       {
		  e_widget_ilist_selected_set(cfdata->trans_list, i);
		  return;
	       }
	  }
     }
   
   e_widget_ilist_unselect(cfdata->trans_list);
}

static void 
_trans_cb_changed(void *data) 
{
   E_Config_Dialog_Data *cfdata;
   const char *t;
   int sel;

   cfdata = data;
   sel = e_widget_ilist_selected_get(cfdata->event_list);

   t = e_widget_ilist_selected_label_get(cfdata->trans_list);
   if (!t) return;

   if (!strcmp(t, _("None"))) t = NULL;
   switch (sel) 
     {
      case 0:
	E_FREE(cfdata->transition_start);
	if (t) cfdata->transition_start = strdup(t);
	break;
      case 1:
	E_FREE(cfdata->transition_desk);
	if (t) cfdata->transition_desk = strdup(t);
	break;
      case 2:
	E_FREE(cfdata->transition_change);
	if (t) cfdata->transition_change = strdup(t);
	break;
      default:
	break;
     }
   if (!t) return;
   _trans_preview_trans_set(cfdata->tp, t);
}

Evas_Object *
_trans_preview_add(Evas *evas, int minw, int minh) 
{
   Evas_Object *obj, *o;
   E_Widget_Data *wd;
   
   obj = e_widget_add(evas);
   e_widget_del_hook_set(obj, _e_wid_del_hook);
   
   wd = calloc(1, sizeof(E_Widget_Data));
   wd->obj = obj;

   o = edje_object_add(evas);
   wd->o_frame = o;
   e_theme_edje_object_set(o, "base/theme/widgets", "e/widgets/preview");
   evas_object_show(o);
   e_widget_sub_object_add(obj, o);
   e_widget_resize_object_set(obj, o);
   
   o = edje_object_add(evas);
   wd->prev_bg = o;
   e_theme_edje_object_set(o, "base/theme/backgrounds", "e/desktop/background");
   evas_object_layer_set(o, -1);
   evas_object_clip_set(o, wd->o_frame);
   evas_object_show(o);
   edje_object_part_swallow(wd->o_frame, "e.swallow.content", wd->prev_bg);
   
   e_widget_data_set(obj, wd);
   e_widget_can_focus_set(obj, 0);
   e_widget_min_size_set(obj, minw, minh);

   return obj;
}

static void 
_trans_preview_trans_set(Evas_Object *obj, const char *trans) 
{
   Evas *evas;
   Evas_Object *o;
   E_Widget_Data *wd;
   char buf[4096];
   
   wd = e_widget_data_get(obj);
   evas = evas_object_evas_get(wd->o_frame);
   
   if (wd->o_trans)
     evas_object_del(wd->o_trans);
   if (wd->bg)
     evas_object_del(wd->bg);
   if (wd->prev_bg)
     evas_object_del(wd->prev_bg);
   
   snprintf(buf, sizeof(buf), "e/transitions/%s", trans);

   o = edje_object_add(evas);
   wd->o_trans = o;
   e_theme_edje_object_set(wd->o_trans, "base/theme/transitions", buf);
   edje_object_signal_callback_add(o, "e,state,done", "*", _e_wid_done, wd);
   evas_object_layer_set(o, -1);
   evas_object_clip_set(o, wd->o_frame);
   evas_object_show(o);
   edje_object_part_swallow(wd->o_frame, "e.swallow.content", wd->o_trans);

   o = edje_object_add(evas);
   wd->bg = o;
   e_theme_edje_object_set(o, "base/theme/icons", "e/icons/enlightenment/e");
   evas_object_layer_set(o, -1);
   evas_object_clip_set(o, wd->o_frame);
   evas_object_show(o);

   o = edje_object_add(evas);
   wd->prev_bg = o;
   e_theme_edje_object_set(o, "base/theme/backgrounds", "e/desktop/background");
   evas_object_layer_set(o, -1);
   evas_object_clip_set(o, wd->o_frame);
   evas_object_show(o);
   
   edje_object_part_swallow(wd->o_trans, "e.swallow.bg.old", wd->prev_bg);
   edje_object_part_swallow(wd->o_trans, "e.swallow.bg.new", wd->bg);
   edje_object_part_swallow(wd->o_frame, "e.swallow.content", wd->o_trans);
   
   edje_object_signal_emit(wd->o_trans, "e,action,start", "e");
}

static void 
_e_wid_del_hook(Evas_Object *obj) 
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   if (wd->o_frame)
     evas_object_del(wd->o_frame);
   if (wd->o_trans)
     evas_object_del(wd->o_trans);
   if (wd->bg)
     evas_object_del(wd->bg);
   if (wd->prev_bg)
     evas_object_del(wd->prev_bg);
   E_FREE(wd);
}

static void 
_e_wid_done(void *data, Evas_Object *obj, const char *emission, const char *source) 
{
   E_Widget_Data *wd;
   Evas_Object *o;
   Evas *evas;
   
   wd = data;
   evas = evas_object_evas_get(wd->o_frame);
   
   if (wd->o_trans) 
     evas_object_del(wd->o_trans);
   if (wd->bg)
     evas_object_del(wd->bg);
   if (wd->prev_bg)
     evas_object_del(wd->prev_bg);

   o = edje_object_add(evas);
   wd->prev_bg = o;
   e_theme_edje_object_set(o, "base/theme/backgrounds", "e/desktop/background");
   evas_object_layer_set(o, -1);
   evas_object_clip_set(o, wd->o_frame);
   evas_object_show(o);
   edje_object_part_swallow(wd->o_frame, "e.swallow.content", wd->prev_bg);
}
