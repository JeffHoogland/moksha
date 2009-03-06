#include "e.h"

/* function protos */
static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static void _fill_remembers(E_Config_Dialog_Data *cfdata);
static void _cb_delete(void *data, void *data2);
static void _cb_list_change(void *data, Evas_Object *obj);

struct _E_Config_Dialog_Data 
{
   Evas_Object *list, *btn, *name, *class, *title, *role;
};

EAPI E_Config_Dialog *
e_int_config_remembers(E_Container *con, const char *params __UNUSED__) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "_config_remembers_dialog")) return NULL;

   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;

   cfd = e_config_dialog_new(con, _("Window Remembers"), "E", 
                             "_config_remembers_dialog", 
                             "enlightenment/window_remembers", 0, v, NULL);
   e_dialog_resizable_set(cfd->dia, 1);
   return cfd;
}

/* private functions */
static int
_cb_sort(const void *data1, const void *data2)
{
   const E_Remember *rem1 = NULL;
   const E_Remember *rem2 = NULL;
   const char *d1 = NULL;
   const char *d2 = NULL;

   if (!(rem1 = data1)) return 1;
   if (!(rem2 = data2)) return -1;

   if (rem1->name)
     d1 = rem1->name;
   else if (rem1->class)
     d1 = rem1->class;
   else if (rem1->title)
     d1 = rem1->title;
   else if (rem1->role)
     d1 = rem1->role;

   if (rem2->name)
     d2 = rem2->name;
   else if (rem2->class)
     d2 = rem2->class;
   else if (rem2->title)
     d2 = rem2->title;
   else if (rem2->role)
     d2 = rem2->role;

   if (!strcmp(d1, d2)) 
     return -1;
   else
     return strcmp(d1, d2);
}

static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   return cfdata;
}

static void 
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   E_FREE(cfdata);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *of, *of2, *ow;

   o = e_widget_list_add(evas, 0, 0);
   of = e_widget_frametable_add(evas, _("Window Remembers"), 0);

   ow = e_widget_button_add(evas, _("Delete Remember(s)"), "list-remove",
			    _cb_delete, cfdata, NULL);
   cfdata->btn = ow;

   ow = e_widget_ilist_add(evas, 24, 24, NULL);
   cfdata->list = ow;
   e_widget_ilist_multi_select_set(ow, 1);
   e_widget_on_change_hook_set(ow, _cb_list_change, cfdata);
   _fill_remembers(cfdata);

   of2 = e_widget_frametable_add(evas, _("Details"), 0);
   ow = e_widget_label_add(evas, _("Name:"));
   e_widget_frametable_object_append(of2, ow, 0, 0, 1, 1, 1, 1, 0, 0);
   ow = e_widget_label_add(evas, _("<No Name>"));
   cfdata->name = ow;
   e_widget_frametable_object_append(of2, cfdata->name, 1, 0, 1, 1, 1, 1, 1, 0);
   ow = e_widget_label_add(evas, _("Class:"));
   e_widget_frametable_object_append(of2, ow, 0, 1, 1, 1, 1, 1, 0, 0);
   ow = e_widget_label_add(evas, _("<No Class>"));
   cfdata->class = ow;
   e_widget_frametable_object_append(of2, cfdata->class, 1, 1, 1, 1, 1, 1, 1, 0);
   ow = e_widget_label_add(evas, _("Title:"));
   e_widget_frametable_object_append(of2, ow, 0, 2, 1, 1, 1, 1, 0, 0);
   ow = e_widget_label_add(evas, _("<No Title>"));
   cfdata->title = ow;
   e_widget_frametable_object_append(of2, cfdata->title, 1, 2, 1, 1, 1, 1, 1, 0);
   ow = e_widget_label_add(evas, _("Role:"));
   e_widget_frametable_object_append(of2, ow, 0, 3, 1, 1, 1, 1, 0, 0);
   ow = e_widget_label_add(evas, _("<No Role>"));
   cfdata->role = ow;
   e_widget_frametable_object_append(of2, cfdata->role, 1, 3, 1, 1, 1, 1, 1, 0);

   e_widget_frametable_object_append(of, cfdata->list, 0, 0, 1, 1, 1, 1, 1, 1);
   e_widget_frametable_object_append(of, of2, 0, 1, 1, 1, 1, 1, 1, 0);
   e_widget_frametable_object_append(of, cfdata->btn, 0, 2, 1, 1, 1, 1, 1, 0);

   e_widget_list_object_append(o, of, 1, 1, 0.5);

   e_widget_disabled_set(cfdata->btn, 1);
   return o;
}

static void 
_fill_remembers(E_Config_Dialog_Data *cfdata) 
{
   Evas *evas;
   Eina_List *l = NULL;
   Eina_List *ll = NULL;
   Evas_Object *ic;
   int w = 0;

   evas = evas_object_evas_get(cfdata->list);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->list);
   e_widget_ilist_clear(cfdata->list);

   ll = e_config->remembers;
   ll = eina_list_sort(ll, -1, _cb_sort);

   ic = edje_object_add(evas);
   e_util_edje_icon_set(ic, "enlightenment/applications");
   e_widget_ilist_header_append(cfdata->list, ic, _("Applications"));

   for (l = ll; l; l = l->next) 
     {
        E_Remember *rem = NULL;

        if (!(rem = l->data)) continue;

        /* Filter out E's own remember */
        if ((rem->name) && (!strcmp(rem->name, "E"))) continue;
        /* Filter out the module config remembers */
        if ((rem->class) && (rem->class[0] == '_')) continue;

        if (rem->name)
          e_widget_ilist_append(cfdata->list, NULL, rem->name, NULL, rem, NULL);
        else if (rem->class)
          e_widget_ilist_append(cfdata->list, NULL, rem->class, NULL, rem, NULL);
        else if (rem->title)
          e_widget_ilist_append(cfdata->list, NULL, rem->title, NULL, rem, NULL);
        else if (rem->role)
          e_widget_ilist_append(cfdata->list, NULL, rem->role, NULL, rem, NULL);
     }

   ic = edje_object_add(evas);
   e_util_edje_icon_set(ic, "enlightenment/e");
   e_widget_ilist_header_append(cfdata->list, ic, _("Enlightenment"));
   for (l = ll; l; l = l->next)
     {
        E_Remember *rem = NULL;

        if (!(rem = l->data)) continue;

        /* Garuntee we add only E's internal remembers */
        if ((rem->name) && (strcmp(rem->name, "E"))) continue;

        e_widget_ilist_append(cfdata->list, NULL, rem->class, NULL, rem, NULL);
     }

   ic = e_icon_add(evas);
   e_util_icon_theme_set(ic, "preferences-plugin");
   e_widget_ilist_header_append(cfdata->list, ic, _("Modules"));
   for (l = ll; l; l = l->next) 
     {
        E_Remember *rem = NULL;

        if (!(rem = l->data)) continue;

        /* Filter out E's own remember */
        if ((rem->name) && (!strcmp(rem->name, "E"))) continue;
        /* Filter out everything except the module config remembers */
        if ((rem->name) && (rem->class[0] != '_')) continue;

        e_widget_ilist_append(cfdata->list, NULL, rem->name, NULL, rem, NULL);
     }

   e_widget_ilist_go(cfdata->list);
   e_widget_min_size_get(cfdata->list, &w, NULL);

   /* NB: make the window look a bit better by not being so small */
//   if (w < 300) w = 300;

   e_widget_min_size_set(cfdata->list, 400, 200);
   e_widget_ilist_thaw(cfdata->list);
   edje_thaw();
   evas_event_thaw(evas);

   e_widget_disabled_set(cfdata->btn, 1);
}

static void 
_cb_delete(void *data, void *data2) 
{
   E_Config_Dialog_Data *cfdata;
   Eina_List *l = NULL;
   int i = 0, changed = 0, deleted = 0;
   int last_selected = -1;

   if (!(cfdata = data)) return;
   for (i = 0, l = e_widget_ilist_items_get(cfdata->list); l; l = l->next, i++) 
     {
        E_Ilist_Item *item = NULL;
        E_Remember *rem = NULL;

        item = l->data;
        if ((!item) || (!item->selected)) continue;
        if (!(rem = e_widget_ilist_nth_data_get(cfdata->list, i))) continue;
        e_remember_del(rem);
	last_selected = i;
        changed = 1;
	++ deleted;
     }

   if (changed) e_config_save_queue();

   _fill_remembers(cfdata);
   if (last_selected >= 0)
     e_widget_ilist_selected_set(cfdata->list, last_selected - deleted + 1);
}

static void 
_cb_list_change(void *data, Evas_Object *obj) 
{
   E_Config_Dialog_Data *cfdata;
   E_Remember *rem = NULL;
   int n = 0;

   if (!(cfdata = data)) return;

   n = e_widget_ilist_selected_get(cfdata->list);
   if ((rem = e_widget_ilist_nth_data_get(cfdata->list, n)))
     {
	e_widget_label_text_set(cfdata->name, rem->name ? 
                                rem->name : _("<No Name>"));
	e_widget_label_text_set(cfdata->class, rem->class ? 
                                rem->class : _("<No Class>"));
	e_widget_label_text_set(cfdata->title, rem->title ? 
                                rem->title : _("<No Title>"));
	e_widget_label_text_set(cfdata->role, rem->role ? 
                                rem->role : _("<No Role>"));
     }

   if (e_widget_ilist_selected_count_get(cfdata->list) < 1)
     e_widget_disabled_set(cfdata->btn, 1);
   else
     e_widget_disabled_set(cfdata->btn, 0);
}
