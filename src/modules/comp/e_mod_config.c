#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include "e_mod_comp.h"

typedef struct _E_Demo_Style_Item
{
   Evas_Object *preview;
   Evas_Object *frame;
   Evas_Object *livethumb;
   Evas_Object *layout;
   Evas_Object *border;
   Evas_Object *client;
} E_Demo_Style_Item;

typedef struct _Match_Config
{
   Match            match;
   E_Config_Dialog *cfd;
   char            *title, *name, *clas, *role;
   int              borderless, dialog, accepts_focus, vkbd;
   int              quickpanel, argb, fullscreen, modal;
} Match_Config;

struct _E_Config_Dialog_Data
{
   int         use_shadow;
   int         engine;
   int         indirect;
   int         texture_from_pixmap;
   int         smooth_windows;
   int         lock_fps;
   int         efl_sync;
   int         loose_sync;
   int         grab;
   int         vsync;

   const char *shadow_style;

   struct
   {
      Eina_List *popups;
      Eina_List *borders;
      Eina_List *overrides;
      Eina_List *menus;
      int        changed;
   } match;

   Evas_Object *popups_il;
   Evas_Object *borders_il;
   Evas_Object *overrides_il;
   Evas_Object *menus_il;

   Evas_Object *edit_il;

   int          keep_unmapped;
   int          max_unmapped_pixels;
   int          max_unmapped_time;
   int          min_unmapped_time;
   int          send_flush;
   int          send_dump;
   int          nocomp_fs;

   int          fps_show;
   int          fps_corner;
   int          fps_average_range;
   double       first_draw_delay;
};

/* Protos */
static void        *_create_data(E_Config_Dialog *cfd);
static void         _free_data(E_Config_Dialog      *cfd,
                               E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog      *cfd,
                                          Evas                 *evas,
                                          E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog      *cfd,
                             E_Config_Dialog_Data *cfdata);

E_Config_Dialog *
e_int_config_comp_module(E_Container       *con,
                         const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   char buf[4096];
   Mod *mod = _comp_mod;

   if (e_config_dialog_find("E", "appearance/comp")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;

   snprintf(buf, sizeof(buf), "%s/e-module-comp.edj",
            e_module_dir_get(mod->module));
   cfd = e_config_dialog_new(con, _("Composite Settings"),
                             "E", "appearance/comp", buf, 0, v, mod);
   mod->config_dialog = cfd;
   return cfd;
}

static void
_match_dup(Match        *m,
           Match_Config *m2)
{
   m2->match = *m;
   if (m2->match.title) m2->match.title = eina_stringshare_add(m2->match.title);
   if (m2->match.name) m2->match.name = eina_stringshare_add(m2->match.name);
   if (m2->match.clas) m2->match.clas = eina_stringshare_add(m2->match.clas);
   if (m2->match.role) m2->match.role = eina_stringshare_add(m2->match.role);
   if (m2->match.shadow_style) m2->match.shadow_style = eina_stringshare_add(m2->match.shadow_style);
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;
   Eina_List *l;
   Match *m;
   Match_Config *m2;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);

   cfdata->use_shadow = _comp_mod->conf->use_shadow;
   cfdata->engine = _comp_mod->conf->engine;
   if ((cfdata->engine != E_EVAS_ENGINE_SOFTWARE_X11) &&
       (cfdata->engine != E_EVAS_ENGINE_GL_X11))
     cfdata->engine = E_EVAS_ENGINE_SOFTWARE_X11;
   cfdata->indirect = _comp_mod->conf->indirect;
   cfdata->texture_from_pixmap = _comp_mod->conf->texture_from_pixmap;
   cfdata->smooth_windows = _comp_mod->conf->smooth_windows;
   cfdata->lock_fps = _comp_mod->conf->lock_fps;
   cfdata->efl_sync = _comp_mod->conf->efl_sync;
   cfdata->loose_sync = _comp_mod->conf->loose_sync;
   cfdata->grab = _comp_mod->conf->grab;
   cfdata->vsync = _comp_mod->conf->vsync;
   if (_comp_mod->conf->shadow_style)
     cfdata->shadow_style = eina_stringshare_add(_comp_mod->conf->shadow_style);

   cfdata->keep_unmapped = _comp_mod->conf->keep_unmapped;
   cfdata->max_unmapped_pixels = _comp_mod->conf->max_unmapped_pixels;
   cfdata->max_unmapped_time = _comp_mod->conf->max_unmapped_time;
   cfdata->min_unmapped_time = _comp_mod->conf->min_unmapped_time;
   cfdata->send_flush = _comp_mod->conf->send_flush;
   cfdata->send_dump = _comp_mod->conf->send_dump;
   cfdata->nocomp_fs = _comp_mod->conf->nocomp_fs;

   cfdata->fps_show = _comp_mod->conf->fps_show;
   cfdata->fps_corner = _comp_mod->conf->fps_corner;
   cfdata->fps_average_range = _comp_mod->conf->fps_average_range;
   if (cfdata->fps_average_range < 1) cfdata->fps_average_range = 12;
   else if (cfdata->fps_average_range > 120)
     cfdata->fps_average_range = 120;
   cfdata->first_draw_delay = _comp_mod->conf->first_draw_delay;

   EINA_LIST_FOREACH(_comp_mod->conf->match.popups, l, m)
     {
        m2 = E_NEW(Match_Config, 1);
        _match_dup(m, m2);
        m2->cfd = cfd;
        cfdata->match.popups = eina_list_append(cfdata->match.popups, m2);
     }

   EINA_LIST_FOREACH(_comp_mod->conf->match.borders, l, m)
     {
        m2 = E_NEW(Match_Config, 1);
        _match_dup(m, m2);
        m2->cfd = cfd;
        cfdata->match.borders = eina_list_append(cfdata->match.borders, m2);
     }

   EINA_LIST_FOREACH(_comp_mod->conf->match.overrides, l, m)
     {
        m2 = E_NEW(Match_Config, 1);
        _match_dup(m, m2);
        m2->cfd = cfd;
        cfdata->match.overrides = eina_list_append(cfdata->match.overrides, m2);
     }

   EINA_LIST_FOREACH(_comp_mod->conf->match.menus, l, m)
     {
        m2 = E_NEW(Match_Config, 1);
        _match_dup(m, m2);
        m2->cfd = cfd;
        cfdata->match.menus = eina_list_append(cfdata->match.menus, m2);
     }

   return cfdata;
}

static void
_match_free(Match_Config *m)
{
   if (m->match.title) eina_stringshare_del(m->match.title);
   if (m->match.name) eina_stringshare_del(m->match.name);
   if (m->match.clas) eina_stringshare_del(m->match.clas);
   if (m->match.role) eina_stringshare_del(m->match.role);
   if (m->match.shadow_style) eina_stringshare_del(m->match.shadow_style);
   if (m->title) free(m->title);
   if (m->name) free(m->name);
   if (m->clas) free(m->clas);
   if (m->role) free(m->role);
   free(m);
}

static void
_free_data(E_Config_Dialog *cfd  __UNUSED__,
           E_Config_Dialog_Data *cfdata)
{
   Match_Config *m;

   _comp_mod->config_dialog = NULL;
   if (cfdata->shadow_style) eina_stringshare_del(cfdata->shadow_style);
   EINA_LIST_FREE(cfdata->match.popups, m)
     {
        _match_free(m);
     }
   EINA_LIST_FREE(cfdata->match.borders, m)
     {
        _match_free(m);
     }
   EINA_LIST_FREE(cfdata->match.overrides, m)
     {
        _match_free(m);
     }
   EINA_LIST_FREE(cfdata->match.menus, m)
     {
        _match_free(m);
     }
   free(cfdata);
}

static void
_shadow_changed(void            *data,
                Evas_Object     *obj,
                void *event_info __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;
   Evas_Object *orec0;
   Eina_List *style_list;
   const E_Demo_Style_Item *it;
   const Eina_List *l;

   orec0 = evas_object_name_find(evas_object_evas_get(obj), "style_shadows");
   style_list = evas_object_data_get(orec0, "list");
   EINA_LIST_FOREACH(style_list, l, it)
     {
        if (cfdata->use_shadow)
          edje_object_signal_emit(it->preview, "e,state,shadow,on", "e");
        else
          edje_object_signal_emit(it->preview, "e,state,shadow,off", "e");
     }
}

static Eina_Bool
_style_demo(void *data)
{
   Eina_List *style_shadows, *l;
   int demo_state;
   const E_Demo_Style_Item *it;

   demo_state = (long)evas_object_data_get(data, "style_demo_state");
   demo_state = (demo_state + 1) % 4;
   evas_object_data_set(data, "style_demo_state", (void *)(long)demo_state);

   style_shadows = evas_object_data_get(data, "style_shadows");
   EINA_LIST_FOREACH(style_shadows, l, it)
     {
        Evas_Object *ob = it->preview;
        Evas_Object *of = it->frame;

        switch (demo_state)
          {
           case 0:
             edje_object_signal_emit(ob, "e,state,visible,on", "e");
             edje_object_signal_emit(ob, "e,state,focus,on", "e");
             edje_object_part_text_set(of, "e.text.label", _("Visible"));
             break;

           case 1:
             edje_object_signal_emit(ob, "e,state,focus,off", "e");
             edje_object_part_text_set(of, "e.text.label", _("Focus-Out"));
             break;

           case 2:
             edje_object_signal_emit(ob, "e,state,focus,on", "e");
             edje_object_part_text_set(of, "e.text.label", _("Focus-In"));
             break;

           case 3:
             edje_object_signal_emit(ob, "e,state,visible,off", "e");
             edje_object_part_text_set(of, "e.text.label", _("Hidden"));
             break;

           default:
             break;
          }
     }
   return ECORE_CALLBACK_RENEW;
}

static void
_style_selector_del(void *data       __UNUSED__,
                    Evas            *e,
                    Evas_Object     *o,
                    void *event_info __UNUSED__)
{
   Eina_List *style_shadows, *style_list;
   Ecore_Timer *timer;
   Evas_Object *orec0;

   orec0 = evas_object_name_find(e, "style_shadows");
   style_list = evas_object_data_get(orec0, "list");

   style_shadows = evas_object_data_get(o, "style_shadows");
   if (style_shadows)
     {
        E_Demo_Style_Item *ds_it;

        EINA_LIST_FREE(style_shadows, ds_it)
          {
             style_list = eina_list_remove(style_list, ds_it);

             evas_object_del(ds_it->client);
             evas_object_del(ds_it->border);
             evas_object_del(ds_it->frame);
             evas_object_del(ds_it->preview);
             evas_object_del(ds_it->layout);
             evas_object_del(ds_it->livethumb);
             free(ds_it);
          }
        evas_object_data_set(o, "style_shadows", NULL);
     }

   timer = evas_object_data_get(o, "style_timer");
   if (timer)
     {
        ecore_timer_del(timer);
        evas_object_data_set(o, "style_timer", NULL);
     }

   evas_object_data_set(orec0, "list", style_list);
}

static Evas_Object *
_style_selector(Evas        *evas,
                int          use_shadow,
                const char **source)
{
   Evas_Object *oi, *ob, *oo, *obd, *orec, *oly, *orec0;
   Eina_List *styles, *l, *style_shadows = NULL, *style_list;
   char *style;
   int n, sel;
   Evas_Coord wmw, wmh;
   Ecore_Timer *timer;

   orec0 = evas_object_name_find(evas, "style_shadows");
   style_list = evas_object_data_get(orec0, "list");
   oi = e_widget_ilist_add(evas, 80, 80, source);
   evas_object_event_callback_add(oi, EVAS_CALLBACK_DEL,
                                  _style_selector_del, oi);
   sel = 0;
   styles = e_theme_comp_list();
   n = 0;
   EINA_LIST_FOREACH(styles, l, style)
     {
        E_Demo_Style_Item *ds_it;
        char buf[PATH_MAX];

        ds_it = malloc(sizeof(E_Demo_Style_Item));

        ob = e_livethumb_add(evas);
        ds_it->livethumb = ob;
        e_livethumb_vsize_set(ob, 240, 240);

        oly = e_layout_add(e_livethumb_evas_get(ob));
        ds_it->layout = ob;
        e_layout_virtual_size_set(oly, 240, 240);
        e_livethumb_thumb_set(ob, oly);
        evas_object_show(oly);

        oo = edje_object_add(e_livethumb_evas_get(ob));
        ds_it->preview = oo;
        snprintf(buf, sizeof(buf), "e/comp/%s", style);
        e_theme_edje_object_set(oo, "base/theme/borders", buf);
        e_layout_pack(oly, oo);
        e_layout_child_move(oo, 39, 39);
        e_layout_child_resize(oo, 162, 162);
        if (use_shadow) edje_object_signal_emit(oo, "e,state,shadow,on", "e");
        edje_object_signal_emit(oo, "e,state,visible,on", "e");
        evas_object_show(oo);

        ds_it->frame = edje_object_add(evas);
        e_theme_edje_object_set
          (ds_it->frame, "base/theme/modules/comp", "e/modules/comp/preview");
        edje_object_part_swallow(ds_it->frame, "e.swallow.preview", ob);
        evas_object_show(ds_it->frame);
        style_shadows = eina_list_append(style_shadows, ds_it);

        obd = edje_object_add(e_livethumb_evas_get(ob));
        ds_it->border = obd;
        e_theme_edje_object_set(obd, "base/theme/borders",
                                "e/widgets/border/default/border");
        edje_object_part_text_set(obd, "e.text.title", _("Title"));
        edje_object_signal_emit(obd, "e,state,focused", "e");
        edje_object_part_swallow(oo, "e.swallow.content", obd);
        evas_object_show(obd);

        orec = evas_object_rectangle_add(e_livethumb_evas_get(ob));
        ds_it->client = orec;
        evas_object_color_set(orec, 255, 255, 255, 255);
        edje_object_part_swallow(obd, "e.swallow.client", orec);
        evas_object_show(orec);

        e_widget_ilist_append(oi, ds_it->frame, style, NULL, NULL, style);
        evas_object_show(ob);
        if (*source)
          {
             if (!strcmp(*source, style)) sel = n;
          }
        n++;

        style_list = eina_list_append(style_list, ds_it);
     }
   evas_object_data_set(orec0, "list", style_list);
   evas_object_data_set(oi, "style_shadows", style_shadows);
   timer = ecore_timer_add(3.0, _style_demo, oi);
   evas_object_data_set(oi, "style_timer", timer);
   evas_object_data_set(oi, "style_demo_state", (void *)1);
   e_widget_size_min_get(oi, &wmw, &wmh);
   e_widget_size_min_set(oi, 160, 100);
   e_widget_ilist_selected_set(oi, sel);
   e_widget_ilist_go(oi);

   return oi;
}

static void
_match_sel(void *data __UNUSED__)
{
//   Match_Config *m = data;
//   E_Config_Dialog *cfd = m->cfd;
}

static const char *
_match_type_label_get(int type)
{
   if (ECORE_X_WINDOW_TYPE_UNKNOWN == type)
     return _("Unused");
   if (ECORE_X_WINDOW_TYPE_COMBO == type)
     return _("Combo");
   if (ECORE_X_WINDOW_TYPE_DESKTOP == type)
     return _("Desktop");
   if (ECORE_X_WINDOW_TYPE_DIALOG == type)
     return _("Dialog");
   if (ECORE_X_WINDOW_TYPE_DOCK == type)
     return _("Dock");
   if (ECORE_X_WINDOW_TYPE_DND == type)
     return _("Drag and Drop");
   if (ECORE_X_WINDOW_TYPE_MENU == type)
     return _("Menu");
   if (ECORE_X_WINDOW_TYPE_DROPDOWN_MENU == type)
     return _("Menu (Dropdown)");
   if (ECORE_X_WINDOW_TYPE_POPUP_MENU == type)
     return _("Menu (Popup)");
   if (ECORE_X_WINDOW_TYPE_NORMAL == type)
     return _("Normal");
   if (ECORE_X_WINDOW_TYPE_NOTIFICATION == type)
     return _("Notification");
   if (ECORE_X_WINDOW_TYPE_SPLASH == type)
     return _("Splash");
   if (ECORE_X_WINDOW_TYPE_TOOLBAR == type)
     return _("Toolbar");
   if (ECORE_X_WINDOW_TYPE_TOOLTIP == type)
     return _("Tooltip");
   if (ECORE_X_WINDOW_TYPE_UTILITY == type)
     return _("Utility");

   return _("Unused");
}

static char *
_match_label_get(Match_Config *m)
{
   char *label;
   Eina_Strbuf *buf = eina_strbuf_new();

   if (m->match.title)
     {
        eina_strbuf_append(buf, _("Title:"));
        eina_strbuf_append(buf, m->match.title);
        eina_strbuf_append(buf, _(" / "));
     }
   if (m->match.primary_type)
     {
        eina_strbuf_append(buf, _("Type:"));
        eina_strbuf_append(buf, _match_type_label_get(m->match.primary_type));
        eina_strbuf_append(buf, _(" / "));
     }
   if (m->match.name)
     {
        eina_strbuf_append(buf, _("Name:"));
        eina_strbuf_append(buf, m->match.name);
        eina_strbuf_append(buf, _(" / "));
     }
   if (m->match.clas)
     {
        eina_strbuf_append(buf, _("Class:"));
        eina_strbuf_append(buf, m->match.clas);
        eina_strbuf_append(buf, _(" / "));
     }
   if (m->match.role)
     {
        eina_strbuf_append(buf, _("Role:"));
        eina_strbuf_append(buf, m->match.role);
        eina_strbuf_append(buf, _(" / "));
     }
   if (m->match.shadow_style)
     {
        eina_strbuf_append(buf, _("Style:"));
        eina_strbuf_append(buf, m->match.shadow_style);
     }

   if (!eina_strbuf_length_get(buf))
     return _("Unknown");

   label = strdup(eina_strbuf_string_get(buf));
   eina_strbuf_free(buf);

   return label;
}

static void
_match_ilist_append(Evas_Object  *il,
                    Match_Config *m,
                    int           pos,
                    int           pre)
{
   char *name = _match_label_get(m);

   if (pos == -1)
     e_widget_ilist_append(il, NULL, name, _match_sel, m, NULL);
   else
     {
        if (pre)
          e_widget_ilist_prepend_relative(il, NULL, name, _match_sel, m, NULL, pos);
        else
          e_widget_ilist_append_relative(il, NULL, name, _match_sel, m, NULL, pos);
     }
   E_FREE(name);
}

static void
_match_list_up(Eina_List   **list,
               Match_Config *m)
{
   Eina_List *l, *lp;

   l = eina_list_data_find_list(*list, m);
   if (!l) return;
   lp = l->prev;
   *list = eina_list_remove_list(*list, l);
   if (lp) *list = eina_list_prepend_relative_list(*list, m, lp);
   else *list = eina_list_prepend(*list, m);
}

static void
_match_list_down(Eina_List   **list,
                 Match_Config *m)
{
   Eina_List *l, *lp;

   l = eina_list_data_find_list(*list, m);
   if (!l) return;
   lp = l->next;
   *list = eina_list_remove_list(*list, l);
   if (lp) *list = eina_list_append_relative_list(*list, m, lp);
   else *list = eina_list_append(*list, m);
}

static void
_match_list_del(Eina_List   **list,
                Match_Config *m)
{
   Eina_List *l, *lp;

   l = eina_list_data_find_list(*list, m);
   if (!l) return;
   lp = l->next;
   *list = eina_list_remove_list(*list, l);
   _match_free(m);
}

static void
_cb_dialog_resize(void            *data,
                  Evas *e          __UNUSED__,
                  Evas_Object     *obj,
                  void *event_info __UNUSED__)
{
   Evas_Object *bg, *of;
   int x, y, w, h;

   of = data;
   bg = evas_object_data_get(of, "bg");
   evas_object_geometry_get(obj, &x, &y, &w, &h);

   evas_object_move(bg, x, y);
   evas_object_resize(bg, w, h);
   evas_object_move(of, x, y);
   evas_object_resize(of, w, h);
}

static void
_edit_ok(void *d1,
         void *d2)
{
   Match_Config *m = d1;
   Evas_Object *dia, *bg, *of = d2;
   Evas_Object *il;
   char *label;
   int n;

   if (m->match.title) eina_stringshare_del(m->match.title);
   m->match.title = NULL;
   if (m->title)
     {
        if (m->title[0]) m->match.title = eina_stringshare_add(m->title);
        free(m->title);
        m->title = NULL;
     }
   if (m->match.name) eina_stringshare_del(m->match.name);
   m->match.name = NULL;
   if (m->name)
     {
        if (m->name[0]) m->match.name = eina_stringshare_add(m->name);
        free(m->name);
        m->name = NULL;
     }
   if (m->match.clas) eina_stringshare_del(m->match.clas);
   m->match.clas = NULL;
   if (m->clas)
     {
        if (m->clas[0]) m->match.clas = eina_stringshare_add(m->clas);
        free(m->clas);
        m->clas = NULL;
     }
   if (m->match.role) eina_stringshare_del(m->match.role);
   m->match.role = NULL;
   if (m->role)
     {
        if (m->role[0]) m->match.role = eina_stringshare_add(m->role);
        free(m->role);
        m->role = NULL;
     }
   m->match.borderless = m->borderless;
   m->match.dialog = m->dialog;
   m->match.accepts_focus = m->accepts_focus;
   m->match.vkbd = m->vkbd;
   m->match.quickpanel = m->quickpanel;
   m->match.argb = m->argb;
   m->match.fullscreen = m->fullscreen;
   m->match.modal = m->modal;

   il = m->cfd->cfdata->edit_il;
   n = e_widget_ilist_selected_get(il);
   label = _match_label_get(m);
   e_widget_ilist_nth_label_set(il, n, label);
   E_FREE(label);
   bg = evas_object_data_get(of, "bg");
   dia = evas_object_data_get(of, "dia");

   evas_object_event_callback_del(dia, EVAS_CALLBACK_RESIZE, _cb_dialog_resize);
   evas_object_del(bg);
   evas_object_del(of);
}

static void
_create_edit_frame(E_Config_Dialog      *cfd,
                   Evas                 *evas,
                   E_Config_Dialog_Data *cfdata,
                   Match_Config         *m)
{
   Evas_Object *of, *oi, *lb, *en, *bt, *tb, *tab2, *o, *sf, *li;
   E_Radio_Group *rg;
   int row;
   int x, y, w, h;

   o = edje_object_add(evas);
   e_theme_edje_object_set(o, "base/theme/dialog", "e/widgets/dialog/main");
   evas_object_geometry_get(cfd->dia->bg_object, &x, &y, &w, &h);
   evas_object_move(o, x, y);
   evas_object_resize(o, w, h);
   evas_object_show(o);

   of = e_widget_frametable_add(evas, _("Edit Match"), 0);
   evas_object_data_set(of, "bg", o);
   evas_object_data_set(of, "dia", cfd->dia->bg_object);
   evas_object_move(of, x, y);
   evas_object_resize(of, w, h);
   evas_object_show(of);

   evas_object_event_callback_add(cfd->dia->bg_object, EVAS_CALLBACK_RESIZE,
                                  _cb_dialog_resize, of);

   tb = e_widget_toolbook_add(evas, 48 * e_scale, 48 * e_scale);

   tab2 = e_widget_table_add(evas, 0);
   if (cfdata->edit_il == cfdata->borders_il)
     {
        if (m->match.title) m->title = strdup(m->match.title);
        else m->title = NULL;
        lb = e_widget_label_add(evas, _("Title"));
        e_widget_table_object_append(tab2, lb, 0, 0, 1, 1, 1, 0, 0, 0);
        en = e_widget_entry_add(evas, &(m->title), NULL, NULL, NULL);
        e_widget_table_object_append(tab2, en, 1, 0, 1, 1, 1, 0, 1, 0);
     }
   if ((cfdata->edit_il == cfdata->borders_il) ||
       (cfdata->edit_il == cfdata->overrides_il) ||
       (cfdata->edit_il == cfdata->popups_il))
     {
        if (m->match.name) m->name = strdup(m->match.name);
        else m->name = NULL;
        lb = e_widget_label_add(evas, _("Name"));
        e_widget_table_object_append(tab2, lb, 0, 1, 1, 1, 1, 0, 0, 0);
        en = e_widget_entry_add(evas, &(m->name), NULL, NULL, NULL);
        e_widget_table_object_append(tab2, en, 1, 1, 1, 1, 1, 0, 1, 0);
     }
   if ((cfdata->edit_il == cfdata->borders_il) ||
       (cfdata->edit_il == cfdata->overrides_il))
     {
        if (m->match.clas) m->clas = strdup(m->match.clas);
        else m->clas = NULL;
        lb = e_widget_label_add(evas, _("Class"));
        e_widget_table_object_append(tab2, lb, 0, 2, 1, 1, 1, 0, 0, 0);
        en = e_widget_entry_add(evas, &(m->clas), NULL, NULL, NULL);
        e_widget_table_object_append(tab2, en, 1, 2, 1, 1, 1, 0, 1, 0);
     }
   if (cfdata->edit_il == cfdata->borders_il)
     {
        if (m->match.role) m->role = strdup(m->match.role);
        else m->role = NULL;
        lb = e_widget_label_add(evas, _("Role"));
        e_widget_table_object_append(tab2, lb, 0, 3, 1, 1, 1, 0, 0, 0);
        en = e_widget_entry_add(evas, &(m->role), NULL, NULL, NULL);
        e_widget_table_object_append(tab2, en, 1, 3, 1, 1, 1, 0, 1, 0);
     }
   e_widget_toolbook_page_append(tb, NULL, _("Names"), tab2, 1, 1, 1, 1, 0.5, 0.0);

   if ((cfdata->edit_il == cfdata->borders_il) ||
       (cfdata->edit_il == cfdata->overrides_il))
     {
        Evas_Coord mw, mh;

        rg = e_widget_radio_group_new(&m->match.primary_type);

        li = e_widget_list_add(evas, 1, 0);

        o = e_widget_radio_add(evas, _("Unused"), ECORE_X_WINDOW_TYPE_UNKNOWN, rg);
        e_widget_list_object_append(li, o, 1, 0, 0.0);

        o = e_widget_radio_add(evas, _("Combo"), ECORE_X_WINDOW_TYPE_COMBO, rg);
        e_widget_list_object_append(li, o, 1, 0, 0.0);
        o = e_widget_radio_add(evas, _("Desktop"), ECORE_X_WINDOW_TYPE_DESKTOP, rg);
        e_widget_list_object_append(li, o, 1, 0, 0.0);
        o = e_widget_radio_add(evas, _("Dialog"), ECORE_X_WINDOW_TYPE_DIALOG, rg);
        e_widget_list_object_append(li, o, 1, 0, 0.0);
        o = e_widget_radio_add(evas, _("Dock"), ECORE_X_WINDOW_TYPE_DOCK, rg);
        e_widget_list_object_append(li, o, 1, 0, 0.0);
        o = e_widget_radio_add(evas, _("Drag and Drop"), ECORE_X_WINDOW_TYPE_DND, rg);
        e_widget_list_object_append(li, o, 1, 0, 0.0);
        o = e_widget_radio_add(evas, _("Menu"), ECORE_X_WINDOW_TYPE_MENU, rg);
        e_widget_list_object_append(li, o, 1, 0, 0.0);
        o = e_widget_radio_add(evas, _("Menu (Dropdown)"), ECORE_X_WINDOW_TYPE_DROPDOWN_MENU, rg);
        e_widget_list_object_append(li, o, 1, 0, 0.0);
        o = e_widget_radio_add(evas, _("Menu (Popup)"), ECORE_X_WINDOW_TYPE_POPUP_MENU, rg);
        e_widget_list_object_append(li, o, 1, 0, 0.0);
        o = e_widget_radio_add(evas, _("Normal"), ECORE_X_WINDOW_TYPE_NORMAL, rg);
        e_widget_list_object_append(li, o, 1, 0, 0.0);
        o = e_widget_radio_add(evas, _("Notification"), ECORE_X_WINDOW_TYPE_NOTIFICATION, rg);
        e_widget_list_object_append(li, o, 1, 0, 0.0);
        o = e_widget_radio_add(evas, _("Splash"), ECORE_X_WINDOW_TYPE_SPLASH, rg);
        e_widget_list_object_append(li, o, 1, 0, 0.0);
        o = e_widget_radio_add(evas, _("Toolbar"), ECORE_X_WINDOW_TYPE_TOOLBAR, rg);
        e_widget_list_object_append(li, o, 1, 0, 0.0);
        o = e_widget_radio_add(evas, _("Tooltip"), ECORE_X_WINDOW_TYPE_TOOLTIP, rg);
        e_widget_list_object_append(li, o, 1, 0, 0.0);
        o = e_widget_radio_add(evas, _("Utility"), ECORE_X_WINDOW_TYPE_UTILITY, rg);
        e_widget_list_object_append(li, o, 1, 0, 0.0);

        e_widget_size_min_get(li, &mw, &mh);
        evas_object_resize(li, mw, mh);

        sf = e_widget_scrollframe_simple_add(evas, li);
        e_widget_toolbook_page_append(tb, NULL, _("Types"), sf,
                                      1, 1, 1, 1, 0.5, 0.0);
     }

   m->borderless = m->match.borderless;
   m->dialog = m->match.dialog;
   m->accepts_focus = m->match.accepts_focus;
   m->vkbd = m->match.vkbd;
   m->quickpanel = m->match.quickpanel;
   m->argb = m->match.argb;
   m->fullscreen = m->match.fullscreen;
   m->modal = m->match.modal;

   row = 0;
   tab2 = e_widget_table_add(evas, 0);
   lb = e_widget_label_add(evas, _("Unused"));
   e_widget_table_object_append(tab2, lb, 1, row, 1, 1, 0, 0, 0, 0);
   lb = e_widget_label_add(evas, _("On"));
   e_widget_table_object_append(tab2, lb, 2, row, 1, 1, 0, 0, 0, 0);
   lb = e_widget_label_add(evas, _("Off"));
   e_widget_table_object_append(tab2, lb, 3, row, 1, 1, 0, 0, 0, 0);
   row++;

   if (cfdata->edit_il == cfdata->borders_il)
     {
        lb = e_widget_label_add(evas, _("Borderless"));
        e_widget_table_object_append(tab2, lb, 0, row, 1, 1, 1, 0, 1, 0);
        rg = e_widget_radio_group_new(&m->borderless);
        o = e_widget_radio_add(evas, NULL, 0, rg);
        e_widget_table_object_append(tab2, o, 1, row, 1, 1, 0, 0, 0, 0);
        o = e_widget_radio_add(evas, NULL, 1, rg);
        e_widget_table_object_append(tab2, o, 2, row, 1, 1, 0, 0, 0, 0);
        o = e_widget_radio_add(evas, NULL, -1, rg);
        e_widget_table_object_append(tab2, o, 3, row, 1, 1, 0, 0, 0, 0);
        row++;
     }
   if (cfdata->edit_il == cfdata->borders_il)
     {
        lb = e_widget_label_add(evas, _("Dialog"));
        e_widget_table_object_append(tab2, lb, 0, row, 1, 1, 1, 0, 1, 0);
        rg = e_widget_radio_group_new(&m->dialog);
        o = e_widget_radio_add(evas, NULL, 0, rg);
        e_widget_table_object_append(tab2, o, 1, row, 1, 1, 0, 0, 0, 0);
        o = e_widget_radio_add(evas, NULL, 1, rg);
        e_widget_table_object_append(tab2, o, 2, row, 1, 1, 0, 0, 0, 0);
        o = e_widget_radio_add(evas, NULL, -1, rg);
        e_widget_table_object_append(tab2, o, 3, row, 1, 1, 0, 0, 0, 0);
        row++;
     }
   if (cfdata->edit_il == cfdata->borders_il)
     {
        lb = e_widget_label_add(evas, _("Accepts Focus"));
        e_widget_table_object_append(tab2, lb, 0, row, 1, 1, 1, 0, 1, 0);
        rg = e_widget_radio_group_new(&m->accepts_focus);
        o = e_widget_radio_add(evas, NULL, 0, rg);
        e_widget_table_object_append(tab2, o, 1, row, 1, 1, 0, 0, 0, 0);
        o = e_widget_radio_add(evas, NULL, 1, rg);
        e_widget_table_object_append(tab2, o, 2, row, 1, 1, 0, 0, 0, 0);
        o = e_widget_radio_add(evas, NULL, -1, rg);
        e_widget_table_object_append(tab2, o, 3, row, 1, 1, 0, 0, 0, 0);
        row++;
     }
   if (cfdata->edit_il == cfdata->borders_il)
     {
        lb = e_widget_label_add(evas, _("Virtual Keyboard"));
        e_widget_table_object_append(tab2, lb, 0, row, 1, 1, 1, 0, 1, 0);
        rg = e_widget_radio_group_new(&m->vkbd);
        o = e_widget_radio_add(evas, NULL, 0, rg);
        e_widget_table_object_append(tab2, o, 1, row, 1, 1, 0, 0, 0, 0);
        o = e_widget_radio_add(evas, NULL, 1, rg);
        e_widget_table_object_append(tab2, o, 2, row, 1, 1, 0, 0, 0, 0);
        o = e_widget_radio_add(evas, NULL, -1, rg);
        e_widget_table_object_append(tab2, o, 3, row, 1, 1, 0, 0, 0, 0);
        row++;
     }
   if (cfdata->edit_il == cfdata->borders_il)
     {
        lb = e_widget_label_add(evas, _("Quick Panel"));
        e_widget_table_object_append(tab2, lb, 0, row, 1, 1, 1, 0, 1, 0);
        rg = e_widget_radio_group_new(&m->quickpanel);
        o = e_widget_radio_add(evas, NULL, 0, rg);
        e_widget_table_object_append(tab2, o, 1, row, 1, 1, 0, 0, 0, 0);
        o = e_widget_radio_add(evas, NULL, 1, rg);
        e_widget_table_object_append(tab2, o, 2, row, 1, 1, 0, 0, 0, 0);
        o = e_widget_radio_add(evas, NULL, -1, rg);
        e_widget_table_object_append(tab2, o, 3, row, 1, 1, 0, 0, 0, 0);
        row++;
     }
   lb = e_widget_label_add(evas, _("ARGB"));
   e_widget_table_object_append(tab2, lb, 0, row, 1, 1, 1, 0, 1, 0);
   rg = e_widget_radio_group_new(&m->argb);
   o = e_widget_radio_add(evas, NULL, 0, rg);
   e_widget_table_object_append(tab2, o, 1, row, 1, 1, 0, 0, 0, 0);
   o = e_widget_radio_add(evas, NULL, 1, rg);
   e_widget_table_object_append(tab2, o, 2, row, 1, 1, 0, 0, 0, 0);
   o = e_widget_radio_add(evas, NULL, -1, rg);
   e_widget_table_object_append(tab2, o, 3, row, 1, 1, 0, 0, 0, 0);
   row++;
   if (cfdata->edit_il == cfdata->borders_il)
     {
        lb = e_widget_label_add(evas, _("Fullscreen"));
        e_widget_table_object_append(tab2, lb, 0, row, 1, 1, 1, 0, 1, 0);
        rg = e_widget_radio_group_new(&m->fullscreen);
        o = e_widget_radio_add(evas, NULL, 0, rg);
        e_widget_table_object_append(tab2, o, 1, row, 1, 1, 0, 0, 0, 0);
        o = e_widget_radio_add(evas, NULL, 1, rg);
        e_widget_table_object_append(tab2, o, 2, row, 1, 1, 0, 0, 0, 0);
        o = e_widget_radio_add(evas, NULL, -1, rg);
        e_widget_table_object_append(tab2, o, 3, row, 1, 1, 0, 0, 0, 0);
        row++;
     }
   if (cfdata->edit_il == cfdata->borders_il)
     {
        lb = e_widget_label_add(evas, _("Modal"));
        e_widget_table_object_append(tab2, lb, 0, row, 1, 1, 1, 0, 1, 0);
        rg = e_widget_radio_group_new(&m->modal);
        o = e_widget_radio_add(evas, NULL, 0, rg);
        e_widget_table_object_append(tab2, o, 1, row, 1, 1, 0, 0, 0, 0);
        o = e_widget_radio_add(evas, NULL, 1, rg);
        e_widget_table_object_append(tab2, o, 2, row, 1, 1, 0, 0, 0, 0);
        o = e_widget_radio_add(evas, NULL, -1, rg);
        e_widget_table_object_append(tab2, o, 3, row, 1, 1, 0, 0, 0, 0);
        row++;
     }
   e_widget_toolbook_page_append(tb, NULL, _("Flags"), tab2,
                                 1, 1, 1, 1, 0.5, 0.0);

   oi = _style_selector(evas, cfdata->use_shadow, &(m->match.shadow_style));
   e_widget_toolbook_page_append(tb, NULL, _("Style"), oi,
                                 1, 1, 1, 1, 0.5, 0.0);

   e_widget_frametable_object_append(of, tb, 0, 0, 1, 1, 1, 1, 1, 1);
   e_widget_toolbook_page_show(tb, 0);

   bt = e_widget_button_add(evas, _("OK"), NULL, _edit_ok, m, of);
   e_widget_frametable_object_append(of, bt, 0, 1, 1, 1, 0, 0, 0, 0);
}

static void
_but_up(void *d1,
        void *d2)
{
   E_Config_Dialog *cfd = d1;
   Evas_Object *il = d2;
   Match_Config *m;
   int n;

   e_widget_ilist_freeze(il);
   n = e_widget_ilist_selected_get(il);
   if (n < 1) return;
   m = e_widget_ilist_nth_data_get(il, n);
   if (!m)
     {
        e_widget_ilist_thaw(il);
        return;
     }
   e_widget_ilist_remove_num(il, n);
   n--;
   _match_ilist_append(il, m, n, 1);
   e_widget_ilist_nth_show(il, n, 0);
   e_widget_ilist_selected_set(il, n);
   e_widget_ilist_thaw(il);
   e_widget_ilist_go(il);
   _match_list_up(&(cfd->cfdata->match.popups), m);
   _match_list_up(&(cfd->cfdata->match.borders), m);
   _match_list_up(&(cfd->cfdata->match.overrides), m);
   _match_list_up(&(cfd->cfdata->match.menus), m);
   cfd->cfdata->match.changed = 1;
}

static void
_but_down(void *d1,
          void *d2)
{
   E_Config_Dialog *cfd = d1;
   Evas_Object *il = d2;
   Match_Config *m;
   int n;

   e_widget_ilist_freeze(il);
   n = e_widget_ilist_selected_get(il);
   if (n >= (e_widget_ilist_count(il) - 1)) return;
   m = e_widget_ilist_nth_data_get(il, n);
   if (!m)
     {
        e_widget_ilist_thaw(il);
        return;
     }
   e_widget_ilist_remove_num(il, n);
   _match_ilist_append(il, m, n, 0);
   e_widget_ilist_nth_show(il, n + 1, 0);
   e_widget_ilist_selected_set(il, n + 1);
   e_widget_ilist_thaw(il);
   e_widget_ilist_go(il);
   _match_list_down(&(cfd->cfdata->match.popups), m);
   _match_list_down(&(cfd->cfdata->match.borders), m);
   _match_list_down(&(cfd->cfdata->match.overrides), m);
   _match_list_down(&(cfd->cfdata->match.menus), m);
   cfd->cfdata->match.changed = 1;
}

static void
_but_add(void *d1,
         void *d2)
{
   E_Config_Dialog *cfd = d1;
   Evas_Object *il = d2;
   Match_Config *m;
   int n;

   m = E_NEW(Match_Config, 1);
   m->cfd = cfd;
   m->match.title = NULL;
   m->match.name = NULL;
   m->match.clas = NULL;
   m->match.role = NULL;
   m->match.shadow_style = eina_stringshare_add("default");

   if (il == cfd->cfdata->popups_il)
     cfd->cfdata->match.popups = eina_list_append(cfd->cfdata->match.popups, m);
   else if (il == cfd->cfdata->borders_il)
     cfd->cfdata->match.borders = eina_list_append(cfd->cfdata->match.borders, m);
   else if (il == cfd->cfdata->overrides_il)
     cfd->cfdata->match.overrides = eina_list_append(cfd->cfdata->match.overrides, m);
   else if (il == cfd->cfdata->menus_il)
     cfd->cfdata->match.menus = eina_list_append(cfd->cfdata->match.menus, m);
   e_widget_ilist_freeze(il);
   _match_ilist_append(il, m, -1, 0);
   e_widget_ilist_thaw(il);
   e_widget_ilist_go(il);
   n = e_widget_ilist_count(il);
   e_widget_ilist_nth_show(il, n - 1, 0);
   e_widget_ilist_selected_set(il, n - 1);

   cfd->cfdata->edit_il = il;
   _create_edit_frame(cfd, evas_object_evas_get(il), cfd->cfdata, m);
   cfd->cfdata->match.changed = 1;
}

static void
_but_del(void *d1,
         void *d2)
{
   E_Config_Dialog *cfd = d1;
   Evas_Object *il = d2;
   Match_Config *m;
   int n;

   e_widget_ilist_freeze(il);
   n = e_widget_ilist_selected_get(il);
   m = e_widget_ilist_nth_data_get(il, n);
   if (!m)
     {
        e_widget_ilist_thaw(il);
        return;
     }
   e_widget_ilist_remove_num(il, n);
   e_widget_ilist_thaw(il);
   e_widget_ilist_go(il);
   _match_list_del(&(cfd->cfdata->match.popups), m);
   _match_list_del(&(cfd->cfdata->match.borders), m);
   _match_list_del(&(cfd->cfdata->match.overrides), m);
   _match_list_del(&(cfd->cfdata->match.menus), m);
   cfd->cfdata->match.changed = 1;
}

static void
_but_edit(void *d1,
          void *d2)
{
   E_Config_Dialog *cfd = d1;
   Evas_Object *il = d2;
   int n;
   Match_Config *m;

   n = e_widget_ilist_selected_get(il);
   m = e_widget_ilist_nth_data_get(il, n);
   if (!m) return;

   cfd->cfdata->edit_il = il;
   _create_edit_frame(cfd, evas_object_evas_get(il), cfd->cfdata, m);
   cfd->cfdata->match.changed = 1;
}

static Evas_Object *
_create_match_editor(E_Config_Dialog             *cfd,
                     Evas                        *evas,
                     E_Config_Dialog_Data *cfdata __UNUSED__,
                     Eina_List                  **matches,
                     Evas_Object                **il_ret)
{
   Evas_Object *tab, *il, *bt;
   Match_Config *m;
   Eina_List *l;

   tab = e_widget_table_add(evas, 0);

   il = e_widget_ilist_add(evas, 16, 16, NULL);
   e_widget_size_min_set(il, 160, 100);

   EINA_LIST_FOREACH(*matches, l, m)
     {
        _match_ilist_append(il, m, -1, 0);
     }

   e_widget_ilist_go(il);
   e_widget_table_object_append(tab, il, 0, 0, 1, 5, 1, 1, 1, 1);

   bt = e_widget_button_add(evas, _("Up"), NULL, _but_up, cfd, il);
   e_widget_table_object_append(tab, bt, 1, 0, 1, 1, 1, 1, 0, 0);
   bt = e_widget_button_add(evas, _("Down"), NULL, _but_down, cfd, il);
   e_widget_table_object_append(tab, bt, 1, 1, 1, 1, 1, 1, 0, 0);
   bt = e_widget_button_add(evas, _("Add"), NULL, _but_add, cfd, il);
   e_widget_table_object_append(tab, bt, 1, 2, 1, 1, 1, 1, 0, 0);
   bt = e_widget_button_add(evas, _("Del"), NULL, _but_del, cfd, il);
   e_widget_table_object_append(tab, bt, 1, 3, 1, 1, 1, 1, 0, 0);
   bt = e_widget_button_add(evas, _("Edit"), NULL, _but_edit, cfd, il);
   e_widget_table_object_append(tab, bt, 1, 4, 1, 1, 1, 1, 0, 0);

   *il_ret = il;

   return tab;
}

static Evas_Object *
_create_styles_toolbook(E_Config_Dialog      *cfd,
                        Evas                 *evas,
                        E_Config_Dialog_Data *cfdata)
{
   Evas_Object *tb, *oi, *il;

   tb = e_widget_toolbook_add(evas, 48 * e_scale, 48 * e_scale);

   oi = _style_selector(evas, cfdata->use_shadow, &(cfdata->shadow_style));
   e_widget_toolbook_page_append(tb, NULL, _("Default"), oi, 1, 1, 1, 1, 0.5, 0.0);

   oi = _create_match_editor(cfd, evas, cfdata, &(cfdata->match.borders), &il);
   cfdata->borders_il = il;
   e_widget_toolbook_page_append(tb, NULL, _("Apps"), oi, 1, 1, 1, 1, 0.5, 0.0);

   oi = _create_match_editor(cfd, evas, cfdata, &(cfdata->match.popups), &il);
   cfdata->popups_il = il;
   e_widget_toolbook_page_append(tb, NULL, _("E"), oi, 1, 1, 1, 1, 0.5, 0.0);

   oi = _create_match_editor(cfd, evas, cfdata, &(cfdata->match.overrides), &il);
   cfdata->overrides_il = il;
   e_widget_toolbook_page_append(tb, NULL, _("Over"), oi, 1, 1, 1, 1, 0.5, 0.0);

   oi = _create_match_editor(cfd, evas, cfdata, &(cfdata->match.menus), &il);
   cfdata->menus_il = il;
   e_widget_toolbook_page_append(tb, NULL, _("Menus"), oi, 1, 1, 1, 1, 0.5, 0.0);

   e_widget_toolbook_page_show(tb, 0);

   return tb;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog      *cfd,
                      Evas                 *evas,
                      E_Config_Dialog_Data *cfdata)
{
   Evas_Object *ob, *ol, *ol2, *of, *otb, *oi, *orec0, *tab;
   E_Radio_Group *rg;

   orec0 = evas_object_rectangle_add(evas);
   evas_object_name_set(orec0, "style_shadows");

   tab = e_widget_table_add(evas, 0);
   evas_object_name_set(tab, "dia_table");

   otb = e_widget_toolbook_add(evas, 48 * e_scale, 48 * e_scale);

   ///////////////////////////////////////////
   ol = e_widget_list_add(evas, 0, 0);
   ob = e_widget_check_add(evas, _("Shadows"), &(cfdata->use_shadow));
   evas_object_smart_callback_add(ob, "changed", _shadow_changed, cfdata);
   e_widget_list_object_append(ol, ob, 1, 0, 0.5);
   ob = e_widget_check_add(evas, _("Limit framerate"), &(cfdata->lock_fps));
   e_widget_list_object_append(ol, ob, 1, 0, 0.5);
   ob = e_widget_check_add(evas, _("Smooth scaling"), &(cfdata->smooth_windows));
   e_widget_list_object_append(ol, ob, 1, 0, 0.5);

   of = e_widget_frametable_add(evas, _("Styles"), 0);
   e_widget_frametable_content_align_set(of, 0.5, 0.5);
   oi = _create_styles_toolbook(cfd, evas, cfdata);
   e_widget_frametable_object_append(of, oi, 0, 0, 1, 1, 1, 1, 1, 1);
   e_widget_list_object_append(ol, of, 1, 1, 0.5);

   e_widget_toolbook_page_append(otb, NULL, _("Effects"), ol, 1, 1, 1, 1, 0.5, 0.0);

   ///////////////////////////////////////////
   ol = e_widget_list_add(evas, 0, 0);
   ob = e_widget_check_add(evas, _("Sync screen (VBlank)"), &(cfdata->vsync));
   e_widget_list_object_append(ol, ob, 1, 1, 0.5);
   ob = e_widget_check_add(evas, _("Sync windows"), &(cfdata->efl_sync));
   e_widget_list_object_append(ol, ob, 1, 1, 0.5);
   ob = e_widget_check_add(evas, _("Loose sync"), &(cfdata->loose_sync));
   e_widget_list_object_append(ol, ob, 1, 1, 0.5);
   ob = e_widget_check_add(evas, _("Grab Server during draw"), &(cfdata->grab));
   e_widget_list_object_append(ol, ob, 1, 1, 0.5);
   ob = e_widget_label_add(evas, _("Initial draw timeout for newly mapped windows"));
   e_widget_list_object_append(ol, ob, 1, 1, 0.5);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.2f Seconds"), 0.01, 0.5, 0.01, 0, &(cfdata->first_draw_delay), NULL, 150);
   e_widget_list_object_append(ol, ob, 1, 1, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Sync"), ol, 0, 0, 0, 0, 0.5, 0.0);

   ol = e_widget_list_add(evas, 0, 0);
   rg = e_widget_radio_group_new(&(cfdata->engine));
   ob = e_widget_radio_add(evas, _("Software"), E_EVAS_ENGINE_SOFTWARE_X11, rg);
   e_widget_list_object_append(ol, ob, 1, 1, 0.5);
   if (!getenv("ECORE_X_NO_XLIB")) 
     {
        if (ecore_evas_engine_type_supported_get(ECORE_EVAS_ENGINE_OPENGL_X11))
          {
             ob = e_widget_radio_add(evas, _("OpenGL"), E_EVAS_ENGINE_GL_X11, rg);
             e_widget_list_object_append(ol, ob, 1, 1, 0.5);

             of = e_widget_framelist_add(evas, _("OpenGL options"), 0);
             e_widget_framelist_content_align_set(of, 0.5, 0.0);
             ob = e_widget_check_add(evas, _("Texture from pixmap"), &(cfdata->texture_from_pixmap));
             e_widget_framelist_object_append(of, ob);
             ob = e_widget_check_add(evas, _("Indirect OpenGL"), &(cfdata->indirect));
             e_widget_framelist_object_append(of, ob);
             e_widget_list_object_append(ol, of, 1, 1, 0.5);
          }
     }
   e_widget_toolbook_page_append(otb, NULL, _("Engine"), ol, 0, 0, 0, 0, 0.5, 0.0);

   ///////////////////////////////////////////
   ol = e_widget_list_add(evas, 0, 0);
   ob = e_widget_check_add(evas, _("Send flush"), &(cfdata->send_flush));
   e_widget_list_object_append(ol, ob, 1, 1, 0.5);
   ob = e_widget_check_add(evas, _("Send dump"), &(cfdata->send_dump));
   e_widget_list_object_append(ol, ob, 1, 1, 0.5);
   ob = e_widget_check_add(evas, _("Don't composite fullscreen"), &(cfdata->nocomp_fs));
   e_widget_list_object_append(ol, ob, 1, 1, 0.5);
   ob = e_widget_check_add(evas, _("Keep hidden windows"), &(cfdata->keep_unmapped));
   e_widget_list_object_append(ol, ob, 1, 1, 0.5);
   of = e_widget_frametable_add(evas, _("Maximum hidden pixels"), 0);
   e_widget_frametable_content_align_set(of, 0.5, 0.5);
   rg = e_widget_radio_group_new(&(cfdata->max_unmapped_pixels));
   ob = e_widget_radio_add(evas, _("1M"), 1 * 1024, rg);
   e_widget_frametable_object_append(of, ob, 0, 0, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("2M"), 2 * 1024, rg);
   e_widget_frametable_object_append(of, ob, 0, 1, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("4M"), 4 * 1024, rg);
   e_widget_frametable_object_append(of, ob, 0, 2, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("8M"), 8 * 1024, rg);
   e_widget_frametable_object_append(of, ob, 1, 0, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("16M"), 16 * 1024, rg);
   e_widget_frametable_object_append(of, ob, 1, 1, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("32M"), 32 * 1024, rg);
   e_widget_frametable_object_append(of, ob, 1, 2, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("64M"), 64 * 1024, rg);
   e_widget_frametable_object_append(of, ob, 2, 0, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("128M"), 128 * 1024, rg);
   e_widget_frametable_object_append(of, ob, 2, 1, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("256M"), 256 * 1024, rg);
   e_widget_frametable_object_append(of, ob, 2, 2, 1, 1, 1, 1, 0, 0);
   e_widget_list_object_append(ol, of, 1, 1, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Memory"), ol, 0, 0, 0, 0, 0.5, 0.0);

   ///////////////////////////////////////////
   ol = e_widget_list_add(evas, 0, 0);
   ol2 = e_widget_list_add(evas, 1, 1);
   of = e_widget_framelist_add(evas, _("Min hidden"), 0);
   e_widget_framelist_content_align_set(of, 0.5, 0.0);
   rg = e_widget_radio_group_new(&(cfdata->min_unmapped_time));
   ob = e_widget_radio_add(evas, _("30 Seconds"), 30, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("1 Minute"), 60, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("5 Minutes"), 5 * 60, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("30 Minutes"), 30 * 60, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("2 Hours"), 2 * 3600, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("10 Hours"), 10 * 3600, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Forever"), 0, rg);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(ol2, of, 1, 1, 0.5);
   of = e_widget_framelist_add(evas, _("Max hidden"), 0);
   e_widget_framelist_content_align_set(of, 0.5, 0.0);
   rg = e_widget_radio_group_new(&(cfdata->max_unmapped_time));
   ob = e_widget_radio_add(evas, _("30 Seconds"), 30, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("1 Minute"), 60, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("5 Minutes"), 5 * 60, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("30 Minutes"), 30 * 60, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("2 Hours"), 2 * 3600, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("10 Hours"), 10 * 3600, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Forever"), 0, rg);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(ol2, of, 1, 1, 0.5);
   e_widget_list_object_append(ol, ol2, 1, 1, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Timeouts"), ol, 0, 0, 0, 0, 0.5, 0.0);

   ///////////////////////////////////////////
   ol = e_widget_list_add(evas, 0, 0);

   ob = e_widget_check_add(evas, _("Show Framerate"), &(cfdata->fps_show));
   e_widget_list_object_append(ol, ob, 1, 1, 0.5);
   ob = e_widget_label_add(evas, _("Rolling average frame count"));
   e_widget_list_object_append(ol, ob, 1, 1, 0.5);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f Frames"), 1, 120, 1, 0,
                            NULL, &(cfdata->fps_average_range), 240);
   e_widget_list_object_append(ol, ob, 1, 1, 0.5);

   of = e_widget_frametable_add(evas, _("Corner"), 0);
   e_widget_frametable_content_align_set(of, 0.5, 0.5);
   rg = e_widget_radio_group_new(&(cfdata->fps_corner));
   ob = e_widget_radio_icon_add(evas, NULL, "preferences-position-top-left",
                                24, 24, 0, rg);
   e_widget_frametable_object_append(of, ob, 0, 0, 1, 1, 1, 1, 1, 1);
   ob = e_widget_radio_icon_add(evas, NULL, "preferences-position-top-right",
                                24, 24, 1, rg);
   e_widget_frametable_object_append(of, ob, 1, 0, 1, 1, 1, 1, 1, 1);
   ob = e_widget_radio_icon_add(evas, NULL, "preferences-position-bottom-left",
                                24, 24, 2, rg);
   e_widget_frametable_object_append(of, ob, 0, 1, 1, 1, 1, 1, 1, 1);
   ob = e_widget_radio_icon_add(evas, NULL, "preferences-position-bottom-right",
                                24, 24, 3, rg);
   e_widget_frametable_object_append(of, ob, 1, 1, 1, 1, 1, 1, 1, 1);
   e_widget_list_object_append(ol, of, 1, 1, 0.5);

   e_widget_toolbook_page_append(otb, NULL, _("Debug"), ol, 0, 0, 0, 0, 0.5, 0.0);

   e_widget_toolbook_page_show(otb, 0);

   e_dialog_resizable_set(cfd->dia, 1);

   e_widget_table_object_append(tab, otb, 0, 0, 1, 1, 1, 1, 1, 1);
   return tab;
}

static void
_match_list_free(Eina_List *list)
{
   Match *m;

   EINA_LIST_FREE(list, m)
     {
        if (m->title) eina_stringshare_del(m->title);
        if (m->name) eina_stringshare_del(m->name);
        if (m->clas) eina_stringshare_del(m->clas);
        if (m->role) eina_stringshare_del(m->role);
        if (m->shadow_style) eina_stringshare_del(m->shadow_style);
        free(m);
     }
}

static void
_match_dup2(Match_Config *m2,
            Match        *m)
{
   *m = m2->match;
   if (m->title) m->title = eina_stringshare_add(m->title);
   if (m->name) m->name = eina_stringshare_add(m->name);
   if (m->clas) m->clas = eina_stringshare_add(m->clas);
   if (m->role) m->role = eina_stringshare_add(m->role);
   if (m->shadow_style) m->shadow_style = eina_stringshare_add(m->shadow_style);
}

static int
_basic_apply_data(E_Config_Dialog *cfd  __UNUSED__,
                  E_Config_Dialog_Data *cfdata)
{
   if ((cfdata->use_shadow != _comp_mod->conf->use_shadow) ||
       (cfdata->lock_fps != _comp_mod->conf->lock_fps) ||
       (cfdata->smooth_windows != _comp_mod->conf->smooth_windows) ||
       (cfdata->grab != _comp_mod->conf->grab) ||
       (cfdata->keep_unmapped != _comp_mod->conf->keep_unmapped) ||
       (cfdata->nocomp_fs != _comp_mod->conf->nocomp_fs) ||
       (cfdata->shadow_style != _comp_mod->conf->shadow_style) ||
       (cfdata->max_unmapped_pixels != _comp_mod->conf->max_unmapped_pixels) ||
       (cfdata->max_unmapped_time != _comp_mod->conf->max_unmapped_time) ||
       (cfdata->min_unmapped_time != _comp_mod->conf->min_unmapped_time) ||
       (cfdata->send_flush != _comp_mod->conf->send_flush) ||
       (cfdata->send_dump != _comp_mod->conf->send_dump) ||
       (cfdata->fps_show != _comp_mod->conf->fps_show) ||
       (cfdata->fps_corner != _comp_mod->conf->fps_corner) ||
       (cfdata->fps_average_range != _comp_mod->conf->fps_average_range) ||
       (cfdata->first_draw_delay != _comp_mod->conf->first_draw_delay) ||
       (cfdata->match.changed)
       )
     {
        if (cfdata->match.changed)
          {
             Eina_List *l;
             Match *m;
             Match_Config *m2;

             _match_list_free(_comp_mod->conf->match.popups);
             _match_list_free(_comp_mod->conf->match.borders);
             _match_list_free(_comp_mod->conf->match.overrides);
             _match_list_free(_comp_mod->conf->match.menus);

             _comp_mod->conf->match.popups = NULL;
             _comp_mod->conf->match.borders = NULL;
             _comp_mod->conf->match.overrides = NULL;
             _comp_mod->conf->match.menus = NULL;

             EINA_LIST_FOREACH(cfdata->match.popups, l, m2)
               {
                  m = E_NEW(Match, 1);
                  _match_dup2(m2, m);
                  _comp_mod->conf->match.popups =
                    eina_list_append(_comp_mod->conf->match.popups, m);
               }
             EINA_LIST_FOREACH(cfdata->match.borders, l, m2)
               {
                  m = E_NEW(Match, 1);
                  _match_dup2(m2, m);
                  _comp_mod->conf->match.borders =
                    eina_list_append(_comp_mod->conf->match.borders, m);
               }
             EINA_LIST_FOREACH(cfdata->match.overrides, l, m2)
               {
                  m = E_NEW(Match, 1);
                  _match_dup2(m2, m);
                  _comp_mod->conf->match.overrides =
                    eina_list_append(_comp_mod->conf->match.overrides, m);
               }
             EINA_LIST_FOREACH(cfdata->match.menus, l, m2)
               {
                  m = E_NEW(Match, 1);
                  _match_dup2(m2, m);
                  _comp_mod->conf->match.menus =
                    eina_list_append(_comp_mod->conf->match.menus, m);
               }
             cfdata->match.changed = 0;
          }
        _comp_mod->conf->use_shadow = cfdata->use_shadow;
        _comp_mod->conf->lock_fps = cfdata->lock_fps;
        _comp_mod->conf->smooth_windows = cfdata->smooth_windows;
        _comp_mod->conf->grab = cfdata->grab;
        _comp_mod->conf->keep_unmapped = cfdata->keep_unmapped;
        _comp_mod->conf->nocomp_fs = cfdata->nocomp_fs;
        _comp_mod->conf->max_unmapped_pixels = cfdata->max_unmapped_pixels;
        _comp_mod->conf->max_unmapped_time = cfdata->max_unmapped_time;
        _comp_mod->conf->min_unmapped_time = cfdata->min_unmapped_time;
        _comp_mod->conf->send_flush = cfdata->send_flush;
        _comp_mod->conf->send_dump = cfdata->send_dump;
        _comp_mod->conf->fps_show = cfdata->fps_show;
        _comp_mod->conf->fps_corner = cfdata->fps_corner;
        _comp_mod->conf->fps_average_range = cfdata->fps_average_range;
        _comp_mod->conf->first_draw_delay = cfdata->first_draw_delay;
        if (_comp_mod->conf->shadow_style)
          eina_stringshare_del(_comp_mod->conf->shadow_style);
        _comp_mod->conf->shadow_style = NULL;
        if (cfdata->shadow_style)
          _comp_mod->conf->shadow_style = eina_stringshare_add(cfdata->shadow_style);
        e_mod_comp_shadow_set();
     }
   if ((cfdata->engine != _comp_mod->conf->engine) ||
       (cfdata->indirect != _comp_mod->conf->indirect) ||
       (cfdata->texture_from_pixmap != _comp_mod->conf->texture_from_pixmap) ||
       (cfdata->efl_sync != _comp_mod->conf->efl_sync) ||
       (cfdata->loose_sync != _comp_mod->conf->loose_sync) ||
       (cfdata->vsync != _comp_mod->conf->vsync))
     {
        E_Action *a;

        _comp_mod->conf->engine = cfdata->engine;
        _comp_mod->conf->indirect = cfdata->indirect;
        _comp_mod->conf->texture_from_pixmap = cfdata->texture_from_pixmap;
        _comp_mod->conf->efl_sync = cfdata->efl_sync;
        _comp_mod->conf->loose_sync = cfdata->loose_sync;
        _comp_mod->conf->vsync = cfdata->vsync;

        a = e_action_find("restart");
        if ((a) && (a->func.go)) a->func.go(NULL, NULL);
//        e_mod_comp_shutdown();
//        e_mod_comp_init();
     }
   e_config_save_queue();
   return 1;
}

