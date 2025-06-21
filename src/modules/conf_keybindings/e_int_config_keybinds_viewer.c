#include "e.h"
#include "e_mod_main.h"

#define TEXT_NONE_ACTION_KEY    _("<None>")

static E_Dialog *dia;
static Eina_Bool show = EINA_FALSE;

static void
_edit_bindings(void *data, E_Dialog *dia)
{
   (void)data;
   (void)dia;
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
   char *mod, *ret;

   b = eina_strbuf_new();
   mod = malloc(sizeof(char) * 32);

   EINA_LIST_FOREACH(e_config->key_bindings, l, bi)
     {
       if (!bi) return NULL;

       bi->key = eina_stringshare_add(bi->key);
       bi->action = eina_stringshare_add(bi->action);
       bi->params = eina_stringshare_ref(bi->params);
       if (!bi->params) bi->params = eina_stringshare_add("");

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
           if (!strcmp(mod, "CTRL ALT space")) continue;
           if (eina_strbuf_length_get(b) > 0)
             eina_strbuf_append(b, "; ");
           if (strlen(bi->key) == 1)
             eina_str_toupper(&mod);
           eina_strbuf_append(b, mod);
           eina_stringshare_del(bi->action);
           eina_stringshare_del(bi->params);
         }
     }

   ret = eina_strbuf_string_steal(b);
   eina_strbuf_free(b);
   free(mod);
   if (ret[0]) return ret;
   free(ret);
   return strdup(TEXT_NONE_ACTION_KEY);
}

static void
_key_dialog_del(void *data)
{
   if (dia == data)
     dia = NULL;
   show = EINA_FALSE;
}

static void
_refresh_dialog(void *data __UNUSED__, E_Dialog *dialog __UNUSED__)
{
   show = EINA_TRUE;
   show_keybidings();
}

static void
fill_dia_data(void *data __UNUSED__, E_Dialog *dialog __UNUSED__)
{
   Evas_Object *o, *ob, *ot;
   Evas_Coord mw, mh, sw, sh;
   char buf[64];

   o = e_widget_table_add(dia->win->evas, 0);

   ot = e_widget_frametable_add(o, _("WINDOW"), 1);
   sprintf(buf, "%s:", _("Close"));
   ob = e_widget_label_add(o, buf);
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 0, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("window_close", ""), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 0, 1, 1, 1, 0, 1, 0);

   sprintf(buf, "%s:", _("Maximize"));
   ob = e_widget_label_add(o, buf);
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 1, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("window_maximized_toggle", ""), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 1, 1, 1, 1, 0, 1, 0);

   sprintf(buf, "%s:", _("Iconic Mode Toggle"));
   ob = e_widget_label_add(o, buf);
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 2, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("window_iconic_toggle", ""), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 2, 1, 1, 1, 0, 1, 0);

   sprintf(buf, "%s:", _("Fullscreen Mode Toggle"));
   ob = e_widget_label_add(o, buf);
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 3, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("window_fullscreen_toggle", ""), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 3, 1, 1, 1, 0, 1, 0);

   sprintf(buf, "%s:", _("Always On Top Toggle"));
   ob = e_widget_label_add(o, buf);
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 4, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("window_stack_top_toggle", ""), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 4, 1, 1, 1, 0, 1, 0);

   sprintf(buf, "%s:", _("Next Window"));
   ob = e_widget_label_add(o, buf);
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 5, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("winlist", "next"), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 5, 1, 1, 1, 0, 1, 0);

   sprintf(buf, "%s:", _("Maximize Right"));
   ob = e_widget_label_add(o, buf);
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 6, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("window_maximized_toggle", "default right"), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 6, 1, 1, 1, 0, 1, 0);

   sprintf(buf, "%s:", _("Maximize Left"));
   ob = e_widget_label_add(o, buf);
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 7, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("window_maximized_toggle", "default left"), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 7, 1, 1, 1, 0, 1, 0);

   sprintf(buf, "%s:", _("Kill"));
   ob = e_widget_label_add(o, buf);
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 8, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("window_kill", ""), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 8, 1, 1, 1, 0, 1, 0);

   sprintf(buf, "%s:", _("Window Menu"));
   ob = e_widget_label_add(o, buf);
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 9, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("window_menu", ""), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 9, 1, 1, 1, 0, 1, 0);

   e_widget_table_object_append(o, ot, 0, 0, 1, 1, 1, 1, 1, 1);

   //----------------------------------------------
   ot = e_widget_frametable_add(o, _("SYSTEM"), 1);
   sprintf(buf, "%s:", _("Open Terminology"));
   ob = e_widget_label_add(o, buf);
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 0, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("exec", "terminology"), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 0, 1, 1, 1, 0, 1, 0);

   sprintf(buf, "%s:", _("Lock"));
   ob = e_widget_label_add(o, buf);
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 1, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("desk_lock", ""), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 1, 1, 1, 1, 0, 1, 0);

   sprintf(buf, "%s:", _("Show Main Menu"));
   ob = e_widget_label_add(o, buf);
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 2, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("menu_show", "main"), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 2, 1, 1, 1, 0, 1, 0);

   sprintf(buf, "%s:", _("Show Favorites Menu"));
   ob = e_widget_label_add(o, buf);
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 3, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("menu_show", "favorites"), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 3, 1, 1, 1, 0, 1, 0);

   sprintf(buf, "%s:", _("System Controls"));
   ob = e_widget_label_add(o, buf);
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 4, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("syscon", ""), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 4, 1, 1, 1, 0, 1, 0);

   e_widget_table_object_append(o, ot, 0, 1, 1, 1, 1, 1, 1, 1);

    //----------------------------------------------
   ot = e_widget_frametable_add(o, _("DESKTOP"), 0);

   sprintf(buf, "%s:", _("Show The Desktop"));
   ob = e_widget_label_add(o, buf);
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 0, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("desk_deskshow_toggle", ""), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 0, 1, 1, 1, 0, 1, 0);

   sprintf(buf, "%s:", _("Switch To Desktop 0"));
   ob = e_widget_label_add(o, buf);
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 1, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("desk_linear_flip_to", "0"), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 1, 1, 1, 1, 0, 1, 0);

   sprintf(buf, "%s:", _("Switch To Desktop 1"));
   ob = e_widget_label_add(o, buf);
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 2, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("desk_linear_flip_to", "1"), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 2, 1, 1, 1, 0, 1, 0);

   sprintf(buf, "%s:", _("Switch To Desktop 2"));
   ob = e_widget_label_add(o, buf);
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 3, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("desk_linear_flip_to", "2"), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 3, 1, 1, 1, 0, 1, 0);

   sprintf(buf, "%s:", _("Switch To Desktop 3"));
   ob = e_widget_label_add(o, buf);
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 4, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("desk_linear_flip_to", "3"), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 4, 1, 1, 1, 0, 1, 0);

   sprintf(buf, "%s:", _("Switch To Desktop 4"));
   ob = e_widget_label_add(o, buf);
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 5, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("desk_linear_flip_to", "4"), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 5, 1, 1, 1, 0, 1, 0);

   sprintf(buf, "%s:", _("Switch To Desktop 5"));
   ob = e_widget_label_add(o, buf);
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 6, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("desk_linear_flip_to", "5"), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 6, 1, 1, 1, 0, 1, 0);

   sprintf(buf, "%s:", _("Switch To Desktop 6"));
   ob = e_widget_label_add(o, buf);
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 7, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("desk_linear_flip_to", "6"), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 7, 1, 1, 1, 0, 1, 0);

   sprintf(buf, "%s:", _("Switch To Desktop 7"));
   ob = e_widget_label_add(o, buf);
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 8, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("desk_linear_flip_to", "7"), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 8, 1, 1, 1, 0, 1, 0);

   sprintf(buf, "%s:", _("Switch To Desktop 8"));
   ob = e_widget_label_add(o, buf);
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 9, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("desk_linear_flip_to", "8"), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 9, 1, 1, 1, 0, 1, 0);

   e_widget_table_object_append(o, ot, 1, 0, 1, 1, 1, 1, 1, 1);

    //----------------------------------------------
   ot = e_widget_frametable_add(o, _("MISCELLANEOUS"), 0);

   sprintf(buf, "%s:", _("Quick Launcher"));
   ob = e_widget_label_add(o, buf);
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 0, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("everything", ""), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 0, 1, 1, 1, 0, 1, 0);

   sprintf(buf, "%s:", _("Take Screenshot"));
   ob = e_widget_label_add(o, buf);
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 1, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("shot", ""), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 1, 1, 1, 1, 0, 1, 0);

   sprintf(buf, "%s:", _("Restart Moksha"));
   ob = e_widget_label_add(o, buf);
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 2, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, _key_binding_get("restart", ""), NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 2, 1, 1, 1, 0, 1, 0);

   sprintf(buf, "%s:", _("Terminal Console"));
   ob = e_widget_label_add(o, buf);
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(ot, ob, 0, 3, 1, 1, 1, 0, 1, 0, 0.0, 0.5, sw, sh, sw, sh);
   ob = e_widget_button_add(o, "CTRL ALT F1..F7", NULL, NULL, NULL, NULL);
   e_widget_frametable_object_append(ot, ob, 1, 3, 1, 1, 1, 0, 1, 0);

   sprintf(buf, "%s:", _("This Help"));
   ob = e_widget_label_add(o, buf);
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
   if (dia && !show) return;
   else e_util_defer_object_del(E_OBJECT(dia));

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
