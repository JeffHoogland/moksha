/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Text_Class_Pair E_Text_Class_Pair;
typedef struct _CFText_Class CFText_Class;

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static void _fill_ilist(E_Config_Dialog_Data *cfdata);

static void _ilist_font_cb_change(void *data, Evas_Object *obj);
static void _enabled_font_cb_change(void *data, Evas_Object *obj);
static void _enabled_fallback_cb_change(void *data, Evas_Object *obj);
static int  _sort_fonts(void *data1, void *data2);
static void _size_cb_change(void *data, Evas_Object *obj);
static void _font_cb_change(void *data, Evas_Object *obj);
static void _style_cb_change(void *data, Evas_Object *obj);

struct _E_Text_Class_Pair
{
   const char	*class_name;
   const char	*class_description;
};

struct _CFText_Class
{
   const char	*class_name;
   const char	*class_description;
   const char	*font;
   const char   *style;
   double	 size;
   unsigned char enabled : 1;
};

const E_Text_Class_Pair text_class_predefined_names[ ] = {
       {  NULL,		    N_("Window Manager")},
       { "title_bar",	    N_("Title Bar")},
       { "menu_item",	    N_("Menu Item")},
       { "menu_title",	    N_("Menu Title")},   
       { "tb_plain",	    N_("Textblock Plain")},
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
       
       {  NULL,		    N_("Widget")},
       { "entry",           N_("Entry")},
       { "frame",           N_("Frame")},
       { "label",           N_("Label")},
       { "button",   	    N_("Buttons")},
       { "slider",	    N_("Slider")},
       { "radio_button",    N_("Radio Buttons")},
       { "check_button",    N_("Check Buttons")},
       { "tlist",           N_("Text List Item")},
       { "ilist_item",	    N_("List Item")},
       { "ilist_header",    N_("List Header")},
     
       {  NULL,		    N_("EFM")},
       { "fileman_typebuf", N_("Typebuf")},
       { "fileman_icon",    N_("Icon")},
       
       {  NULL,		    N_("Module")},
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
   Evas_List	*text_classes;

   /* Font Data */
   Evas_Hash    *font_hash;
   Evas_List    *font_list;
   Evas_List    *style_list;

   /* Current data */
   char		*cur_font;
   char         *cur_style;
   double	 cur_size;
   int		 cur_enabled;
   int		 cur_index;
   
   /* Font Fallbacks */
   int		 cur_fallbacks_enabled;
   
   /* Font Hinting */
   int hinting;
   
   struct
     {
	/* Font Classes */
	Evas_Object *class_list;
	Evas_Object *font_list;
	Evas_Object *style_list;
	   
	Evas_Object *font;
	Evas_Object *size;
	Evas_Object *enabled;

	/* Font Fallbacks */
	Evas_Object *fallback_list; /* Selecting a list entry starts edit*/
     }
   gui;
};

EAPI E_Config_Dialog *
e_int_config_fonts(E_Container *con)
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
   
   cfd = e_config_dialog_new(con,
			     _("Font Settings"),
			     "E", "_config_fonts_dialog",
			     "enlightenment/fonts", 0, v, NULL);
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   Evas_List *font_list;
   Evas_List *next;
   E_Font_Default *efd;
   CFText_Class *tc;
   int i;
   
   font_list = e_font_default_list();
   
   /* Fill out the font list */
   for (i = 0; text_class_predefined_names[i].class_description; i++ )
     {
	tc = E_NEW(CFText_Class, 1);	
	tc->class_name = text_class_predefined_names[i].class_name;
	tc->class_description = _(text_class_predefined_names[i].class_description);
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
			    tc->font = evas_stringshare_add(efp->name);
			    if (efp->styles) tc->style = efp->styles->data;
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
			    tc->font = evas_stringshare_add(efp->name);
			    if (efp->styles) tc->style = efp->styles->data;

			 }
		       tc->size = efd->size;
		    }
	       }
	  }
	
	/* Append the class */
	cfdata->text_classes = evas_list_append(cfdata->text_classes, tc);
     }
   
   /* Fill Hinting */
   cfdata->hinting = e_config->font_hinting;
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
   Evas_List *l;

   while ((l = cfdata->text_classes))
     {
        CFText_Class *tc;

        tc = l->data;
        cfdata->text_classes = evas_list_remove_list(cfdata->text_classes, l);
        if (tc->font) evas_stringshare_del(tc->font);
	if (tc->style) evas_stringshare_del(tc->style);
        E_FREE(tc);
     }
   
   E_FREE(cfdata->cur_font);
   E_FREE(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   Evas_List *next;
   CFText_Class *tc;
      
   /* Save current data */
   if (cfdata->cur_index >= 0)
     {
	tc = evas_list_nth(cfdata->text_classes, cfdata->cur_index);
	tc->enabled = cfdata->cur_enabled;
	tc->size = cfdata->cur_size;
	if (cfdata->cur_font)
	  tc->font = evas_stringshare_add(cfdata->cur_font);
	if (cfdata->cur_style)
	  tc->style = evas_stringshare_add(cfdata->cur_style);
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
	     printf("Adding: %s = %s\n", tc->class_name, name);
	     evas_stringshare_del(name);
	  }
	else
	  e_font_default_remove(tc->class_name);
     }

   e_font_apply();
   e_config_save_queue();
   return 1;
}

static Evas_Bool
_font_hash_cb(Evas_Hash *hash, const char *key, void *data, void *fdata)
{
   E_Config_Dialog_Data *cfdata;
   E_Font_Properties *efp;

   cfdata = fdata;
   efp = data;
   cfdata->font_list = evas_list_append(cfdata->font_list, efp->name);

   return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ob, *ot, *ott;
   Evas_List *fonts, *l = NULL;

   cfdata->cur_index = -1;
   cfdata->evas = evas;
   cfdata->gui.font = NULL;

   o = e_widget_list_add(evas, 0, 0);
   ot = e_widget_table_add(evas, 1);
   
   /* Create Font Class Widgets */ 
   of = e_widget_framelist_add(evas, _("Font Classes"), 0);
   cfdata->gui.class_list = e_widget_ilist_add(evas, 16, 16, NULL);
   e_widget_ilist_multi_select_set(cfdata->gui.class_list, 1);
   e_widget_min_size_set(cfdata->gui.class_list, 80, 250);
   e_widget_on_change_hook_set(cfdata->gui.class_list, _ilist_font_cb_change, cfdata);
   _fill_ilist(cfdata);
   e_widget_framelist_object_append(of, cfdata->gui.class_list);
   e_widget_table_object_append(ot, of, 0, 0, 1, 5, 1, 1, 1, 1);

   ott = e_widget_table_add(evas, 0);
   cfdata->gui.enabled = e_widget_check_add(evas, _("Enable Font Class"), &(cfdata->cur_enabled));
   e_widget_disabled_set(cfdata->gui.enabled, 1);
   e_widget_table_object_append(ott, cfdata->gui.enabled, 0, 0, 3, 1, 1, 0, 1, 0);
   e_widget_on_change_hook_set(cfdata->gui.enabled, _enabled_font_cb_change, cfdata);

   ob = e_widget_label_add(evas, _("Font Size:"));
   e_widget_table_object_append(ott, ob, 0, 1, 1, 1, 1, 0, 1, 0);
   
   cfdata->gui.size = e_widget_slider_add(evas, 1, 0, _("%2.1f pixels"), 5.0, 25.0, 0.5, 0, &(cfdata->cur_size), NULL, 25);
   e_widget_disabled_set(cfdata->gui.size, 1);
   e_widget_min_size_set(cfdata->gui.size, 80, 25);
   e_widget_on_change_hook_set(cfdata->gui.size, _size_cb_change, cfdata);
   e_widget_table_object_append(ott, cfdata->gui.size, 
				1, 1, 2, 1, 1, 0, 1, 0);
   e_widget_table_object_append(ot, ott, 
				1, 0, 2, 1, 1, 1, 1, 1);
   /* Font List */
   of = e_widget_framelist_add(evas, _("Fonts"), 1);
   cfdata->gui.font_list = e_widget_ilist_add(evas, 16, 16, &(cfdata->cur_font));
   e_widget_on_change_hook_set(cfdata->gui.font_list, _font_cb_change, cfdata);
   evas_event_freeze(evas_object_evas_get(cfdata->gui.font_list));
   edje_freeze();
   e_widget_ilist_freeze(cfdata->gui.font_list);

   if (cfdata->font_hash == NULL)
     {
	fonts = evas_font_available_list(evas);
	cfdata->font_hash = e_font_available_list_parse(fonts);
	evas_hash_foreach(cfdata->font_hash, _font_hash_cb, cfdata);
	evas_font_available_list_free(evas, fonts);
     }

   l = cfdata->font_list;

   if (l) 
     {
	l = evas_list_sort(l, evas_list_count(l), _sort_fonts);
	for (fonts = l; fonts; fonts = fonts->next) 
	  {
	     char *f;
	     
	     f = fonts->data;
	     e_widget_ilist_append(cfdata->gui.font_list, NULL, f, NULL, NULL, f);
	  }
     }
   
   e_widget_ilist_go(cfdata->gui.font_list);
   e_widget_min_size_set(cfdata->gui.font_list, 75, 200);
   e_widget_ilist_thaw(cfdata->gui.font_list);
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(cfdata->gui.font_list));
   e_widget_framelist_object_append(of, cfdata->gui.font_list);
   e_widget_table_object_append(ot, of, 1, 1, 1, 4, 1, 1, 1, 1);

   /* Empty Style List */
   of = e_widget_framelist_add(evas, _("Styles"), 1);
   cfdata->gui.style_list = e_widget_ilist_add(evas, 16, 16, &(cfdata->cur_style));
   e_widget_on_change_hook_set(cfdata->gui.style_list, _style_cb_change, cfdata);
   e_widget_ilist_go(cfdata->gui.style_list);
   e_widget_min_size_set(cfdata->gui.style_list, 75, 200);
   e_widget_framelist_object_append(of, cfdata->gui.style_list);

   e_widget_table_object_append(ot, of, 2, 1, 1, 4, 1, 1, 1, 1);
   
   e_widget_list_object_append(o, ot, 1, 1, 0.5);
   return o;
}

static int
_advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   int i;
   Evas_List *next;
   CFText_Class *tc;
  
   /* Save current data */
   if (cfdata->cur_index >= 0)
     {
	tc = evas_list_nth(cfdata->text_classes, cfdata->cur_index);
	tc->enabled = cfdata->cur_enabled;
	tc->size = cfdata->cur_size;
	if (cfdata->cur_font)
	  tc->font = evas_stringshare_add(cfdata->cur_font);
     }
   
   for (next = cfdata->text_classes; next; next = next->next)
     {
	tc = next->data;
	if (!tc->class_name) continue;
	if (tc->enabled && tc->font) 
	  e_font_default_set(tc->class_name, tc->font, tc->size);
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
	     fallback = e_widget_config_list_nth_get(cfdata->gui.fallback_list, i);
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
   Evas_Object *o, *of, *ob;
   E_Radio_Group *rg;
   Evas_List *next;
   int option_enable;

   cfdata->cur_index = -1;
   cfdata->evas = evas;
   cfdata->gui.font_list = NULL;
   
   o = e_widget_list_add(evas, 0, 0);
  
   /* Create Font Class Widgets */ 
   of = e_widget_frametable_add(evas, _("Font Class Configuration"), 0);
   cfdata->gui.class_list = e_widget_ilist_add(evas, 16, 16, NULL);
   e_widget_ilist_multi_select_set(cfdata->gui.class_list, 1);
   e_widget_min_size_set(cfdata->gui.class_list, 100, 100);
   e_widget_on_change_hook_set(cfdata->gui.class_list, _ilist_font_cb_change, cfdata);
   _fill_ilist(cfdata);
   e_widget_frametable_object_append(of, cfdata->gui.class_list, 
				     0, 0, 1, 5, 1, 1, 1, 1);

   cfdata->gui.enabled = e_widget_check_add(evas, _("Enable Font Class"), &(cfdata->cur_enabled));
   e_widget_disabled_set(cfdata->gui.enabled, 1);
   e_widget_frametable_object_append(of, cfdata->gui.enabled, 
				     1, 0, 2, 1, 1, 1, 1, 1);
   e_widget_on_change_hook_set(cfdata->gui.enabled, _enabled_font_cb_change, cfdata);
   
   ob = e_widget_label_add(evas, _("Font"));
   e_widget_frametable_object_append(of, ob, 
				     1, 2, 1, 1, 1, 1, 1, 1);
   
   cfdata->gui.font = e_widget_entry_add(evas, &(cfdata->cur_font));
   e_widget_on_change_hook_set(cfdata->gui.font, _font_cb_change, cfdata);
   e_widget_disabled_set(cfdata->gui.font, 1);
   e_widget_min_size_set(cfdata->gui.font, 100, 25);
   e_widget_frametable_object_append(of, cfdata->gui.font, 
				     2, 2, 1, 1, 1, 1, 1, 1);
     
   ob = e_widget_label_add(evas, _("Font Size"));
   e_widget_frametable_object_append(of, ob, 
				     1, 3, 1, 1, 1, 1, 1, 1);
   
   cfdata->gui.size = e_widget_slider_add(evas, 1, 0, _("%2.1f pixels"), 5.0, 25.0, 0.5, 0, &(cfdata->cur_size), NULL, 25);
   e_widget_on_change_hook_set(cfdata->gui.size, _size_cb_change, cfdata);
   e_widget_disabled_set(cfdata->gui.size, 1);
   e_widget_min_size_set(cfdata->gui.size, 180, 25);
   e_widget_frametable_object_append(of, cfdata->gui.size, 
				     2, 3, 1, 1, 1, 1, 1, 1);
   
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   /* Create Hinting Widgets */
   of = e_widget_framelist_add(evas, _("Hinting"), 1);
   e_widget_framelist_content_align_set(of, 0.5, 0.5);
   rg = e_widget_radio_group_new(&(cfdata->hinting));
   
   option_enable = evas_font_hinting_can_hint(evas, EVAS_FONT_HINTING_BYTECODE);
   ob = e_widget_radio_add(evas, _("Bytecode"), 0, rg);
   e_widget_disabled_set(ob, !option_enable);
   e_widget_framelist_object_append(of, ob);

   option_enable = evas_font_hinting_can_hint(evas, EVAS_FONT_HINTING_AUTO);
   ob = e_widget_radio_add(evas, _("Automatic"), 1, rg);
   e_widget_disabled_set(ob, !option_enable);
   e_widget_framelist_object_append(of, ob);

   option_enable = evas_font_hinting_can_hint(evas, EVAS_FONT_HINTING_NONE);
   ob = e_widget_radio_add(evas, _("None"), 2, rg);
   e_widget_disabled_set(ob, !option_enable);
   e_widget_framelist_object_append(of, ob);
   
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   /* Create Fallbacks Widgets */
   of = e_widget_framelist_add(evas, _("Font Fallbacks"), 0);
   e_widget_framelist_content_align_set(of, 0.5, 0.5);

   ob = e_widget_config_list_add(evas, e_widget_entry_add, _("Fallback Name"), 2);
   cfdata->gui.fallback_list = ob;
   e_widget_framelist_object_append(of, ob);
   
   /* Fill In Ilist */
   option_enable = 0;
   for (next = e_font_fallback_list(); next; next = next->next)
     {
	E_Font_Fallback *eff;
	
	eff = next->data;
	e_widget_config_list_append(ob, eff->name);
	option_enable = 1;
     }
   
   ob = e_widget_check_add(evas, _("Enable Fallbacks"), &(cfdata->cur_fallbacks_enabled));
   e_widget_config_list_object_append(cfdata->gui.fallback_list, ob, 
				      0, 3, 2, 1, 1, 1, 1, 1);
   e_widget_on_change_hook_set(ob, _enabled_fallback_cb_change, cfdata);
   e_widget_check_checked_set(ob, option_enable);
   e_widget_change(ob);
   
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   return o;
}

/* Private Font Class Functions */
static void 
_fill_ilist(E_Config_Dialog_Data *cfdata) 
{
   Evas *evas;
   Evas_List *next;
   
   if (!cfdata->gui.class_list) return;
   evas = evas_object_evas_get(cfdata->gui.class_list);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->gui.class_list);
   e_widget_ilist_clear(cfdata->gui.class_list);
   
   /* Fill In Ilist */
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
		       ic = edje_object_add(evas);
		       e_util_edje_icon_set(ic, "enlightenment/e");
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

static void
_ilist_font_cb_change(void *data, Evas_Object *obj)
{
   int indx;
   E_Config_Dialog_Data *cfdata;
   CFText_Class *tc;
   
   cfdata = data;
   if (!cfdata) return;
   
   /* Save Current */
   if (cfdata->cur_index >= 0)
     {
	tc = evas_list_nth(cfdata->text_classes, cfdata->cur_index);
	tc->enabled = cfdata->cur_enabled;
	tc->size = cfdata->cur_size;
	if (tc->font) evas_stringshare_del(tc->font);
	if (cfdata->cur_font)
	  tc->font = evas_stringshare_add(cfdata->cur_font);
	if (tc->style) evas_stringshare_del(tc->style);
	if (cfdata->cur_style)
	  tc->style = evas_stringshare_add(cfdata->cur_style);
	if (cfdata->gui.style_list)
	  e_widget_ilist_unselect(cfdata->gui.style_list);
     }
   
   indx = e_widget_ilist_selected_get(cfdata->gui.class_list);
   if (indx < 0) 
     {
	e_widget_disabled_set(cfdata->gui.enabled, 1);
	e_widget_disabled_set(cfdata->gui.font, 1);
	e_widget_disabled_set(cfdata->gui.size, 1);
	e_widget_check_checked_set(cfdata->gui.enabled, 0);
	if (cfdata->gui.font_list)
	  e_widget_ilist_unselect(cfdata->gui.font_list);
	return;
     }
   
   tc = evas_list_nth(cfdata->text_classes, indx);
   cfdata->cur_index = indx;

   e_widget_disabled_set(cfdata->gui.enabled, 0);
   e_widget_disabled_set(cfdata->gui.font, !tc->enabled);
   e_widget_disabled_set(cfdata->gui.size, !tc->enabled);
   e_widget_check_checked_set(cfdata->gui.enabled, tc->enabled);

   if (tc) printf("Trying to setup f:%s, s:%s\n", tc->font, tc->style);
   /* Basic GUI Parts */
   if (cfdata->gui.font_list) 
     {	
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
   /* Advanced GUI */
   if (cfdata->gui.font)
     e_widget_entry_text_set(cfdata->gui.font, tc->font);
   e_widget_slider_value_double_set(cfdata->gui.size, tc->size);
}

static void
_enabled_font_cb_change(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata;
   Evas_List *l;
   int n;
   
   cfdata = data;   
   if (!cfdata) return;
   e_widget_disabled_set(cfdata->gui.font, !cfdata->cur_enabled);
   e_widget_disabled_set(cfdata->gui.size, !cfdata->cur_enabled);
  
   /* Search class list fot selected and enable Icon */ 
   for (n = 0, l = e_widget_ilist_items_get(cfdata->gui.class_list); l; l = l->next, n++) 
     {
	E_Ilist_Item *i;
	Evas_Object *icon = NULL;
	CFText_Class *tc;

	i = l->data;
	if (!i) continue;
	if (!i->selected) continue;

	tc = evas_list_nth(cfdata->text_classes, n);
	tc->enabled = cfdata->cur_enabled;
	tc->size = cfdata->cur_size;
	if (tc->font) evas_stringshare_del(tc->font);
	if (cfdata->cur_font)
	  tc->font = evas_stringshare_add(cfdata->cur_font);
	if (cfdata->cur_enabled) 
	  {
	     icon = edje_object_add(cfdata->evas);
	     e_util_edje_icon_set(icon, "enlightenment/e");
	  }
	e_widget_ilist_nth_icon_set(cfdata->gui.class_list, n, icon);
     }
}

static void
_size_cb_change(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata;
   Evas_List *l;
   int n;
   
   cfdata = data;
   if (!cfdata) return;
   
   for (n = 0, l = e_widget_ilist_items_get(cfdata->gui.class_list); l; l = l->next, n++) 
     {
	E_Ilist_Item *i;
	CFText_Class *tc;

	i = l->data;
	if (!i) continue;
	if (!i->selected) continue;

	tc = evas_list_nth(cfdata->text_classes, n);
	tc->size = cfdata->cur_size;
     }
}

static void 
_font_cb_change(void *data, Evas_Object *obj) 
{
   E_Config_Dialog_Data *cfdata;
   CFText_Class *tc;
   Evas_List *l;
   int n;
   
   tc = NULL;
   cfdata = data;
   if (!cfdata) return;
 
   /* Set up the new font name */
   for (n = 0, l = e_widget_ilist_items_get(cfdata->gui.class_list); l; l = l->next, n++) 
     {
	E_Ilist_Item *i;
	
	i = l->data;
	if (!i) continue;
	if (!i->selected) continue;

	tc = evas_list_nth(cfdata->text_classes, n);
	if (tc->font) evas_stringshare_del(tc->font);
	if (cfdata->cur_font)
	  tc->font = evas_stringshare_add(cfdata->cur_font);

	printf("Current Font %s\n", cfdata->cur_font);
     }

   printf("Changed Font\n");  

   /* Load the style list */
   if (cfdata->cur_font)
     {
	E_Font_Properties *efp;
	Evas_List *next;

	efp = evas_hash_find(cfdata->font_hash, cfdata->cur_font);
	evas_event_freeze(evas_object_evas_get(cfdata->gui.style_list));
	edje_freeze();
	e_widget_ilist_freeze(cfdata->gui.style_list);

	e_widget_ilist_clear(cfdata->gui.style_list);
	for (next = efp->styles; next; next = next->next)
	  {
	     const char *style;
	     style = next->data;
	     e_widget_ilist_append(cfdata->gui.style_list, NULL, style, NULL, NULL, style);
	  }

	e_widget_ilist_go(cfdata->gui.style_list);
	e_widget_ilist_thaw(cfdata->gui.style_list);
	edje_thaw();
	evas_event_thaw(evas_object_evas_get(cfdata->gui.style_list));
     }

   printf("Trying to change style!\n");
   
   if (tc && tc->style) 
     {	
	printf("Changing style %s\n", tc->style);
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
}

static void 
_style_cb_change(void *data, Evas_Object *obj) 
{
   E_Config_Dialog_Data *cfdata;
   Evas_List *l;
   int n;
   
   cfdata = data;
   if (!cfdata) return;
   
   /* Setup the new style name */
   for (n = 0, l = e_widget_ilist_items_get(cfdata->gui.class_list); l; l = l->next, n++) 
     {
	E_Ilist_Item *i;
	CFText_Class *tc;

	i = l->data;
	if (!i) continue;
	if (!i->selected) continue;

	tc = evas_list_nth(cfdata->text_classes, n);
	if (tc->style) evas_stringshare_del(tc->style);
	if (cfdata->cur_style)
	  tc->style = evas_stringshare_add(cfdata->cur_style);

	printf("Cur style %s\n", cfdata->cur_style);
     }
}

/* Private Font Fallback Functions */
static void 
_enabled_fallback_cb_change(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
   if (!cfdata) return;
  
   e_widget_disabled_set(cfdata->gui.fallback_list, !cfdata->cur_fallbacks_enabled); 
}

static int 
_sort_fonts(void *data1, void *data2) 
{
   if (!data1) return 1;
   if (!data2) return -1;
   
   return e_util_strcmp(data1, data2);
}
