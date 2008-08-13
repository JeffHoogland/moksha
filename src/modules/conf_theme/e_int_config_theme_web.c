/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

#include "e.h"
#include "e_int_config_theme.h"
#include "e_int_config_theme_web.h"

#define	MAGIC_WEB 0x425581cb
#define TEMPLATE "/tmp/themeXXXXXX"

typedef struct _Web Web;

struct _Web
{
   int magic;

   E_Config_Dialog *parent;
   E_Config_Dialog_Data *cfdata;
   E_Dialog *dia;
};

struct _E_Config_Dialog_Data
{
   Evas_Object *ofm, *o, *osfm, *ol;
   Ecore_List *thumbs, *names, *medias;
   Ecore_Con_Url *ecu;
   Ecore_Event_Handler *hdata, *hcomplete;
   FILE *feed;
   int ready_for_edj, pending_downloads, busy;
   char *edj, *ol_val, *tmpdir;
   const char *source;
};

static void _web_ok_cb(void *data, E_Dialog *dia);
static void _web_close_cb(void *data, E_Dialog *dia);
static void _web_del_cb(void *obj);
static void _file_double_click_cb(void *data, Evas_Object *obj, void *ev_info);
static void _file_click_cb(void *data, Evas_Object *obj, void *ev_info);
static int _list_find(const char *str1, const char *str2);
static void _source_sel_cb(void *data);
static void _reset(void *data);
static void _get_feed(char *url, void *data);
static int  _feed_complete(void *data, int type, void *event);
static int  _feed_data(void *data, int type, void *event);
static void _parse_feed(void *data);
static void _get_thumb_complete(void *data, const char *file, int status);
static int  _download_media_progress_cb(void *data, const char *file, long int dltotal, long int dlnow, long int ultotal, long int ulnow);
static void _download_media_complete_cb(void *data, const char *file, int status);

EAPI E_Dialog *
e_int_config_theme_web(E_Config_Dialog *parent)
{
   Evas *evas;
   E_Dialog *dia;
   Web *web;
   E_Config_Dialog_Data *cfdata;
   Evas_Object *o, *ol, *of, *ofm, *osfm;
   Evas_Coord mw, mh;
   E_Fm2_Config fmc;
   
   web = E_NEW(Web, 1);
   if (!web) return NULL;

   web->magic = MAGIC_WEB;

   dia = e_dialog_new(parent->con, "E", "_theme_web_dialog");
   if (!dia)
     {
	E_FREE(web);
	return NULL;
     }

   dia->data = web;
   e_object_del_attach_func_set(E_OBJECT(dia), _web_del_cb);
   e_win_centered_set(dia->win, 1);

   evas = e_win_evas_get(dia->win);
     
   cfdata = E_NEW(E_Config_Dialog_Data, 1);

   cfdata->ecu = ecore_con_url_new("http://fake.url");

   cfdata->ready_for_edj = 0;
   cfdata->pending_downloads = 0;
   cfdata->busy = 0;
   web->cfdata = cfdata;
   web->dia = dia;

   web->parent = parent;

   e_dialog_title_set(dia, _("Choose a website from list..."));

   o = e_widget_list_add(evas, 0, 1);
   cfdata->o = o;
   cfdata->thumbs = ecore_list_new();
   ecore_list_free_cb_set(cfdata->thumbs, free);
   cfdata->names = ecore_list_new();
   ecore_list_free_cb_set(cfdata->names, free);
   cfdata->medias = ecore_list_new();
   ecore_list_free_cb_set(cfdata->medias, free);

   of = e_widget_framelist_add(evas, "Sources", 1);
   ol = e_widget_ilist_add(evas, 24, 24, &cfdata->ol_val);
   cfdata->ol = ol;
   e_widget_ilist_append(ol, NULL, _("get-e.org"),
                         _source_sel_cb, web,
                         "http://get-e.org/Themes/E17/feed.xml");
			 
   e_widget_ilist_go(ol);
   e_widget_min_size_get(ol, &mw, NULL);
   e_widget_min_size_set(ol, mw, 320);
   e_widget_framelist_object_append(of, ol);
   e_widget_list_object_append(o, of, 1, 0, 0.5);

   ofm = e_fm2_add(evas);
   memset(&fmc, 0, sizeof(E_Fm2_Config));
   cfdata->ofm = ofm;
   fmc.view.mode = E_FM2_VIEW_MODE_GRID_ICONS;
   fmc.view.open_dirs_in_place = 1;
   fmc.view.selector = 1;
   fmc.view.single_click = 0;
   fmc.view.no_subdir_jump = 0;
   fmc.icon.list.w = 48;
   fmc.icon.list.h = 48;
   fmc.icon.icon.w = 128;
   fmc.icon.icon.h = 128;
   fmc.icon.fixed.w = 0;
   fmc.icon.fixed.h = 0;
   fmc.icon.extension.show = 0;
   fmc.icon.key_hint = NULL;
   fmc.list.sort.no_case = 1;
   fmc.list.sort.dirs.first = 0;
   fmc.list.sort.dirs.last = 1;
   fmc.selection.single = 1;
   fmc.selection.windows_modifiers = 0;
   e_fm2_config_set(ofm, &fmc);
   e_fm2_icon_menu_flags_set(ofm, E_FM2_MENU_NO_SHOW_HIDDEN);

   evas_object_smart_callback_add(ofm, "selected",
                                  _file_double_click_cb, web);
   evas_object_smart_callback_add(ofm, "selection_change",
                                  _file_click_cb, web);

   osfm = e_widget_scrollframe_pan_add(evas, ofm,
                                       e_fm2_pan_set,
                                       e_fm2_pan_get,
                                       e_fm2_pan_max_get,
                                       e_fm2_pan_child_size_get);
   cfdata->osfm = osfm;
   e_widget_list_object_append(cfdata->o, cfdata->osfm, 1, 1, 0.5);
   e_widget_min_size_set(osfm, 320, 320);

//   e_widget_min_size_set(o, 580, 370);
   e_widget_min_size_get(o, NULL, &mh);
   e_dialog_content_set(dia, o, 480, mh);

   e_dialog_button_add(dia, _("OK"), NULL, _web_ok_cb, web);
   e_dialog_button_add(dia, _("Cancel"), NULL, _web_close_cb, web);
   e_dialog_button_disable_num_set(dia, 0, 1);

   e_dialog_resizable_set(dia, 1);
   e_dialog_show(dia);

   e_dialog_border_icon_set(dia, "enlightenment/theme");

   return dia;
}

void
e_int_config_theme_web_del(E_Dialog *dia)
{
   Web *web;
   E_Config_Dialog_Data *cfdata;

   web = dia->data;
   cfdata = web->cfdata;

   if (cfdata->pending_downloads == 1)
     ecore_file_download_abort_all();
   if (cfdata->ecu)
     ecore_con_url_destroy(cfdata->ecu);

   if (cfdata->hdata)
     ecore_event_handler_del(cfdata->hdata);
   if (cfdata->hcomplete)
     ecore_event_handler_del(cfdata->hcomplete);

   if (cfdata->tmpdir)
     {
	if (ecore_file_is_dir(cfdata->tmpdir))
	  {
	     ecore_file_recursive_rm(cfdata->tmpdir);
	     ecore_file_rmdir(cfdata->tmpdir);
	  }
	free(cfdata->tmpdir);
     }
   ecore_list_destroy(cfdata->thumbs);
   ecore_list_destroy(cfdata->names);
   ecore_list_destroy(cfdata->medias);

   e_int_config_theme_web_done(web->parent);
   E_FREE(web->cfdata);
   E_FREE(web);
   e_object_unref(E_OBJECT(dia));
}

static int
_download_media_progress_cb(void *data, const char *file, long int dltotal,
			    long int dlnow, long int ultotal, long int ulnow)
{
   Web *web;
   double status;
   char title[4096];

   web = data;

   if ((dlnow == 0) || (dltotal == 0)) return 0;

   if (dlnow != dltotal)
     {
	status = (double) ((double) dlnow) / ((double) dltotal);
        snprintf(title, sizeof(title), _("[%s] Downloading of edje file... %d%% done"),
                 web->cfdata->source, (int) (status * 100.0));
	e_dialog_title_set(web->dia, title);
     }

   return 0;
}

static void
_download_media_complete_cb(void *data, const char *file, int status)
{
   Web *web;
   char dest[4096];

   web = data;
   web->cfdata->pending_downloads = 0;
   snprintf(dest, sizeof(dest), "%s/.e/e/themes/%s",
            e_user_homedir_get(), ecore_file_file_get(web->cfdata->edj));
   e_int_config_theme_update(web->parent, dest);
   e_int_config_theme_web_del(web->dia);
}

static void
_download_media(Web *web)
{
   Web *i;
   E_Config_Dialog_Data *cfdata;
   const char *file;
   char buf[4096], title[4096];

   i = web;
   cfdata = i->cfdata;

   cfdata->pending_downloads = 1;
   file = ecore_file_file_get(cfdata->edj);
   snprintf(buf, sizeof(buf), "%s/.e/e/themes/%s",
            e_user_homedir_get(), file);
   snprintf(title, sizeof(title), _("[%s] Downloading of edje file..."),
            cfdata->source);
   e_dialog_title_set(i->dia, title);
   ecore_file_download(cfdata->edj, buf,
                       _download_media_complete_cb,
                       _download_media_progress_cb, i);
}

static void
_get_thumb_complete(void *data, const char *file, int status)
{
   Web *web;
   E_Config_Dialog_Data *cfdata;
   char title[4096], dst[4096];
   static int got = 1;

   web = data;
   cfdata = web->cfdata;
   if (got != ecore_list_count(cfdata->thumbs))
     {
        snprintf(title, sizeof(title), _("[%s] Download %d images of %d"),
                 cfdata->source, got, ecore_list_index(cfdata->thumbs));
        e_dialog_title_set(web->dia, title);
        cfdata->ready_for_edj = 0;
        snprintf(dst, sizeof(dst), "%s/%s", cfdata->tmpdir,
                 ecore_file_file_get(file));
        ecore_file_mv(file, dst);
        got++;
     }
   else
     {
        got = 1;
        cfdata->busy = 0;
        cfdata->ready_for_edj = 1;
        snprintf(title, sizeof(title), _("[%s] Choose an image from list"),
                 cfdata->source);
        e_dialog_title_set(web->dia, title);
        e_dialog_button_disable_num_set(web->dia, 0, 0);
        cfdata->pending_downloads = 0;
     }
}

static void
_get_thumbs(void *data)
{
   Web *web;
   E_Config_Dialog_Data *cfdata;
   char *src, *name, *ext;
   char dtmp[4096], dest[4096];

   web = data;
   cfdata = web->cfdata;
   cfdata->pending_downloads = 1;
   snprintf(dtmp, sizeof(dtmp), "%s/.tmp", cfdata->tmpdir);
   ecore_file_mkdir(dtmp);
   ecore_list_first_goto(cfdata->thumbs);
   ecore_list_first_goto(cfdata->names);
   while ((src = ecore_list_next(cfdata->thumbs)))
     {
	name = ecore_list_next(cfdata->names);
	ext = strrchr(src, '.');
        snprintf(dest, sizeof(dest), "%s/%s%s", dtmp, name, ext);
	ecore_file_download(src, dest, _get_thumb_complete, NULL, web);
     }
}

static void
_get_feed(char *url, void *data)
{
   Web *web;
   E_Config_Dialog_Data *cfdata;
   extern int errno;
   char title[4096];

   web = data;
   cfdata = web->cfdata;

   cfdata->tmpdir = mkdtemp(strdup(TEMPLATE));

   ecore_con_url_url_set(cfdata->ecu, url);
   ecore_file_download_abort_all();
   if (cfdata->hdata) ecore_event_handler_del(cfdata->hdata);
   if (cfdata->hcomplete) ecore_event_handler_del(cfdata->hcomplete);
   if (cfdata->feed) fclose(cfdata->feed);

   cfdata->hdata =
     ecore_event_handler_add(ECORE_CON_EVENT_URL_DATA, _feed_data, web);
   cfdata->hcomplete =
     ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE,
                             _feed_complete, web);

   snprintf(title, sizeof(title), _("[%s] Getting feed..."), cfdata->source);
   e_dialog_title_set(web->dia, title); //
   cfdata->feed = fopen("/tmp/themefeed.xml", "w+");
   ecore_con_url_send(cfdata->ecu, NULL, 0, NULL);
}

static void
_reset(void *data)
{
   Web *web;
   E_Config_Dialog_Data *cfdata;

   web = data;
   cfdata = web->cfdata;

   // If there's pending downloads, stop it
   if (cfdata->pending_downloads == 1)
     {
	ecore_file_download_abort_all();
	//ecore_file_download_shutdown();
     }
   cfdata->pending_downloads = 0;

   // Reset busy state
   cfdata->busy = 0;

   // Clean lists
   if (!ecore_list_empty_is(cfdata->thumbs))
     ecore_list_clear(cfdata->thumbs);
   if (!ecore_list_empty_is(cfdata->names))
     ecore_list_clear(cfdata->names);
   if (!ecore_list_empty_is(cfdata->medias))
     ecore_list_clear(cfdata->medias);

   // Clean existing data
   if (ecore_file_exists("/tmp/themefeed.xml")) ecore_file_unlink("/tmp/themefeed.xml");

   // Remove temporary data
   if (cfdata->tmpdir)
     {
	if (ecore_file_is_dir(cfdata->tmpdir))
	  {
	     ecore_file_recursive_rm(cfdata->tmpdir);
	     ecore_file_rmdir(cfdata->tmpdir);
	  }
     }

   // Disable OK button
   e_dialog_button_disable_num_set(web->dia, 0, 1);
}

static void
_parse_feed(void *data)
{
   Web *web;
   E_Config_Dialog_Data *cfdata;
   FILE *fh;
   char instr[1024];
   char *edj, *img, *name;
   char title[4096];
   int state = -1;

   web = data;
   cfdata = web->cfdata;

   cfdata->pending_downloads = 0;
   fh = fopen("/tmp/themefeed.xml", "r");
   while (fgets(instr, sizeof(instr), fh) != NULL)
     {
	if (strstr(instr, "<rss version") != NULL) state = 0;
	if ((strstr(instr, "<item>") != NULL) && (state == 0))
	  {
	     edj = NULL;
	     img = NULL;
	     state = 1;
	  }

	if ((strstr(instr, "<title>") !=  NULL) && (state == 1))
	  {
	     char *p;

	     name = strchr(instr, '>');
	     name++;
	     p = strchr(name, '<');
	     *p = 0;
	     name = strdup(name);
	     state = 2;
	  }

	if ((strstr(instr, "<link>") !=  NULL) && (state == 2))
	  {
	     char *p;

	     edj = strchr(instr, '>');
	     edj++;
	     p = strchr(edj, '<');
	     *p = 0;
	     p = strrchr(ecore_file_file_get(edj), '.');
	     if (p)
	       {
		   if (!strcmp(p, ".edj"))
		     {
		         edj = strdup(edj);
			 state = 3;
		     }
	       }
	  }

	if ((strstr(instr, "<enclosure") != NULL) && (state == 3))
	  {
	     char *p;

	     img = strstr(instr, "url=");
	     img += 5;
	     p = strchr(img, '"');
	     *p = 0;
	     img = strdup(img);
	     state = 4;
	  }

	if ((strstr(instr, "</item>") != NULL) && (state == 4))
	  {
	     ecore_list_append(cfdata->thumbs, img);
	     ecore_list_append(cfdata->names, name);
	     ecore_list_append(cfdata->medias, edj);
	     state = 0;
	  }
     }
   fclose(fh);

   if (state == 0)
     {
        snprintf(title, sizeof(title), _("[%s] Parsing feed... DONE!"),
                 cfdata->source);
	e_dialog_title_set(web->dia, title);
	e_fm2_path_set(cfdata->ofm, cfdata->tmpdir, "/");
	_get_thumbs(web);
     }
   else
     {
        snprintf(title, sizeof(title), _("[%s] Parsing feed... FAILED!"),
                 cfdata->source);
	cfdata->busy = 0;
	e_dialog_title_set(web->dia, title);
     }
}

static int
_feed_complete(void *data, int type, void *event)
{
   Ecore_Con_Event_Url_Complete *euc;
   E_Config_Dialog_Data *cfdata;
   Web *web;
   char title[4096];

   euc = (Ecore_Con_Event_Url_Complete *)event;
   web = data;
   if (web->magic != MAGIC_WEB) return 1;

   cfdata = web->cfdata;
   if (cfdata->ecu != euc->url_con) return 1;

   fclose(cfdata->feed);
   cfdata->feed = NULL;
   ecore_event_handler_del(cfdata->hdata);
   ecore_event_handler_del(cfdata->hcomplete);
   cfdata->hdata = NULL;
   cfdata->hcomplete = NULL;
   if (euc->status == 200)
     {
        snprintf(title, sizeof(title), _("[%s] Getting feed... DONE!"),
                 cfdata->source);
	e_dialog_title_set(web->dia, title);
	_parse_feed(data);
	return 0;
     }
   else
     {
        snprintf(title, sizeof(title), _("[%s] Getting feed... FAILED!"),
                 cfdata->source);
	e_dialog_title_set(web->dia, title);
     }
   return 0;
}

static int
_feed_data(void *data, int type, void *event)
{
   Ecore_Con_Event_Url_Data *eud;
   E_Config_Dialog_Data *cfdata;
   Web *web;

   eud = (Ecore_Con_Event_Url_Data *)event;
   web = data;
   if (web->magic != MAGIC_WEB) return 1;

   cfdata = web->cfdata;

   if (cfdata->ecu != eud->url_con) return 1;

   fwrite(eud->data, sizeof(unsigned char), eud->size, cfdata->feed);
   return 0;
}

static void
_source_sel_cb(void *data)
{
   Web *web;
   E_Config_Dialog_Data *cfdata;

   web = data;
   cfdata = web->cfdata;
   if ((cfdata->busy == 0) && (cfdata->pending_downloads == 0))
     {
	cfdata->source = e_widget_ilist_selected_label_get(cfdata->ol);
	_reset(web);
	cfdata->busy = 1;
	_get_feed(cfdata->ol_val, web);
     }
   else
     e_widget_ilist_unselect(cfdata->ol);
}

static int
_list_find(const char *str1, const char *str2)
{
   return strcmp(str1, ecore_file_strip_ext(str2));
}

static void
_file_click_cb(void *data, Evas_Object *obj, void *ev_info)
{
   Web *web;
   E_Config_Dialog_Data *cfdata;
   Evas_List *sels;
   E_Fm2_Icon_Info *icon_info;

   web = data;
   cfdata = web->cfdata;
   sels = e_fm2_selected_list_get(cfdata->ofm);
   if (!sels) return;
   //if (cfdata->ready_for_edj == 0) return;

   icon_info = sels->data;
   if (ecore_list_find(cfdata->names, ECORE_COMPARE_CB(_list_find), icon_info->file))
     cfdata->edj = ecore_list_index_goto(cfdata->medias, ecore_list_index(cfdata->names));
}

static void
_file_double_click_cb(void *data, Evas_Object *obj, void *ev_info)
{
   /*E_Config_Dialog_Data *cfdata;
   Evas_List *sels;
   E_Fm2_Icon_Info *icon_info;

   cfdata = data;
   sels = e_fm2_selected_list_get(cfdata->ofm);
   if (!sels)
      return;
   icon_info = sels->data;
   printf("[double click] %s\n", icon_info->file);*/

   // Unused atm, interesting to simulate click on Ok button
}

static void
_web_del_cb(void *obj)
{
   E_Dialog *dia = obj;

   e_int_config_theme_web_del(dia);
}

static void
_web_ok_cb(void *data, E_Dialog *dia)
{
   Web *web;
   E_Config_Dialog_Data *cfdata;
   Evas_List *sels;

   web = data;
   cfdata = web->cfdata;
   sels = e_fm2_selected_list_get(cfdata->ofm);
   if (sels)
     _download_media(web);
   else
     e_int_config_theme_web_del(dia);
}

static void
_web_close_cb(void *data, E_Dialog *dia)
{
   e_int_config_theme_web_del(dia);
}
