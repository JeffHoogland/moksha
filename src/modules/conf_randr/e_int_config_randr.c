#include "e.h"
#include "e_mod_main.h"
#include "e_int_config_randr.h"
#include "e_smart_randr.h"
#include "e_smart_monitor.h"

/* local structures */
typedef struct _Randr_Info Randr_Info;
struct _Randr_Info
{
   E_Win *win;
   Evas_Object *o_bg;
   Evas_Object *o_scroll;
};

/* local function prototypes */
static Randr_Info *_randr_info_new(E_Container *con);
static void _randr_info_free(void);
static void _win_cb_resize(E_Win *win);
static void _win_cb_delete(E_Win *win __UNUSED__);
static void _close_cb_click(void *data __UNUSED__, void *data2 __UNUSED__);

/* local variables */
static Randr_Info *_randr_info = NULL;

E_Config_Dialog *
e_int_config_randr(E_Container *con, const char *params __UNUSED__)
{
   if (e_randr_screen_info.randr_version < ECORE_X_RANDR_1_2) 
     return NULL;

   if (_randr_info)
     {
        /* already have an existing window, show it */
        e_win_show(_randr_info->win);
        e_win_raise(_randr_info->win);
        return NULL;
     }

   /* create new window */
   _randr_info = _randr_info_new(con);

   return NULL;
}

/* local functions */
static Randr_Info *
_randr_info_new(E_Container *con)
{
   Eina_List *l;
   E_Randr_Crtc_Info *crtc;
   Randr_Info *info = NULL;
   Evas *evas;
   Evas_Object *ob;
   int mw = 0, mh = 0;

   if (!(info = calloc(1, sizeof(Randr_Info)))) return NULL;

   /* create window */
   if (!(info->win = e_win_new(con)))
     {
        free(info);
        return NULL;
     }

   info->win->data = info;

   /* set window properties */
   e_win_dialog_set(info->win, EINA_FALSE);
   e_win_title_set(info->win, _("Screen Setup"));
   e_win_name_class_set(info->win, "E", "_config::screen/screen_setup");
   e_win_resize_callback_set(info->win, _win_cb_resize);
   e_win_delete_callback_set(info->win, _win_cb_delete);

   evas = e_win_evas_get(info->win);

   /* create background */
   info->o_bg = edje_object_add(evas);
   e_theme_edje_object_set(info->o_bg, "base/theme/widgets", 
                           "e/conf/randr/main/window");
   evas_object_move(info->o_bg, 0, 0);
   evas_object_show(info->o_bg);

   printf("Max Size: %d %d\n", E_RANDR_12->max_size.width, 
          E_RANDR_12->max_size.height);

   /* create scrolling widget */
   info->o_scroll = e_smart_randr_add(evas);
   e_smart_randr_virtual_size_set(info->o_scroll, 
                                  E_RANDR_12->max_size.width,
                                  E_RANDR_12->max_size.height);
   edje_object_part_swallow(info->o_bg, "e.swallow.content", info->o_scroll);

   /* create monitors based on 'CRTCS' */
   /* int i = 0; */
   EINA_LIST_FOREACH(E_RANDR_12->crtcs, l, crtc)
     {
        Evas_Object *m;

        /* if (i > 0) break; */
        if (!crtc) continue;

        printf("ADD CRTC %d\n", crtc->xid);

        if (!(m = e_smart_monitor_add(evas))) continue;
        e_smart_monitor_crtc_set(m, crtc);
        e_smart_randr_monitor_add(info->o_scroll, m);
        evas_object_show(m);
        /* i++; */
     }

   /* create close button */
   ob = e_widget_button_add(evas, _("Close"), NULL, 
                            _close_cb_click, info->win, NULL);
//   e_widget_on_focus_hook_set(ob, _close_cb_focus, win);
   e_widget_size_min_get(ob, &mw, &mh);
   edje_extern_object_min_size_set(ob, mw, mh);
   edje_object_part_swallow(info->o_bg, "e.swallow.button", ob);

   /* set window minimum size */
   edje_object_size_min_calc(info->o_bg, &mw, &mh);
   e_win_size_min_set(info->win, mw, mh);
   e_util_win_auto_resize_fill(info->win);

   e_win_centered_set(info->win, EINA_TRUE);
   e_win_show(info->win);
   e_win_border_icon_set(info->win, "preferences-system-screen-resolution");

   return info;
}

static void 
_randr_info_free(void)
{
   if (!_randr_info) return;

   /* delete scroller */
   if (_randr_info->o_scroll) evas_object_del(_randr_info->o_scroll);
   _randr_info->o_scroll = NULL;

   /* delete background */
   if (_randr_info->o_bg) evas_object_del(_randr_info->o_bg);
   _randr_info->o_bg = NULL;

   /* delete window */
   if (_randr_info->win) e_object_del(E_OBJECT(_randr_info->win));
   _randr_info->win = NULL;

   /* free structure */
   E_FREE(_randr_info);
}

static void 
_win_cb_resize(E_Win *win)
{
   Randr_Info *info = NULL;

   if (!(info = win->data)) return;
   evas_object_resize(info->o_bg, win->w, win->h);
}

static void 
_win_cb_delete(E_Win *win __UNUSED__)
{
   _randr_info_free();
}

static void 
_close_cb_click(void *data __UNUSED__, void *data2 __UNUSED__)
{
   _randr_info_free();
}
