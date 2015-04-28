#include "e.h"

/* local structures */
struct _E_Config_Dialog_Data 
{
   int show_favs, show_apps;
   int show_name, show_generic, show_comment;
   int menu_gadcon_client_toplevel;
   double scroll_speed, fast_mouse_move_threshhold;
   double click_drag_timeout;
   int autoscroll_margin, autoscroll_cursor_margin;
   const char *default_system_menu;
};

/* local function prototypes */
static void *_create_data(E_Config_Dialog *cfd);
static void _fill_data(E_Config_Dialog_Data *cfdata);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);

E_Config_Dialog *
e_int_config_menus(E_Container *con, const char *params __UNUSED__) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "menus/menu_settings")) return NULL;

   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;
   v->basic.check_changed = _basic_check_changed;

   cfd = e_config_dialog_new(con, _("Menu Settings"), "E", "menus/menu_settings", 
                             "preferences-menus", 0, v, NULL);
   return cfd;
}

/* local functions */
static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfdata);
   return cfdata;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata __UNUSED__)
{
   if (e_config->default_system_menu)
     cfdata->default_system_menu = 
     eina_stringshare_add(e_config->default_system_menu);
   else
     cfdata->default_system_menu = NULL;
   cfdata->show_favs = e_config->menu_favorites_show;
   cfdata->show_apps = e_config->menu_apps_show;
   cfdata->show_name = e_config->menu_eap_name_show;
   cfdata->show_generic = e_config->menu_eap_generic_show;
   cfdata->show_comment = e_config->menu_eap_comment_show;
   cfdata->menu_gadcon_client_toplevel = e_config->menu_gadcon_client_toplevel;
   cfdata->scroll_speed = e_config->menus_scroll_speed;
   cfdata->fast_mouse_move_threshhold = 
     e_config->menus_fast_mouse_move_threshhold;
   cfdata->click_drag_timeout = e_config->menus_click_drag_timeout;
   cfdata->autoscroll_margin = e_config->menu_autoscroll_margin;
   cfdata->autoscroll_cursor_margin = e_config->menu_autoscroll_cursor_margin;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   if (cfdata->default_system_menu) 
     eina_stringshare_del(cfdata->default_system_menu);
   E_FREE(cfdata);
}

static void
check_menu_dir(const char *dir, Eina_List **menus)
{
   char buf[PATH_MAX], *file;
   Eina_List *files;
   
   snprintf(buf, sizeof(buf), "%s/menus", dir);
   files = ecore_file_ls(buf);
   EINA_LIST_FREE(files, file)
     {
        if (e_util_glob_match(file, "*.menu"))
          {
             snprintf(buf, sizeof(buf), "%s/menus/%s", dir, file);
             *menus = eina_list_append(*menus, strdup(buf));
          }
        free(file);
     }
}
                            
void
get_menus(Eina_List **menus)
{
   char buf[PATH_MAX];
   const char *dirs[] =
     {
        "/etc/xdg",
        "/usr/etc/xdg",
        "/usr/local/etc/xdg",
        "/usr/opt/etc/xdg",
        "/usr/opt/xdg",
        "/usr/local/opt/etc/xdg",
        "/usr/local/opt/xdg",
        "/opt/etc/xdg",
        "/opt/xdg",
        // FIXME: add more "known locations"
        NULL
     };
   int i, newdir;
   
   e_user_homedir_concat(buf, sizeof(buf), ".config");
   check_menu_dir(buf, menus);
   
   for (i = 0; dirs[i]; i++) check_menu_dir(dirs[i], menus);
   
   newdir = 1;
   snprintf(buf, sizeof(buf), "%s/etc/xdg", e_prefix_get());
   for (i = 0; dirs[i]; i++)
     {
        if (!strcmp(dirs[i], buf))
          {
             newdir = 0;
             break;
          }
     }
   if (newdir) check_menu_dir(buf, menus);
}

static Evas_Object *
_create_menus_list(Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Eina_List *menus = NULL;
   Evas_Object *ob;
   char *file;
   int sel = -1, i = 0;
   
   get_menus(&menus);
   ob = e_widget_ilist_add(evas, (32 * e_scale), (32 * e_scale), 
                           &(cfdata->default_system_menu));
   e_widget_size_min_set(ob, (100 * e_scale), (140 * e_scale));
   e_widget_ilist_freeze(ob);
   
   EINA_LIST_FREE(menus, file)
     {
        char buf[PATH_MAX], buf2[PATH_MAX], *p, *p2, *tlabel, *tdesc;
        const char *label;
        
        label = file;
        tlabel = NULL;
        tdesc = NULL;
        e_user_homedir_concat(buf, sizeof(buf), 
                              ".config/menus/applications.menu");
        snprintf(buf2, sizeof(buf2), "%s/etc/xdg/menus/e-applications.menu", 
                 e_prefix_get());
        if (!strcmp("/etc/xdg/menus/applications.menu", file))
          {
             label = _("System Default");
             if (!cfdata->default_system_menu) sel = i;
          }
        else if (!strcmp(buf2, file))
          {
             label = _("Enlightenment Default");
             if (cfdata->default_system_menu)
               {
                  if (!strcmp(cfdata->default_system_menu, file)) sel = i;
               }
          }
        else if (!strcmp(buf, file))
          {
             label = _("Personal Default");
             if (cfdata->default_system_menu)
               {
                  if (!strcmp(cfdata->default_system_menu, file)) sel = i;
               }
          }
        else
          {
             p = strrchr(file, '/');
             if (p)
               {
                  p++;
                  p2 = strchr(p, '-');
                  if (!p2) p2 = strrchr(p, '.');
                  if (p2)
                    {
                       tlabel = malloc(p2 - p + 1);
                       if (tlabel)
                         {
                            eina_strlcpy(tlabel, p, p2 - p + 1);
                            tlabel[0] = toupper(tlabel[0]);
                            if (*p2 == '-')
                              {
                                 p2++;
                                 p = strrchr(p2, '.');
                                 if (p)
                                   {
                                      tdesc = malloc(p - p2 + 1);
                                      if (tdesc)
                                        {
                                           eina_strlcpy(tdesc, p2, p - p2 + 1);
                                           tdesc[0] = toupper(tdesc[0]);
                                           snprintf(buf, sizeof(buf), "%s (%s)", tlabel, tdesc);
                                        }
                                      else
                                        snprintf(buf, sizeof(buf), "%s", tlabel);
                                   }
                                 else
                                   snprintf(buf, sizeof(buf), "%s", tlabel);
                              }
                            else
                              snprintf(buf, sizeof(buf), "%s", tlabel);
                            label = buf;
                         }
                    }
                  else
                    label = p;
               }
             if (cfdata->default_system_menu)
               {
                  if (!strcmp(cfdata->default_system_menu, file)) sel = i;
               }
          }
        e_widget_ilist_append(ob, NULL, label, NULL, NULL, file);
        free(tlabel);
        free(tdesc);
        free(file);
        i++;
     }
   e_widget_ilist_go(ob);
   e_widget_ilist_thaw(ob);
   
   if (sel >= 0) e_widget_ilist_selected_set(ob, sel);
   
   return ob;
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *otb, *ol, *of, *ow;

   otb = e_widget_toolbook_add(evas, (24 * e_scale), (24 * e_scale));

   ol = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("Main Menu"), 0); 
   ow = e_widget_check_add(evas, _("Favorites"), &(cfdata->show_favs));
   e_widget_framelist_object_append(of, ow); 
   ow = e_widget_check_add(evas, _("Applications"), &(cfdata->show_apps));
   e_widget_framelist_object_append(of, ow);
   e_widget_list_object_append(ol, of, 1, 0, 0.5);

   of = e_widget_framelist_add(evas, _("Applications Display"), 0); 
   ow = e_widget_check_add(evas, _("Name"), &(cfdata->show_name));
   e_widget_framelist_object_append(of, ow); 
   ow = e_widget_check_add(evas, _("Generic"), &(cfdata->show_generic));
   e_widget_framelist_object_append(of, ow); 
   ow = e_widget_check_add(evas, _("Comments"), &(cfdata->show_comment));
   e_widget_framelist_object_append(of, ow); 
   e_widget_list_object_append(ol, of, 1, 0, 0.5);

   of = e_widget_framelist_add(evas, _("Gadgets"), 0); 
   ow = e_widget_check_add(evas, _("Show gadget settings in top-level"), &(cfdata->menu_gadcon_client_toplevel));
   e_widget_framelist_object_append(of, ow);
   e_widget_list_object_append(ol, of, 1, 1, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Menus"), ol, 1, 0, 1, 0, 
                                 0.5, 0.0);

   ol = e_widget_list_add(evas, 0, 0);
   ow = _create_menus_list(evas, cfdata);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);   
   e_widget_toolbook_page_append(otb, NULL, _("Applications"), ol, 1, 0, 1, 0, 
                                 0.5, 0.0);

   ol = e_widget_list_add(evas, 0, 0);
   ow = e_widget_label_add(evas, _("Margin"));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%2.0f pixels"), 0, 50, 1, 0, NULL, 
                            &(cfdata->autoscroll_margin), 100);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_label_add(evas, _("Cursor Margin"));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%2.0f pixels"), 0, 50, 1, 0, NULL, 
                            &(cfdata->autoscroll_cursor_margin), 100);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Autoscroll"), ol, 1, 0, 1, 0, 
                                 0.5, 0.0);

   ol = e_widget_list_add(evas, 0, 0);
   ow = e_widget_label_add(evas, _("Menu Scroll Speed"));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%5.0f pixels/s"), 0, 20000, 100, 
                            0, &(cfdata->scroll_speed), NULL, 100);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_label_add(evas, _("Fast Mouse Move Threshold"));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%4.0f pixels/s"), 0, 2000, 10, 
                            0, &(cfdata->fast_mouse_move_threshhold), NULL, 100);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_label_add(evas, _("Click Drag Timeout"));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%2.2f s"), 0, 10, 0.25, 
                            0, &(cfdata->click_drag_timeout), NULL, 100);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Miscellaneous"), ol, 1, 0, 1, 0, 
                                 0.5, 0.0);

   e_widget_toolbook_page_show(otb, 0);
   return otb;
}

static int
_basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   e_config->menu_favorites_show = cfdata->show_favs;
   e_config->menu_apps_show = cfdata->show_apps;
   e_config->menu_eap_name_show = cfdata->show_name;
   e_config->menu_eap_generic_show = cfdata->show_generic;
   e_config->menu_eap_comment_show = cfdata->show_comment;
   e_config->menu_gadcon_client_toplevel = cfdata->menu_gadcon_client_toplevel;

   if (cfdata->scroll_speed == 0) e_config->menus_scroll_speed = 1.0;
   else e_config->menus_scroll_speed = cfdata->scroll_speed;

   if (cfdata->fast_mouse_move_threshhold == 0)
     e_config->menus_fast_mouse_move_threshhold = 1.0;
   else 
     {
        e_config->menus_fast_mouse_move_threshhold = 
          cfdata->fast_mouse_move_threshhold;
     }
   e_config->menus_click_drag_timeout = cfdata->click_drag_timeout;
   e_config->menu_autoscroll_margin = cfdata->autoscroll_margin;
   e_config->menu_autoscroll_cursor_margin = cfdata->autoscroll_cursor_margin;
   if (cfdata->default_system_menu)
     {
        if (e_config->default_system_menu)
          eina_stringshare_del(e_config->default_system_menu);
        e_config->default_system_menu = 
          eina_stringshare_add(cfdata->default_system_menu);
     }
   else
     {
        if (e_config->default_system_menu)
          eina_stringshare_del(e_config->default_system_menu);
        e_config->default_system_menu = NULL;
     }
   e_config_save_queue();
   return 1;
}

static int
_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   double scroll_speed, move_threshold;

   if (cfdata->scroll_speed == 0)
     scroll_speed = 1.0;
   else
     scroll_speed = cfdata->scroll_speed;

   if (cfdata->fast_mouse_move_threshhold == 0)
     move_threshold = 1.0;
   else
     move_threshold = cfdata->fast_mouse_move_threshhold;

   return ((e_config->menu_favorites_show != cfdata->show_favs) ||
	   (e_config->menu_apps_show != cfdata->show_apps) ||
	   (e_config->menu_eap_name_show != cfdata->show_name) ||
	   (e_config->menu_eap_generic_show != cfdata->show_generic) ||
	   (e_config->menu_eap_comment_show != cfdata->show_comment) ||
	   (e_config->menus_click_drag_timeout != cfdata->click_drag_timeout) ||
	   (e_config->menu_autoscroll_margin != cfdata->autoscroll_margin) ||
	   (e_config->menu_autoscroll_cursor_margin != cfdata->autoscroll_cursor_margin) ||
	   (e_config->menus_scroll_speed != scroll_speed) ||
	   (e_config->menus_fast_mouse_move_threshhold != move_threshold) ||
	   (e_config->menu_gadcon_client_toplevel != cfdata->menu_gadcon_client_toplevel) ||
           (!((cfdata->default_system_menu) &&
              (e_config->default_system_menu) &&
              (!strcmp(cfdata->default_system_menu,
                       e_config->default_system_menu)))));
}
