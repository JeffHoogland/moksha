/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Font_Size_Data E_Font_Size_Data;
typedef struct _E_Text_Class_Pair E_Text_Class_Pair;
typedef struct _CFText_Class CFText_Class;

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

static void _basic_font_cb_change(void *data, Evas_Object *obj);
static void _basic_enable_cb_change(void *data, Evas_Object *obj);
static void _basic_init_data_fill(E_Config_Dialog_Data *cfdata);

static void _adv_class_cb_change(void *data, Evas_Object *obj);
static void _adv_enabled_font_cb_change(void *data, Evas_Object *obj);
static void _adv_enabled_fallback_cb_change(void *data, Evas_Object *obj);
static void _adv_font_cb_change(void *data, Evas_Object *obj);
static void _adv_style_cb_change(void *data, Evas_Object *obj);
static void _size_cb_change(void *data);

static int  _sort_fonts(const void *data1, const void *data2);
static void _font_list_load(E_Config_Dialog_Data *cfdata, const char *cur_font);
static void _size_list_load(E_Config_Dialog_Data *cfdata, Eina_List *size_list, Evas_Font_Size cur_size, int clear);
static void _class_list_load(E_Config_Dialog_Data *cfdata);
static void _font_preview_update(E_Config_Dialog_Data *cfdata);

static Eina_Bool _font_hash_cb(const Eina_Hash *hash __UNUSED__, const void *key __UNUSED__, void *data, void *fdata);

struct _E_Font_Size_Data
{
   E_Config_Dialog_Data *cfdata;
   const char           *size_str;
   Evas_Font_Size        size;
};

struct _E_Text_Class_Pair
{
   const char *class_name;
   const char *class_description;
};

struct _CFText_Class
{
   const char	  *class_name;
   const char	  *class_description;
   const char	  *font;
   const char     *style;
   Evas_Font_Size  size;
   unsigned char   enabled : 1;
};

const E_Text_Class_Pair text_class_predefined_names[ ] = {
     {  NULL,		  N_("Core")},
     { "title_bar",	  N_("Title Bar")},
     { "menu_item",	  N_("Menu Item")},
     { "menu_title",	  N_("Menu Title")},   
     { "tb_plain",	  N_("Textblock Plain")},
     { "tb_light",        N_("Textblock Light")},
     { "tb_big",          N_("Textblock Big")},
     { "move_text",       N_("Move Text")},
     { "resize_text",     N_("Resize Text")},
     { "winlist_title",   N_("Winlist Title")},
     { "configure",       N_("Configure Heading")},
     { "about_title",     N_("About Title")},
     { "about_version",   N_("About Version")},
     { "button_text",     N_("About Text")},
     { "desklock_title",  N_("Desklock Title")},
     { "desklock_passwd", N_("Desklock Password")},
     { "dialog_error",    N_("Dialog Error")},
     { "exebuf_command",  N_("Exebuf Command")},
     { "init_title",      N_("Splash Title")},
     { "init_text",       N_("Splash Text")},
     { "init_version",    N_("Splash Version")},
   
     {  NULL,		  N_("Widgets")},
     { "entry",           N_("Entry")},
     { "frame",           N_("Frame")},
     { "label",           N_("Label")},
     { "button",   	  N_("Buttons")},
     { "slider",	  N_("Slider")},
     { "radio_button",    N_("Radio Buttons")},
     { "check_button",    N_("Check Buttons")},
     { "tlist",           N_("Text List Item")},
     { "ilist_item",	  N_("List Item")},
     { "ilist_header",    N_("List Header")},

     {  NULL,		  N_("Filemanager")},
     { "fileman_typebuf", N_("Typebuf")},
     { "fileman_icon",    N_("Icon")},
     { "desktop_icon",    N_("Desktop Icon")},

     {  NULL,		  N_("Modules")},
     { "module_small",    N_("Small")},
     { "module_normal",   N_("Normal")},
     { "module_large",    N_("Large")},
     { "module_small_s",  N_("Small Styled")},
     { "module_normal_s", N_("Normal Styled")},
     { "module_large_s",  N_("Large Styled")},
   
     { NULL, NULL}
};

struct _E_Config_Dialog_Data
{
   E_Config_Dialog *cfd;
   Evas *evas;

   /* Text Classes */
   Eina_List	*text_classes;

   /* Font Data */
   Eina_Hash    *font_hash;
   Eina_List    *font_list;
   Eina_List    *font_px_list;
   Eina_List    *font_scale_list;

   /* Current data */
   char		*cur_font;
   char         *cur_style;
   double	 cur_size;
   int		 cur_enabled;
   int		 cur_index;
   
   /* Font Fallbacks */
   int		 cur_fallbacks_enabled;
   
   /* Font Hinting */
   int           hinting;
   
   struct
     {
	/* Font Classes */
	Evas_Object *class_list;
	Evas_Object *font_list;
	Evas_Object *style_list;
	Evas_Object *size_list;
	Evas_Object *enabled;
	Evas_Object *preview;

	/* Font Fallbacks */
	Evas_Object *fallback_list; /* Selecting a list entry starts edit*/
     } gui;
};

EAPI E_Config_Dialog *
e_int_config_fonts(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "_config_fonts_dialog")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);
   
   v->create_cfdata           = _create_data;
   v->free_cfdata             = _free_data;
   v->basic.create_widgets    = _basic_create_widgets;
   v->basic.apply_cfdata      = _basic_apply_data;
   v->advanced.create_widgets = _advanced_create_widgets;
   v->advanced.apply_cfdata   = _advanced_apply_data;
   
   cfd = e_config_dialog_new(con, _("Font Settings"),
			     "E", "_config_fonts_dialog",
			     "enlightenment/fonts", 0, v, NULL);
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   Eina_List *font_list;
   Eina_List *next;
   E_Font_Default *efd;
   E_Font_Size_Data *size_data;
   CFText_Class *tc;
   int i;
   
   font_list = e_font_default_list();
   
   /* Fill out the font list */
   for (i = 0; text_class_predefined_names[i].class_description; i++ )
     {
	tc = E_NEW(CFText_Class, 1);	
	tc->class_name = text_class_predefined_names[i].class_name;
	tc->class_description = 
          _(text_class_predefined_names[i].class_description);
	tc->font = NULL;
	tc->size = 0;
	tc->enabled = 0;

	if (text_class_predefined_names[i].class_name)
	  {
	     /* Search manually because we dont want to fallback */
	     for (next = font_list; next ; next = next->next)
	       {
		  efd = next->data;

		  if (!strcmp(tc->class_name, efd->text_class))
		    {
		       if (efd->font) 
			 {
			    E_Font_Properties *efp;

			    efp = e_font_fontconfig_name_parse(efd->font);
			    if (efp->name)
			      tc->font = eina_stringshare_add(efp->name);
			    if (efp->styles) 
			      tc->style = eina_stringshare_add(efp->styles->data);
			    e_font_properties_free(efp);
			 }
		       tc->size = efd->size;
		       tc->enabled = 1;
		    }
	       }

	     if (!tc->enabled)
	       {
		  /* search with default fallbacks */
		  efd = e_font_default_get(tc->class_name); 
		  if (efd)
		    { 
		       if (efd->font) 
			 {
			    E_Font_Properties *efp;

			    efp = e_font_fontconfig_name_parse(efd->font);
			    if (efp->name)
			      tc->font = eina_stringshare_add(efp->name);
			    if (efp->styles) 
			      tc->style = eina_stringshare_add(efp->styles->data);
			    e_font_properties_free(efp);
			 }
		       tc->size = efd->size;
		    }
	       }
	  }

	/* Append the class */
	cfdata->text_classes = eina_list_append(cfdata->text_classes, tc);
     }

   /* Fill Hinting */
   cfdata->hinting = e_config->font_hinting;

   size_data = E_NEW(E_Font_Size_Data, 1);
   size_data->cfdata = cfdata;
   size_data->size_str = eina_stringshare_add(_("Tiny"));
   size_data->size = -50;
   cfdata->font_scale_list = eina_list_append(cfdata->font_scale_list, size_data);

   size_data = E_NEW(E_Font_Size_Data, 1);
   size_data->cfdata = cfdata;
   size_data->size_str = eina_stringshare_add(_("Small"));
   size_data->size = -80;
   cfdata->font_scale_list = eina_list_append(cfdata->font_scale_list, size_data);

   size_data = E_NEW(E_Font_Size_Data, 1);
   size_data->cfdata = cfdata;
   size_data->size_str = eina_stringshare_add(_("Normal"));
   size_data->size = -100;
   cfdata->font_scale_list = eina_list_append(cfdata->font_scale_list, size_data);

   size_data = E_NEW(E_Font_Size_Data, 1);
   size_data->cfdata = cfdata;
   size_data->size_str = eina_stringshare_add(_("Big"));
   size_data->size = -150;
   cfdata->font_scale_list = eina_list_append(cfdata->font_scale_list, size_data);

   size_data = E_NEW(E_Font_Size_Data, 1);
   size_data->cfdata = cfdata;
   size_data->size_str = eina_stringshare_add(_("Really Big"));
   size_data->size = -190;
   cfdata->font_scale_list = eina_list_append(cfdata->font_scale_list, size_data);

   size_data = E_NEW(E_Font_Size_Data, 1);
   size_data->cfdata = cfdata;
   size_data->size_str = eina_stringshare_add(_("Huge"));
   size_data->size = -250;
   cfdata->font_scale_list = eina_list_append(cfdata->font_scale_list, size_data);

   for (i = 5; i < 21; i++)
     {
	char str[16];

	str[0] = 0;
	snprintf(str, sizeof(str), _("%d pixels"), i);

	size_data = E_NEW(E_Font_Size_Data, 1);   
	size_data->cfdata = cfdata;      
	size_data->size_str = eina_stringshare_add(str);         
	size_data->size = i;
	cfdata->font_px_list = eina_list_append(cfdata->font_px_list, size_data);
     }
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
   Eina_List *l;

   e_font_available_hash_free(cfdata->font_hash);
   while (cfdata->font_list)
     cfdata->font_list = eina_list_remove_list(cfdata->font_list, cfdata->font_list);

   while ((l = cfdata->text_classes))
     {
        CFText_Class *tc;

        tc = l->data;
        cfdata->text_classes = eina_list_remove_list(cfdata->text_classes, l);
        if (tc->font) eina_stringshare_del(tc->font);
	if (tc->style) eina_stringshare_del(tc->style);
        E_FREE(tc);
     }

   while ((l = cfdata->font_scale_list))
     {
        E_Font_Size_Data *size_data;

        size_data = l->data;
        cfdata->font_scale_list = eina_list_remove_list(cfdata->font_scale_list, l);
 	if (size_data->size_str) eina_stringshare_del(size_data->size_str);
        E_FREE(size_data);
     }

   while ((l = cfdata->font_px_list))
     {
        E_Font_Size_Data *size_data;

        size_data = l->data;
	cfdata->font_px_list = eina_list_remove_list(cfdata->font_scale_list, l);
        if (size_data->size_str) eina_stringshare_del(size_data->size_str);
        E_FREE(size_data);
     }

   E_FREE(cfdata->cur_font);
   E_FREE(cfdata->cur_style);
   E_FREE(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   Eina_List *next;
   int i;

   if (cfdata->cur_enabled && cfdata->cur_font == NULL)
     return 0;

   for (i = 0; text_class_predefined_names[i].class_description; i++ )
     {
	if (!text_class_predefined_names[i].class_name) continue; 

	if (cfdata->cur_enabled)
	  {
	     const char *class_name;
	     const char *font_name;

	     class_name = text_class_predefined_names[i].class_name;
	     font_name = 
               e_font_fontconfig_name_get(cfdata->cur_font, cfdata->cur_style);
	     e_font_default_set(class_name, font_name, cfdata->cur_size);

	     if (i == 1)
	       e_font_default_set("e_basic_font", font_name, cfdata->cur_size);

	     eina_stringshare_del(font_name);
	  }
	else
	  {
	     e_font_default_remove(text_class_predefined_names[i].class_name);
	     if (i == 1)
	       e_font_default_remove("e_basic_font");
	  }
     }

   e_font_apply();
   e_config_save_queue();

   /* Apply to advanced */
   for (next = cfdata->text_classes; next; next = next->next)
     {
	CFText_Class *tc;

	tc = next->data;
	tc->size = cfdata->cur_size;

	if (tc->font) eina_stringshare_del(tc->font);
	if (cfdata->cur_font)
	  tc->font = eina_stringshare_add(cfdata->cur_font);
	else
	  tc->font = NULL;

	if (tc->style) eina_stringshare_del(tc->style);
	if (cfdata->cur_style)
	  tc->style = eina_stringshare_add(cfdata->cur_style);
	else
	  tc->style = NULL;
	tc->enabled = cfdata->cur_enabled;
     }

   return 1;
}

static Evas_Bool
_font_hash_cb(const Eina_Hash *hash __UNUSED__, const void *key __UNUSED__, void *data, void *fdata)
{
   E_Config_Dialog_Data *cfdata;
   E_Font_Properties *efp;

   cfdata = fdata;
   efp = data;
   cfdata->font_list = eina_list_append(cfdata->font_list, efp->name);

   return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *ot, *ob, *of;

   cfdata->cur_index = -1;
   cfdata->evas = evas;

   ot = e_widget_table_add(evas, 0);

   cfdata->gui.class_list = NULL;

   ob = e_widget_check_add(evas, _("Enable Custom Font Classes"), 
                           &(cfdata->cur_enabled));
   cfdata->gui.enabled = ob;
   e_widget_on_change_hook_set(ob, _basic_enable_cb_change, cfdata);
   e_widget_disabled_set(ob, 0);
   e_widget_table_object_append(ot, ob, 0, 0, 1, 1, 1, 0, 1, 0);

   of = e_widget_framelist_add(evas, _("Fonts"), 1);
   ob = e_widget_ilist_add(evas, 16, 16, &(cfdata->cur_font));
   cfdata->gui.font_list = ob;
   e_widget_on_change_hook_set(ob, _basic_font_cb_change, cfdata);
   e_widget_ilist_go(ob);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 0, 1, 1, 1, 1, 1, 1, 1);

   of = e_widget_framelist_add(evas, _("Size"), 1);
   ob = e_widget_ilist_add(evas, 16, 16, NULL);
   cfdata->gui.size_list = ob;
   e_widget_ilist_go(ob);
   e_widget_min_size_set(ob, 100, 200);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 1, 1, 1, 1, 1, 1, 1, 1);

   ob = 
     e_widget_font_preview_add(evas, _("Basic preview text: 123: 我的天空！"));
   cfdata->gui.preview = ob;
   e_widget_table_object_append(ot, ob, 0, 2, 2, 1, 1, 0, 1, 0);

   _basic_init_data_fill(cfdata);

   e_dialog_resizable_set(cfd->dia, 1);
   return ot;
}

static void
_basic_font_cb_change(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   if (!cfdata) return;
   _font_preview_update(cfdata);
}

static void
_basic_enable_cb_change(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   if (!cfdata) return;

   e_widget_disabled_set(cfdata->gui.font_list, !cfdata->cur_enabled);
   e_widget_disabled_set(cfdata->gui.size_list, !cfdata->cur_enabled);

   if (!cfdata->cur_enabled)
     {
	e_widget_ilist_unselect(cfdata->gui.font_list);
	e_widget_ilist_unselect(cfdata->gui.size_list);
     }
}

/* fill the basic dialog with inital data and select it */
static void
_basic_init_data_fill(E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *ob;
   E_Font_Default *efd;
   const char *init_font;
   Evas_Font_Size init_size;

   init_font = NULL;
   init_size = -100;

   /* Get inital basic basic */
   efd = e_font_default_get("e_basic_font"); 
   if (efd)
     { 
	if (efd->font) 
	  {
	     E_Font_Properties *efp;

	     efp = e_font_fontconfig_name_parse(efd->font);
	     init_font = eina_stringshare_add(efp->name);
	     e_font_properties_free(efp);
	  }
	init_size = efd->size;		    
     }

   /* Check based on efd */
   ob = cfdata->gui.enabled;
   if (efd == NULL)
     e_widget_check_checked_set(ob, 0);
   else if (!strcmp(efd->text_class, "default"))
     e_widget_check_checked_set(ob, 0);
   else 
     e_widget_check_checked_set(ob, 1);

   /* Populate font list (Select current font) */
   _font_list_load(cfdata, init_font);

   /* Populate size list (Select current font) */
   _size_list_load(cfdata, cfdata->font_scale_list, init_size, 1);
}

static int
_advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   int i;
   Eina_List *next;
   CFText_Class *tc;

   /* Save current data */
   if (cfdata->cur_index >= 0)
     {
	tc = eina_list_nth(cfdata->text_classes, cfdata->cur_index);
	tc->enabled = cfdata->cur_enabled;
	tc->size = cfdata->cur_size;
	if (cfdata->cur_font)
	  tc->font = eina_stringshare_add(cfdata->cur_font);
	if (cfdata->cur_style)
	  tc->style = eina_stringshare_add(cfdata->cur_style);
     }

   for (next = cfdata->text_classes; next; next = next->next)
     {
	tc = next->data;
	if (!tc->class_name) continue;
	if (tc->enabled && tc->font) 
	  {
	     const char *name;

	     name = e_font_fontconfig_name_get(tc->font, tc->style);
	     e_font_default_set(tc->class_name, name, tc->size);
	     eina_stringshare_del(name);
	  }
	else
	  e_font_default_remove(tc->class_name);
     }

   /* Fallbacks configure */
   e_font_fallback_clear();

   if (cfdata->cur_fallbacks_enabled)
     {
	for (i = 0; i < e_widget_config_list_count(cfdata->gui.fallback_list); i++)
	  {
	     const char *fallback;

	     fallback = 
               e_widget_config_list_nth_get(cfdata->gui.fallback_list, i);
	     if ((fallback) && (fallback[0]))
	       e_font_fallback_append(fallback);
	  }
     }
   e_font_apply();

   /* Apply Hinting */
   e_config->font_hinting = cfdata->hinting;
   e_config_save_queue();
   e_canvas_rehint();
   return 1;
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *ot, *ob, *of;
   Evas_Coord w;
   E_Radio_Group *rg;
   Eina_List *next = NULL;
   int option_enable;

   cfdata->cur_index = -1;
   cfdata->evas = evas;

   ot = e_widget_table_add(evas, 0);
   of = e_widget_frametable_add(evas, _("Font Classes"), 0);
   ob = e_widget_ilist_add(evas, 16, 16, NULL);
   cfdata->gui.class_list = ob;
   _class_list_load(cfdata);
   e_widget_ilist_multi_select_set(ob, 1);
   e_widget_min_size_get(ob, &w, NULL);
   e_widget_min_size_set(ob, w, 180);
   e_widget_on_change_hook_set(ob, _adv_class_cb_change, cfdata);
   e_widget_frametable_object_append(of, ob, 0, 0, 1, 1, 1, 1, 1, 1);
   ob = e_widget_check_add(evas, _("Enable Font Class"), &(cfdata->cur_enabled));
   cfdata->gui.enabled = ob;
   e_widget_on_change_hook_set(ob, _adv_enabled_font_cb_change, cfdata);
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 0, 1, 1, 1, 1, 0, 1, 0);
   e_widget_table_object_append(ot, of, 0, 0, 1, 3, 1, 1, 1, 1);

   of = e_widget_framelist_add(evas, _("Fonts"), 1);
   ob = e_widget_ilist_add(evas, 16, 16, &(cfdata->cur_font));
   cfdata->gui.font_list = ob;
   e_widget_on_change_hook_set(ob, _adv_font_cb_change, cfdata);
   _font_list_load(cfdata, NULL);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 1, 0, 1, 3, 1, 1, 1, 1);

   of = e_widget_framelist_add(evas, _("Styles"), 1);
   ob = e_widget_ilist_add(evas, 16, 16, &(cfdata->cur_style));
   cfdata->gui.style_list = ob;
   e_widget_on_change_hook_set(ob, _adv_style_cb_change, cfdata);
   e_widget_ilist_go(ob);
   e_widget_min_size_set(ob, 90, 90);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 2, 0, 1, 1, 1, 1, 1, 1);

   of = e_widget_framelist_add(evas, _("Size"), 1);
   ob = e_widget_ilist_add(evas, 16, 16, NULL);
   cfdata->gui.size_list = ob;
   e_widget_ilist_go(ob);
   e_widget_min_size_set(ob, 90, 90);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 2, 1, 1, 2, 1, 1, 1, 1);

   of = e_widget_frametable_add(evas, _("Hinting"), 0);
   rg = e_widget_radio_group_new(&(cfdata->hinting));
   option_enable = evas_font_hinting_can_hint(evas, EVAS_FONT_HINTING_BYTECODE);
   ob = e_widget_radio_add(evas, _("Bytecode"), 0, rg);
   e_widget_disabled_set(ob, !option_enable);
   e_widget_frametable_object_append(of, ob, 0, 0, 1, 1, 1, 0, 1, 0);
   option_enable = evas_font_hinting_can_hint(evas, EVAS_FONT_HINTING_AUTO);
   ob = e_widget_radio_add(evas, _("Automatic"), 1, rg);
   e_widget_disabled_set(ob, !option_enable);
   e_widget_frametable_object_append(of, ob, 0, 1, 1, 1, 1, 0, 1, 0);
   option_enable = evas_font_hinting_can_hint(evas, EVAS_FONT_HINTING_NONE);
   ob = e_widget_radio_add(evas, _("None"), 2, rg);
   e_widget_disabled_set(ob, !option_enable);
   e_widget_frametable_object_append(of, ob, 0, 2, 1, 1, 1, 0, 1, 0);
   e_widget_table_object_append(ot, of, 3, 0, 1, 1, 1, 1, 1, 0);

   of = e_widget_framelist_add(evas, _("Font Fallbacks"), 0);
   ob = e_widget_config_list_add(evas, e_widget_entry_add, _("Fallback Name"), 2);
   cfdata->gui.fallback_list = ob;
   option_enable = 0;
   for (next = e_font_fallback_list(); next; next = next->next)
     {
	E_Font_Fallback *eff;

	eff = next->data;
	e_widget_config_list_append(ob, eff->name);
	option_enable = 1;
     }
   if (next) eina_list_free(next);
   ob = e_widget_check_add(evas, _("Enable Fallbacks"), &(cfdata->cur_fallbacks_enabled));
   e_widget_config_list_object_append(cfdata->gui.fallback_list, ob, 
				      0, 0, 2, 1, 1, 0, 1, 0);
   e_widget_on_change_hook_set(ob, _adv_enabled_fallback_cb_change, cfdata);
   e_widget_check_checked_set(ob, option_enable);
   e_widget_change(ob);
   e_widget_framelist_object_append(of, cfdata->gui.fallback_list);
   e_widget_table_object_append(ot, of, 3, 1, 1, 2, 1, 1, 1, 1);

   ob = e_widget_font_preview_add(evas, _("Advanced Preview Text.. 我真的会写中文"));
   cfdata->gui.preview = ob;
   e_widget_table_object_append(ot, ob, 0, 3, 4, 1, 1, 0, 1, 0);

   e_dialog_resizable_set(cfd->dia, 1);
   return ot;
}

/* Private Font Class Functions */
static void 
_class_list_load(E_Config_Dialog_Data *cfdata) 
{
   Evas *evas;
   Eina_List *next;

   if (!cfdata->gui.class_list) return;
   evas = evas_object_evas_get(cfdata->gui.class_list);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->gui.class_list);
   e_widget_ilist_clear(cfdata->gui.class_list);

   /* Fill in Class Ilist */
   for (next = cfdata->text_classes; next; next = next->next)
     {
	CFText_Class *tc;
	Evas_Object *ic;

	tc = next->data;
	if (tc)
	  {
	     if (tc->class_name)
	       {
		  if (tc->enabled)
		    {
		       ic = e_icon_add(evas);
		       e_util_icon_theme_set(ic, "dialog-ok-apply");
		    }
		  else
		    ic = NULL;
		  e_widget_ilist_append(cfdata->gui.class_list, ic, tc->class_description, NULL, NULL, NULL);
	       }
	     else
	       e_widget_ilist_header_append(cfdata->gui.class_list, NULL, tc->class_description);
	  }
     }
   e_widget_ilist_go(cfdata->gui.class_list);
   e_widget_ilist_thaw(cfdata->gui.class_list);
   edje_thaw();
   evas_event_thaw(evas);
}

/* Called whenever class list selection changes */
static void
_adv_class_cb_change(void *data, Evas_Object *obj)
{
   int indx;
   E_Config_Dialog_Data *cfdata;
   CFText_Class *tc;

   if (!(cfdata = data)) return;

   /* Save Current */
   if (cfdata->cur_index >= 0)
     {
	tc = eina_list_nth(cfdata->text_classes, cfdata->cur_index);
	tc->enabled = cfdata->cur_enabled;
	tc->size = cfdata->cur_size;
	if (tc->font) eina_stringshare_del(tc->font);
	if (cfdata->cur_font)
	  tc->font = eina_stringshare_add(cfdata->cur_font);
	else
	  tc->font = NULL;
	if (tc->style) eina_stringshare_del(tc->style);
	if (cfdata->cur_style)
	  tc->style = eina_stringshare_add(cfdata->cur_style);
	else
	  tc->style = NULL;
	if (cfdata->gui.style_list)
	  e_widget_ilist_unselect(cfdata->gui.style_list);
	if (cfdata->gui.size_list)
	  e_widget_ilist_unselect(cfdata->gui.size_list);
     }

   /* If no class is selected unselect all and return */
   indx = e_widget_ilist_selected_get(cfdata->gui.class_list);
   if (indx < 0) 
     {
	e_widget_disabled_set(cfdata->gui.enabled, 1);
	e_widget_disabled_set(cfdata->gui.font_list, 1);
	e_widget_disabled_set(cfdata->gui.size_list, 1);
	e_widget_check_checked_set(cfdata->gui.enabled, 0);
	if (cfdata->gui.font_list)
	  e_widget_ilist_unselect(cfdata->gui.font_list);
	return;
     }

   tc = eina_list_nth(cfdata->text_classes, indx);
   cfdata->cur_index = indx;

   e_widget_disabled_set(cfdata->gui.enabled, 0);
   e_widget_disabled_set(cfdata->gui.font_list, !tc->enabled);
   e_widget_disabled_set(cfdata->gui.size_list, !tc->enabled);
   e_widget_check_checked_set(cfdata->gui.enabled, tc->enabled);

   if (cfdata->gui.font_list) 
     {	
	/* Select the configured font */
	for (indx = 0; indx < e_widget_ilist_count(cfdata->gui.font_list); indx++) 
	  {
	     const char *f;

	     f = e_widget_ilist_nth_label_get(cfdata->gui.font_list, indx);
	     if (tc->font && !strcasecmp(f, tc->font)) 
	       {  
		  e_widget_ilist_selected_set(cfdata->gui.font_list, indx);
		  break;
	       }
	  }
     }
}

static void
_adv_enabled_font_cb_change(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata;
   Eina_List *l;
   int n;

   if (!(cfdata = data)) return;
   e_widget_disabled_set(cfdata->gui.font_list, !cfdata->cur_enabled);
   e_widget_disabled_set(cfdata->gui.style_list, !cfdata->cur_enabled);
   e_widget_disabled_set(cfdata->gui.size_list, !cfdata->cur_enabled);

   /* Search class list fot selected and enable Icon */ 
   for (n = 0, l = e_widget_ilist_items_get(cfdata->gui.class_list); l; l = l->next, n++) 
     {
	E_Ilist_Item *i;
	Evas_Object *icon = NULL;
	CFText_Class *tc;

        if (!(i = l->data)) continue;
	if (!i->selected) continue;

	tc = eina_list_nth(cfdata->text_classes, n);
	tc->enabled = cfdata->cur_enabled;
	tc->size = cfdata->cur_size;
	if (tc->font) eina_stringshare_del(tc->font);
	if (cfdata->cur_font)
	  tc->font = eina_stringshare_add(cfdata->cur_font);
	if (cfdata->cur_enabled) 
	  {
	     icon = edje_object_add(cfdata->evas);
	     e_util_edje_icon_set(icon, "enlightenment/e");
	  }
	e_widget_ilist_nth_icon_set(cfdata->gui.class_list, n, icon);
     }
}

static void
_size_cb_change(void *data)
{
   E_Config_Dialog_Data *cfdata;
   E_Font_Size_Data *size_data;
   Eina_List *l;
   int n;

   size_data = data;
   if (!(cfdata = size_data->cfdata)) return;

   cfdata->cur_size = size_data->size;

   _font_preview_update(cfdata);

   if (!cfdata->gui.class_list) return;

   for (n = 0, l = e_widget_ilist_items_get(cfdata->gui.class_list); l; l = l->next, n++) 
     {
	E_Ilist_Item *i;
	CFText_Class *tc;

        if (!(i = l->data)) continue;
	if (!i->selected) continue;

	tc = eina_list_nth(cfdata->text_classes, n);
	tc->size = cfdata->cur_size;
     }   
}

static void 
_adv_font_cb_change(void *data, Evas_Object *obj) 
{
   E_Config_Dialog_Data *cfdata;
   CFText_Class *tc;
   Eina_List *l;
   int n;

   tc = NULL;
   if (!(cfdata = data)) return;

   /* Set up the new font name for each selected class */
   for (n = 0, l = e_widget_ilist_items_get(cfdata->gui.class_list); l; l = l->next, n++) 
     {
	E_Ilist_Item *i;

        if (!(i = l->data)) continue;
	if (!i->selected) continue;

	tc = eina_list_nth(cfdata->text_classes, n);
	if (tc->font) eina_stringshare_del(tc->font);
	if (cfdata->cur_font)
	  tc->font = eina_stringshare_add(cfdata->cur_font);
     }

   /* Load the style list */
   if (cfdata->cur_font)
     {
	E_Font_Properties *efp;
	Eina_List *next;

	efp = eina_hash_find(cfdata->font_hash, cfdata->cur_font);
	evas_event_freeze(evas_object_evas_get(cfdata->gui.style_list));
	edje_freeze();
	e_widget_ilist_freeze(cfdata->gui.style_list);
	e_widget_ilist_clear(cfdata->gui.style_list);

	for (next = efp->styles; next; next = next->next)
	  {
	     const char *style;

	     style = next->data;
	     e_widget_ilist_append(cfdata->gui.style_list, NULL, style, 
                                   NULL, NULL, style);
	  }

	e_widget_ilist_go(cfdata->gui.style_list);
	e_widget_ilist_thaw(cfdata->gui.style_list);
	edje_thaw();
	evas_event_thaw(evas_object_evas_get(cfdata->gui.style_list));
     }

   /* select configured style from list */ 
   if (tc && tc->style) 
     {	
	for (n = 0; n < e_widget_ilist_count(cfdata->gui.style_list); n++) 
	  {
	     const char *f;

	     f = e_widget_ilist_nth_label_get(cfdata->gui.style_list, n);
	     if (!strcasecmp(f, tc->style)) 
	       {  
		  e_widget_ilist_selected_set(cfdata->gui.style_list, n);
		  break;
	       }
	  }
     }

   /* load and select size list */
   if (tc)
     {
	cfdata->cur_size = tc->size;
	_size_list_load(cfdata, cfdata->font_scale_list, tc->size, 1);
	_size_list_load(cfdata, cfdata->font_px_list, tc->size, 0);
     }

   _font_preview_update(cfdata);
}

static void
_size_list_load(E_Config_Dialog_Data *cfdata, Eina_List *size_list, Evas_Font_Size cur_size, int clear)
{
   Eina_List *next;
   Evas_Object *ob;
   Evas *evas;
   int n;

   ob = cfdata->gui.size_list;
   evas = evas_object_evas_get(ob);

   evas_event_freeze(evas);	
   edje_freeze();	
   e_widget_ilist_freeze(ob);
   if (clear) e_widget_ilist_clear(ob);

   for (next = size_list; next; next = next->next)	  
     {     
	E_Font_Size_Data *size_data;

	size_data = next->data;
	e_widget_ilist_append(ob, NULL, size_data->size_str, _size_cb_change, 
			      size_data, NULL);     
     }

   e_widget_ilist_go(ob);	
   e_widget_ilist_thaw(ob);	
   edje_thaw();
   evas_event_thaw(evas);

   for (n = 0; n < e_widget_ilist_count(ob); n++) 	  
     {
       E_Font_Size_Data *size_data;

       size_data = e_widget_ilist_nth_data_get(ob, n); 
       if (cur_size == size_data->size)
	  {
	     e_widget_ilist_selected_set(ob, n);
	     break;
	  }     
     }
}

static void
_font_list_load(E_Config_Dialog_Data *cfdata, const char *cur_font)
{
   int n;
   Eina_List *next;
   Evas_Object *ob;
   Evas *evas;
   Evas_Coord w;

   ob = cfdata->gui.font_list;
   evas = evas_object_evas_get(ob);

   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(ob);

   /* Load Hash a single time */
   if (cfdata->font_hash == NULL)
     {
	Eina_List *fonts;

	fonts = evas_font_available_list(evas);
	cfdata->font_hash = e_font_available_list_parse(fonts);
	eina_hash_foreach(cfdata->font_hash, _font_hash_cb, cfdata);
	if (cfdata->font_list)
	  {
	     cfdata->font_list = 
               eina_list_sort(cfdata->font_list, 
                              eina_list_count(cfdata->font_list), 
                              _sort_fonts);
	  }
	evas_font_available_list_free(evas, fonts);
     }

   /* Load the list */
   if (cfdata->font_list) 
     {
	Eina_List *next;

	for (next = cfdata->font_list; next; next = next->next) 
	  {
	     const char *f;

	     f = next->data;
	     e_widget_ilist_append(ob, NULL, f, NULL, NULL, f);
	  }
     }

   e_widget_ilist_go(ob);
   e_widget_min_size_get(ob, &w, NULL);
   e_widget_min_size_set(ob, w, 250);
   e_widget_ilist_thaw(ob);
   edje_thaw();
   evas_event_thaw(evas);

   if (!cur_font) return;

   /* Select Current Font */
   n = 0;
   for (next = cfdata->font_list; next; next = next->next) 
     {
	const char *f;

	f = next->data;
	if ((cur_font) && (!strcasecmp(f, cur_font)))
	  {  
	     e_widget_ilist_selected_set(ob, n);
	     break;
	  }     
	n++;	  
     }
}

static void 
_adv_style_cb_change(void *data, Evas_Object *obj) 
{
   E_Config_Dialog_Data *cfdata;
   Eina_List *l;
   int n;

   if (!(cfdata = data)) return;

   /* Setup the new style name */
   for (n = 0, l = e_widget_ilist_items_get(cfdata->gui.class_list); l; l = l->next, n++) 
     {
	E_Ilist_Item *i;
	CFText_Class *tc;

        if (!(i = l->data)) continue;
	if (!i->selected) continue;

	tc = eina_list_nth(cfdata->text_classes, n);
	if (tc->style) eina_stringshare_del(tc->style);
	if (cfdata->cur_style)
	  tc->style = eina_stringshare_add(cfdata->cur_style);
	else
	  tc->style = NULL;
     }

   _font_preview_update(cfdata);
}

/* Private Font Fallback Functions */
static void 
_adv_enabled_fallback_cb_change(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata;

   if (!(cfdata = data)) return;
   e_widget_disabled_set(cfdata->gui.fallback_list, 
                         !cfdata->cur_fallbacks_enabled); 
}

static int 
_sort_fonts(const void *data1, const void *data2) 
{
   if (!data1) return 1;
   if (!data2) return -1;
   return e_util_strcmp(data1, data2);
}

static void
_font_preview_update(E_Config_Dialog_Data *cfdata)
{
   /* update preview */
   if (cfdata->cur_font)
     {
	const char *name;     

	name = e_font_fontconfig_name_get(cfdata->cur_font, cfdata->cur_style);
	e_widget_font_preview_font_set(cfdata->gui.preview, name, 
                                       cfdata->cur_size);
	eina_stringshare_del(name);
     }
}
