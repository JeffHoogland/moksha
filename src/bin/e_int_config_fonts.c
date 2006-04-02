/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

#define CLASS_ENABLED 0
#define CLASS_DISABLED 1

typedef struct _E_Text_Class_Pair E_Text_Class_Pair;
typedef struct _CFText_Class CFText_Class;
typedef struct _E_Widget_Data E_Widget_Data;

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

static void _ilist_cb_change(void *data, Evas_Object *obj);
static void _enabled_cb_change(void *data, Evas_Object *obj);


struct _E_Widget_Data
{
   E_Radio_Group *group;
   Evas_Object *o_radio;
   int valnum;
};

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
       { "title_bar",	    "Title Bar"},
       { "menu_item",	    "Menu Item"},
       { "ilist_item",	    "List Item"},
       { "ilist_header",    "List Header"},
       { "tb_plain",	    "Textblock Plain"},
       { "tb_light",        "Textblock Light"},
       { "tb_big",          "Textblock Big"},
       { "frame",           "Frame"},
       { "label",           "Label"},
       { "button",   	    "Buttons"},
       { "radio_button",    "Radio Buttons"},
       { "check_button",    "Check Buttons"},
       { "move_text",       "Move Text"},
       { "resize_text",     "Resize Text"},
       { "tlist",           "Text List Item"},
       { "winlist_title",   "Winlist Title"},
       { "configure",       "Configure Heading"},
       { "about_title",     "About Title"},
       { "about_version",   "About Version"},
       { "button_text",     "About Text"},
       { "desklock_title",  "Desklock Title"},
       { "desklock_passwd", "Desklock Password"},
       { "dialog_error",    "Dialog Error"},
       { "exebuf_command",  "Exebuf Command"},
       { "fileman_typebuf", "EFM Typebuf"},
       { "fileman_icon",    "EFM Icon"},
       { NULL,		NULL}
};
     
struct _E_Config_Dialog_Data
{
   E_Config_Dialog *cfd;

   /* Text Classes */
   Evas_List	*text_classes;
   /* Current data */
   char		*cur_font;
   double	 cur_size;
   int		 cur_enabled;
   int		 cur_index;
   
   /* Font Fallbacks */
   Evas_List *font_fallbacks;
   int fallbacks_state;
   
   /* Font Hinting */
   int hinting;
   
   struct
     {
	Evas_Object *class_list;
	   
	Evas_Object *font;
	Evas_Object *size;
	Evas_Object *enabled;
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
   
   cfd = e_config_dialog_new(con, _("Font Settings"), NULL, 0, v, NULL);
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   Evas_List *enabled_list;
   Evas_List *next;
   E_Font_Default *efd;
   CFText_Class *tc;
   int i;
   
   enabled_list = e_font_default_list();
   
   /* Fill out the font list */
   for (i = 0; text_class_predefined_names[i].class_name; i++ )
     {
	tc = E_NEW(CFText_Class, 1);
	
	tc->class_name = text_class_predefined_names[i].class_name;
	tc->class_description = _(text_class_predefined_names[i].class_description);
	tc->font = NULL;
	tc->size = 0;
	tc->enabled = 0;
	
	for (next = enabled_list; next ; next = next->next)
	  {
	     efd = next->data;
	     
	     if (!strcmp(tc->class_name, efd->text_class))
	       {
		tc->font = evas_stringshare_add(efd->font);
		tc->size = efd->size;
		tc->enabled = 1;
	       }
	  }

	if (!tc->enabled)
	  {
	     efd = e_font_default_get(tc->class_name); 
	     if (efd)
	       {
		  tc->font = evas_stringshare_add(efd->font);
		  tc->size = efd->size;
	       }
	     else 
	       {
		  tc->font = evas_stringshare_add("");
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
        evas_stringshare_del(tc->font);
        E_FREE(tc);
     }
   free(cfdata);
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
     }
   
   for (next = cfdata->text_classes; next; next = next->next)
     {
	tc = next->data;

	if (tc->enabled && tc->font) 
	  {
	     e_font_default_set(tc->class_name, tc->font, tc->size);
	  }
	else
	  {
	     e_font_default_remove(tc->class_name);
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
   int option_enable;
   int i;

   cfdata->cur_index = -1;
   o = e_widget_list_add(evas, 0, 0);
  
   /* Create Font Class Widgets */ 
   of = e_widget_frametable_add(evas, _("Font Class Configuration"), 0);
   cfdata->gui.class_list = e_widget_ilist_add(evas, 16, 16, NULL);
   e_widget_min_size_set(cfdata->gui.class_list, 100, 100);
   e_widget_on_change_hook_set(cfdata->gui.class_list, _ilist_cb_change, cfdata);

   /* Fill In Ilist */
   for (i = 0; i < evas_list_count(cfdata->text_classes); i++)
     {
	CFText_Class *tc;
	Evas_Object *ic;
	
	tc = evas_list_nth(cfdata->text_classes, i);
	if (tc)
	  {
	     ic = edje_object_add(evas);
	     if (tc->enabled)
	       e_util_edje_icon_set(ic, "enlightenment/e");
	     else
	       e_util_edje_icon_set(ic, "");
	     e_widget_ilist_append(cfdata->gui.class_list, ic, tc->class_description, NULL, NULL, NULL);
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
   e_widget_on_change_hook_set(cfdata->gui.enabled, _enabled_cb_change, cfdata);
   
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   /* Create Hinting Widgets */
   of = e_widget_framelist_add(evas, _("Hinting"), 0);
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
   return o;
}

static void
_ilist_cb_change(void *data, Evas_Object *obj)
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
_enabled_cb_change(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata;
   cfdata = data;   
   if (!cfdata) return;
   
   if (cfdata->cur_enabled)
     {
	e_widget_disabled_set(cfdata->gui.font, 0);
	e_widget_disabled_set(cfdata->gui.size, 0);
     }
   else
     {
	e_widget_disabled_set(cfdata->gui.font, 1);
	e_widget_disabled_set(cfdata->gui.size, 1);
     }
}
