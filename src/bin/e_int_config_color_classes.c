#include "e.h"

typedef struct _CFColor_Class CFColor_Class;
typedef struct _CFColor_Hash CFColor_Hash;

static void        *_create_data          (E_Config_Dialog *cfd);
static void         _free_data            (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int          _basic_apply_data     (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets (E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int          _adv_apply_data       (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_adv_create_widgets   (E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

static void         _load_color_classes   (Evas_Object *obj, E_Config_Dialog_Data *cfdata);
static void         _radio_cb_change      (void *data, Evas_Object *obj, void *event_info);
static void         _list_cb_change       (void *data, Evas_Object *obj);
static void         _update_colors        (E_Config_Dialog_Data *cfdata, CFColor_Class *cc);
static void         _color1_cb_change     (void *data, Evas_Object *obj);
static void         _color2_cb_change     (void *data, Evas_Object *obj);
static void         _color3_cb_change     (void *data, Evas_Object *obj);

struct _CFColor_Hash 
{
   const char *key;
   const char *name;
};

struct _CFColor_Class 
{
   const char *key;
   const char *name;
   int enabled;

   int r, g, b, a;
   int r2, g2, b2, a2;
   int r3, g3, b3, a3;   
};

struct _E_Config_Dialog_Data 
{
   char *cur_class;
   int state;
   E_Color *color1, *color2, *color3;
   Evas_List *classes;
   struct 
     {
	Evas_Object *ilist;
	Evas_Object *renable, *rdisable;
	Evas_Object *c1, *c2, *c3;
     } gui;
};

/* Key Pairs for color classes
 * 
 * These can/should be changed to "official" key/names
 */
const CFColor_Hash _color_hash[] = 
{
     {NULL,                 N_("Window Manager")},
     {"about_title",        N_("About Dialog Title")},
     {"about_version",      N_("About Dialog Version")},
     {"menu_title_default", N_("Menu Title")},
     {"menu_title_active",  N_("Menu Title Active")},
     {"menu_item_default",  N_("Menu Item")},

     {NULL, N_("Widgets")},
     {"button_text_enabled",  N_("Button Text Enabled")},
     {"button_text_disabled", N_("Button Text Disabled")},

     {NULL, N_("Modules")},
     {"ibar_label_default", N_("IBar Label")},
     {"ibar_label_active",  N_("IBar Label Active")},
     {NULL, NULL}
};

EAPI E_Config_Dialog *
e_int_config_color_classes(E_Container *con) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return NULL;
   
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->advanced.apply_cfdata = _adv_apply_data;
   v->advanced.create_widgets = _adv_create_widgets;
   
   cfd = e_config_dialog_new(con, _("Colors"), "E", "_config_color_classes",
			     "enlightenment/themes", 0, v, NULL);
   return cfd;
}

static void 
_fill_data(E_Config_Dialog_Data *cfdata) 
{
   Evas_List *l, *cclist;
   Evas_Hash *color_hash;
   int i = 0;

   cclist = edje_color_class_list();
   cfdata->cur_class = NULL;
   cfdata->state = 0;
   
   for (i = 0; _color_hash[i].name; i++) 
     {
	CFColor_Class *cfc;
	E_Color_Class *cc;

	cfc = E_NEW(CFColor_Class, 1);
	cfc->enabled = 0;
	cfc->key = NULL;

	if (_color_hash[i].key) 
	  {
	     cfc->key = evas_stringshare_add(_color_hash[i].key);
	     cfc->name = evas_stringshare_add(_(_color_hash[i].name));
	     cc = e_color_class_find(cfc->key);
	     if (cc) 
	       {
		  cfc->enabled = 1;
		  cfc->r = cc->r;
		  cfc->g = cc->g;
		  cfc->b = cc->b;
		  cfc->a = cc->a;
		  cfc->r2 = cc->r2;
		  cfc->g2 = cc->g2;
		  cfc->b2 = cc->b2;
		  cfc->a2 = cc->a2;
		  cfc->r3 = cc->r3;
		  cfc->g3 = cc->g3;
		  cfc->b3 = cc->b3;
		  cfc->a3 = cc->a3;
	       }
	     else 
	       {
		  cfc->r = 255;
		  cfc->g = 255;
		  cfc->b = 255;
		  cfc->a = 255;
		  cfc->r2 = 0;
		  cfc->g2 = 0;
		  cfc->b2 = 0;
		  cfc->a2 = 255;
		  cfc->r3 = 0;
		  cfc->g3 = 0;
		  cfc->b3 = 0;
		  cfc->a3 = 255;
	       }
	  }
	else 
	  cfc->name = evas_stringshare_add(_color_hash[i].name);
	
	cfdata->classes = evas_list_append(cfdata->classes, cfc);
     }
   
   cfdata->color1 = calloc(1, sizeof(E_Color));
   cfdata->color2 = calloc(1, sizeof(E_Color));
   cfdata->color3 = calloc(1, sizeof(E_Color));
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
   while (cfdata->classes) 
     {
	CFColor_Class *cfc;
	
	cfc = cfdata->classes->data;
	if (!cfc) continue;
	if (cfc->name)
	  evas_stringshare_del(cfc->name);
	if (cfc->key)
	  evas_stringshare_del(cfc->key);
	
	cfdata->classes = evas_list_remove_list(cfdata->classes, cfdata->classes);
     }
   E_FREE(cfdata->color1);
   E_FREE(cfdata->color2);
   E_FREE(cfdata->color3);
   E_FREE(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   Evas_List *l;
   
   for (l = cfdata->classes; l; l = l->next) 
     {
	CFColor_Class *c;
	E_Color_Class *cc;
	
	c = l->data;
	if (!c) continue;
	if (!c->key) continue;

	cc = e_color_class_find(c->key);
	if (!c->enabled) 
	  {
	     if (cc)
	       e_color_class_del(cc->name);
	  }
	else 
	  {
	     if (cc) 
	       {
		  e_color_class_set(cc->name, c->r, c->g, c->b, c->a,
				    c->r2, c->g2, c->b2, c->a2,
				    c->r3, c->g3, c->b3, c->a3);
	       }
	     else 
	       {
		  e_color_class_set(c->key, c->r, c->g, c->b, c->a,
				    c->r2, c->g2, c->b2, c->a2,
				    c->r3, c->g3, c->b3, c->a3);
	       }
	  }
     }
   return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *of, *ob, *ot;
   E_Radio_Group *rg;
   Evas_List *l;

   o = e_widget_list_add(evas, 0, 0);
   ot = e_widget_table_add(evas, 1);
   
   of = e_widget_framelist_add(evas, _("Color Classes"), 0);
   e_widget_framelist_content_align_set(of, 0.0, 0.0);
   ob = e_widget_ilist_add(evas, 16, 16, NULL);
   cfdata->gui.ilist = ob;
   e_widget_on_change_hook_set(ob, _list_cb_change, cfdata);
   _load_color_classes(ob, cfdata);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 0, 0, 1, 4, 1, 1, 1, 1);

   of = e_widget_framelist_add(evas, _("State"), 0);
   e_widget_framelist_content_align_set(of, 0.0, 0.0);
   rg = e_widget_radio_group_new(&(cfdata->state));
   ob = e_widget_radio_add(evas, _("Enabled"), 1, rg);
   cfdata->gui.renable = ob;
   evas_object_smart_callback_add(ob, "changed", _radio_cb_change, cfdata);   
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Disabled"), 0, rg);
   cfdata->gui.rdisable = ob;
   evas_object_smart_callback_add(ob, "changed", _radio_cb_change, cfdata);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 1, 0, 1, 1, 1, 1, 1, 1);
   
   e_widget_list_object_append(o, ot, 1, 1, 0.5);
   e_dialog_resizable_set(cfd->dia, 1);
   return o;
}

static int
_adv_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   Evas_List *l;
   
   for (l = cfdata->classes; l; l = l->next) 
     {
	CFColor_Class *c;
	E_Color_Class *cc;
	
	c = l->data;
	if (!c) continue;
	if (!c->key) continue;

	cc = e_color_class_find(c->key);
	if (!c->enabled) 
	  {
	     if (cc)
	       e_color_class_del(cc->name);
	  }
	else 
	  {
	     printf("Alpha: %i\n", c->a);
	     printf("Alpha2: %i\n", c->a2);
	     printf("Alpha3: %i\n", c->a3);
	     if (cc) 
	       {
		  e_color_class_set(cc->name, c->r, c->g, c->b, c->a,
				    c->r2, c->g2, c->b2, c->a2,
				    c->r3, c->g3, c->b3, c->a3);
	       }
	     else 
	       {
		  e_color_class_set(c->key, c->r, c->g, c->b, c->a,
				    c->r2, c->g2, c->b2, c->a2,
				    c->r3, c->g3, c->b3, c->a3);
	       }
	  }
     }
   return 1;
}

static Evas_Object *
_adv_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *of, *ob, *ot;
   E_Radio_Group *rg;
   Evas_List *l;
   
   o = e_widget_list_add(evas, 0, 0);
   ot = e_widget_table_add(evas, 1);
   
   of = e_widget_framelist_add(evas, _("Color Classes"), 0);
   e_widget_framelist_content_align_set(of, 0.0, 0.0);
   ob = e_widget_ilist_add(evas, 16, 16, NULL);
   cfdata->gui.ilist = ob;
   e_widget_on_change_hook_set(ob, _list_cb_change, cfdata);
   _load_color_classes(ob, cfdata);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 0, 0, 1, 4, 1, 1, 1, 1);
   
   of = e_widget_framelist_add(evas, _("Object Color"), 1);
   ob = e_widget_color_well_add(evas, cfdata->color1, 1);
   cfdata->gui.c1 = ob;
   e_widget_on_change_hook_set(ob, _color1_cb_change, cfdata);
   e_widget_disabled_set(ob, 1);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 1, 1, 1, 1, 1, 1, 1, 1);

   of = e_widget_framelist_add(evas, _("Outline Color"), 1);
   ob = e_widget_color_well_add(evas, cfdata->color2, 1);
   cfdata->gui.c2 = ob;
   e_widget_on_change_hook_set(ob, _color2_cb_change, cfdata);
   e_widget_disabled_set(ob, 1);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 1, 2, 1, 1, 1, 1, 1, 1);

   of = e_widget_framelist_add(evas, _("Shadow Color"), 1);
   ob = e_widget_color_well_add(evas, cfdata->color3, 1);
   cfdata->gui.c3 = ob;
   e_widget_on_change_hook_set(ob, _color3_cb_change, cfdata);
   e_widget_disabled_set(ob, 1);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 1, 3, 1, 1, 1, 1, 1, 1);

   of = e_widget_framelist_add(evas, _("State"), 0);
   e_widget_framelist_content_align_set(of, 0.0, 0.0);
   rg = e_widget_radio_group_new(&(cfdata->state));
   ob = e_widget_radio_add(evas, _("Enabled"), 1, rg);
   cfdata->gui.renable = ob;
   evas_object_smart_callback_add(ob, "changed", _radio_cb_change, cfdata);   
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Disabled"), 0, rg);
   cfdata->gui.rdisable = ob;
   evas_object_smart_callback_add(ob, "changed", _radio_cb_change, cfdata);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 1, 0, 1, 1, 1, 1, 1, 1);
   
   e_widget_list_object_append(o, ot, 1, 1, 0.5);
   e_dialog_resizable_set(cfd->dia, 1);
   return o;
}

static void 
_load_color_classes(Evas_Object *obj, E_Config_Dialog_Data *cfdata) 
{
   Evas_List *l;
   Evas_Coord w, h;
   
   e_widget_ilist_clear(obj);
   for (l = cfdata->classes; l; l = l->next) 
     {
	CFColor_Class *cfc;
	Evas_Object *icon;
	
	cfc = l->data;
	if (!cfc) continue;
	if (!cfc->key) 
	  e_widget_ilist_header_append(obj, NULL, cfc->name);
	else 
	  {
	     if (cfc->enabled) 
	       {
		  icon = edje_object_add(evas_object_evas_get(obj));
		  e_util_edje_icon_set(icon, "enlightenment/e");
	       }
	     else
	       icon = NULL;

	     e_widget_ilist_append(obj, icon, cfc->name, NULL, NULL, NULL);
	  }
     }
   e_widget_ilist_go(obj);
   e_widget_min_size_get(obj, &w, &h);
   e_widget_min_size_set(obj, w, 275);
}

static void 
_radio_cb_change(void *data, Evas_Object *obj, void *event_info) 
{
   E_Config_Dialog_Data *cfdata;
   Evas_List *l;
   Evas_Object *icon;
   int n;
   
   cfdata = data;
   if (!cfdata) return;

   n = e_widget_ilist_selected_get(cfdata->gui.ilist);
   
   for (l = cfdata->classes; l; l = l->next) 
     {
	CFColor_Class *c;
	
	c = l->data;
	if (!c) continue;
	if (!c->key) continue;
	if (strcmp(c->name, cfdata->cur_class)) continue;
	c->enabled = cfdata->state;
	if (c->enabled) 
	  {
	     icon = edje_object_add(evas_object_evas_get(cfdata->gui.ilist));
	     e_util_edje_icon_set(icon, "enlightenment/e");
	  }
	else
	  icon = NULL;

	e_widget_ilist_nth_icon_set(cfdata->gui.ilist, n, icon);
	break;
     }

   if (!cfdata->gui.c1) return;   
   if (cfdata->state == 0) 
     {
	e_widget_disabled_set(cfdata->gui.c1, 1);
	e_widget_disabled_set(cfdata->gui.c2, 1);
	e_widget_disabled_set(cfdata->gui.c3, 1);
     }
   else if (cfdata->state == 1) 
     {
	e_widget_disabled_set(cfdata->gui.c1, 0);
	e_widget_disabled_set(cfdata->gui.c2, 0);
	e_widget_disabled_set(cfdata->gui.c3, 0);
     }
}

static void 
_list_cb_change(void *data, Evas_Object *obj) 
{
   E_Config_Dialog_Data *cfdata;
   Evas_List *l;
   
   cfdata = data;
   if (!cfdata) return;
   
   cfdata->cur_class = (char *)e_widget_ilist_selected_label_get(obj);
   
   for (l = cfdata->classes; l; l = l->next) 
     {
	CFColor_Class *c;
	int enable = 0;
	
	c = l->data;
	if (!c) continue;
	if (!c->key) continue;
	if (!strcmp(c->name, cfdata->cur_class)) 
	  {
	     enable = c->enabled;
	     cfdata->state = enable;
	     if (enable) 
	       {
		  e_widget_disabled_set(cfdata->gui.c1, 0);
		  e_widget_disabled_set(cfdata->gui.c2, 0);
		  e_widget_disabled_set(cfdata->gui.c3, 0);
		  e_widget_radio_toggle_set(cfdata->gui.renable, 1);
		  e_widget_radio_toggle_set(cfdata->gui.rdisable, 0);
	       }
	     else 
	       {
		  e_widget_disabled_set(cfdata->gui.c1, 1);
		  e_widget_disabled_set(cfdata->gui.c2, 1);
		  e_widget_disabled_set(cfdata->gui.c3, 1);
		  e_widget_radio_toggle_set(cfdata->gui.renable, 0);
		  e_widget_radio_toggle_set(cfdata->gui.rdisable, 1);
	       }
	     _update_colors(cfdata, c);
	     break;
	  }	
     }   
}

static void 
_update_colors(E_Config_Dialog_Data *cfdata, CFColor_Class *cc) 
{
   cfdata->color1->a = cc->a;
   cfdata->color1->r = cc->r;
   cfdata->color1->g = cc->g;
   cfdata->color1->b = cc->b;

   cfdata->color2->a = cc->a2;
   cfdata->color2->r = cc->r2;
   cfdata->color2->g = cc->g2;
   cfdata->color2->b = cc->b2;

   cfdata->color3->a = cc->a3;
   cfdata->color3->r = cc->r3;
   cfdata->color3->g = cc->g3;
   cfdata->color3->b = cc->b3;

   e_color_update_rgb(cfdata->color1);
   e_color_update_rgb(cfdata->color2);
   e_color_update_rgb(cfdata->color3);
   
   e_widget_color_well_update(cfdata->gui.c1);
   e_widget_color_well_update(cfdata->gui.c2);
   e_widget_color_well_update(cfdata->gui.c3);
}

static void 
_color1_cb_change(void *data, Evas_Object *obj) 
{
   E_Config_Dialog_Data *cfdata;
   Evas_List *l;
   
   cfdata = data;
   if (!cfdata) return;

   e_color_update_rgb(cfdata->color1);
   
   for (l = cfdata->classes; l; l = l->next) 
     {
	CFColor_Class *c;
	
	c = l->data;
	if (!c) continue;
	if (!c->key) continue;
	if (!strcmp(c->name, cfdata->cur_class)) 
	  {
	     c->r = cfdata->color1->r;
	     c->g = cfdata->color1->g;
	     c->b = cfdata->color1->b;
	     c->a = cfdata->color1->a;
	     break;
	  }
     }
}

static void 
_color2_cb_change(void *data, Evas_Object *obj) 
{
   E_Config_Dialog_Data *cfdata;
   Evas_List *l;
   
   cfdata = data;
   if (!cfdata) return;

   e_color_update_rgb(cfdata->color2);
   
   for (l = cfdata->classes; l; l = l->next) 
     {
	CFColor_Class *c;
	
	c = l->data;
	if (!c) continue;
	if (!c->key) continue;
	if (!strcmp(c->name, cfdata->cur_class)) 
	  {
	     c->r2 = cfdata->color2->r;
	     c->g2 = cfdata->color2->g;
	     c->b2 = cfdata->color2->b;
	     c->a2 = cfdata->color2->a;
	     break;
	  }
     }
}

static void 
_color3_cb_change(void *data, Evas_Object *obj) 
{
   E_Config_Dialog_Data *cfdata;
   Evas_List *l;
   
   cfdata = data;
   if (!cfdata) return;

   e_color_update_rgb(cfdata->color3);
   
   for (l = cfdata->classes; l; l = l->next) 
     {
	CFColor_Class *c;
	
	c = l->data;
	if (!c) continue;
	if (!c->key) continue;
	if (!strcmp(c->name, cfdata->cur_class)) 
	  {
	     c->r3 = cfdata->color3->r;
	     c->g3 = cfdata->color3->g;
	     c->b3 = cfdata->color3->b;
	     c->a3 = cfdata->color3->a;
	     break;
	  }
     }
}
