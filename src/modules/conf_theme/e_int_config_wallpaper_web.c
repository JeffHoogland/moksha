#include "e.h"
#include "e_int_config_wallpaper.h"
#include "e_int_config_wallpaper_web.h"

#ifdef HAVE_EXCHANGE
#include <Exchange.h>

typedef struct _Web Web;
struct _Web
{
   E_Config_Dialog *parent;
   E_Dialog *dia;
   Evas_Object *list;
   Evas_Object *textblock;
   Evas_Object *image;
   Eina_List *jobs;
   Eina_List *objs;
};

enum {
   BTN_DOWNLOAD,
   BTN_APPLY,
   BTN_CLOSE
};

void
e_int_config_wallpaper_web_del(E_Dialog *dia)
{
   Web *web;
   Ecore_File_Download_Job *job;
   Exchange_Object *wp;
   Eina_List *l;

   web = dia->data;
   e_int_config_wallpaper_web_done(web->parent);

   EINA_LIST_FOREACH(web->jobs, l, job)
      ecore_file_download_abort(job);

   EINA_LIST_FREE(web->objs, wp)
      exchange_object_free(wp);

   evas_object_del(web->list);
   evas_object_del(web->textblock);
   evas_object_del(web->image);

   E_FREE(web);
   e_object_unref(E_OBJECT(dia));
   exchange_shutdown();
}

static void
_web_del_cb(void *data)
{
   E_Dialog *dia;

   dia = data;
   e_int_config_wallpaper_web_del(dia);
}

static void
_web_download_complete_cb(Exchange_Object *obj __UNUSED__, const char *file __UNUSED__, void *data)
{
   Exchange_Object *wp = data;
   Web *web = exchange_obj_data_get(wp);

   if (wp == e_widget_ilist_selected_data_get(web->list))
     {
	e_dialog_button_disable_num_set(web->dia, BTN_APPLY, 0);
	e_dialog_button_disable_num_set(web->dia, BTN_DOWNLOAD, 1);
     }
}

static void
_web_download_btn_cb(void *data, E_Dialog *dia __UNUSED__)
{
   Web *web = data;
   Exchange_Object *wp;
   char buf[PATH_MAX];

   wp = e_widget_ilist_selected_data_get(web->list);
   if (!wp) return;

   e_user_dir_concat(buf, sizeof(buf), "backgrounds");
   exchange_obj_download(wp, buf, _web_download_complete_cb, wp);

   e_dialog_button_disable_num_set(web->dia, BTN_DOWNLOAD, 1);
}

static void
_web_apply_btn_cb(void *data, E_Dialog *dia __UNUSED__)
{
   Web *web = data;
   Exchange_Object *wp;
   char buf[PATH_MAX];

   wp = e_widget_ilist_selected_data_get(web->list);
   if (!wp) return;

   e_user_dir_snprintf(buf, sizeof(buf), "backgrounds/%s",
			    exchange_obj_file_name_get(wp));

   if (!ecore_file_exists(buf)) return;

   e_bg_default_set(buf);
   e_bg_update();
   e_config_save_queue();
}

static void
_web_close_btn_cb(void *data __UNUSED__, E_Dialog *dia)
{
   e_int_config_wallpaper_web_del(dia);
}

static void
_screenshot_get_cb(Exchange_Object *obj, const char *file, void *data)
{
   Web *web = data;

   if (obj == e_widget_ilist_selected_data_get(web->list))
      e_widget_image_file_set(web->image, file);
}

static void
_list_selection_changed(void *data, Evas_Object *obj)
{
   char buf[PATH_MAX];
   Web *web = data;
   Exchange_Object *wp;
   Eina_Strbuf *sbuf;

   wp = e_widget_ilist_selected_data_get(obj);
   exchange_obj_screenshot_get(wp, _screenshot_get_cb, web);
   // TODO inform the user that the download has started (spinner somewhere)

   // enable download button ?
   e_user_dir_concat(buf, sizeof(buf), "backgrounds");
   if (exchange_obj_update_available(wp, buf))
      e_dialog_button_disable_num_set(web->dia, BTN_DOWNLOAD, 0);
   else
      e_dialog_button_disable_num_set(web->dia, BTN_DOWNLOAD, 1);

   // enable apply button ?
   e_user_dir_snprintf(buf, sizeof(buf), "backgrounds/%s",
                       exchange_obj_file_name_get(wp));
   if (ecore_file_exists(buf))
      e_dialog_button_disable_num_set(web->dia, BTN_APPLY, 0);
   else
      e_dialog_button_disable_num_set(web->dia, BTN_APPLY, 1);

   // update textblock
   sbuf = eina_strbuf_new();
   e_widget_textblock_plain_set(web->textblock, "");
   
   if (exchange_obj_name_get(wp))
      eina_strbuf_append_printf(sbuf,"<b>%s</b>", exchange_obj_name_get(wp));
   if (exchange_obj_version_get(wp))
      eina_strbuf_append_printf(sbuf, " v %s", exchange_obj_version_get(wp));
   eina_strbuf_append(sbuf, "<br>");
   if (exchange_obj_author_get(wp))
      eina_strbuf_append_printf(sbuf, "%s %s<br>", _("By"),
                                exchange_obj_author_get(wp));
   if (exchange_obj_description_get(wp))
     {
	printf("\n%s\n",exchange_obj_description_get(wp)); // TODO Fix '&' markup error
	eina_strbuf_append_printf(sbuf, "<br>%s<br>", exchange_obj_description_get(wp));
     }
   e_widget_textblock_markup_set(web->textblock, eina_strbuf_string_get(sbuf));
   eina_strbuf_free(sbuf);
}

static void
_thumbnail_download_cb(Exchange_Object *obj, const char *file, void *data)
{
   Web *web = data;
   Evas_Object *ic;

   ic = e_widget_image_add_from_file(evas_object_evas_get(web->list),
                                     file, 0, 0);
   e_widget_ilist_append(web->list, ic, exchange_obj_name_get(obj),
                         NULL, obj, NULL);
}

static void
_exchange_query_cb(Eina_List *results, void *data)
{
   Web *web = data;
   Exchange_Object *wp;
   Eina_List *l;

   eina_list_free(web->jobs);
   web->jobs = NULL;

   e_widget_ilist_clear(web->list);
   if (!results)
     {
	e_widget_ilist_append(web->list, NULL, _("Error getting data !"),
					 NULL, NULL, NULL);
	return;
     }
   EINA_LIST_FOREACH(results, l, wp)
     {
	exchange_obj_thumbnail_get(wp, _thumbnail_download_cb, web);
	exchange_obj_data_set(wp, web);
     }

   web->objs = results;
}

E_Dialog *
e_int_config_wallpaper_web(E_Config_Dialog *parent)
{
   E_Dialog *dia;
   Web *web;
   Evas_Object *o, *ot, *ot2;
   Evas *evas;
   Ecore_File_Download_Job *job;

   if (!exchange_init())
      return NULL;

   // e_dialog
   dia = e_dialog_new(parent->con, "E", "_wallpaper_web_dialog");
   if (!dia) return NULL;

   web = E_NEW(Web, 1);
   if (!web) return NULL;

   web->dia = dia;
   web->parent = parent;

   e_dialog_title_set(dia, _("Exchange wallpapers"));
   e_dialog_resizable_set(dia, 1);
   e_dialog_button_add(dia, _("Download"), NULL, _web_download_btn_cb, web);
   e_dialog_button_add(dia, _("Apply"), NULL, _web_apply_btn_cb, web);
   e_dialog_button_add(dia, _("Close"), NULL, _web_close_btn_cb, web);
   e_dialog_button_disable_num_set(dia, BTN_DOWNLOAD, 1);
   e_dialog_button_disable_num_set(dia, BTN_APPLY, 1);

   dia->data = web;
   e_object_del_attach_func_set(E_OBJECT(dia), _web_del_cb);
   e_win_centered_set(dia->win, 1);
   evas = e_win_evas_get(dia->win);

   // main table
   ot = e_widget_table_add(evas, 0);

   // themes list
   o = e_widget_ilist_add(evas, 50, 50, NULL);
   e_widget_size_min_set(o, 200, 200);
   e_widget_ilist_multi_select_set(o, 0);
   e_widget_on_change_hook_set(o, _list_selection_changed, web);
   e_widget_ilist_append(o, NULL, _("Getting data, please wait..."),
                                    NULL, NULL, NULL);
   e_widget_table_object_append(ot, o, 0, 0, 1, 1, 0, 1, 0, 1);
   web->list = o;

   // second table
   ot2 = e_widget_table_add(evas, 0);
   e_widget_table_object_append(ot, ot2, 1, 0, 1, 1, 1, 1, 1, 1);

   // textblock
   o = e_widget_textblock_add(evas);
   e_widget_size_min_set(o, 100, 100);
   e_widget_textblock_plain_set(o, _("Select a background from the list."));
   e_widget_table_object_append(ot2, o, 0, 0, 1, 1, 1, 1, 1, 0);
   web->textblock = o;
   
   // preview image
   o = e_widget_image_add_from_file(evas, NULL, 100, 100);
   e_widget_table_object_append(ot2, o, 0, 1, 1, 1, 1, 1, 1, 1);
   web->image = o;

   // request list from exchange
   job = exchange_query(NULL, "e/desktop/background", 0, 0, 0, NULL, 0, 0,
			_exchange_query_cb, web);
   if (!job)
     {
	e_widget_ilist_clear(web->list);
	e_widget_ilist_append(web->list, NULL, _("Error: can't start the request."),
			      NULL, NULL, NULL);
	e_widget_textblock_plain_set(web->textblock, "");
     }
   else web->jobs = eina_list_append(web->jobs, job);

   // show the dialog
   e_dialog_content_set(dia, ot, 300, 220);
   e_dialog_show(dia);
   e_dialog_border_icon_set(dia, "network-website");

   return dia;
}
#endif
