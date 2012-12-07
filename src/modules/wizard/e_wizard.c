#include "e_wizard.h"

static void     _e_wizard_next_eval(void);
static E_Popup *_e_wizard_main_new(E_Zone *zone);
static E_Popup *_e_wizard_extra_new(E_Zone *zone);
static void     _e_wizard_cb_key_down(void *data, Evas *e, Evas_Object *obj, void *event);
static void     _e_wizard_cb_next(void *data, Evas_Object *obj, const char *emission, const char *source);

static Eina_Bool _e_wizard_check_xdg(void);
static void      _e_wizard_next_xdg(void);
static Eina_Bool _e_wizard_cb_next_page(void *data);
static Eina_Bool _e_wizard_cb_desktops_update(void *data, int ev_type, void *ev);
static Eina_Bool _e_wizard_cb_icons_update(void *data, int ev_type, void *ev);

static E_Popup *pop = NULL;
static Eina_List *pops = NULL;
static Evas_Object *o_bg = NULL;
static Evas_Object *o_content = NULL;
static E_Wizard_Page *pages = NULL;
static E_Wizard_Page *curpage = NULL;
static int next_ok = 1;
static int next_prev = 0;
static int next_can = 0;

static Eina_List *handlers = NULL;
static Eina_Bool got_desktops = EINA_FALSE;
static Eina_Bool got_icons = EINA_FALSE;
#if (EFREET_VERSION_MAJOR > 1) || (EFREET_VERSION_MINOR >= 8)
static Eina_Bool xdg_error = EINA_FALSE;
#endif
static Eina_Bool need_xdg_desktops = EINA_FALSE;
static Eina_Bool need_xdg_icons = EINA_FALSE;

static Ecore_Timer *next_timer = NULL;

EAPI int
e_wizard_init(void)
{
   E_Manager *man;
   Eina_List *l;

   EINA_LIST_FOREACH(e_manager_list(), l, man)
     {
        E_Container *con;
        Eina_List *l2;

        EINA_LIST_FOREACH(man->containers, l2, con)
          {
             Eina_List *l3;
             E_Zone *zone;

             EINA_LIST_FOREACH(con->zones, l3, zone)
               {
                  if (!pop)
                    pop = _e_wizard_main_new(zone);
                  else
                    pops = eina_list_append(pops, _e_wizard_extra_new(zone));
               }
          }
     }

   E_LIST_HANDLER_APPEND(handlers, EFREET_EVENT_DESKTOP_CACHE_BUILD,
                         _e_wizard_cb_desktops_update, NULL);

   E_LIST_HANDLER_APPEND(handlers, EFREET_EVENT_ICON_CACHE_UPDATE,
                         _e_wizard_cb_icons_update, NULL);
   return 1;
}

EAPI int
e_wizard_shutdown(void)
{
   E_Object *eo;

   if (pop)
     {
        e_object_del(E_OBJECT(pop));
        pop = NULL;
     }

   EINA_LIST_FREE(pops, eo)
     e_object_del(eo);
   while (pages)
     e_wizard_page_del(pages);

   if (next_timer) ecore_timer_del(next_timer);
   next_timer = NULL;
   E_FREE_LIST(handlers, ecore_event_handler_del);
   return 1;
}

EAPI void
e_wizard_go(void)
{
   if (!curpage)
     curpage = pages;
   if (curpage)
     {
        if (curpage->init) curpage->init(curpage, &need_xdg_desktops, &need_xdg_icons);
        curpage->state++;
        _e_wizard_next_eval();
        if (_e_wizard_check_xdg())
          {
             if ((curpage->show) && (!curpage->show(curpage)))
               {
                  curpage->state++;
                  e_wizard_next();
               }
             else
               curpage->state++;
          }
     }
}

EAPI void
e_wizard_apply(void)
{
   E_Wizard_Page *pg;

   EINA_INLIST_FOREACH(EINA_INLIST_GET(pages), pg)
     if (pg->apply) pg->apply(pg);
}

EAPI void
e_wizard_next(void)
{
   if (!curpage)
     {
        /* FINISH */
        e_wizard_apply();
        e_wizard_shutdown();
        return;
     }
   if (curpage->hide)
     curpage->hide(curpage);
   curpage->state++;
   curpage = EINA_INLIST_CONTAINER_GET(EINA_INLIST_GET(curpage)->next, E_Wizard_Page);
   if (!curpage)
     {
        e_wizard_next();
        return;
     }

   e_wizard_button_next_enable_set(1);
   need_xdg_desktops = EINA_FALSE;
   need_xdg_icons = EINA_FALSE;
   if (curpage->init)
     curpage->init(curpage, &need_xdg_desktops, &need_xdg_icons);
   curpage->state++;
   if (!_e_wizard_check_xdg()) return;

   _e_wizard_next_eval();
   curpage->state++;
   if (curpage->show && curpage->show(curpage)) return;
   e_wizard_next();
}

EAPI void
e_wizard_page_show(Evas_Object *obj)
{
   if (o_content) evas_object_del(o_content);
   o_content = obj;
   if (obj)
     {
        Evas_Coord minw, minh;

        e_widget_size_min_get(obj, &minw, &minh);
        edje_extern_object_min_size_set(obj, minw, minh);
        edje_object_part_swallow(o_bg, "e.swallow.content", obj);
        evas_object_show(obj);
        e_widget_focus_set(obj, 1);
        edje_object_signal_emit(o_bg, "e,action,page,new", "e");
     }
}

EAPI E_Wizard_Page *
e_wizard_page_add(void *handle,
                  int (*init_cb)(E_Wizard_Page *pg, Eina_Bool *need_xdg_desktops, Eina_Bool *need_xdg_icons),
                  int (*shutdown_cb)(E_Wizard_Page *pg),
                  int (*show_cb)(E_Wizard_Page *pg),
                  int (*hide_cb)(E_Wizard_Page *pg),
                  int (*apply_cb)(E_Wizard_Page *pg)
                  )
{
   E_Wizard_Page *pg;

   pg = E_NEW(E_Wizard_Page, 1);
   if (!pg) return NULL;

   pg->handle = handle;
   pg->evas = pop->evas;

   pg->init = init_cb;
   pg->shutdown = shutdown_cb;
   pg->show = show_cb;
   pg->hide = hide_cb;
   pg->apply = apply_cb;

   pages = (E_Wizard_Page*)eina_inlist_append(EINA_INLIST_GET(pages), EINA_INLIST_GET(pg));

   return pg;
}

EAPI void
e_wizard_page_del(E_Wizard_Page *pg)
{
// rare thing.. if we do e_wizard_next() from a page and we end up finishing
// ther page seq.. we cant REALLY dlclose... not a problem as wizard runs
// once only then e restarts itself with final wizard page
//   if (pg->handle) dlclose(pg->handle);
   if (pg->shutdown) pg->shutdown(pg);
   pages = (E_Wizard_Page*)eina_inlist_remove(EINA_INLIST_GET(pages), EINA_INLIST_GET(pg));
   free(pg);
}

EAPI void
e_wizard_button_next_enable_set(int enable)
{
   next_ok = enable;
   _e_wizard_next_eval();
}

EAPI void
e_wizard_title_set(const char *title)
{
   edje_object_part_text_set(o_bg, "e.text.title", title);
}

EAPI void
e_wizard_labels_update(void)
{
   edje_object_part_text_set(o_bg, "e.text.label", _("Next"));
}

EAPI const char *
e_wizard_dir_get(void)
{
   return e_module_dir_get(wiz_module);
}

EAPI void
e_wizard_xdg_desktops_reset(void)
{
#if (EFREET_VERSION_MAJOR > 1) || (EFREET_VERSION_MINOR >= 8)
   if (xdg_error) return;
#endif
   got_desktops = EINA_FALSE;
}

static void
_e_wizard_next_eval(void)
{
   int ok;

   ok = next_can;
   if (!next_ok) ok = 0;
   if (next_prev != ok)
     {
        if (ok)
          {
             edje_object_part_text_set(o_bg, "e.text.label", _("Next"));
             edje_object_signal_emit(o_bg, "e,state,next,enable", "e");
          }
        else
          {
             edje_object_part_text_set(o_bg, "e.text.label", _("Please Wait..."));
             edje_object_signal_emit(o_bg, "e,state,next,disable", "e");
          }
        next_prev = ok;
     }
}

static E_Popup *
_e_wizard_main_new(E_Zone *zone)
{
   E_Popup *popup;
   Evas_Object *o;
   Evas_Modifier_Mask mask;
   Eina_Bool kg;

   popup = e_popup_new(zone, 0, 0, zone->w, zone->h);
   e_popup_layer_set(popup, E_LAYER_TOP);
   o = edje_object_add(popup->evas);

   e_theme_edje_object_set(o, "base/theme/wizard", "e/wizard/main");
   evas_object_move(o, 0, 0);
   evas_object_resize(o, zone->w, zone->h);
   evas_object_show(o);
   edje_object_signal_callback_add(o, "e,action,next", "",
                                   _e_wizard_cb_next, popup);
   o_bg = o;

   o = evas_object_rectangle_add(popup->evas);
   mask = 0;
   kg = evas_object_key_grab(o, "Tab", mask, ~mask, 0);
   if (!kg)
     fprintf(stderr, "ERROR: unable to redirect \"Tab\" key events to object %p.\n", o);
   mask = evas_key_modifier_mask_get(popup->evas, "Shift");
   kg = evas_object_key_grab(o, "Tab", mask, ~mask, 0);
   if (!kg)
     fprintf(stderr, "ERROR: unable to redirect \"Tab\" key events to object %p.\n", o);
   mask = 0;
   kg = evas_object_key_grab(o, "Return", mask, ~mask, 0);
   if (!kg)
     fprintf(stderr, "ERROR: unable to redirect \"Return\" key events to object %p.\n", o);
   mask = 0;
   kg = evas_object_key_grab(o, "KP_Enter", mask, ~mask, 0);
   if (!kg)
     fprintf(stderr, "ERROR: unable to redirect \"KP_Enter\" key events to object %p.\n", o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_KEY_DOWN,
                                  _e_wizard_cb_key_down, popup);

   /* set up next/prev buttons */
   edje_object_part_text_set(o_bg, "e.text.title", _("Welcome to Enlightenment"));
//   edje_object_signal_emit(o_bg, "e,state,next,disable", "e");
   e_wizard_labels_update();

   e_popup_edje_bg_object_set(popup, o_bg);
   e_popup_show(popup);
   if (!e_grabinput_get(ecore_evas_software_x11_window_get(popup->ecore_evas),
                        1, ecore_evas_software_x11_window_get(popup->ecore_evas)))
     {
        e_object_del(E_OBJECT(popup));
        popup = NULL;
     }
   return popup;
}

static E_Popup *
_e_wizard_extra_new(E_Zone *zone)
{
   E_Popup *popup;
   Evas_Object *o;

   popup = e_popup_new(zone, 0, 0, zone->w, zone->h);
   e_popup_layer_set(popup, E_LAYER_TOP);
   o = edje_object_add(popup->evas);
   e_theme_edje_object_set(o, "base/theme/wizard", "e/wizard/extra");
   evas_object_move(o, 0, 0);
   evas_object_resize(o, zone->w, zone->h);
   evas_object_show(o);
   e_popup_edje_bg_object_set(popup, o);
   e_popup_show(popup);
   return popup;
}

static void
_e_wizard_cb_key_down(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event)
{
   Evas_Event_Key_Down *ev;

   ev = event;
   if (!o_content) return;
   if (!strcmp(ev->keyname, "Tab"))
     {
        if (evas_key_modifier_is_set(ev->modifiers, "Shift"))
          e_widget_focus_jump(o_content, 0);
        else
          e_widget_focus_jump(o_content, 1);
     }
   else if (((!strcmp(ev->keyname, "Return")) ||
             (!strcmp(ev->keyname, "KP_Enter")) ||
             (!strcmp(ev->keyname, "space"))))
     {
        Evas_Object *o;

        o = e_widget_focused_object_get(o_content);
        if (o) e_widget_activate(o);
     }
}

static void
_e_wizard_cb_next(void *data __UNUSED__, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   /* TODO: Disable button in theme */
   if (next_can)
     e_wizard_next();
}

static Eina_Bool
_e_wizard_check_xdg(void)
{
   if ((need_xdg_desktops) && (!got_desktops))
     {
        /* Advance within 15 secs if no xdg event */
        if (!next_timer)
          next_timer = ecore_timer_add(7.0, _e_wizard_cb_next_page, NULL);
        next_can = 0;
        _e_wizard_next_eval();
        return 0;
     }
   if ((need_xdg_icons) && (!got_icons))
     {
        char path[PATH_MAX];

        /* Check if cache already exists */
        snprintf(path, sizeof(path), "%s/efreet/icon_themes_%s.eet",
             efreet_cache_home_get(), efreet_hostname_get());
        if (ecore_file_exists(path))
          {
             got_icons = EINA_TRUE;
          }
        else
          {
             /* Advance within 15 secs if no xdg event */
             if (!next_timer)
               next_timer = ecore_timer_add(7.0, _e_wizard_cb_next_page, NULL);
             next_can = 0;
             _e_wizard_next_eval();
             return 0;
          }
     }
   next_can = 1;
   need_xdg_desktops = EINA_FALSE;
   need_xdg_icons = EINA_FALSE;
   return 1;
}

static void
_e_wizard_next_xdg(void)
{
   need_xdg_desktops = EINA_FALSE;
   need_xdg_icons = EINA_FALSE;

   if (next_timer) ecore_timer_del(next_timer);
   next_timer = NULL;
   if (curpage->state != E_WIZARD_PAGE_STATE_SHOW)
     {
        if (next_ok) return; // waiting for user
        e_wizard_next();
     }
   else if ((curpage->show) && (!curpage->show(curpage)))
     {
        curpage->state++;
        e_wizard_next();
     }
   else
     curpage->state++;
}

static Eina_Bool
_e_wizard_cb_next_page(void *data __UNUSED__)
{
   next_timer = NULL;
   _e_wizard_next_xdg();
   return ECORE_CALLBACK_CANCEL;
}


static Eina_Bool
_e_wizard_cb_desktops_update(void *data __UNUSED__, int ev_type __UNUSED__, void *ev)
{
#if (EFREET_VERSION_MAJOR > 1) || (EFREET_VERSION_MINOR >= 8)
   Efreet_Event_Cache_Update *e;

   e = ev;
   /* TODO: Tell user he should fix his dbus setup */
   xdg_error = !!e->error;

#endif
   got_desktops = EINA_TRUE;
   if (_e_wizard_check_xdg())
     _e_wizard_next_xdg();
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_wizard_cb_icons_update(void *data __UNUSED__, int ev_type __UNUSED__, void *ev __UNUSED__)
{
   got_icons = EINA_TRUE;
   if (_e_wizard_check_xdg())
     _e_wizard_next_xdg();
   return ECORE_CALLBACK_PASS_ON;
}
