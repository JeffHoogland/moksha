/**
 * @addtogroup Optional_Look
 * @{
 *
 * @defgroup Module_Shot Screenshot
 *
 * Takes screen shots and can submit them to http://enlightenment.org
 *
 * @}
 */
#include "e_mod_main.h"

E_Module *shot_module = NULL;

static E_Action *border_act = NULL, *delay_act = NULL, *act = NULL;
static E_Int_Menu_Augmentation *maug = NULL;
static Ecore_Timer *timer = NULL, *border_timer = NULL;
static Evas_Object *snap = NULL;
static E_Border_Menu_Hook *border_hook = NULL;
static E_Object_Delfn *delfn_client = NULL;
static E_Object_Delfn *delfn_zone = NULL;

static E_Border *shot_bd = NULL;
static E_Zone *shot_zone = NULL;
static char *shot_params;

static void
_cb_border_del(void *data __UNUSED__, void *obj __UNUSED__)
{
   if (delfn_client)
     {
        e_object_delfn_del(E_OBJECT(shot_bd), delfn_client);
        delfn_client = NULL;
     }
   if (delfn_zone)
     {
        e_object_delfn_del(E_OBJECT(shot_zone), delfn_zone);
        delfn_zone = NULL;
     }
   if (timer)
     {
        ecore_timer_del(timer);
        timer = NULL;
     }
   if (border_timer)
     {
        ecore_timer_del(border_timer);
        border_timer = NULL;
     }
   E_FREE_FUNC(snap, evas_object_del);
   E_FREE(shot_params);
}

static void
_cb_zone_del(void *data, void *obj)
{
   _cb_border_del(data, obj);
}

static void
_shot_post(void *buffer, Evas *e, void *event __UNUSED__)
{
   int x, y, w, h;
   evas_object_geometry_get(snap, &x, &y, &w, &h);
   evas_event_callback_del(e, EVAS_CALLBACK_RENDER_POST, _shot_post);
   preview_dialog_show(shot_zone, shot_bd, shot_params,
                       //~ (void *)evas_object_image_data_get(snap, 0),
                       buffer,
                       x, y, w, h);
   E_FREE_FUNC(snap, evas_object_del);
   if (delfn_client)
     {
        e_object_delfn_del(E_OBJECT(shot_bd), delfn_client);
        delfn_client = NULL;
     }
   if (delfn_zone)
     {
        e_object_delfn_del(E_OBJECT(shot_zone), delfn_zone);
        delfn_zone = NULL;
     }
   shot_bd = NULL;
   shot_zone = NULL;
   E_FREE(shot_params);
}

static void
_shot_now(E_Zone *zone, E_Border *bd, const char *params)
{
   E_Manager *sman = NULL;
   Ecore_X_Image *img;
   unsigned char *src;
   unsigned int *dst;
   int x, y, w, h;
   int bpl = 0, rows = 0, bpp = 0, sw, sh;
   Ecore_X_Window xwin, root;
   Ecore_X_Visual visual;
   Ecore_X_Display *display;
   Ecore_X_Screen *scr;
   Ecore_X_Window_Attributes watt;
   Ecore_X_Colormap colormap;

   if (preview_have() || share_have() || (snap)) return;
   if ((!zone) && (!bd)) return;

   Ecore_Evas *ee = ecore_evas_buffer_new(1, 1);
   Evas *e = ecore_evas_get(ee);

   watt.visual = 0;
   if (zone)
     {
        sman = zone->container->manager;
        xwin = sman->root;
        w = sw = sman->w;
        h = sh = sman->h;
        x = y = 0;
     }
   else
     {
        root = bd->zone->container->manager->root;
        xwin = bd->client.win;
        while (xwin != root)
          {
             if (ecore_x_window_parent_get(xwin) == root) break;
             xwin = ecore_x_window_parent_get(xwin);
          }
        ecore_x_window_geometry_get(xwin, &x, &y, &sw, &sh);
        w = sw;
        h = sh;
        xwin = root;
        x = E_CLAMP(bd->x, bd->zone->x, bd->zone->x + bd->zone->w);
        y = E_CLAMP(bd->y, bd->zone->y, bd->zone->y + bd->zone->h);
        sw = E_CLAMP(sw, 1, bd->zone->x + bd->zone->w - x);
        sh = E_CLAMP(sh, 1, bd->zone->y + bd->zone->h - y);
     }

   if (!ecore_x_window_attributes_get(xwin, &watt)) return;
   visual = watt.visual;
   img = ecore_x_image_new(w, h, visual, ecore_x_window_depth_get(xwin));
   ecore_x_image_get(img, xwin, x, y, 0, 0, sw, sh);
   src = ecore_x_image_data_get(img, &bpl, &rows, &bpp);
   display = ecore_x_display_get();
   scr = ecore_x_default_screen_get();
   colormap = ecore_x_default_colormap_get(display, scr);
   dst = malloc(sw * sh * sizeof(unsigned int)); 
   ecore_x_image_to_argb_convert(src, bpp, bpl, colormap, visual,
                                 0, 0, sw, sh,
                                 dst, (sw * sizeof(int)), 0, 0);

   if (eina_streq(ecore_evas_engine_name_get(ee), "buffer"))
     {
        preview_dialog_show(zone, bd, params,
                            dst,
                            x, y, w, h);
        if (delfn_client)
          {
             e_object_delfn_del(E_OBJECT(bd), delfn_client);
             delfn_client = NULL;
          }
        if (delfn_zone)
          {
             e_object_delfn_del(E_OBJECT(zone), delfn_zone);
             delfn_zone = NULL;
          }
        return;
     }

   shot_bd = bd;
   shot_zone = zone;
   shot_params = eina_strdup(params);
   snap = evas_object_image_filled_add(e);
   evas_object_pass_events_set(snap, 1);
   evas_object_layer_set(snap, EVAS_LAYER_MAX);
   evas_object_image_snapshot_set(snap, 1);
   evas_object_geometry_set(snap, x, y, w, h);
   evas_object_show(snap);
   evas_object_image_data_update_add(snap, 0, 0, w, h);
   evas_object_image_pixels_dirty_set(snap, 1);
   evas_event_callback_add(e, EVAS_CALLBACK_RENDER_POST, _shot_post, dst);
   ecore_x_image_free(img);
   ecore_evas_manual_render(ee);
}

static Eina_Bool
_shot_delay(void *data)
{
   timer = NULL;
   _shot_now(data, NULL, NULL);
   return EINA_FALSE;
}

static Eina_Bool
_shot_delay_border_padded(void *data)
{
   char buf[128];

   border_timer = NULL;
   snprintf(buf, sizeof(buf), "pad %i", 64);
   _shot_now(NULL, data, buf);
   return EINA_FALSE;
}

static void
_shot_border_padded(E_Border *bd)
{
   if (border_timer) ecore_timer_del(border_timer);
   border_timer = ecore_timer_loop_add(1.0, _shot_delay_border_padded, bd);
   delfn_client = e_object_delfn_add(E_OBJECT(bd), _cb_border_del, NULL);
}

static void
_shot(E_Zone *zone)
{
   if (timer) ecore_timer_del(timer);
   timer = ecore_timer_loop_add(1.0, _shot_delay, zone);
   delfn_zone = e_object_delfn_add(E_OBJECT(zone), _cb_zone_del, NULL);
}

static void
_e_mod_menu_border_padded_cb(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   _shot_border_padded(data);
}

static void
_e_mod_menu_cb(void *data __UNUSED__, E_Menu *m, E_Menu_Item *mi __UNUSED__)
{
   if (m->zone) _shot(m->zone);
}

static void
_e_mod_action_border_cb(E_Object *obj __UNUSED__, const char *params)
{
   E_Border *bd;

   bd = e_border_focused_get();
   if (!bd) return;
   if (border_timer)
     {
        ecore_timer_del(border_timer);
        border_timer = NULL;
     }
   _shot_now(NULL, bd, params);
}

typedef struct
{
   E_Zone *zone;
   char *params;
} Delayed_Shot;

static void
_delayed_shot(void *data)
{
   Delayed_Shot *ds = data;

   _shot_now(ds->zone, NULL, ds->params);
   e_object_unref(E_OBJECT(ds->zone));
   free(ds->params);
   free(ds);
}

static Eina_Bool
_delayed_shot_timer(void *data)
{
   timer = NULL;
   _delayed_shot(data);
   return EINA_FALSE;
}

static void
_e_mod_action_delay_cb(E_Object *obj, const char *params)
{
   E_Zone *zone = NULL;
   Delayed_Shot *ds;
   double delay = 0.0;
   
   E_Container *con = e_container_current_get(e_manager_current_get());

   if (obj)
     {
        if (obj->type == E_ZONE_TYPE) zone = e_zone_current_get(con);
        else if (obj->type == E_ZONE_TYPE) zone = ((void *)obj);
        else zone = e_zone_current_get(con);
     }
   if (!zone) zone = e_zone_current_get(con);
   if (!zone) return;
   E_FREE_FUNC(timer, ecore_timer_del);
   ds = E_NEW(Delayed_Shot, 1);
   e_object_ref(E_OBJECT(zone));
   ds->zone = zone;
   ds->params = params ? strdup(params) : NULL;
   if (params) delay = (double)atoi(params) / 1000.0;
   if (timer) ecore_timer_del(timer);
   timer = ecore_timer_loop_add(delay, _delayed_shot_timer, ds);
   delfn_zone = e_object_delfn_add(E_OBJECT(zone), _cb_zone_del, NULL);
}

static void
_e_mod_action_cb(E_Object *obj, const char *params)
{
   E_Zone *zone = NULL;
   Delayed_Shot *ds;
   
   E_Container *con = e_container_current_get(e_manager_current_get());

   if (obj)
     {
        if (obj->type == E_ZONE_TYPE) zone = e_zone_current_get(con);
        else if (obj->type == E_ZONE_TYPE) zone = ((void *)obj);
        else zone = e_zone_current_get(con);
     }
   if (!zone) zone = e_zone_current_get(con);
   if (!zone) return;
   E_FREE_FUNC(timer, ecore_timer_del);
   ds = E_NEW(Delayed_Shot, 1);
   e_object_ref(E_OBJECT(zone));
   ds->zone = zone;
   ds->params = params ? strdup(params) : NULL;
   // forced main loop iteration in screenshots causes bugs if the action
   // executes immediately
   ecore_job_add(_delayed_shot, ds);
}

static void
_bd_hook(void *d __UNUSED__, E_Border *bd)
{
   E_Menu_Item *mi;
   E_Menu *m;
   Eina_List *l;

   if (!bd->border_menu) return;
   if (bd->iconic || (bd->desk != e_desk_current_get(bd->zone))) return;
   m = bd->border_menu;

   // position menu item just before first separator
   EINA_LIST_FOREACH(m->items, l, mi)
     {
        if (mi->separator) break;
     }
   if ((!mi) || (!mi->separator)) return;
   l = eina_list_prev(l);
   mi = eina_list_data_get(l);
   if (!mi) return;

   mi = e_menu_item_new_relative(m, mi);
   e_menu_item_label_set(mi, _("Take Shot"));
   e_util_menu_item_theme_icon_set(mi, "screenshot");
   e_menu_item_callback_set(mi, _e_mod_menu_border_padded_cb, bd);
}

static void
_e_mod_menu_add(void *data __UNUSED__, E_Menu *m)
{
   E_Menu_Item *mi;

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Take Screenshot"));
   e_util_menu_item_theme_icon_set(mi, "screenshot");
   e_menu_item_callback_set(mi, _e_mod_menu_cb, NULL);
}

/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
     "Shot"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   if (!ecore_con_url_init())
     {
        e_util_dialog_show(_("Shot Error"),
                           _("Cannot initialize network"));
        return NULL;
     }

   shot_module = m;
   act = e_action_add("shot");
   if (act)
     {
        act->func.go = _e_mod_action_cb;
        e_action_predef_name_set(N_("Screen"), N_("Take Screenshot"),
                                 "shot", NULL,
                                 "syntax: [share|save [perfect|high|medium|low|QUALITY current|all|SCREEN-NUM]", 1);
     }
   delay_act = e_action_add("shot_delay");
   if (delay_act)
     {
        delay_act->func.go = _e_mod_action_delay_cb;
        e_action_predef_name_set(N_("Screen"), N_("Take Screenshot with Delay"),
                                 "shot_delay", NULL,
                                 "syntax: delay_ms (e.g. 3000)", 1);
     }
   border_act = e_action_add("border_shot");
   if (border_act)
     {
        border_act->func.go = _e_mod_action_border_cb;
        e_action_predef_name_set(N_("Window : Actions"), N_("Take Shot"),
                                 "border_shot", NULL,
                                 "syntax: [share|save perfect|high|medium|low|QUALITY all|current] [pad N]", 1);
     }
   maug = e_int_menus_menu_augmentation_add_sorted
     ("main/2",  _("Take Screenshot"), _e_mod_menu_add, NULL, NULL, NULL);
   border_hook = e_int_border_menu_hook_add(_bd_hook, NULL);

   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   share_abort();
   preview_abort();
   delay_abort();
   if (delfn_client)
     {
        e_object_delfn_del(E_OBJECT(shot_bd), delfn_client);
        delfn_client = NULL;
     }
   if (delfn_zone)
     {
        e_object_delfn_del(E_OBJECT(shot_zone), delfn_zone);
        delfn_zone = NULL;
     }
   if (timer)
     {
        ecore_timer_del(timer);
        timer = NULL;
     }
   if (border_timer)
     {
        ecore_timer_del(border_timer);
        border_timer = NULL;
     }
   E_FREE_FUNC(snap, evas_object_del);
   E_FREE(shot_params);
   if (maug)
     {
        e_int_menus_menu_augmentation_del("main/2", maug);
        maug = NULL;
     }
   if (act)
     {
        e_action_predef_name_del("Screen", "Take Screenshot");
        e_action_del("shot");
        act = NULL;
     }

   shot_module = NULL;
   e_int_border_menu_hook_del(border_hook);
   ecore_con_url_shutdown();
   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   return 1;
}
