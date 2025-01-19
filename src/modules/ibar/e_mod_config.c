#include "e.h"
#include "e_mod_main.h"

struct _E_Config_Dialog_Data
{
   const char       *dir;
   int               show_label, eap_label, show_label_adjac;
   int               lock_move;
   int               track_launch;
   int               dont_add_nonorder;
   int               icon_menu_mouseover;
   int               focus_flash;
   int               control;

   Evas_Object      *tlist;
   Evas_Object      *radio_name;
   Evas_Object      *radio_comment;
   Evas_Object      *radio_generic;
   Evas_Object      *label_adj;
   Evas_Object      *label;
   E_Confirm_Dialog *dialog_delete;
};

/* Protos */
static void        *_create_data(E_Config_Dialog *cfd);
static void         _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int          _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static void         _cb_add(void *data, void *data2);
static void         _cb_del(void *data, void *data2);
static void         _cb_config(void *data, void *data2);
static void         _cb_entry_ok(void *data, char *text);
static void         _cb_confirm_dialog_yes(void *data);
static void         _cb_confirm_dialog_destroy(void *data);
static void         _load_tlist(E_Config_Dialog_Data *cfdata);
static void         _show_label_cb_change(void *data, Evas_Object *obj);
static void         _show_label_adj_cb_change(void *data, Evas_Object *obj __UNUSED__);

void
_config_ibar_module(Config_Item *ci)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   char buf[4096];

   v = E_NEW(E_Config_Dialog_View, 1);

   /* Dialog Methods */
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->advanced.apply_cfdata = NULL;
   v->advanced.create_widgets = NULL;

   snprintf(buf, sizeof(buf), "%s/e-module-ibar.edj",
            e_module_dir_get(ibar_config->module));

   /* Create The Dialog */
   cfd = e_config_dialog_new(e_container_current_get(e_manager_current_get()),
                             _("IBar Settings"),
                             "E", "_e_mod_ibar_config_dialog",
                             buf, 0, v, ci);

   ibar_config->config_dialog = cfd;
}

static void
_fill_data(Config_Item *ci, E_Config_Dialog_Data *cfdata)
{
   if (ci->dir)
     cfdata->dir = eina_stringshare_ref(ci->dir);
   else
     cfdata->dir = eina_stringshare_add("");
   cfdata->show_label = ci->show_label;
   cfdata->focus_flash = ci->focus_flash;
   cfdata->control = ci->control;
   cfdata->eap_label = ci->eap_label;
   cfdata->lock_move = ci->lock_move;
   cfdata->dont_add_nonorder = ci->dont_add_nonorder;
   cfdata->track_launch = !ci->dont_track_launch;
   cfdata->icon_menu_mouseover = !ci->dont_icon_menu_mouseover;
   cfdata->show_label_adjac = ci->show_label_adjac;
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;
   Config_Item *ci;

   ci = cfd->data;
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(ci, cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   if (cfdata->dir) eina_stringshare_del(cfdata->dir);
   if (cfdata->dialog_delete) e_object_del(E_OBJECT(cfdata->dialog_delete));
   ibar_config->config_dialog = NULL;
   E_FREE(cfdata);
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ol, *ob, *ot;
   E_Radio_Group *rg;

   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_frametable_add(evas, _("Selected source"), 0);
   ol = e_widget_ilist_add(evas, 32, 32, &(cfdata->dir));
   cfdata->tlist = ol;
   _load_tlist(cfdata);
   e_widget_size_min_set(ol, 140, 40);
   e_widget_frametable_object_append(of, ol, 0, 0, 1, 2, 1, 1, 1, 0);

   ot = e_widget_table_add(evas, 0);
   ob = e_widget_button_add(evas, _("Add"), "list-add", _cb_add, cfdata, NULL);
   e_widget_table_object_append(ot, ob, 0, 0, 1, 1, 1, 1, 1, 0);
   ob = e_widget_button_add(evas, _("Delete"), "list-remove", _cb_del, cfdata, NULL);
   e_widget_table_object_append(ot, ob, 0, 1, 1, 1, 1, 1, 1, 0);
   ob = e_widget_button_add(evas, _("Setup"), "configure", _cb_config, cfdata, NULL);
   e_widget_table_object_append(ot, ob, 0, 2, 1, 1, 1, 1, 1, 0);

   if (!e_configure_registry_exists("applications/ibar_applications"))
     e_widget_disabled_set(ob, 1);

   e_widget_frametable_object_append(of, ot, 1, 0, 1, 1, 1, 1, 1, 0);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   of = e_widget_framelist_add(evas, _("Icon Labels"), 0);
    cfdata->label = e_widget_check_add(evas, _("Overlapping Label"), &(cfdata->show_label));
   e_widget_on_change_hook_set(cfdata->label, _show_label_cb_change, cfdata);
   e_widget_framelist_object_append(of, cfdata->label);

   cfdata->label_adj = e_widget_check_add(evas, _("Adjacent Label"), &(cfdata->show_label_adjac));
   e_widget_on_change_hook_set(cfdata->label_adj, _show_label_adj_cb_change, cfdata);
   e_widget_framelist_object_append(of, cfdata->label_adj);

   rg = e_widget_radio_group_new(&(cfdata->eap_label));

   cfdata->radio_name = e_widget_radio_add(evas, _("Name"), 0, rg);
   e_widget_framelist_object_append(of, cfdata->radio_name);
   if ((!cfdata->show_label) && (!cfdata->show_label_adjac)) e_widget_disabled_set(cfdata->radio_name, 1);

   cfdata->radio_comment = e_widget_radio_add(evas, _("Comment"), 1, rg);
   e_widget_framelist_object_append(of, cfdata->radio_comment);
   if ((!cfdata->show_label) && (!cfdata->show_label_adjac)) e_widget_disabled_set(cfdata->radio_comment, 1);

   cfdata->radio_generic = e_widget_radio_add(evas, _("Generic"), 2, rg);
   e_widget_framelist_object_append(of, cfdata->radio_generic);
   if ((!cfdata->show_label) && (!cfdata->show_label_adjac)) e_widget_disabled_set(cfdata->radio_generic, 1);

   e_widget_list_object_append(o, of, 1, 1, 0.5);

   of = e_widget_framelist_add(evas, _("Misc"), 0);
   ob = e_widget_check_add(evas, _("Icon focus flash"), &(cfdata->focus_flash));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Lock icon move"), &(cfdata->lock_move));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Don't add items on launch"), &(cfdata->dont_add_nonorder));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Track launch"), &(cfdata->track_launch));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Menu on mouse over"), &(cfdata->icon_menu_mouseover));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Moksha style control"), &(cfdata->control));
   e_widget_framelist_object_append(of, ob);

   e_widget_list_object_append(o, of, 1, 1, 0.5);

   return o;
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   Config_Item *ci;

   ci = cfd->data;
   if (ci->dir) eina_stringshare_del(ci->dir);
   ci->dir = NULL;
   if (cfdata->dir) ci->dir = eina_stringshare_ref(cfdata->dir);
   ci->show_label = cfdata->show_label;
   ci->eap_label = cfdata->eap_label;
   ci->lock_move = cfdata->lock_move;
   ci->dont_add_nonorder = cfdata->dont_add_nonorder;
   ci->dont_track_launch = !cfdata->track_launch;
   ci->focus_flash = cfdata->focus_flash;
   ci->control = cfdata->control;
   ci->dont_icon_menu_mouseover = !cfdata->icon_menu_mouseover;
   ci->show_label_adjac = cfdata->show_label_adjac;
   _ibar_config_update(ci);
   e_config_save_queue();
   return 1;
}

static void
_cb_add(void *data, void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   e_entry_dialog_show(_("Create new IBar source"), "enlightenment",
                       _("Enter a name for this new source:"), "", NULL, NULL,
                       _cb_entry_ok, NULL, cfdata);
}

static void
_cb_del(void *data, void *data2 __UNUSED__)
{
   char buf[4096];
   E_Config_Dialog_Data *cfdata;
   E_Confirm_Dialog *dialog;

   cfdata = data;
   if (cfdata->dialog_delete) return;

   snprintf(buf, sizeof(buf), _("You requested to delete \"%s\".<br><br>"
                                "Are you sure you want to delete this bar source?"),
            cfdata->dir);

   dialog = e_confirm_dialog_show(_("Are you sure you want to delete this bar source?"),
                                  "application-exit", buf, _("Delete"), _("Keep"),
                                  _cb_confirm_dialog_yes, NULL, cfdata, NULL,
                                  _cb_confirm_dialog_destroy, cfdata);
   cfdata->dialog_delete = dialog;
}

static void
_cb_config(void *data, void *data2 __UNUSED__)
{
   char path[PATH_MAX];
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   e_user_dir_snprintf(path, sizeof(path), "applications/bar/%s/.order",
                       cfdata->dir);
   e_configure_registry_call("internal/ibar_other",
                             e_container_current_get(e_manager_current_get()),
                             path);
}

static void
_cb_entry_ok(void *data, char *text)
{
   char buf[4096];
   char tmp[4096] = {0};
   FILE *f;
   size_t len;

   len = e_user_dir_snprintf(buf, sizeof(buf), "applications/bar/%s", text);
   if (len + sizeof("/.order") >= sizeof(buf)) return;
   while (!ecore_file_exists(buf))
     {
        ecore_file_mkdir(buf);
        memcpy(buf + len, "/.order", sizeof("/.order"));
        e_user_dir_concat_static(tmp, "applications/bar/default/.order");
        if (ecore_file_cp(tmp, buf)) break;
        f = fopen(buf, "w");
        if (!f) break;
        /* Populate this .order file with some defaults */
        snprintf(tmp, sizeof(tmp), 
                 "terminology.desktop\n"
                 "sylpheed.desktop\n"
                 "firefox.desktop\n"
                 "openoffice.desktop\n" 
                 "xchat.desktop\n"
                 "gimp.desktop\n");
        fwrite(tmp, sizeof(char), strlen(tmp), f);
        fclose(f);
        break;
     }

   _load_tlist(data);
}

static void
_cb_confirm_dialog_yes(void *data)
{
   E_Config_Dialog_Data *cfdata;
   char buf[4096];

   cfdata = data;
   if (e_user_dir_snprintf(buf, sizeof(buf), "applications/bar/%s", cfdata->dir) >= sizeof(buf))
     return;
   if (ecore_file_is_dir(buf))
     ecore_file_recursive_rm(buf);

   _load_tlist(cfdata);
}

static void
_cb_confirm_dialog_destroy(void *data)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   cfdata->dialog_delete = NULL;
}

static void
_load_tlist(E_Config_Dialog_Data *cfdata)
{
   Eina_List *dirs;
   char buf[4096], *file;
   int selnum = -1;
   int i = 0;
   size_t len;

   e_widget_ilist_clear(cfdata->tlist);

   len = e_user_dir_concat_static(buf, "applications/bar");
   if (len + 2 >= sizeof(buf)) return;
   dirs = ecore_file_ls(buf);

   buf[len] = '/';
   len++;

   EINA_LIST_FREE(dirs, file)
     {
        if (file[0] == '.') continue;
        if (eina_strlcpy(buf + len, file, sizeof(buf) - len) >= sizeof(buf) - len)
          continue;
        if (ecore_file_is_dir(buf))
          {
             e_widget_ilist_append(cfdata->tlist, NULL, file, NULL, NULL, file);
             if ((cfdata->dir) && (!strcmp(cfdata->dir, file)))
               selnum = i;
             i++;
          }

        free(file);
     }

   e_widget_ilist_go(cfdata->tlist);
   if (selnum >= 0) e_widget_ilist_selected_set(cfdata->tlist, selnum);
}

static void
labels_show(void *data)
{
   E_Config_Dialog_Data *cfdata = data;

   if (!cfdata) return;
   if (cfdata->show_label || cfdata->show_label_adjac)
     {
       e_widget_disabled_set(cfdata->radio_name, 0);
       e_widget_disabled_set(cfdata->radio_comment, 0);
       e_widget_disabled_set(cfdata->radio_generic, 0);
     }
   else
     {
       e_widget_disabled_set(cfdata->radio_name, 1);
       e_widget_disabled_set(cfdata->radio_comment, 1);
       e_widget_disabled_set(cfdata->radio_generic, 1);
     }
}

static void
_show_label_cb_change(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   if (!cfdata) return;

   labels_show(cfdata);
}

static void
_show_label_adj_cb_change(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   if (!cfdata) return;

   labels_show(cfdata);
}
