#include "e.h"
#include "e_mod_main.h"

#define TEXT_NONE_ACTION_KEY    _("<None>")

static E_Dialog *dia;

static void
_edit_bindings()
{
   E_Container *con;

   con = e_container_current_get(e_manager_current_get());
   e_int_config_keybindings(con, NULL);
}

static const char *
_key_binding_get(const char *action, const char *params)
{
   Eina_List *l = NULL;
   E_Config_Binding_Key *bi;
   Eina_Strbuf *b;
   char mod[PATH_MAX];
   char *ret;

   b = eina_strbuf_new();

   EINA_LIST_FOREACH(e_config->key_bindings, l, bi)
     {
       if (!bi) return NULL;

       bi->key = eina_stringshare_add(bi->key);
       bi->action = eina_stringshare_add(bi->action);
       bi->params = eina_stringshare_ref(bi->params);
       if (!bi->params) bi->params =  eina_stringshare_add("");

       if ((!e_util_strcmp(bi->action, action)) &&
           (!e_util_strcmp(bi->params, params)) &&
            (e_util_strcmp(bi->key, "Execute")))
         {
           switch (bi->modifiers)
             {
               case 0:
                      sprintf(mod, "%s", bi->key);
                      break;
               case 1:
                      sprintf(mod, "SHIFT %s", bi->key);
                      break;
               case 2:
                      sprintf(mod, "CTRL %s", bi->key);
                      break;
               case 3:
                      sprintf(mod, "CTRL SHIFT %s", bi->key);
                      break;
               case 4:
                      sprintf(mod, "ALT %s", bi->key);
                      break;
               case 5:
                      sprintf(mod, "ALT SHIFT %s", bi->key);
                      break;
               case 6:
                      sprintf(mod, "CTRL ALT %s", bi->key);
                      break;
               case 8:
                      sprintf(mod, "WIN %s", bi->key);
                      break;
             }

           if (eina_strbuf_length_get(b) > 0)
             eina_strbuf_append(b, "; ");
           eina_strbuf_append(b, mod);
           eina_stringshare_del(bi->action);
           eina_stringshare_del(bi->params);
         }
     }

   ret = eina_strbuf_string_steal(b);
   eina_strbuf_free(b);
   if (ret[0]) return ret;
   free(ret);
   return strdup(TEXT_NONE_ACTION_KEY);
}

static void
_key_dialog_del(void *data)
{
   if (dia == data)
     dia = NULL;
}

static void
_refresh_dialog(void *data __UNUSED__, E_Dialog *dialog __UNUSED__)
{
  show_keybidings();
}

static void
fill_dia_data(void *data __UNUSED__, E_Dialog *dialog __UNUSED__)
{
   Evas_Object *o, *ob, *ot;
   Evas_Coord mw, mh, sw, sh;

   o = e_widget_table_add(dia->win->evas, 0);

   ot = e_widget_frametable_add(o, _("WINDOWS"), 1);
   ob = e_widget_label_add(o, _("Close Window:"));
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 0, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("window_close", ""), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 0, 1, 1, 1, 0, 1, 0);

   ob = e_widget_label_add(o, _("Maximize Window:"));
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 1, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("window_maximized_toggle",  ""), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 1, 1, 1, 1, 0, 1, 0);

   ob = e_widget_label_add(o, _("Iconic Mode Toggle:"));
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 2, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("window_iconic_toggle",  ""), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 2, 1, 1, 1, 0, 1, 0);

   ob = e_widget_label_add(o, _("Fullscreen Mode Toggle:"));
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 3, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("window_fullscreen_toggle", ""), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 3, 1, 1, 1, 0, 1, 0);

   ob = e_widget_label_add(o, _("Always On Top Toggle:"));
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 4, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("window_stack_top_toggle", ""), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 4, 1, 1, 1, 0, 1, 0);

   ob = e_widget_label_add(o, _("Next Window:"));
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 5, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("winlist", "next"), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 5, 1, 1, 1, 0, 1, 0);

   ob = e_widget_label_add(o, _("Tile Right:"));
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 6, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("window_maximized_toggle", "default right"), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 6, 1, 1, 1, 0, 1, 0);

   ob = e_widget_label_add(o, _("Tile Left:"));
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 7, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("window_maximized_toggle", "default left"), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 7, 1, 1, 1, 0, 1, 0);

   ob = e_widget_label_add(o, _("Kill Window:"));
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 8, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("window_kill", ""), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 8, 1, 1, 1, 0, 1, 0);

   ob = e_widget_label_add(o, _("Window Menu:"));
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 9, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("window_menu", ""), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 9, 1, 1, 1, 0, 1, 0);

   e_widget_table_object_append(o, ot, 0, 0, 1, 1, 1, 1, 1, 1);

   //----------------------------------------------
   ot = e_widget_frametable_add(o, _("SYSTEM"), 1);
   ob = e_widget_label_add(o, _("Open Terminology:"));
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 0, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("exec", "terminology"), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 0, 1, 1, 1, 0, 1, 0);

   ob = e_widget_label_add(o, _("Lock System:"));
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 1, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("desk_lock", ""), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 1, 1, 1, 1, 0, 1, 0);

   ob = e_widget_label_add(o, _("Show Main Menu:"));
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 2, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("menu_show", "main"), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 2, 1, 1, 1, 0, 1, 0);

   ob = e_widget_label_add(o, _("Show Favorites Menu:"));
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 3, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("menu_show", "favorites"), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 3, 1, 1, 1, 0, 1, 0);

   ob = e_widget_label_add(o, _("System Controls:"));
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 4, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("syscon", ""), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 4, 1, 1, 1, 0, 1, 0);

   e_widget_table_object_append(o, ot, 0, 1, 1, 1, 1, 1, 1, 1);

    //----------------------------------------------
   ot = e_widget_frametable_add(o, _("DESKTOP"), 0);

   ob = e_widget_label_add(o, _(" Show the Desktop:"));
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 0, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("desk_deskshow_toggle", ""), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 0, 1, 1, 1, 0, 1, 0);

   ob = e_widget_label_add(o, _(" Switch to Desktop 0:"));
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 1, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("desk_linear_flip_to", "0"), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 1, 1, 1, 1, 0, 1, 0);

   ob = e_widget_label_add(o, _(" Switch to Desktop 1:"));
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 2, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("desk_linear_flip_to", "1"), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 2, 1, 1, 1, 0, 1, 0);

   ob = e_widget_label_add(o, _(" Switch to Desktop 2:"));
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 3, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("desk_linear_flip_to", "2"), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 3, 1, 1, 1, 0, 1, 0);

   ob = e_widget_label_add(o, _(" Switch to Desktop 3:"));
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 4, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("desk_linear_flip_to", "3"), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 4, 1, 1, 1, 0, 1, 0);

   ob = e_widget_label_add(o, _(" Switch to Desktop 4:"));
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 5, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("desk_linear_flip_to", "4"), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 5, 1, 1, 1, 0, 1, 0);

   ob = e_widget_label_add(o, _(" Switch to Desktop 5:"));
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 6, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("desk_linear_flip_to", "5"), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 6, 1, 1, 1, 0, 1, 0);

   ob = e_widget_label_add(o, _(" Switch to Desktop 6:"));
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 7, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("desk_linear_flip_to", "6"), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 7, 1, 1, 1, 0, 1, 0);

   ob = e_widget_label_add(o, _(" Switch to Desktop 7:"));
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 8, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("desk_linear_flip_to", "7"), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 8, 1, 1, 1, 0, 1, 0);

   ob = e_widget_label_add(o, _(" Switch to Desktop 8:"));
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 9, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("desk_linear_flip_to", "8"), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 9, 1, 1, 1, 0, 1, 0);

   e_widget_table_object_append(o, ot, 1, 0, 1, 1, 1, 1, 1, 1);

    //----------------------------------------------
   ot = e_widget_frametable_add(o, _("MISCELLANEOUS"), 0);
   ob = e_widget_label_add(o, _("Quick Launcher:"));
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 0, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("everything", ""), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 0, 1, 1, 1, 0, 1, 0);

   ob = e_widget_label_add(o, _("Take Screenshot:"));
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 1, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("shot", ""), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 1, 1, 1, 1, 0, 1, 0);

   ob = e_widget_label_add(o, _("Restart Moksha:"));
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 2, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("restart", ""), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 2, 1, 1, 1, 0, 1, 0);

   ob = e_widget_label_add(o, _("Terminal console:"));
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 3, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, "CTRL ALT F1..F7", NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 3, 1, 1, 1, 0, 1, 0);

   ob = e_widget_label_add(o, _("This Help:"));
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 4, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("show_keybinds", ""), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 4, 1, 1, 1, 0, 1, 0);

   e_widget_table_object_append(o, ot, 1, 1, 1, 1, 1, 1, 1, 1);

   e_widget_size_min_get(o, &mw, &mh);
   e_dialog_content_set(dia, o, mw, mh);
}

void
show_keybidings()
{
   if (dia) e_util_defer_object_del(E_OBJECT(dia));

   dia = e_dialog_new(e_container_current_get(e_manager_current_get()),
                      "E", "_show_keybindings");
   if (!dia) return;
   e_dialog_title_set(dia, _("Basic Moksha Key Bindings"));
   e_dialog_button_add(dia, _("OK"), NULL, NULL, NULL);
   e_dialog_button_add(dia, _("Edit"), NULL, _edit_bindings, NULL);
   e_dialog_button_add(dia, _("Refresh"), "view-refresh", _refresh_dialog, NULL);

   e_win_centered_set(dia->win, 1);
   e_dialog_resizable_set(dia, 0);
   e_object_del_attach_func_set(E_OBJECT(dia), _key_dialog_del);

   fill_dia_data(NULL, dia);
   e_dialog_show(dia);
   e_dialog_border_icon_set(dia, "preferences-desktop-keyboard-shortcuts");
}

