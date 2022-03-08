#include "e.h"

static E_Config_Dialog_View *_config_view_new(void);

static void                 *_create_data(E_Config_Dialog *cfd);
static void                  _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int                   _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object          *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int                   _basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static void                  _fill_data(E_Config_Dialog_Data *cfdata);
static void                  _basic_apply_border(E_Config_Dialog_Data *cfdata);

struct _E_Config_Dialog_Data
{
   E_Border    *border;
   E_Container *container;
   const char  *bordername;
   int          remember_border;
};

E_Config_Dialog *
e_int_config_borders(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "appearance/borders")) return NULL;
   v = _config_view_new();
   if (!v) return NULL;
   cfd = e_config_dialog_new(con, _("Default Border Style"),
                             "E", "appearance/borders",
                             "preferences-system-windows", 0, v, con);
   return cfd;
}

E_Config_Dialog *
e_int_config_borders_border(E_Container *con __UNUSED__, const char *params)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   E_Border *bd;

   if (!params) return NULL;
   bd = NULL;
   sscanf(params, "%p", &bd);
   if (!bd) return NULL;
   v = _config_view_new();
   if (!v) return NULL;
   cfd = e_config_dialog_new(bd->zone->container,
                             _("Window Border Selection"),
                             "E", "_config_border_border_style_dialog",
                             "preferences-system-windows", 0, v, bd);
   bd->border_border_dialog = cfd;
   return cfd;
}

static E_Config_Dialog_View *
_config_view_new(void)
{
   E_Config_Dialog_View *v;

   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return NULL;
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;
   v->basic.check_changed = _basic_check_changed;
   v->override_auto_apply = 1;
   return v;
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->container = NULL;
   cfdata->border = NULL;
   if (E_OBJECT(cfd->data)->type == E_CONTAINER_TYPE)
     cfdata->container = cfd->data;
   else
     cfdata->border = cfd->data;

   _fill_data(cfdata);
   return cfdata;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   if (cfdata->border)
     {
        if ((cfdata->border->remember) &&
            (cfdata->border->remember->apply & E_REMEMBER_APPLY_BORDER))
          {
             cfdata->remember_border = 1;
          }
        cfdata->bordername = eina_stringshare_add(cfdata->border->client.border.name);
     }
   else
     cfdata->bordername = eina_stringshare_add(e_config->theme_default_border_style);
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   if (cfdata->border)
     cfdata->border->border_border_dialog = NULL;

   eina_stringshare_del(cfdata->bordername);
   E_FREE(cfdata);
}

static int
_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   Eina_Bool remch = ((cfdata->remember_border && 
                       !((cfdata->border->remember) &&
                         (cfdata->border->remember->apply & E_REMEMBER_APPLY_BORDER))) ||
                      (!cfdata->remember_border && cfdata->border &&
                          ((cfdata->border->remember) &&
                              (cfdata->border->remember->apply & E_REMEMBER_APPLY_BORDER))));
   if (cfdata->border)
     return (cfdata->bordername != cfdata->border->client.border.name) || (remch);
   else
     return (cfdata->bordername != e_config->theme_default_border_style) || (remch);
}

static int
_basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   if (cfdata->border)
     _basic_apply_border(cfdata);
   else if (cfdata->container)
     {
        Eina_List *l;
        E_Border *bd;
        eina_stringshare_replace(&e_config->theme_default_border_style, cfdata->bordername);
        EINA_LIST_FOREACH(e_border_client_list(), l, bd)
          {
             bd->changed = 1;
             bd->client.border.changed = 1;
          }
     }
   e_config_save_queue();
   return 1;
}

static void
_basic_apply_border(E_Config_Dialog_Data *cfdata)
{
   if ((!cfdata->border->lock_border) && (!cfdata->border->shaded))
     {
        eina_stringshare_replace(&cfdata->border->bordername, cfdata->bordername);
        cfdata->border->client.border.changed = 1;
        cfdata->border->changed = 1;
     }
   if (cfdata->remember_border)
     {
        E_Remember *rem = cfdata->border->remember;

        if (!rem)
          {
             rem = e_remember_new();
             if (rem) e_remember_use(rem);
          }
        if (rem)
          {
             rem->apply |= E_REMEMBER_APPLY_BORDER;
             e_remember_default_match_set(rem, cfdata->border);
             eina_stringshare_replace(&rem->prop.border, cfdata->border->bordername);
             cfdata->border->remember = rem;
             e_remember_update(cfdata->border);
          }
     }
   else
     {
        if (cfdata->border->remember)
          {
             cfdata->border->remember->apply &= ~E_REMEMBER_APPLY_BORDER;
             if (cfdata->border->remember->apply == 0)
               {
                  e_remember_unuse(cfdata->border->remember);
                  e_remember_del(cfdata->border->remember);
                  cfdata->border->remember = NULL;
               }
          }
     }
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *ol, *ob, *oj, *orect, *of;
   Evas_Coord w, h;
   Eina_List *borders;
   int n = 1, sel = 0;
   const char *str, *tmp;

   e_dialog_resizable_set(cfd->dia, 1);
   if (cfdata->border)
     tmp = cfdata->border->client.border.name;
   else
     tmp = e_config->theme_default_border_style;

   o = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("Default Border Style"), 0);
   e_widget_framelist_content_align_set(of, 0.0, 0.0);
   ol = e_widget_ilist_add(evas, 96, 96, &(cfdata->bordername));
   borders = e_theme_border_list();
   orect = evas_object_rectangle_add(evas);
   evas_object_color_set(orect, 0, 0, 0, 128);

   evas_event_freeze(evas_object_evas_get(ol));
   edje_freeze();
   e_widget_ilist_freeze(ol);
   e_widget_ilist_append(ol, orect, "borderless", NULL, NULL, "borderless");

   EINA_LIST_FREE(borders, str)
     {
        char buf[4096];

        ob = e_livethumb_add(evas);
        e_livethumb_vsize_set(ob, 128, 128);
        oj = edje_object_add(e_livethumb_evas_get(ob));
        snprintf(buf, sizeof(buf), "e/widgets/border/%s/border", str);
        e_theme_edje_object_set(oj, "base/theme/borders", buf);
        e_livethumb_thumb_set(ob, oj);
        orect = evas_object_rectangle_add(e_livethumb_evas_get(ob));
        evas_object_color_set(orect, 0, 0, 0, 128);
        evas_object_show(orect);
        edje_object_part_swallow(oj, "e.swallow.client", orect);
        e_widget_ilist_append(ol, ob, (char *)str, NULL, NULL, str);
        if (tmp == str) sel = n;
        n++;
        eina_stringshare_del(str);
     }

   e_widget_size_min_get(ol, &w, &h);
   e_widget_size_min_set(ol, w > 200 ? w : 200, 280);

   e_widget_ilist_go(ol);
   e_widget_ilist_selected_set(ol, sel);
   e_widget_ilist_thaw(ol);
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(ol));

   e_widget_framelist_object_append(of, ol);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   if (cfdata->border)
     {
        ob = e_widget_check_add(evas, _("Remember this Border for this window next time it appears"),
                                &(cfdata->remember_border));
        e_widget_list_object_append(o, ob, 1, 0, 0.0);
     }

   return o;
}

