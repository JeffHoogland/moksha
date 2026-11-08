#include "e_mod_main.h"
#include "x_clipboard.h"

/* The history window: an ilist (with thumbnails of images) and a column of
 * buttons next to it. Double click or Enter on an item puts it in the
 * clipboard and closes the window. */

/* e_icon_object_set() is deprecated but it is the way to hand an e_icon an
 * image object we prepared ourselves (loaded scaled down, see _item_icon) */
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#define LIST_W 380
#define LIST_H 360

typedef struct _List_UI
{
   E_Dialog    *dia;
   Evas_Object *ilist;
   Evas_Object *b_use, *b_add, *b_edit, *b_lock, *b_del;
   Evas_Object *ck_ignore;
   int          ignore;     /* value of ck_ignore */
   unsigned int sel_id;     /* id of the selected item, 0 if none */
   Eina_Bool    closing;    /* deletion of the window is pending */
} List_UI;

static List_UI ui;

static void _list_fill(unsigned int keep_id);

static Clip_Data *
_sel_get(void)
{
   return ui.sel_id ? clip_find(ui.sel_id) : NULL;
}

/* Enable/disable the buttons according to the selected item */
static void
_sel_update(void)
{
   Clip_Data *cd;

   if (!ui.dia) return;
   cd = _sel_get();
   if (!cd) ui.sel_id = 0;

   e_widget_disabled_set(ui.b_use,  !cd);
   e_widget_disabled_set(ui.b_edit, !cd || (cd->type != CLIP_TYPE_TEXT));
   e_widget_disabled_set(ui.b_lock, !cd);
   e_widget_disabled_set(ui.b_del,  !cd);
   e_widget_button_label_set(ui.b_lock,
                             (cd && !strcmp(cd->lock, "L")) ? _("Unlock") : _("Lock"));
}

/* Thumbnail for an image item, NULL for text (those rows stay compact).
 * The image is loaded scaled down: a screenshot would otherwise be decoded
 * in full just to be drawn 64 pixels wide. */
static Evas_Object *
_item_icon(Evas *evas, const Clip_Data *cd)
{
   Evas_Object *img, *ic;
   char path[PATH_MAX];
   unsigned int small_side;
   int factor;

   if (cd->type != CLIP_TYPE_IMAGE) return NULL;
   if (!clip_image_path_get(cd->content, path, sizeof(path))) return NULL;

   img = evas_object_image_add(evas);
   small_side = (cd->w < cd->h) ? cd->w : cd->h;
   factor = (int) (small_side / (2 * IMG_THUMB_SIZE));
   if (factor > 1)
     evas_object_image_load_scale_down_set(img, factor);
   evas_object_image_file_set(img, path, NULL);
   if (evas_object_image_load_error_get(img) != EVAS_LOAD_ERROR_NONE)
     {
        evas_object_del(img);
        return NULL;
     }
   ic = e_icon_add(evas);
   e_icon_fill_inside_set(ic, EINA_TRUE);
   e_icon_object_set(ic, img);
   return ic;
}

static void
_list_fill(unsigned int keep_id)
{
   Evas *evas;
   Clip_Data *cd;
   Evas_Object *icon, *end;
   unsigned int count, i, sel = 0;
   Eina_Bool found = EINA_FALSE;

   if (!ui.dia) return;
   evas = e_win_evas_get(ui.dia->win);

   clip_labels_refresh();
   count = eina_list_count(clip_inst->items);
   /* items are queued in the widget and added a moment later */
   e_widget_ilist_clear(ui.ilist);

   if (!count)
     {
        e_widget_ilist_header_append(ui.ilist, NULL, _("Empty"));
        ui.sel_id = 0;
        _sel_update();
        return;
     }

   for (i = 0; i < count; i++)
     {
        /* the newest item is first in the list */
        cd = eina_list_nth(clip_inst->items,
                           clip_cfg->hist_reverse ? (count - 1 - i) : i);
        icon = _item_icon(evas, cd);
        end = NULL;
        if (!strcmp(cd->lock, "L"))
          {
             end = e_icon_add(evas);
             e_util_icon_theme_set(end, "emblem-readonly");
             evas_object_size_hint_min_set(end, 24 * e_scale, 24 * e_scale);
          }
        /* the item data is the id, never a pointer */
        e_widget_ilist_append_full(ui.ilist, icon, end, cd->name, NULL,
                                   (void *) (uintptr_t) cd->id, NULL);
        if (cd->id == keep_id)
          {
             sel = i;
             found = EINA_TRUE;
          }
        if (!i && !found)
          ui.sel_id = cd->id;
     }
   if (found) ui.sel_id = keep_id;
   e_widget_ilist_selected_set(ui.ilist, sel);
   _sel_update();
}

void
clip_list_refresh(void)
{
   if (!ui.dia || ui.closing) return;
   /* keep the selection if the item is still there */
   _list_fill(_sel_get() ? ui.sel_id : 0);
}

/* ---- callbacks ---- */

static void
_cb_ilist_change(void *data __UNUSED__, Evas_Object *obj)
{
   ui.sel_id = (unsigned int) (uintptr_t) e_widget_ilist_selected_data_get(obj);
   _sel_update();
}

static void
_use_selected(void)
{
   Clip_Data *cd = _sel_get();

   if (!cd) return;
   clip_use_item(cd);
   clip_list_close(EINA_FALSE);
}

/* double click or Enter */
static void
_cb_ilist_activate(void *data __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   _use_selected();
}

static void
_cb_btn_use(void *data __UNUSED__, void *data2 __UNUSED__)
{
   _use_selected();
}

static void
_cb_add_ok(void *data __UNUSED__, char *text)
{
   clip_add_text(text);
}

static void
_cb_btn_add(void *data __UNUSED__, void *data2 __UNUSED__)
{
   e_entry_dialog_show(_("Clipboard: Add Item"), "list-add",
                       _("Enter the content of the new item:"), "",
                       NULL, NULL, _cb_add_ok, NULL, NULL);
}

static void
_cb_edit_ok(void *data, char *text)
{
   clip_edit_text((unsigned int) (uintptr_t) data, text);
}

static void
_cb_btn_edit(void *data __UNUSED__, void *data2 __UNUSED__)
{
   Clip_Data *cd = _sel_get();

   if (!cd || (cd->type != CLIP_TYPE_TEXT)) return;
   e_entry_dialog_show(_("Clipboard: Edit Item"), "edit-rename",
                       _("Edit the item content:"), cd->content,
                       NULL, NULL, _cb_edit_ok, NULL,
                       (void *) (uintptr_t) cd->id);
}

static void
_cb_btn_lock(void *data __UNUSED__, void *data2 __UNUSED__)
{
   Clip_Data *cd = _sel_get();

   if (cd) clip_toggle_lock(cd);
}

static void
_cb_btn_del(void *data __UNUSED__, void *data2 __UNUSED__)
{
   Clip_Data *cd = _sel_get();

   if (cd) clip_delete_request(cd);
}

static void
_cb_dia_clear(void *data __UNUSED__, E_Dialog *dia __UNUSED__)
{
   clip_clear_request();
}

static void
_cb_dia_settings(void *data __UNUSED__, E_Dialog *dia __UNUSED__)
{
   clip_config_show();
}

static void
_cb_ignore_change(void *data __UNUSED__, Evas_Object *obj __UNUSED__)
{
   clip_cfg->ignore_hist = ui.ignore ? 1 : 0;
   e_config_save_queue();
}

/* the window is going away: forget all the widgets in it */
static void
_cb_dia_del(void *obj __UNUSED__)
{
   memset(&ui, 0, sizeof(ui));
}

static Evas_Object *
_btn_add(Evas *evas, Evas_Object *box, const char *label, const char *icon,
         void (*func)(void *data, void *data2))
{
   Evas_Object *ob = e_widget_button_add(evas, label, icon, func, NULL, NULL);

   e_widget_list_object_append(box, ob, 1, 0, 0.5);
   return ob;
}

/* ---- public ---- */

void
clip_list_show(void)
{
   E_Container *con;
   Evas *evas;
   Evas_Object *tb, *bl;
   int mw, mh;

   EINA_SAFETY_ON_NULL_RETURN(clip_cfg);
   EINA_SAFETY_ON_NULL_RETURN(clip_inst);

   if (ui.dia)
     {
        if (!ui.closing) e_win_raise(ui.dia->win);
        return;
     }

   con = e_container_current_get(e_manager_current_get());
   ui.dia = e_dialog_new(con, "E", "_clipboard_history");
   if (!ui.dia) return;
   e_object_del_attach_func_set(E_OBJECT(ui.dia), _cb_dia_del);
   e_dialog_title_set(ui.dia, _("Clipboard"));
   e_dialog_border_icon_set(ui.dia, "edit-paste");
   evas = e_win_evas_get(ui.dia->win);

   /* ilist on the left, buttons on the right */
   tb = e_widget_table_add(evas, 0);

   ui.ilist = e_widget_ilist_add(evas, IMG_THUMB_SIZE, IMG_THUMB_SIZE, NULL);
   e_widget_size_min_set(ui.ilist, LIST_W * e_scale, LIST_H * e_scale);
   e_widget_on_change_hook_set(ui.ilist, _cb_ilist_change, NULL);
   evas_object_smart_callback_add(ui.ilist, "selected", _cb_ilist_activate, NULL);
   /* the widget needs a go() before it reports selection changes */
   e_widget_ilist_go(ui.ilist);
   e_widget_table_object_append(tb, ui.ilist, 0, 0, 1, 1, 1, 1, 1, 1);

   bl = e_widget_list_add(evas, 0, 0);
   ui.b_use  = _btn_add(evas, bl, _("Use"),    "edit-paste",    _cb_btn_use);
   ui.b_add  = _btn_add(evas, bl, _("Add"),    "list-add",      _cb_btn_add);
   ui.b_edit = _btn_add(evas, bl, _("Edit"),   "edit-rename",   _cb_btn_edit);
   ui.b_lock = _btn_add(evas, bl, _("Lock"),   "emblem-readonly", _cb_btn_lock);
   ui.b_del  = _btn_add(evas, bl, _("Delete"), "edit-delete",   _cb_btn_del);

   ui.ignore = clip_cfg->ignore_hist ? 1 : 0;
   ui.ck_ignore = e_widget_check_add(evas, _("Ignore history"), &ui.ignore);
   e_widget_on_change_hook_set(ui.ck_ignore, _cb_ignore_change, NULL);
   e_widget_list_object_append(bl, ui.ck_ignore, 1, 0, 0.5);

   e_widget_table_object_align_append(tb, bl, 1, 0, 1, 1, 1, 0, 0, 0, 0.5, 0.0);

   e_widget_size_min_get(tb, &mw, &mh);
   e_dialog_content_set(ui.dia, tb, mw, mh);

   e_dialog_button_add(ui.dia, _("Clear"),    "edit-clear",         _cb_dia_clear,    NULL);
   e_dialog_button_add(ui.dia, _("Settings"), "preferences-system", _cb_dia_settings, NULL);
   e_dialog_button_add(ui.dia, _("Close"),    "window-close",       NULL,             NULL);
   e_dialog_resizable_set(ui.dia, 1);

   _list_fill(0);

   e_win_centered_set(ui.dia->win, 1);
   e_dialog_show(ui.dia);
   e_widget_focus_set(ui.ilist, 1);
}

void
clip_list_close(Eina_Bool now)
{
   if (!ui.dia) return;
   if (now)
     {
        /* _cb_dia_del() clears ui */
        e_object_del(E_OBJECT(ui.dia));
        return;
     }
   if (ui.closing) return;
   /* We are usually called from a callback of the window itself */
   ui.closing = EINA_TRUE;
   e_util_defer_object_del(E_OBJECT(ui.dia));
}

void
clip_list_toggle(void)
{
   if (ui.dia && !ui.closing)
     clip_list_close(EINA_FALSE);
   else
     clip_list_show();
}
