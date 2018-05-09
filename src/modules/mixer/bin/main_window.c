#include "main_window.h"

#include "../lib/epulse.h"
#include "playbacks_view.h"
#include "sinks_view.h"
#include "sources_view.h"

#define MAIN_WINDOW_DATA "mainwindow.data"

enum MAIN_SUBVIEWS {
   PLAYBACKS,
   OUTPUTS,
   INPUTS
};

typedef struct _Main_Window Main_Window;
struct _Main_Window
{
   Evas_Object *win;
   Evas_Object *toolbar;
   Evas_Object *layout;
   Evas_Object *naviframe;
   Evas_Object *playbacks;
   Evas_Object *inputs;
   Evas_Object *outputs;
   Elm_Object_Item *toolbar_items[3];
   Elm_Object_Item *views[3];
};

static void
_del_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *o EINA_UNUSED,
        void *event_info EINA_UNUSED)
{
   Main_Window *mw = data;

   free(mw);
}

static void
_delete_request_cb(void *data EINA_UNUSED, Evas_Object *o EINA_UNUSED,
                   void *event_info EINA_UNUSED)
{
   elm_exit();
}

static void
_toolbar_item_cb(void *data, Evas_Object *obj EINA_UNUSED,
                 void *event_info EINA_UNUSED)
{
   Main_Window *mw = data;
   Elm_Object_Item *it = elm_toolbar_selected_item_get(obj);

   if (!mw->views[PLAYBACKS] || !mw->views[OUTPUTS] ||
       !mw->views[INPUTS])
      return;

   if (it == mw->toolbar_items[PLAYBACKS])
      elm_naviframe_item_promote(mw->views[PLAYBACKS]);
   else if (it == mw->toolbar_items[OUTPUTS])
      elm_naviframe_item_promote(mw->views[OUTPUTS]);
   else
      elm_naviframe_item_promote(mw->views[INPUTS]);
}

Evas_Object *
main_window_add(void)
{
   Main_Window *mw;
   Evas_Object *tmp, *box, *icon;
   char buf[4096];

   elm_theme_extension_add(NULL, EPULSE_THEME);
   mw = calloc(1, sizeof(Main_Window));
   if (!mw)
     {
        ERR("Could not allocate memmory to main window");
        return NULL;
     }

   tmp = elm_win_add(NULL, PACKAGE_NAME, ELM_WIN_BASIC);
   EINA_SAFETY_ON_NULL_GOTO(tmp, win_err);
   evas_object_data_set(tmp, MAIN_WINDOW_DATA, mw);
   evas_object_event_callback_add(tmp, EVAS_CALLBACK_DEL, _del_cb, mw);
   elm_win_autodel_set(tmp, EINA_TRUE);
   elm_win_title_set(tmp, _("Efl Volume Control"));
   mw->win = tmp;

   icon = evas_object_image_add(evas_object_evas_get(mw->win));
   snprintf(buf, sizeof(buf), "%s/icons/terminology.png",
            elm_app_data_dir_get());
   evas_object_image_file_set(icon, buf, NULL);
   elm_win_icon_object_set(mw->win, icon);
   elm_win_icon_name_set(mw->win, "epulse");

   tmp = elm_bg_add(mw->win);
   evas_object_size_hint_weight_set(tmp, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(tmp, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_win_resize_object_add(mw->win, tmp);
   evas_object_show(tmp);

   box = elm_box_add(mw->win);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_horizontal_set(box, EINA_FALSE);
   elm_win_resize_object_add(mw->win, box);
   evas_object_show(box);

   tmp = elm_toolbar_add(mw->win);
   evas_object_size_hint_weight_set(tmp, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(tmp, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_toolbar_select_mode_set(tmp, ELM_OBJECT_SELECT_MODE_ALWAYS);
   elm_box_pack_start(box, tmp);
   mw->toolbar = tmp;
   evas_object_show(tmp);

   tmp = elm_naviframe_add(mw->win);
   elm_object_style_set(tmp, "no_transition");
   elm_naviframe_prev_btn_auto_pushed_set(tmp, EINA_FALSE);
   evas_object_size_hint_weight_set(tmp, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(tmp, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(box, tmp);
   evas_object_show(tmp);
   mw->naviframe = tmp;

   mw->toolbar_items[0] = elm_toolbar_item_append(mw->toolbar, NULL,
                                                  _("Playback"),
                                                  _toolbar_item_cb, mw);
   mw->toolbar_items[1] = elm_toolbar_item_append(mw->toolbar, NULL,
                                                  _("Outputs"),
                                                  _toolbar_item_cb, mw);
   mw->toolbar_items[2] = elm_toolbar_item_append(mw->toolbar, NULL,
                                                  _("Inputs"),
                                                  _toolbar_item_cb, mw);

   evas_object_smart_callback_add(mw->win, "delete,request",
                                  _delete_request_cb, mw);

   /* Creating the playbacks view */
   mw->playbacks = playbacks_view_add(mw->win);
   evas_object_size_hint_weight_set(mw->playbacks, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(mw->playbacks, EVAS_HINT_FILL,
                                   EVAS_HINT_FILL);
   evas_object_show(mw->playbacks);

   /* Creating the outputs view */
   mw->outputs = sinks_view_add(mw->win);
   evas_object_size_hint_weight_set(mw->outputs, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(mw->outputs, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(mw->outputs);

   /* Creating the inputs view */
   mw->inputs = sources_view_add(mw->win);
   evas_object_size_hint_weight_set(mw->inputs, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(mw->inputs, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(mw->inputs);

   mw->views[INPUTS] = elm_naviframe_item_simple_push(mw->naviframe,
                                                      mw->inputs);
   mw->views[OUTPUTS] = elm_naviframe_item_simple_push(mw->naviframe,
                                                       mw->outputs);
   mw->views[PLAYBACKS] = elm_naviframe_item_simple_push(mw->naviframe,
                                                         mw->playbacks);

   return mw->win;

 win_err:
   free(mw);
   return NULL;
}
