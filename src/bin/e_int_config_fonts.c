/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

#define ILIST_ICON_WITH_DEFINED_FONT	"enlightenment/e"
#define ILIST_ICON_WITHOUT_DEFINED_FONT ""

typedef struct _E_Text_Class_Pair E_Text_Class_Pair;
typedef struct _CFText_Class CFText_Class;

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

static void _ilist_font_cb_change(void *data, Evas_Object *obj);
static void _enabled_font_cb_change(void *data, Evas_Object *obj);

static void _enabled_fallback_cb_change(void *data, Evas_Object *obj);

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
   double	 size;
   unsigned char enabled : 1;
};

const E_Text_Class_Pair text_class_predefined_names[ ] = {
       {  NULL,		    N_("Window Manager")},
       { "title_bar",	    N_("Title Bar")},
       { "menu_item",	    N_("Menu Item")},
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
   /* Current data */
   char		*cur_font;
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

   v = E_NEW(E_Config_Dialog_View, 1);
   
   v->create_cfdata           = _create_data;
   v->free_cfdata             = _free_data;
   v->basic.create_widgets    = _basic_create_widgets;
   v->basic.apply_cfdata      = _basic_apply_data;
   
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
	     for (next = font_list; next ; next = next->next)
	       {
		  efd = next->data;
		  
		  if (!strcmp(tc->class_name, efd->text_class))
		    {
		       if (efd->font)
			 tc->font = evas_stringshare_add(efd->font);
		       else
			 tc->font = evas_stringshare_add("");
		       
		       tc->size = efd->size;
		       tc->enabled = 1;
		    }
	       }
	     
	     if (!tc->enabled)
	       {
		  efd = e_font_default_get(tc->class_name); 
		  if (efd)
		    { 
		       if (efd->font)
			 tc->font = evas_stringshare_add(efd->font);
		       else
			 tc->font = evas_stringshare_add("");
		       
		       tc->size = efd->size;
		    }
		  else 
		    {
		       tc->font = evas_stringshare_add("");
		    }
	       }
	  }
	
	/* Append the class */
	cfdata->text_classes = evas_list_append(cfdata->text_classes, tc);
     }

   /* Fill Hinting */
   cfdata->hinting = e_config->font_hinting;
   
   /* Font fallbacks configured in widgets */
   
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
        E_FREE(tc);
     }
   
   E_FREE(cfdata->cur_font);
   free(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
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
	  {
	     e_font_default_set(tc->class_name, tc->font, tc->size);
	  }
	else
	  {
	     e_font_default_remove(tc->class_name);
	  }
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
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ob;
   E_Radio_Group *rg;
   Evas_List *next;
   int option_enable;

   cfdata->cur_index = -1;
   cfdata->evas = evas;
   o = e_widget_list_add(evas, 0, 0);
  
   /* Create Font Class Widgets */ 
   of = e_widget_frametable_add(evas, _("Font Class Configuration"), 0);
   cfdata->gui.class_list = e_widget_ilist_add(evas, 16, 16, NULL);
   e_widget_min_size_set(cfdata->gui.class_list, 100, 100);
   e_widget_on_change_hook_set(cfdata->gui.class_list, _ilist_font_cb_change, cfdata);

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
		       e_util_edje_icon_set(ic, ILIST_ICON_WITH_DEFINED_FONT);
		    }
		  else
		    {
		       ic = NULL;
		    }
		  e_widget_ilist_append(cfdata->gui.class_list, ic, tc->class_description, NULL, NULL, NULL);
	       }
	     else
	       {
		  e_widget_ilist_header_append(cfdata->gui.class_list, NULL, tc->class_description);
	       }
	  }
     }

   e_widget_ilist_go(cfdata->gui.class_list);
   e_widget_frametable_object_append(	of, 
					cfdata->gui.class_list, 
					0, /* Col Start*/
					0, /* Row Start */
					1, /* Col Span*/
					5, /* Row Span*/
					1, 1, 1, 1);
  
   ob = e_widget_label_add(evas, _("Font"));
   e_widget_frametable_object_append(	of, 
					ob, 
					1, 0, 1, 1,
					1, 1, 1, 1);
   
   cfdata->gui.font = e_widget_entry_add(evas, &(cfdata->cur_font));
   e_widget_disabled_set(cfdata->gui.font, 1);
   e_widget_min_size_set(cfdata->gui.font, 100, 25);
   e_widget_frametable_object_append(of, cfdata->gui.font, 
					2, 0, 1, 1, 
					1, 1, 1, 1);
     
   ob = e_widget_label_add(evas, _("Font Size"));
   e_widget_frametable_object_append(	of, 
					ob, 
					1, 1, 1, 1,
					1, 1, 1, 1);
   
   cfdata->gui.size = e_widget_slider_add(evas, 1, 0, _("%2.1f pixels"), 5.0, 25.0, 0.5, 0, &(cfdata->cur_size), NULL, 25);
   e_widget_disabled_set(cfdata->gui.size, 1);
   e_widget_min_size_set(cfdata->gui.size, 180, 25);
   e_widget_frametable_object_append(of, cfdata->gui.size, 
					2, 1, 1, 1,
					1, 1, 1, 1);

   cfdata->gui.enabled = e_widget_check_add(evas, _("Enable Font Class"), &(cfdata->cur_enabled));
   e_widget_disabled_set(cfdata->gui.enabled, 1);
   e_widget_frametable_object_append(of, cfdata->gui.enabled, 
					1, 3, 2, 1, 
					1, 1, 1, 1);
   e_widget_on_change_hook_set(cfdata->gui.enabled, _enabled_font_cb_change, cfdata);
   
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
   e_widget_config_list_object_append(	cfdata->gui.fallback_list, 
					ob, 
					0, 3, 2, 1, 
					1, 1, 1, 1);
   e_widget_on_change_hook_set(ob, _enabled_fallback_cb_change, cfdata);
   e_widget_check_checked_set(ob, option_enable);
   e_widget_change(ob);
   
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   return o;
}

/* Private Font Class Functions */

static void
_ilist_font_cb_change(void *data, Evas_Object *obj)
{
   int indx;
   E_Config_Dialog_Data *cfdata;
   CFText_Class *tc;
   
   cfdata = data;
   if (!cfdata) return;
  
   /* Save old data */
   if (cfdata->cur_index >= 0)
     {
	tc = evas_list_nth(cfdata->text_classes, cfdata->cur_index);
	tc->enabled = cfdata->cur_enabled;
	tc->size = cfdata->cur_size;
	if (cfdata->cur_font)
	  tc->font = evas_stringshare_add(cfdata->cur_font);
     }
   
   /* Fillout form with new data */ 
   indx = e_widget_ilist_selected_get(cfdata->gui.class_list);
   tc = evas_list_nth(cfdata->text_classes, indx);
   cfdata->cur_index = indx;

   e_widget_disabled_set(cfdata->gui.enabled, 0);
   e_widget_disabled_set(cfdata->gui.font, !tc->enabled);
   e_widget_disabled_set(cfdata->gui.size, !tc->enabled);
   
   e_widget_check_checked_set(cfdata->gui.enabled, tc->enabled);
   e_widget_entry_text_set(cfdata->gui.font, tc->font);
   e_widget_slider_value_double_set(cfdata->gui.size, tc->size);
   
}

static void
_enabled_font_cb_change(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata;
   Evas_Object *icon;
   int n;
   
   cfdata = data;   
   if (!cfdata) return;
   
   n = e_widget_ilist_selected_get(cfdata->gui.class_list);
   if (cfdata->cur_enabled)
     {
	e_widget_disabled_set(cfdata->gui.font, 0);
	e_widget_disabled_set(cfdata->gui.size, 0);
	icon = edje_object_add(cfdata->evas);
	e_util_edje_icon_set(icon, ILIST_ICON_WITH_DEFINED_FONT);
     }
   else
     {
	e_widget_disabled_set(cfdata->gui.font, 1);
	e_widget_disabled_set(cfdata->gui.size, 1);
	icon = NULL;
     }
   e_widget_ilist_nth_icon_set(cfdata->gui.class_list, n, icon);
}

/* Private Font Fallback Functions */
static void _enabled_fallback_cb_change(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
   if (!cfdata) return;
  
   e_widget_disabled_set(cfdata->gui.fallback_list, !cfdata->cur_fallbacks_enabled); 
}

