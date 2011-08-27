#include "e.h"

typedef struct _E_Config_Data
{
   const char *title, *icon, *dialog, *filename;
   Eina_Bool show_autostart;
} E_Config_Data;

typedef struct _E_Config_App_List
{
   E_Config_Dialog_Data *cfdata;
   Evas_Object *o_list, *o_add, *o_del, *o_desc;
   Eina_List *desks;
} E_Config_App_List;

struct _E_Config_Dialog_Data
{
   E_Config_Data *data;
   Evas_Object *o_list, *o_up, *o_down, *o_del;
   Eina_List *apps;
   Ecore_Timer *fill_delay;
   E_Config_App_List apps_user;
   E_Config_App_List apps_xdg; /* xdg autostart apps */
};

/* local function prototypes */
static E_Config_Dialog *_create_dialog(E_Container *con, E_Config_Data *data);
static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);
static Eina_List *_load_menu(const char *path);
static Eina_List *_load_order(const char *path);
static int _save_menu(E_Config_Dialog_Data *cfdata);
static int _save_order(E_Config_Dialog_Data *cfdata);
static void _fill_apps_list(E_Config_App_List *apps);
static void _fill_xdg_list(E_Config_App_List *apps);
static void _fill_order_list(E_Config_Dialog_Data *cfdata);
static void _cb_apps_list_selected(void *data);
static void _cb_order_list_selected(void *data);
static int _cb_desks_sort(const void *data1, const void *data2);
static int _cb_desks_name(const void *data1, const void *data2);
static int _cb_desks_sort(const void *data1, const void *data2);
static void _cb_add(void *data, void *data2 __UNUSED__);
static void _cb_del(void *data, void *data2 __UNUSED__);
static void _cb_up(void *data, void *data2 __UNUSED__);
static void _cb_down(void *data, void *data2 __UNUSED__);
static void _cb_order_del(void *data, void *data2 __UNUSED__);
static Eina_Bool _cb_fill_delay(void *data);
static void _list_items_state_set(E_Config_App_List *apps);

E_Config_Dialog *
e_int_config_apps_add(E_Container *con, const char *params __UNUSED__)
{
   E_Desktop_Edit *ed;

   if (!(ed = e_desktop_edit(con, NULL))) return NULL;
   return ed->cfd;
}

E_Config_Dialog *
e_int_config_apps_favs(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Data *data;
   char buff[PATH_MAX];

   e_user_dir_concat_static(buff, "applications/menu/favorite.menu");
   data = E_NEW(E_Config_Data, 1);
   data->title = eina_stringshare_add(_("Favorite Applications"));
   data->dialog = eina_stringshare_add("menus/favorites_menu");
   data->icon = eina_stringshare_add("user-bookmarks");
   data->filename = eina_stringshare_add(buff);
   return _create_dialog(con, data);
}

E_Config_Dialog *
e_int_config_apps_ibar(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Data *data;
   char buff[PATH_MAX];

   e_user_dir_concat_static(buff, "applications/bar/default/.order");
   data = E_NEW(E_Config_Data, 1);
   data->title = eina_stringshare_add(_("IBar Applications"));
   data->dialog = eina_stringshare_add("applications/ibar_applications");
   data->icon = eina_stringshare_add("preferences-applications-ibar");
   data->filename = eina_stringshare_add(buff);
   return _create_dialog(con, data);
}

E_Config_Dialog *
e_int_config_apps_ibar_other(E_Container *con, const char *path)
{
   E_Config_Data *data;

   if (!path) return NULL;
   data = E_NEW(E_Config_Data, 1);
   data->title = eina_stringshare_add(_("IBar Applications"));
   data->dialog = eina_stringshare_add("internal/ibar_other");
   data->icon = eina_stringshare_add("preferences-applications-ibar");
   data->filename = eina_stringshare_add(path);
   return _create_dialog(con, data);
}

E_Config_Dialog *
e_int_config_apps_startup(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Data *data;
   char buff[PATH_MAX];

   e_user_dir_concat_static(buff, "applications/startup/.order");
   data = E_NEW(E_Config_Data, 1);
   data->title = eina_stringshare_add(_("Startup Applications"));
   data->dialog = eina_stringshare_add("applications/startup_applications");
   data->icon = eina_stringshare_add("preferences-applications-startup");
   data->filename = eina_stringshare_add(buff);
   data->show_autostart = EINA_TRUE;
   return _create_dialog(con, data);
}

E_Config_Dialog *
e_int_config_apps_restart(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Data *data;
   char buff[PATH_MAX];

   e_user_dir_concat_static(buff, "applications/restart/.order");
   data = E_NEW(E_Config_Data, 1);
   data->title = eina_stringshare_add(_("Restart Applications"));
   data->dialog = eina_stringshare_add("applications/restart_applications");
   data->icon = eina_stringshare_add("preferences-applications-restart");
   data->filename = eina_stringshare_add(buff);
   return _create_dialog(con, data);
}

/* local function prototypes */
static E_Config_Dialog *
_create_dialog(E_Container *con, E_Config_Data *data)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", data->dialog))
     {
        if (data->title) eina_stringshare_del(data->title);
        if (data->dialog) eina_stringshare_del(data->dialog);
        if (data->icon) eina_stringshare_del(data->icon);
        if (data->filename) eina_stringshare_del(data->filename);
        E_FREE(data);
        return NULL;
     }

   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;

   cfd = e_config_dialog_new(con, data->title, "E", data->dialog,
			     data->icon, 0, v, data);
   return cfd;
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;
   E_Config_Data *data;
   const char *ext;

   if (!(data = cfd->data)) return NULL;
   if (!data->filename) return NULL;
   if (!(ext = strrchr(data->filename, '.'))) return NULL;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->data = data;
   cfdata->apps_xdg.cfdata = cfdata;
   cfdata->apps_user.cfdata = cfdata;
   if (!strcmp(ext, ".menu"))
     cfdata->apps = _load_menu(data->filename);
   else if (!strcmp(ext, ".order"))
     cfdata->apps = _load_order(data->filename);

   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   E_Config_Data *data;
   Efreet_Desktop *desk;

   if (cfdata->fill_delay) ecore_timer_del(cfdata->fill_delay);

   if ((data = cfdata->data))
     {
        if (data->title) eina_stringshare_del(data->title);
        if (data->dialog) eina_stringshare_del(data->dialog);
        if (data->icon) eina_stringshare_del(data->icon);
        if (data->filename) eina_stringshare_del(data->filename);
        E_FREE(data);
     }
   EINA_LIST_FREE(cfdata->apps, desk)
     efreet_desktop_free(desk);
   EINA_LIST_FREE(cfdata->apps_user.desks, desk)
     efreet_desktop_free(desk);
   EINA_LIST_FREE(cfdata->apps_xdg.desks, desk)
     efreet_desktop_free(desk);
   E_FREE(cfdata);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *otb, *ot;
   int mw;

   otb = e_widget_toolbook_add(evas, 24, 24);

   if (cfdata->data->show_autostart)
     {
	/* XDG autostart page */
	ot = e_widget_table_add(evas, EINA_FALSE);
	cfdata->apps_xdg.o_list = e_widget_ilist_add(evas, 24, 24, NULL);
	e_widget_ilist_multi_select_set(cfdata->apps_xdg.o_list, EINA_TRUE);
	e_widget_size_min_get(cfdata->apps_xdg.o_list, &mw, NULL);
	if (mw < (200 * e_scale)) mw = (200 * e_scale);
	e_widget_size_min_set(cfdata->apps_xdg.o_list, mw, (75 * e_scale));
	e_widget_table_object_append(ot, cfdata->apps_xdg.o_list, 0, 0, 2, 1, 1, 1, 1, 1);

	cfdata->apps_xdg.o_desc = e_widget_textblock_add(evas);
	e_widget_size_min_set(cfdata->apps_xdg.o_desc, 100, (45 * e_scale));
	e_widget_table_object_append(ot, cfdata->apps_xdg.o_desc, 0, 1, 2, 1, 1, 1, 0, 0);

	cfdata->apps_xdg.o_add = e_widget_button_add(evas, _("Add"), "list-add",
						     _cb_add, &cfdata->apps_xdg, NULL);
	e_widget_disabled_set(cfdata->apps_xdg.o_add, EINA_TRUE);
	e_widget_table_object_append(ot, cfdata->apps_xdg.o_add, 0, 2, 1, 1, 1, 1, 1, 0);
	cfdata->apps_xdg.o_del = e_widget_button_add(evas, _("Remove"), "list-remove",
						     _cb_del, &cfdata->apps_xdg, NULL);
	e_widget_disabled_set(cfdata->apps_xdg.o_del, EINA_TRUE);
	e_widget_table_object_append(ot, cfdata->apps_xdg.o_del, 1, 2, 1, 1, 1, 1, 1, 0);

	e_widget_toolbook_page_append(otb, NULL, _("System"), ot,
				      1, 1, 1, 1, 0.5, 0.0);
     }

   /* Selection page */
   ot = e_widget_table_add(evas, EINA_FALSE);
   cfdata->apps_user.o_list = e_widget_ilist_add(evas, 24, 24, NULL);
   e_widget_ilist_multi_select_set(cfdata->apps_user.o_list, EINA_TRUE);
   e_widget_size_min_get(cfdata->apps_user.o_list, &mw, NULL);
   if (mw < (200 * e_scale)) mw = (200 * e_scale);
   e_widget_size_min_set(cfdata->apps_user.o_list, mw, (75 * e_scale));
   e_widget_table_object_append(ot, cfdata->apps_user.o_list, 0, 0, 2, 1, 1, 1, 1, 1);
   cfdata->apps_user.o_add = e_widget_button_add(evas, _("Add"), "list-add",
						 _cb_add, &cfdata->apps_user, NULL);
   e_widget_disabled_set(cfdata->apps_user.o_add, EINA_TRUE);
   e_widget_table_object_append(ot, cfdata->apps_user.o_add, 0, 1, 1, 1, 1, 1, 1, 0);
   cfdata->apps_user.o_del = e_widget_button_add(evas, _("Remove"), "list-remove",
						 _cb_del, &cfdata->apps_user, NULL);
   e_widget_disabled_set(cfdata->apps_user.o_del, EINA_TRUE);
   e_widget_table_object_append(ot, cfdata->apps_user.o_del, 1, 1, 1, 1, 1, 1, 1, 0);
   e_widget_toolbook_page_append(otb, NULL, _("Applications"), ot,
                                 1, 1, 1, 1, 0.5, 0.0);

   /* Order page */
   ot = e_widget_table_add(evas, EINA_FALSE);
   cfdata->o_list = e_widget_ilist_add(evas, 24, 24, NULL);
   _fill_order_list(cfdata);
   e_widget_table_object_append(ot, cfdata->o_list, 0, 0, 3, 1, 1, 1, 1, 1);
   cfdata->o_up = e_widget_button_add(evas, _("Up"), "go-up",
                                      _cb_up, cfdata, NULL);
   e_widget_disabled_set(cfdata->o_up, EINA_TRUE);
   e_widget_table_object_append(ot, cfdata->o_up, 0, 1, 1, 1, 1, 1, 1, 0);
   cfdata->o_down = e_widget_button_add(evas, _("Down"), "go-down",
                                        _cb_down, cfdata, NULL);
   e_widget_disabled_set(cfdata->o_down, EINA_TRUE);
   e_widget_table_object_append(ot, cfdata->o_down, 1, 1, 1, 1, 1, 1, 1, 0);
   cfdata->o_del = e_widget_button_add(evas, _("Remove"), "list-remove",
                                        _cb_order_del, cfdata, NULL);
   e_widget_disabled_set(cfdata->o_del, EINA_TRUE);
   e_widget_table_object_append(ot, cfdata->o_del, 2, 1, 1, 1, 1, 1, 1, 0);
   e_widget_toolbook_page_append(otb, NULL, _("Order"), ot,
                                 1, 1, 1, 1, 0.5, 0.0);

   e_widget_toolbook_page_show(otb, 0);

   if (cfdata->fill_delay) ecore_timer_del(cfdata->fill_delay);
   cfdata->fill_delay = ecore_timer_add(0.2, _cb_fill_delay, cfdata);

   e_dialog_resizable_set(cfd->dia, 1);
   e_win_centered_set(cfd->dia->win, 1);
   return otb;
}

static int
_basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   const char *ext;

   if ((!cfdata->data) || (!cfdata->data->filename)) return 0;
   if (!(ext = strrchr(cfdata->data->filename, '.'))) return 0;
   if (!strcmp(ext, ".menu"))
     return _save_menu(cfdata);
   else if (!strcmp(ext, ".order"))
     return _save_order(cfdata);
   return 1;
}

static Eina_List *
_load_menu(const char *path)
{
   Efreet_Menu *menu, *entry;
   Eina_List *apps = NULL, *l;

   menu = efreet_menu_parse(path);
   if ((!menu) || (!menu->entries)) return NULL;
   EINA_LIST_FOREACH(menu->entries, l, entry)
     {
        if (entry->type != EFREET_MENU_ENTRY_DESKTOP) continue;
        efreet_desktop_ref(entry->desktop);
        apps = eina_list_append(apps, entry->desktop);
     }
   efreet_menu_free(menu);
   return apps;
}

static Eina_List *
_load_order(const char *path)
{
   E_Order *order = NULL;
   Eina_List *apps = NULL, *l;
   Efreet_Desktop *desk;

   if (!path) return NULL;
   if (!(order = e_order_new(path))) return NULL;
   EINA_LIST_FOREACH(order->desktops, l, desk)
     {
        efreet_desktop_ref(desk);
        apps = eina_list_append(apps, desk);
     }
   e_object_del(E_OBJECT(order));
   return apps;
}

static int
_save_menu(E_Config_Dialog_Data *cfdata)
{
   Eina_List *l;
   Efreet_Menu *menu = NULL;
   Efreet_Desktop *desk;
   int ret;

   menu = efreet_menu_new("Favorites");
   EINA_LIST_FOREACH(cfdata->apps, l, desk)
     {
        if (!desk) continue;
        efreet_menu_desktop_insert(menu, desk, -1);
     }
   ret = efreet_menu_save(menu, cfdata->data->filename);
   efreet_menu_free(menu);
   return ret;
}

static int
_save_order(E_Config_Dialog_Data *cfdata)
{
   Eina_List *l;
   E_Order *order = NULL;
   Efreet_Desktop *desk;

   if (!(order = e_order_new(cfdata->data->filename))) return 0;
   e_order_clear(order);
   EINA_LIST_FOREACH(cfdata->apps, l, desk)
     {
        if (!desk) continue;
        e_order_append(order, desk);
     }
   e_object_del(E_OBJECT(order));
   return 1;
}

static void
_list_items_state_set(E_Config_App_List *apps)
{
   Evas *evas;
   Eina_List *l;
   Efreet_Desktop *desk;

   if (!apps->o_list)
     return;

   evas = evas_object_evas_get(apps->o_list);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(apps->o_list);
   e_widget_ilist_clear(apps->o_list);

   EINA_LIST_FOREACH(apps->desks, l, desk)
     {
        Evas_Object *icon = NULL, *end = NULL;

        end = edje_object_add(evas);
        if (!e_theme_edje_object_set(end, "base/theme/widgets",
                                     "e/widgets/ilist/toggle_end"))
          {
             evas_object_del(end);
             end = NULL;
          }

	if (!end) break;

        if (eina_list_search_unsorted(apps->cfdata->apps, _cb_desks_sort, desk))
          {
             edje_object_signal_emit(end, "e,state,checked", "e");
          }
        else
          {
            edje_object_signal_emit(end, "e,state,unchecked", "e");
          }

        icon = e_util_desktop_icon_add(desk, 24, evas);
        e_widget_ilist_append_full(apps->o_list, icon, end, desk->name,
                                   _cb_apps_list_selected, apps, NULL);
     }

   e_widget_ilist_go(apps->o_list);
   e_widget_ilist_thaw(apps->o_list);
   edje_thaw();
   evas_event_thaw(evas);
}

static void
_fill_apps_list(E_Config_App_List *apps)
{
   Eina_List *desks = NULL;
   Efreet_Desktop *desk = NULL;

   desks = efreet_util_desktop_name_glob_list("*");
   EINA_LIST_FREE(desks, desk)
     {
        Eina_List *ll;

        if (desk->no_display)
          {
             efreet_desktop_free(desk);
             continue;
          }
        ll = eina_list_search_unsorted_list(apps->desks, _cb_desks_sort, desk);
        if (ll)
          {
             Efreet_Desktop *old;

             old = eina_list_data_get(ll);
             /*
              * This fixes when we have several .desktop with the same name,
              * and the only difference is that some of them are for specific
              * desktops.
              */
             if ((old->only_show_in) && (!desk->only_show_in))
               {
                  efreet_desktop_free(old);
                  eina_list_data_set(ll, desk);
               }
             else
               efreet_desktop_free(desk);
          }
        else
          apps->desks = eina_list_append(apps->desks, desk);
     }
   apps->desks = eina_list_sort(apps->desks, -1, _cb_desks_sort);

   _list_items_state_set(apps);
}

static void
_fill_xdg_list(E_Config_App_List *apps)
{
   Eina_List *files;
   Efreet_Desktop *desk = NULL;
   const char *path = "/etc/xdg/autostart";
   char *file, *ext;
   char buf[4096];

   files = ecore_file_ls(path);
   EINA_LIST_FREE(files, file)
     {
	Eina_List *ll;

	if ((file[0] == '.') || !(ext = strrchr(file, '.')) || (strcmp(ext, ".desktop")))
	  {
	     free(file);
	     continue;
	  }
	snprintf(buf, sizeof(buf), "%s/%s", path, file);
	free(file);

	desk = efreet_desktop_new(buf);
	if (!desk)
	  continue;

        ll = eina_list_search_unsorted_list(apps->desks, _cb_desks_sort, desk);
        if (ll)
          {
             Efreet_Desktop *old;

             old = eina_list_data_get(ll);
             /*
              * This fixes when we have several .desktop with the same name,
              * and the only difference is that some of them are for specific
              * desktops.
              */
             if ((old->only_show_in) && (!desk->only_show_in))
               {
                  efreet_desktop_free(old);
                  eina_list_data_set(ll, desk);
               }
             else
               efreet_desktop_free(desk);
          }
        else
          apps->desks = eina_list_append(apps->desks, desk);
     }

   apps->desks = eina_list_sort(apps->desks, -1, _cb_desks_sort);

   _list_items_state_set(apps);
}

static void
_fill_order_list(E_Config_Dialog_Data *cfdata)
{
   Eina_List *l = NULL;
   Efreet_Desktop *desk = NULL;
   Evas *evas;

   evas = evas_object_evas_get(cfdata->o_list);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->o_list);
   e_widget_ilist_clear(cfdata->o_list);

   EINA_LIST_FOREACH(cfdata->apps, l, desk)
     {
        Evas_Object *icon = NULL;

        icon = e_util_desktop_icon_add(desk, 24, evas);
        e_widget_ilist_append(cfdata->o_list, icon, desk->name,
                              _cb_order_list_selected, cfdata, NULL);
     }

   e_widget_ilist_go(cfdata->o_list);
   e_widget_ilist_thaw(cfdata->o_list);
   edje_thaw();
   evas_event_thaw(evas);
}

static void
_cb_apps_list_selected(void *data)
{
   E_Config_App_List *apps;
   const E_Ilist_Item *it;
   Eina_List *l;
   unsigned int enabled = 0, disabled = 0;

   if (!(apps = data)) return;
   EINA_LIST_FOREACH(e_widget_ilist_items_get(apps->o_list), l, it)
     {
        if ((!it->selected) || (it->header)) continue;
        if (eina_list_search_unsorted(apps->cfdata->apps, _cb_desks_name, it->label))
          enabled++;
        else
          disabled++;
     }

   if (apps->o_desc)
     {
	Efreet_Desktop *desk;
	int sel;

	sel = e_widget_ilist_selected_get(apps->o_list);
	desk = eina_list_nth(apps->desks, sel);
	if (desk)
	  e_widget_textblock_markup_set(apps->o_desc, desk->comment);
     }

   e_widget_disabled_set(apps->o_add, !disabled);
   e_widget_disabled_set(apps->o_del, !enabled);
}

static void
_cb_order_list_selected(void *data)
{
   E_Config_Dialog_Data *cfdata;
   int sel, count;

   if (!(cfdata = data)) return;
   sel = e_widget_ilist_selected_get(cfdata->o_list);
   count = eina_list_count(cfdata->apps);
   e_widget_disabled_set(cfdata->o_up, (sel == 0));
   e_widget_disabled_set(cfdata->o_down, !(sel < (count - 1)));
   e_widget_disabled_set(cfdata->o_del, EINA_FALSE);
}

static int
_cb_desks_name(const void *data1, const void *data2)
{
   const Efreet_Desktop *d1;
   const char *d2;

   if (!(d1 = data1)) return 1;
   if (!d1->name) return 1;
   if (!(d2 = data2)) return -1;
   return strcmp(d1->name, d2);
}

static int
_cb_desks_sort(const void *data1, const void *data2)
{
   const Efreet_Desktop *d1, *d2;

   if (!(d1 = data1)) return 1;
   if (!d1->name) return 1;
   if (!(d2 = data2)) return -1;
   if (!d2->name) return -1;
   return strcmp(d1->name, d2->name);
}

static void
_cb_add(void *data, void *data2 __UNUSED__)
{
   E_Config_App_List *apps;
   const E_Ilist_Item *it;
   Eina_List *l;

   if (!(apps = data)) return;
   EINA_LIST_FOREACH(e_widget_ilist_items_get(apps->o_list), l, it)
     {
        Efreet_Desktop *desk;

        if ((!it->selected) || (it->header)) continue;
        if (!(desk = eina_list_search_unsorted(apps->desks, _cb_desks_name, it->label))) continue;
        if (!eina_list_search_unsorted(apps->cfdata->apps, _cb_desks_sort, desk))
          {
             Evas_Object *end;

             end = e_widget_ilist_item_end_get(it);
             if (end) edje_object_signal_emit(end, "e,state,checked", "e");
             efreet_desktop_ref(desk);
             apps->cfdata->apps = eina_list_append(apps->cfdata->apps, desk);
          }
     }
   e_widget_ilist_unselect(apps->o_list);
   e_widget_disabled_set(apps->o_add, EINA_TRUE);
   e_widget_disabled_set(apps->o_del, EINA_TRUE);
   _fill_order_list(apps->cfdata);
}

static void
_cb_del(void *data, void *data2 __UNUSED__)
{
   E_Config_App_List *apps;
   const E_Ilist_Item *it;
   Eina_List *l;

   if (!(apps = data)) return;
   EINA_LIST_FOREACH(e_widget_ilist_items_get(apps->o_list), l, it)
     {
        Efreet_Desktop *desk;

        if ((!it->selected) || (it->header)) continue;
        if ((desk = eina_list_search_unsorted(apps->cfdata->apps, _cb_desks_name, it->label)))
          {
             Evas_Object *end;

             end = e_widget_ilist_item_end_get(it);
             if (end) edje_object_signal_emit(end, "e,state,unchecked", "e");
             apps->cfdata->apps = eina_list_remove(apps->cfdata->apps, desk);
             efreet_desktop_unref(desk);
          }
     }

   e_widget_ilist_unselect(apps->o_list);
   e_widget_disabled_set(apps->o_add, EINA_TRUE);
   e_widget_disabled_set(apps->o_del, EINA_TRUE);
   _fill_order_list(apps->cfdata);
}

static void
_cb_order_del(void *data, void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   const E_Ilist_Item *it;
   Eina_List *l;

   if (!(cfdata = data)) return;
   EINA_LIST_FOREACH(e_widget_ilist_items_get(cfdata->o_list), l, it)
     {
	Efreet_Desktop *desk;

        if ((!it->selected) || (it->header)) continue;

	if ((desk = eina_list_search_unsorted(cfdata->apps, _cb_desks_name, it->label)))
	  {
	     cfdata->apps = eina_list_remove(cfdata->apps, desk);
	     efreet_desktop_unref(desk);
	  }
     }

   _list_items_state_set(&(cfdata->apps_xdg));
   _list_items_state_set(&(cfdata->apps_user));

   e_widget_ilist_unselect(cfdata->o_list);
   e_widget_disabled_set(cfdata->o_del, EINA_TRUE);
   _fill_order_list(cfdata);
}

static void
_cb_up(void *data, void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   Eina_List *l;
   Evas *evas;
   const char *lbl;
   int sel;

   if (!(cfdata = data)) return;
   evas = evas_object_evas_get(cfdata->o_list);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->o_list);

   sel = e_widget_ilist_selected_get(cfdata->o_list);
   lbl = e_widget_ilist_selected_label_get(cfdata->o_list);
   if ((l = eina_list_search_unsorted_list(cfdata->apps, _cb_desks_name, lbl)))
     {
        Efreet_Desktop *desk;
        Evas_Object *icon = NULL;

        desk = eina_list_data_get(l);
        if (l->prev)
          {
             Eina_List *ll;

             ll = l->prev;
             cfdata->apps = eina_list_remove_list(cfdata->apps, l);
             cfdata->apps = eina_list_prepend_relative_list(cfdata->apps, desk, ll);

             e_widget_ilist_remove_num(cfdata->o_list, sel);
             e_widget_ilist_go(cfdata->o_list);
             icon = e_util_desktop_icon_add(desk, 24, evas);
             e_widget_ilist_prepend_relative(cfdata->o_list, icon, desk->name,
                                             _cb_order_list_selected, cfdata,
                                             NULL, (sel - 1));
             e_widget_ilist_selected_set(cfdata->o_list, (sel - 1));
          }
     }

   e_widget_ilist_go(cfdata->o_list);
   e_widget_ilist_thaw(cfdata->o_list);
   edje_thaw();
   evas_event_thaw(evas);
}

static void
_cb_down(void *data, void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   Eina_List *l;
   Evas *evas;
   const char *lbl;
   int sel;

   if (!(cfdata = data)) return;
   evas = evas_object_evas_get(cfdata->o_list);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->o_list);

   sel = e_widget_ilist_selected_get(cfdata->o_list);
   lbl = e_widget_ilist_selected_label_get(cfdata->o_list);
   if ((l = eina_list_search_unsorted_list(cfdata->apps, _cb_desks_name, lbl)))
     {
        Efreet_Desktop *desk;
        Evas_Object *icon = NULL;

        desk = eina_list_data_get(l);
        if (l->next)
          {
             Eina_List *ll;

             ll = l->next;
             cfdata->apps = eina_list_remove_list(cfdata->apps, l);
             cfdata->apps = eina_list_append_relative_list(cfdata->apps, desk, ll);

             e_widget_ilist_remove_num(cfdata->o_list, sel);
             e_widget_ilist_go(cfdata->o_list);
             icon = e_util_desktop_icon_add(desk, 24, evas);
             e_widget_ilist_append_relative(cfdata->o_list, icon, desk->name,
                                            _cb_order_list_selected, cfdata,
                                            NULL, sel);
             e_widget_ilist_selected_set(cfdata->o_list, (sel + 1));
          }
     }

   e_widget_ilist_go(cfdata->o_list);
   e_widget_ilist_thaw(cfdata->o_list);
   edje_thaw();
   evas_event_thaw(evas);
}

static Eina_Bool
_cb_fill_delay(void *data)
{
   E_Config_Dialog_Data *cfdata;
   int mw;

   if (!(cfdata = data)) return ECORE_CALLBACK_CANCEL;
   _fill_apps_list(&cfdata->apps_user);
   e_widget_size_min_get(cfdata->apps_user.o_list, &mw, NULL);
   if (mw < (200 * e_scale)) mw = (200 * e_scale);
   e_widget_size_min_set(cfdata->apps_user.o_list, mw, (175 * e_scale));

   if (cfdata->data->show_autostart)
     {
	_fill_xdg_list(&cfdata->apps_xdg);
	e_widget_size_min_get(cfdata->apps_xdg.o_list, &mw, NULL);
	if (mw < (200 * e_scale)) mw = (200 * e_scale);
	e_widget_size_min_set(cfdata->apps_xdg.o_list, mw, (175 * e_scale));
     }

   cfdata->fill_delay = NULL;
   return ECORE_CALLBACK_CANCEL;
}
