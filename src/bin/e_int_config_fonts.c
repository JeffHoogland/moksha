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

static void _ilist_font_cb_change(void *data, Evas_Object *obj);
static void _enabled_font_cb_change(void *data, Evas_Object *obj);

static void _ilist_fallback_cb_change(void *data, Evas_Object *obj);
static void _enabled_fallback_cb_change(void *data, Evas_Object *obj);
static void _button_fallback_cb_add(void *data, void *obj);
static void _button_fallback_cb_remove(void *data, void *obj);
static void _button_fallback_cb_up(void *data, void *obj);
static void _button_fallback_cb_down(void *data, void *obj);
static void _list_select_num(E_Config_Dialog_Data *cfdata, int indx);

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
   char		*cur_fallback;
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
	
	Evas_Object *fallback;	/* Text Entry */
	Evas_Object *fallback_up; /* Move selected list entry up */
	Evas_Object *fallback_down; /* Move selected list entry down */
	Evas_Object *fallback_add; /* create and select a new list entry */
	Evas_Object *fallback_remove; /* remove the selected entry */
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
   Evas_List *font_list;
   Evas_List *next;
   E_Font_Default *efd;
   CFText_Class *tc;
   int i;
   
   font_list = e_font_default_list();
   
   /* Fill out the font list */
   for (i = 0; text_class_predefined_names[i].class_name; i++ )
     {
	tc = E_NEW(CFText_Class, 1);
	
	tc->class_name = text_class_predefined_names[i].class_name;
	tc->class_description = _(text_class_predefined_names[i].class_description);
	tc->font = NULL;
	tc->size = 0;
	tc->enabled = 0;
	
	for (next = font_list; next ; next = next->next)
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
        evas_stringshare_del(tc->font);
        E_FREE(tc);
     }
   
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
	for (i = 0; i < e_widget_ilist_count(cfdata->gui.fallback_list); i++)
	  {
	     const char *fallback;
	     fallback = e_widget_ilist_nth_label_get(cfdata->gui.fallback_list, i);
	     if (fallback != NULL && strlen(fallback) > 0)
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
   int option_enable;
   int i;

   cfdata->cur_index = -1;
   o = e_widget_list_add(evas, 0, 0);
  
   /* Create Font Class Widgets */ 
   of = e_widget_frametable_add(evas, _("Font Class Configuration"), 0);
   cfdata->gui.class_list = e_widget_ilist_add(evas, 16, 16, NULL);
   e_widget_min_size_set(cfdata->gui.class_list, 100, 100);
   e_widget_on_change_hook_set(cfdata->gui.class_list, _ilist_font_cb_change, cfdata);

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
   e_widget_on_change_hook_set(cfdata->gui.enabled, _enabled_font_cb_change, cfdata);
   
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
   
   /* Create Fallbacks Widgets */
   option_enable = (e_font_fallback_list() != NULL);
   of = e_widget_frametable_add(evas, _("Font Fallbacks"), 1);

   ob = e_widget_button_add(evas, _("Move Up"), "widget/up_arrow", _button_fallback_cb_up, cfdata, NULL);
   cfdata->gui.fallback_up = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(	of, 
					ob, 
					1, 0, 1, 1,
					1, 1, 1, 1);
   
   
   ob = e_widget_button_add(evas, _("Move Down"), "widget/down_arrow", _button_fallback_cb_down, cfdata, NULL); 
   cfdata->gui.fallback_down = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(	of, 
					ob, 
					1, 3, 1, 1,
					1, 1, 1, 1);
   
   
   cfdata->gui.fallback_list = e_widget_ilist_add(evas, 16, 16, NULL);
   e_widget_disabled_set(cfdata->gui.fallback_list, !option_enable);
   e_widget_min_size_set(cfdata->gui.fallback_list, 100, 100);
   e_widget_on_change_hook_set(cfdata->gui.fallback_list, _ilist_fallback_cb_change, cfdata);
   /* Fill In Ilist */
   for (i = 0; i < evas_list_count(e_font_fallback_list()); i++)
     {
	E_Font_Fallback *eff;
	
	eff = evas_list_nth(e_font_fallback_list(), i);
	e_widget_ilist_append(cfdata->gui.fallback_list, NULL, eff->name, NULL, NULL, NULL);
     }
   e_widget_ilist_go(cfdata->gui.fallback_list);
   e_widget_frametable_object_append(	of, 
					cfdata->gui.fallback_list, 
					2, /* Col Start*/
					0, /* Row Start */
					1, /* Col Span*/
					4, /* Row Span*/
					1, 1, 1, 1);
  
   ob = e_widget_label_add(evas, _("Fallback Name"));
   e_widget_frametable_object_append(	of, 
					ob, 
					0, 0, 1, 1,
					1, 1, 1, 1); 
   
   cfdata->gui.fallback = e_widget_entry_add(evas, &(cfdata->cur_fallback));
   e_widget_disabled_set(cfdata->gui.fallback, !option_enable);
   e_widget_min_size_set(cfdata->gui.fallback, 100, 25);
   e_widget_frametable_object_append(of, cfdata->gui.fallback, 
					0, 1, 1, 1, 
					1, 1, 1, 1);

   cfdata->gui.fallback_add = e_widget_button_add(evas, _("Add"), NULL, _button_fallback_cb_add, cfdata, NULL); 
   e_widget_disabled_set(cfdata->gui.fallback_add, !option_enable);
   e_widget_frametable_object_append(	of, 
					cfdata->gui.fallback_add, 
					1, 1, 1, 1,
					1, 1, 1, 1);
   
   cfdata->gui.fallback_remove = e_widget_button_add(evas, _("Remove"), NULL, _button_fallback_cb_remove, cfdata, NULL); 
   e_widget_disabled_set(cfdata->gui.fallback_remove, !option_enable);
   e_widget_frametable_object_append(	of, 
					cfdata->gui.fallback_remove , 
					1, 2, 1, 1,
					1, 1, 1, 1);
    	   
   ob = e_widget_check_add(evas, _("Enable Fallbacks"), &(cfdata->cur_fallbacks_enabled));
   e_widget_frametable_object_append(of, ob, 
					0, 3, 1, 1, 
					1, 1, 1, 1);
   e_widget_check_checked_set(ob, option_enable);
   e_widget_on_change_hook_set(ob, _enabled_fallback_cb_change, cfdata); 
  
   if (e_widget_ilist_count(cfdata->gui.fallback_list) > 0)
     {
	e_widget_ilist_selected_set(cfdata->gui.fallback_list, 0);
     } 
   
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

/* Private Font Fallback Functions */
static void _ilist_fallback_cb_change(void *data, Evas_Object *obj)
{
   int indx;
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
   if (!cfdata) return;

   indx = e_widget_ilist_selected_get(cfdata->gui.fallback_list);
   _list_select_num(cfdata, indx);
}
static void _enabled_fallback_cb_change(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
   if (!cfdata) return;
   
   if (cfdata->cur_fallbacks_enabled)
     {
	e_widget_disabled_set(cfdata->gui.fallback_list, 0);	
	e_widget_disabled_set(cfdata->gui.fallback_add, 0);
	e_widget_disabled_set(cfdata->gui.fallback, 0);
     }
   else
     {
	_list_select_num(cfdata, -1);
	e_widget_disabled_set(cfdata->gui.fallback_list, 1);
	e_widget_disabled_set(cfdata->gui.fallback, 1);
	e_widget_disabled_set(cfdata->gui.fallback_add, 1);
     }
}
static void _button_fallback_cb_add(void *data, void *obj)
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
   if (!cfdata) return;

   if (cfdata->cur_fallback && strlen(cfdata->cur_fallback) > 0)
     {
	int count;
	int i;	
	
	/* If it already esists just select the existing one */
	for (i = 0; i < e_widget_ilist_count(cfdata->gui.fallback_list); i++)
	  {
	     const char *fallback;
	     fallback = e_widget_ilist_nth_label_get(cfdata->gui.fallback_list, i);
	     if (  fallback != NULL && 
		   strlen(fallback) > 0 && 
		   !strcmp(fallback, cfdata->cur_fallback) )
	       {
		  e_widget_ilist_selected_set(cfdata->gui.fallback_list, i);
		  return;
	       }
	  }
	
	e_widget_ilist_append(cfdata->gui.fallback_list, NULL, cfdata->cur_fallback, NULL, NULL, NULL);
	e_widget_ilist_go(cfdata->gui.fallback_list);
	count = e_widget_ilist_count(cfdata->gui.fallback_list);
	e_widget_ilist_selected_set(cfdata->gui.fallback_list, count - 1);
	
	e_widget_entry_text_set(cfdata->gui.fallback, "");	     
     }
}

static void _button_fallback_cb_remove(void *data, void *obj)
{
   int indx;
   int count;
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
   if (!cfdata) return;

   indx = e_widget_ilist_selected_get(cfdata->gui.fallback_list);
   count = e_widget_ilist_count(cfdata->gui.fallback_list);
   
   e_widget_ilist_remove_num(cfdata->gui.fallback_list, indx);

   e_widget_ilist_go(cfdata->gui.fallback_list);	
   e_widget_ilist_selected_set(cfdata->gui.fallback_list, indx);
   if (count == 1)
     {
	_list_select_num(cfdata, -1);
     }   
}
static void _button_fallback_cb_up(void *data, void *obj)
{
   E_Config_Dialog_Data *cfdata;
   int idx_sel;
   const char *label_sel;
   const char *label_rep;
   
   cfdata = data;
   if (!cfdata) return;

   idx_sel = e_widget_ilist_selected_get(cfdata->gui.fallback_list);

   label_sel = e_widget_ilist_nth_label_get(cfdata->gui.fallback_list, idx_sel);
   label_rep = e_widget_ilist_nth_label_get(cfdata->gui.fallback_list, idx_sel - 1);

   e_widget_ilist_nth_label_set(cfdata->gui.fallback_list, idx_sel - 1, label_sel);
   e_widget_ilist_nth_label_set(cfdata->gui.fallback_list, idx_sel, label_rep);

   e_widget_ilist_selected_set(cfdata->gui.fallback_list, idx_sel - 1);
}
static void _button_fallback_cb_down(void *data, void *obj)
{
   E_Config_Dialog_Data *cfdata;
   int idx_sel;
   const char *label_sel;
   const char *label_rep;
   
   cfdata = data;
   if (!cfdata) return;

   idx_sel = e_widget_ilist_selected_get(cfdata->gui.fallback_list);

   label_sel = e_widget_ilist_nth_label_get(cfdata->gui.fallback_list, idx_sel);
   label_rep = e_widget_ilist_nth_label_get(cfdata->gui.fallback_list, idx_sel + 1);

   e_widget_ilist_nth_label_set(cfdata->gui.fallback_list, idx_sel + 1, label_sel);
   e_widget_ilist_nth_label_set(cfdata->gui.fallback_list, idx_sel, label_rep);

   e_widget_ilist_selected_set(cfdata->gui.fallback_list, idx_sel + 1);
}

static void _list_select_num(E_Config_Dialog_Data *cfdata, int indx)
{
   int count;

   /* Disable selecting the list while we are disabled */
   if (indx >= 0 && !cfdata->cur_fallbacks_enabled) return;

   count = e_widget_ilist_count(cfdata->gui.fallback_list);
 
   if (count == 0 || indx < 0)
     {
	e_widget_disabled_set(cfdata->gui.fallback_remove, 1);
     }
   else 
     {
	e_widget_disabled_set(cfdata->gui.fallback_remove, 0);
     }
   
   if (count == 1 || indx < 0)
     {
	e_widget_disabled_set(cfdata->gui.fallback_up, 1);
	e_widget_disabled_set(cfdata->gui.fallback_down, 1);
     }
   else if (indx == 0)
     {
	e_widget_disabled_set(cfdata->gui.fallback_up, 1);
	e_widget_disabled_set(cfdata->gui.fallback_down, 0);
     }
   else if (indx + 1 == count)
     {
	e_widget_disabled_set(cfdata->gui.fallback_up, 0);
	e_widget_disabled_set(cfdata->gui.fallback_down, 1);
     }
   else
     {
	e_widget_disabled_set(cfdata->gui.fallback_up, 0);
	e_widget_disabled_set(cfdata->gui.fallback_down, 0);
     } 
}
