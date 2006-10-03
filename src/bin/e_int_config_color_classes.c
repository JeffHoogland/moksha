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
};

struct _E_Config_Dialog_Data 
{
   char *cur_class;
   int state;
   E_Color *color1, *color2, *color3;
   Evas_List *classes;
};

const CFColor_Hash _color_hash[] = 
{
     {NULL, N_("Window Manager")},
     {"about_title", N_("About Dialog Title")},
     {"about_version", N_("About Dialog Version")},
     {"menu_title_default", N_("Menu Title")},
     {"menu_title_active", N_("Menu Title Active")},
     {"menu_item_default", N_("Menu Item")},

     {NULL, N_("Widgets")},
     {"button_text_enabled", N_("Button Text Enabled")},
     {"button_text_disabled", N_("Button Text Disabled")},

     {NULL, N_("Modules")},
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
	     cc = e_color_class_find(cfc->name);
	     if (cc) 
	       cfc->enabled = 1;
	  }
	else 
	  cfc->name = evas_stringshare_add(_color_hash[i].name);
	
	cfdata->classes = evas_list_append(cfdata->classes, cfc);
     }
   
   cfdata->color1 = calloc(1, sizeof(E_Color));
   cfdata->color1->a = 255;
   cfdata->color1->r = 255;
   cfdata->color1->g = 255;
   cfdata->color1->b = 255;
   
   cfdata->color2 = calloc(1, sizeof(E_Color));
   cfdata->color2->a = 255;
   cfdata->color2->r = 0;
   cfdata->color2->g = 0;
   cfdata->color2->b = 0;

   cfdata->color3 = calloc(1, sizeof(E_Color));
   cfdata->color3->a = 255;
   cfdata->color3->r = 0;
   cfdata->color3->g = 0;
   cfdata->color3->b = 0;

   e_color_update_rgb(cfdata->color1);
   e_color_update_rgb(cfdata->color2);
   e_color_update_rgb(cfdata->color3);
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
   return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *of, *ob;
   Evas_List *l;
   
   o = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("Color Classes"), 0);
   e_widget_framelist_content_align_set(of, 0.0, 0.0);
   ob = e_widget_ilist_add(evas, 16, 16, &(cfdata->cur_class));
   _load_color_classes(ob, cfdata);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   return o;
}

static int
_adv_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
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
   ob = e_widget_ilist_add(evas, 16, 16, &(cfdata->cur_class));
   _load_color_classes(ob, cfdata);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 0, 0, 1, 4, 1, 1, 1, 1);
   
   of = e_widget_framelist_add(evas, _("State"), 0);
   e_widget_framelist_content_align_set(of, 0.0, 0.0);
   rg = e_widget_radio_group_new(&(cfdata->state));
   ob = e_widget_radio_add(evas, _("Enabled"), 1, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Disabled"), 0, rg);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 1, 0, 1, 1, 1, 1, 1, 1);

   of = e_widget_framelist_add(evas, _("Object Color"), 1);
   ob = e_widget_color_well_add(evas, cfdata->color1, 1);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 1, 1, 1, 1, 1, 1, 1, 1);

   of = e_widget_framelist_add(evas, _("Outline Color"), 1);
   ob = e_widget_color_well_add(evas, cfdata->color2, 1);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 1, 2, 1, 1, 1, 1, 1, 1);

   of = e_widget_framelist_add(evas, _("Shadow Color"), 1);
   ob = e_widget_color_well_add(evas, cfdata->color3, 1);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 1, 3, 1, 1, 1, 1, 1, 1);
   
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
	     printf("Adding: %s\n", cfc->name);
	     e_widget_ilist_append(obj, icon, cfc->name, NULL, NULL, NULL);
	  }
     }
   e_widget_ilist_go(obj);
   e_widget_min_size_get(obj, &w, &h);
   e_widget_min_size_set(obj, w, 275);
}
