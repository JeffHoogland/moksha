/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;

/* subsystem functions */
static void _list_cb_change(void *data, Evas_Object *obj);
static void _button_cb_add(void *data, void *obj);
static void _button_cb_remove(void *data, void *obj);
static void _button_cb_up(void *data, void *obj);
static void _button_cb_down(void *data, void *obj);
static void _list_select_num(E_Widget_Data *wd, int indx);
static void _e_wid_disable_hook(Evas_Object *obj);
static void _e_wid_del_hook(Evas_Object *obj);

struct _E_Widget_Data
{
   /* Current Data */
   char *cur_entry;
   int   cur_enabled;

   struct
     {
	Evas_Object *list; /* Selecting a list entry starts edit*/
	Evas_Object *table;
	Evas_Object *entry;	/* Generic Entry */
	Evas_Object *up; /* Move selected list entry up */
	Evas_Object *down; /* Move selected list entry down */
	Evas_Object *add; /* create and select a new list entry */
	Evas_Object *remove; /* remove the selected entry */
     } gui;
};

/* Externally accessible functions */
EAPI Evas_Object *
e_widget_config_list_add(Evas *evas, Evas_Object* (*func_entry_add) (Evas *evas, char **val, void (*func) (void *data, void *data2), void *data, void *data2), const char *label, int listspan)
{
   Evas_Object *obj, *o;
   E_Widget_Data *wd;
   Evas_Coord mw, mh;

   obj = e_widget_add(evas);
   e_widget_del_hook_set(obj, _e_wid_del_hook);
   e_widget_disable_hook_set(obj, _e_wid_disable_hook);
   wd = E_NEW(E_Widget_Data, 1);
   e_widget_data_set(obj, wd);

   o = e_widget_table_add(evas, 0);
   wd->gui.table = o;
   e_widget_sub_object_add(obj, o);

   o = e_widget_button_add(evas, _("Add"), NULL, _button_cb_add, wd, obj); 
   wd->gui.add = o;
   e_widget_disabled_set(o, 1);
   e_widget_table_object_append(wd->gui.table, o, 0, 1, 1, 1, 1, 0, 1, 0);

   o = e_widget_button_add(evas, _("Remove"), NULL, _button_cb_remove, wd, obj);
   wd->gui.remove = o;
   e_widget_disabled_set(o, 1);
   e_widget_table_object_append(wd->gui.table, o, 1, 1, 1, 1, 1, 0, 1, 0);

   o = e_widget_label_add(evas, label);
   e_widget_table_object_append(wd->gui.table, o, 0, 2, 2, 1, 1, 0, 1, 0);

   o = func_entry_add(evas, &(wd->cur_entry), NULL, NULL, NULL);
   wd->gui.entry = o;
   e_widget_disabled_set(o, 1);
   e_widget_min_size_set(o, 100, 25);
   e_widget_table_object_append(wd->gui.table, o, 0, 3, 2, 1, 1, 0, 1, 0);

   o = e_widget_button_add(evas, _("Up"), "widget/up_arrow", 
			   _button_cb_up, wd, NULL);
   wd->gui.up = o;
   e_widget_disabled_set(o, 1);
   e_widget_table_object_append(wd->gui.table, o, 0, 4, 1, 1, 1, 0, 1, 0);

   o = e_widget_button_add(evas, _("Down"), "widget/down_arrow", 
			   _button_cb_down, wd, NULL); 
   wd->gui.down = o;
   e_widget_disabled_set(o, 1);
   e_widget_table_object_append(wd->gui.table, o, 1, 4, 1, 1, 1, 0, 1, 0);

   o = e_widget_ilist_add(evas, 0, 0, NULL);
   wd->gui.list = o;
   e_widget_disabled_set(o, 1);
   e_widget_min_size_set(o, 80, 40);
   e_widget_on_change_hook_set(o, _list_cb_change, wd);
   e_widget_ilist_go(o);
   e_widget_table_object_append(wd->gui.table, o, 0, 5, 2, 1, 1, 1, 1, 1);

   e_widget_min_size_get(wd->gui.table, &mw, &mh);
   e_widget_min_size_set(obj, mw, mh);
   e_widget_resize_object_set(obj, wd->gui.table);

   return obj;
}

EAPI int 
e_widget_config_list_count(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   return e_widget_ilist_count(wd->gui.list);   
}

EAPI void
e_widget_config_list_clear(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   e_widget_ilist_clear(wd->gui.list); 
   _list_select_num(wd, -1); 
}

EAPI const char *
e_widget_config_list_nth_get(Evas_Object *obj, int n)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   return e_widget_ilist_nth_label_get(wd->gui.list, n);   
}

EAPI void 
e_widget_config_list_append(Evas_Object *obj, const char *entry)
{
   E_Widget_Data *wd;
   int count;

   wd = e_widget_data_get(obj);
   e_widget_ilist_append(wd->gui.list, NULL, entry, NULL, NULL, NULL);
   e_widget_ilist_go(wd->gui.list);
   count = e_widget_ilist_count(wd->gui.list);
   e_widget_ilist_selected_set(wd->gui.list, count - 1);
}

EAPI void 
e_widget_config_list_object_append(Evas_Object *obj, Evas_Object *sobj, int col, int row, int colspan, int rowspan, int fill_w, int fill_h, int expand_w, int expand_h)
{
   E_Widget_Data *wd;
   Evas_Coord mw, mh;

   wd = e_widget_data_get(obj);
   e_widget_table_object_append(wd->gui.table, sobj, col, row, 
				colspan, rowspan, fill_w, fill_h, 
				expand_w, expand_h);
   e_widget_min_size_get(wd->gui.table, &mw, &mh);
   e_widget_min_size_set(obj, mw, mh);
}

/* Private Function defs */
static void 
_list_cb_change(void *data, Evas_Object *obj)
{
   E_Widget_Data *wd;
   int indx;

   if (!(wd = data)) return;

   indx = e_widget_ilist_selected_get(wd->gui.list);
   _list_select_num(wd, indx);
}

static void 
_button_cb_add(void *data, void *obj)
{
   E_Widget_Data *wd;
   Evas_Object *o;

   o = obj;
   if (!o) return;
   if (!(wd = data)) return;
   if ((wd->cur_entry) && (wd->cur_entry[0]))
     {
	int i;	

	/* If it already esists just select the existing one */
	for (i = 0; i < e_widget_ilist_count(wd->gui.list); i++)
	  {
	     const char *label;

	     label = e_widget_ilist_nth_label_get(wd->gui.list, i);
	     if ((label) && (!strcmp(label, wd->cur_entry)))
	       {
		  e_widget_ilist_selected_set(wd->gui.list, i);
		  return;
	       }
	  }
	e_widget_config_list_append(o, wd->cur_entry);	
	e_widget_entry_text_set(wd->gui.entry, "");	     
     }
}

static void 
_button_cb_remove(void *data, void *obj)
{
   E_Widget_Data *wd;
   int indx;
   int count;

   if (!(wd = data)) return;

   indx = e_widget_ilist_selected_get(wd->gui.list);
   count = e_widget_ilist_count(wd->gui.list);

   e_widget_ilist_remove_num(wd->gui.list, indx);

   e_widget_ilist_go(wd->gui.list);	
   e_widget_ilist_selected_set(wd->gui.list, indx);
   if (count == 1) _list_select_num(wd, -1);
}

static void 
_button_cb_up(void *data, void *obj)
{
   E_Widget_Data *wd;
   int idx_sel;
   const char *label_sel;
   const char *label_rep;

   if (!(wd = data)) return;

   idx_sel = e_widget_ilist_selected_get(wd->gui.list);

   label_sel = e_widget_ilist_nth_label_get(wd->gui.list, idx_sel);
   label_rep = e_widget_ilist_nth_label_get(wd->gui.list, idx_sel - 1);

   e_widget_ilist_nth_label_set(wd->gui.list, idx_sel - 1, label_sel);
   e_widget_ilist_nth_label_set(wd->gui.list, idx_sel, label_rep);

   e_widget_ilist_selected_set(wd->gui.list, idx_sel - 1);
}

static void 
_button_cb_down(void *data, void *obj)
{
   E_Widget_Data *wd;
   int idx_sel;
   const char *label_sel;
   const char *label_rep;

   if (!(wd = data)) return;

   idx_sel = e_widget_ilist_selected_get(wd->gui.list);

   label_sel = e_widget_ilist_nth_label_get(wd->gui.list, idx_sel);
   label_rep = e_widget_ilist_nth_label_get(wd->gui.list, idx_sel + 1);

   e_widget_ilist_nth_label_set(wd->gui.list, idx_sel + 1, label_sel);
   e_widget_ilist_nth_label_set(wd->gui.list, idx_sel, label_rep);

   e_widget_ilist_selected_set(wd->gui.list, idx_sel + 1);
}

static void 
_list_select_num(E_Widget_Data *wd, int indx)
{
   int count;

   /* Disable selecting the list while we are disabled */
   if (indx >= 0 && !wd->cur_enabled) return;

   count = e_widget_ilist_count(wd->gui.list);

   if ((count == 0) || (indx < 0))
     e_widget_disabled_set(wd->gui.remove, 1);
   else 
     e_widget_disabled_set(wd->gui.remove, 0);

   if ((count == 1) || (indx < 0))
     {
	e_widget_disabled_set(wd->gui.up, 1);
	e_widget_disabled_set(wd->gui.down, 1);
     }
   else if (indx == 0)
     {
	e_widget_disabled_set(wd->gui.up, 1);
	e_widget_disabled_set(wd->gui.down, 0);
     }
   else if ((indx + 1) == count)
     {
	e_widget_disabled_set(wd->gui.up, 0);
	e_widget_disabled_set(wd->gui.down, 1);
     }
   else
     {
	e_widget_disabled_set(wd->gui.up, 0);
	e_widget_disabled_set(wd->gui.down, 0);
     } 
}

/* Callback Functions */
static void
_e_wid_del_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   E_FREE(wd->cur_entry);
   E_FREE(wd);
}

static void
_e_wid_disable_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   if (!wd) return;

   wd->cur_enabled = !e_widget_disabled_get(obj);

   if (wd->cur_enabled)
     {
	e_widget_disabled_set(wd->gui.list, 0);	
	e_widget_disabled_set(wd->gui.add, 0);
	e_widget_disabled_set(wd->gui.entry, 0);
     }
   else
     {
	_list_select_num(wd, -1);
	e_widget_disabled_set(wd->gui.list, 1);
	e_widget_disabled_set(wd->gui.entry, 1);
	e_widget_disabled_set(wd->gui.add, 1);
     }
}
