/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data
{
   Evas_Object *o_widget, *o_tb, *o_bar;
   Eina_List *content;
};

static void _e_wid_del_hook(Evas_Object *obj);
static void _item_sel(void *data1, void *data2);

/* externally accessible functions */
EAPI Evas_Object *
e_widget_toolbook_add(Evas *evas, int icon_w, int icon_h)
{
   Evas_Object *obj, *o;
   E_Widget_Data *wd;
   Evas_Coord mw, mh;
   
   obj = e_widget_add(evas);
   e_widget_del_hook_set(obj, _e_wid_del_hook);
   wd = calloc(1, sizeof(E_Widget_Data));
   e_widget_data_set(obj, wd);
   wd->o_widget = obj;

   o = e_widget_table_add(evas, 0);
   e_widget_resize_object_set(obj, o);
   evas_object_show(o);
   e_widget_sub_object_add(obj, o);
   wd->o_tb = o;
   
   o = e_widget_toolbar_add(evas, icon_w, icon_h);
   e_widget_table_object_append(wd->o_tb, o, 0, 0, 1, 1, 1, 1, 1, 0);
   wd->o_bar = o;
   evas_object_show(o);

   return obj;
}

EAPI void
e_widget_toolbook_page_append(Evas_Object *toolbook, Evas_Object *icon, const char *label, Evas_Object *content, int expand_w, int expand_h, int fill_w, int fill_h, double ax, double ay)
{
   E_Widget_Data *wd;
   Evas_Coord minw, minh;

   wd = e_widget_data_get(toolbook);
   e_widget_toolbar_item_append(wd->o_bar, icon, label, _item_sel, 
                                toolbook, content);
   e_widget_table_object_align_append(wd->o_tb, content, 
                                      0, 1, 1, 1, 
                                      fill_w, fill_h, expand_w, expand_h, 
                                      ax, ay);
   evas_object_hide(content);
   wd->content = eina_list_append(wd->content, content);
   e_widget_min_size_get(wd->o_tb, &minw, &minh);
   e_widget_min_size_set(toolbook, minw, minh);
}

EAPI void
e_widget_toolbook_page_show(Evas_Object *toolbook, int n)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(toolbook);
   e_widget_toolbar_item_select(wd->o_bar, n);
}

/* Private functions */
static void
_e_wid_del_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   eina_list_free(wd->content);
   free(wd);
}

static void
_item_sel(void *data1, void *data2)
{
   E_Widget_Data *wd;
   Evas_Object *obj, *sobj;
   Eina_List *l;
   
   obj = data1;
   sobj = data2;
   wd = e_widget_data_get(obj);
   
   for (l = wd->content; l; l = l->next)
     {
        Evas_Object *o;
        
        o = l->data;
        if (o == sobj) evas_object_show(o);
        else evas_object_hide(o);
     }
}
