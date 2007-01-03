/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

#define OVERCLIP 128

/* FIXME: use edje messages & embryo for scrolling of bg's */

/* FIXME: this is NOT complete. dnd not complete (started). only list view
 * works. in icon view it needs to be much better about placement of icons and
 * being able to save/load icon placement. it doesn't support backgrounds or
 * custom frames or icons yet
 */

typedef enum _E_Fm2_Action_Type
{
   FILE_ADD,
     FILE_DEL,
     FILE_CHANGE
} E_Fm2_Action_Type;

typedef enum _E_Fm2_Fop_Type
{
   FOP_DELETE,
     FOP_MOVE,
     FOP_LINK,
     FOP_ADD
} E_Fm2_Fop_Type;

typedef struct _E_Fm2_Smart_Data E_Fm2_Smart_Data;
typedef struct _E_Fm2_Region     E_Fm2_Region;
typedef struct _E_Fm2_Icon       E_Fm2_Icon;
typedef struct _E_Fm2_Action     E_Fm2_Action;
typedef struct _E_Fm2_Fop        E_Fm2_Fop;
typedef struct _E_Fm2_Fop_Item   E_Fm2_Fop_Item;

struct _E_Fm2_Smart_Data
{
   Evas_Coord        x, y, w, h;
   Evas_Object      *obj;
   Evas_Object      *clip;
   Evas_Object      *underlay;
   Evas_Object      *overlay;
   Evas_Object      *drop;
   Evas_Object      *drop_in;
   const char       *dev;
   const char       *path;
   const char       *realpath;
   
   struct {
      Evas_Coord     w, h;
   } max, pmax;
   struct {
      Evas_Coord     x, y;
   } pos;
   struct {
      Evas_List     *list;
      int            member_max;
   } regions;
   struct {
      struct {
	 void (*func) (void *data, Evas_Object *obj, E_Menu *m, E_Fm2_Icon_Info *info);
	 void *data;
      } start, end, replace;
      E_Fm2_Menu_Flags flags;
   } icon_menu;
   
   Evas_List        *icons;
   Evas_List        *queue;
   Ecore_Idler      *scan_idler;
   Ecore_Timer      *scan_timer;
   Ecore_Timer      *sort_idler;
   Ecore_Job        *scroll_job;
   Ecore_Job        *resize_job;
   Ecore_Job        *refresh_job;
   DIR              *dir;
   FILE             *order;
   E_Menu           *menu;
   E_Entry_Dialog   *entry_dialog;
   unsigned char     iconlist_changed : 1;
   unsigned char     order_file : 1;
   unsigned char     typebuf_visible : 1;
   unsigned char     show_hidden_files : 1;

   E_Fm2_Config     *config;

   struct {
      Evas_Object      *obj, *obj2;
      Evas_List        *last_insert;
      Evas_List        **list_index;
      int              iter;
   } tmp;
   
   struct {
      Evas_List       *actions;
      Ecore_Idler     *idler;
      Ecore_Timer     *timer;
      unsigned char    deletions : 1;
   } live;
   
   struct {
      Ecore_File_Monitor *monitor;
   } monitor;
   
   struct {
      char            *buf;
   } typebuf;
   
   E_Fm2_Fop          *fop;
   int                 busy_count;
   
   E_Object           *eobj;
   E_Drop_Handler     *drop_handler;
   E_Fm2_Icon         *drop_icon;
   char                drop_after;
   unsigned char       drop_show : 1;
   unsigned char       drop_in_show : 1;
   unsigned char       drop_all : 1;
   unsigned char       drag : 1;
};
 
struct _E_Fm2_Region
{
   E_Fm2_Smart_Data *sd;
   Evas_Coord        x, y, w, h;
   Evas_List        *list;
   unsigned char     realized : 1;
};

struct _E_Fm2_Icon
{
   E_Fm2_Smart_Data *sd;
   E_Fm2_Region     *region;
   Evas_Coord        x, y, w, h, min_w, min_h;
   Evas_Object      *obj, *obj_icon;
   int               saved_x, saved_y;
   int               saved_rel;
   E_Menu           *menu;
   E_Entry_Dialog   *entry_dialog;
   E_Dialog         *dialog;

   E_Fm2_Icon_Info   info;
   
   struct {
      Evas_Coord     x, y;
      unsigned char  start : 1;
      unsigned char  dnd : 1;
   } drag;
   
   unsigned char     realized : 1;
   unsigned char     selected : 1;
   unsigned char     last_selected : 1;
   unsigned char     saved_pos : 1;
   unsigned char     odd : 1;
   unsigned char     down_sel : 1;
};

struct _E_Fm2_Action
{
   E_Fm2_Action_Type  type;
   const char        *file;
   const char        *file2;
   int                flags;
};

struct _E_Fm2_Fop
{
   Evas_Object *obj;
   const char  *dir;
   Evas_List   *items;
   Ecore_Idler *idler;
   Ecore_Timer *timer;
};

struct _E_Fm2_Fop_Item
{
   E_Fm2_Fop_Type  type;
   E_Fm2_Fop      *fop;
   const char     *file;
   const char     *file2;
   const char     *file3;
   int             after;
   DIR            *dir;
   unsigned char   is_dir : 1;
   unsigned char   file_add : 1;
};

static const char *_e_fm2_dev_path_map(const char *dev, const char *path);
static void _e_fm2_file_add(Evas_Object *obj, const char *file, int unique, const char *file_rel, int after);
static void _e_fm2_file_del(Evas_Object *obj, const char *file);
static void _e_fm2_scan_start(Evas_Object *obj);
static void _e_fm2_scan_stop(Evas_Object *obj);
static void _e_fm2_queue_process(Evas_Object *obj); 
static void _e_fm2_queue_free(Evas_Object *obj);
static void _e_fm2_regions_free(Evas_Object *obj);
static void _e_fm2_regions_populate(Evas_Object *obj);
static void _e_fm2_icons_place(Evas_Object *obj);
static void _e_fm2_icons_free(Evas_Object *obj);
static void _e_fm2_regions_eval(Evas_Object *obj);
static void _e_fm2_config_free(E_Fm2_Config *cfg);

static E_Fm2_Icon *_e_fm2_icon_new(E_Fm2_Smart_Data *sd, const char *file);
static void _e_fm2_icon_unfill(E_Fm2_Icon *ic);
static int _e_fm2_icon_fill(E_Fm2_Icon *ic);
static void _e_fm2_icon_free(E_Fm2_Icon *ic);
static void _e_fm2_icon_realize(E_Fm2_Icon *ic);
static void _e_fm2_icon_unrealize(E_Fm2_Icon *ic);
static int _e_fm2_icon_visible(E_Fm2_Icon *ic);
static void _e_fm2_icon_label_set(E_Fm2_Icon *ic, Evas_Object *obj);
static Evas_Object *_e_fm2_icon_icon_direct_set(E_Fm2_Icon *ic, Evas_Object *o, void (*gen_func) (void *data, Evas_Object *obj, void *event_info), void *data, int force_gen);
static void _e_fm2_icon_icon_set(E_Fm2_Icon *ic);
static void _e_fm2_icon_thumb(E_Fm2_Icon *ic, Evas_Object *oic, int force);
static void _e_fm2_icon_select(E_Fm2_Icon *ic);
static void _e_fm2_icon_deselect(E_Fm2_Icon *ic);
static int _e_fm2_icon_desktop_load(E_Fm2_Icon *ic);

static E_Fm2_Region *_e_fm2_region_new(E_Fm2_Smart_Data *sd);
static void _e_fm2_region_free(E_Fm2_Region *rg);
static void _e_fm2_region_realize(E_Fm2_Region *rg);
static void _e_fm2_region_unrealize(E_Fm2_Region *rg);
static int _e_fm2_region_visible(E_Fm2_Region *rg);

static void _e_fm2_icon_make_visible(E_Fm2_Icon *ic);
static void _e_fm2_icon_desel_any(Evas_Object *obj);
static E_Fm2_Icon *_e_fm2_icon_first_selected_find(Evas_Object *obj);
static void _e_fm2_icon_sel_first(Evas_Object *obj);
static void _e_fm2_icon_sel_last(Evas_Object *obj);
static void _e_fm2_icon_sel_prev(Evas_Object *obj);
static void _e_fm2_icon_sel_next(Evas_Object *obj);
static void _e_fm2_typebuf_show(Evas_Object *obj);
static void _e_fm2_typebuf_hide(Evas_Object *obj);
static void _e_fm2_typebuf_history_prev(Evas_Object *obj);
static void _e_fm2_typebuf_history_next(Evas_Object *obj);
static void _e_fm2_typebuf_run(Evas_Object *obj);
static void _e_fm2_typebuf_match(Evas_Object *obj);
static void _e_fm2_typebuf_complete(Evas_Object *obj);
static void _e_fm2_typebuf_char_append(Evas_Object *obj, const char *ch);
static void _e_fm2_typebuf_char_backspace(Evas_Object *obj);

static void _e_fm2_cb_dnd_enter(void *data, const char *type, void *event);
static void _e_fm2_cb_dnd_move(void *data, const char *type, void *event);
static void _e_fm2_cb_dnd_leave(void *data, const char *type, void *event);
static void _e_fm2_cb_dnd_drop(void *data, const char *type, void *event);
static void _e_fm2_cb_icon_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_fm2_cb_icon_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_fm2_cb_icon_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_fm2_cb_icon_thumb_dnd_gen(void *data, Evas_Object *obj, void *event_info);
static void _e_fm2_cb_icon_thumb_gen(void *data, Evas_Object *obj, void *event_info);
static void _e_fm2_cb_key_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_fm2_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_fm2_cb_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_fm2_cb_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_fm2_cb_scroll_job(void *data);
static void _e_fm2_cb_resize_job(void *data);
static int _e_fm2_cb_icon_sort(void *data1, void *data2);
static int _e_fm2_cb_scan_idler(void *data);
static int _e_fm2_cb_scan_timer(void *data);
static int _e_fm2_cb_sort_idler(void *data);

static void _e_fm2_obj_icons_place(E_Fm2_Smart_Data *sd);

static void _e_fm2_smart_add(Evas_Object *object);
static void _e_fm2_smart_del(Evas_Object *object);
static void _e_fm2_smart_move(Evas_Object *object, Evas_Coord x, Evas_Coord y);
static void _e_fm2_smart_resize(Evas_Object *object, Evas_Coord w, Evas_Coord h);
static void _e_fm2_smart_show(Evas_Object *object);
static void _e_fm2_smart_hide(Evas_Object *object);
static void _e_fm2_smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _e_fm2_smart_clip_set(Evas_Object *obj, Evas_Object * clip);
static void _e_fm2_smart_clip_unset(Evas_Object *obj);

static void _e_fm2_order_file_rewrite(Evas_Object *obj);
static void _e_fm2_menu(Evas_Object *obj, unsigned int timestamp);
static void _e_fm2_menu_post_cb(void *data, E_Menu *m);
static void _e_fm2_icon_menu(E_Fm2_Icon *ic, Evas_Object *obj, unsigned int timestamp);
static void _e_fm2_icon_menu_post_cb(void *data, E_Menu *m);
static void _e_fm2_refresh(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_fm2_toggle_hidden_files(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_fm2_toggle_ordering(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_fm2_sort(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_fm2_new_directory(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_fm2_new_directory_delete_cb(void *obj);
static void _e_fm2_new_directory_yes_cb(char *text, void *data);
static void _e_fm2_new_directory_no_cb(void *data);
static void _e_fm2_file_rename(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_fm2_file_rename_delete_cb(void *obj);
static void _e_fm2_file_rename_yes_cb(char *text, void *data);
static void _e_fm2_file_rename_no_cb(void *data);
static void _e_fm2_file_properties(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_fm2_file_delete(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_fm2_file_delete_delete_cb(void *obj);
static void _e_fm2_file_delete_yes_cb(void *data, E_Dialog *dialog);
static void _e_fm2_file_delete_no_cb(void *data, E_Dialog *dialog);
static void _e_fm2_refresh_job_cb(void *data);

static void _e_fm2_live_file_add(Evas_Object *obj, const char *file, const char *file_rel, int after);
static void _e_fm2_live_file_del(Evas_Object *obj, const char *file);
static void _e_fm2_live_file_changed(Evas_Object *obj, char *file);
static void _e_fm2_live_process_begin(Evas_Object *obj);
static void _e_fm2_live_process_end(Evas_Object *obj);
static void _e_fm2_live_process(Evas_Object *obj);
static int _e_fm2_cb_live_idler(void *data);
static int _e_fm2_cb_live_timer(void *data);
static void _e_fm2_cb_file_monitor(void *data, Ecore_File_Monitor *em, Ecore_File_Event event, const char *path);

static E_Fm2_Fop *_e_fm2_fop_add(E_Fm2_Smart_Data *sd);
static void _e_fm2_fop_del(E_Fm2_Fop *fop);
static void _e_fm2_fop_detach(E_Fm2_Smart_Data *sd);
static int _e_fm2_fop_process(E_Fm2_Fop *fop);
static int _e_fm2_cb_fop_idler(void *data);
static int _e_fm2_cb_fop_timer(void *data);

static char *_e_fm2_meta_path = NULL;
static Evas_Smart *_e_fm2_smart = NULL;
static Evas_List *_e_fm2_list = NULL;
static Evas_List *_e_fm2_fop_list = NULL;

/* externally accessible functions */
EAPI int
e_fm2_init(void)
{
   const char *homedir;
   char  path[PATH_MAX];

   homedir = e_user_homedir_get();
   snprintf(path, sizeof(path), "%s/.e/e/fileman/metadata", homedir);
   ecore_file_mkpath(path);
   _e_fm2_meta_path = strdup(path);

   _e_fm2_smart = evas_smart_new("e_fm",
				 _e_fm2_smart_add, /* add */
				 _e_fm2_smart_del, /* del */
				 NULL, NULL, NULL, NULL, NULL,
				 _e_fm2_smart_move, /* move */
				 _e_fm2_smart_resize, /* resize */
				 _e_fm2_smart_show,/* show */
				 _e_fm2_smart_hide,/* hide */
				 _e_fm2_smart_color_set, /* color_set */
				 _e_fm2_smart_clip_set, /* clip_set */
				 _e_fm2_smart_clip_unset, /* clip_unset */
				 NULL); /* data*/
   return 1;
}

EAPI int
e_fm2_shutdown(void)
{
   evas_smart_free(_e_fm2_smart);
   _e_fm2_smart = NULL;
   E_FREE(_e_fm2_meta_path);
   return 1;
}

EAPI Evas_Object *
e_fm2_add(Evas *evas)
{
   return evas_object_smart_add(evas, _e_fm2_smart);
}

EAPI void
e_fm2_path_set(Evas_Object *obj, const char *dev, const char *path)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety

   _e_fm2_fop_detach(sd);
   
   /* internal config for now - don't see a pont making this configurable */
   sd->regions.member_max = 64;

   if (!sd->config)
     {
	sd->config = E_NEW(E_Fm2_Config, 1);
	if (!sd->config) return;
//	sd->config->view.mode = E_FM2_VIEW_MODE_ICONS;
	sd->config->view.mode = E_FM2_VIEW_MODE_LIST;
	sd->config->view.open_dirs_in_place = 1;
	sd->config->view.selector = 1;
	sd->config->view.single_click = 0;
	sd->config->view.no_subdir_jump = 0;
	sd->config->icon.icon.w = 128;
	sd->config->icon.icon.h = 128;
	sd->config->icon.list.w = 24;
	sd->config->icon.list.h = 24;
	sd->config->icon.fixed.w = 1;
	sd->config->icon.fixed.h = 1;
	sd->config->icon.extension.show = 0;
	sd->config->list.sort.no_case = 1;
	sd->config->list.sort.dirs.first = 1;
	sd->config->list.sort.dirs.last = 0;
	sd->config->selection.single = 0;
	sd->config->selection.windows_modifiers = 0;
	sd->config->theme.background = NULL;
	sd->config->theme.frame = NULL;
	sd->config->theme.icons = NULL;
	sd->config->theme.fixed = 0;
     }
   
   if (sd->dev) evas_stringshare_del(sd->dev);
   if (sd->path) evas_stringshare_del(sd->path);
   if (sd->realpath) evas_stringshare_del(sd->realpath);
   sd->dev = sd->path = sd->realpath = NULL;
   if (dev) sd->dev = evas_stringshare_add(dev);
   sd->path = evas_stringshare_add(path);
   sd->realpath = _e_fm2_dev_path_map(sd->dev, sd->path);
   
   _e_fm2_scan_stop(obj);
   _e_fm2_queue_free(obj);
   _e_fm2_regions_free(obj);
   _e_fm2_icons_free(obj);
   edje_object_part_text_set(sd->overlay, "e.text.busy_label", "");
   
   _e_fm2_scan_start(obj);
   evas_object_smart_callback_call(obj, "dir_changed", NULL);
   sd->tmp.iter = 0;
}

EAPI void
e_fm2_path_get(Evas_Object *obj, const char **dev, const char **path)
{
   E_Fm2_Smart_Data *sd;

   if (dev) *dev = NULL;
   if (path) *path = NULL;
   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety
   if (dev) *dev = sd->dev;
   if (path) *path = sd->path;
}

EAPI void
e_fm2_refresh(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety

   _e_fm2_scan_stop(obj);
   _e_fm2_queue_free(obj);
   _e_fm2_regions_free(obj);
   _e_fm2_icons_free(obj);
   
   _e_fm2_scan_start(obj);
//   evas_object_smart_callback_call(obj, "dir_changed", NULL);
   sd->tmp.iter = 0;
}

EAPI int
e_fm2_has_parent_get(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return 0; // safety
   if (!evas_object_type_get(obj)) return 0; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return 0; // safety
   if (!sd->path) return 0;
   if ((sd->path[0] == 0) || (!strcmp(sd->path, "/"))) return 0;
   return 1;
}

EAPI const char *
e_fm2_real_path_get(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return NULL; // safety
   if (!evas_object_type_get(obj)) return NULL; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return NULL; // safety
   return sd->realpath;
}

EAPI void
e_fm2_parent_go(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   char *path, *dev = NULL, *p;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety
   if (!sd->path) return;
   path = strdup(sd->path);
   if (sd->dev) dev = strdup(sd->dev);
   p = strrchr(path, '/');
   if (p) *p = 0;
   e_fm2_path_set(obj, dev, path);
   E_FREE(dev);
   E_FREE(path);
}

EAPI void
e_fm2_config_set(Evas_Object *obj, E_Fm2_Config *cfg)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety
   if (sd->config) _e_fm2_config_free(sd->config);
   sd->config = NULL;
   if (!cfg) return;
   sd->config = E_NEW(E_Fm2_Config, 1);
   if (!sd->config) return;
   memcpy(sd->config, cfg, sizeof(E_Fm2_Config));
   if (cfg->view.extra_file_source) sd->config->view.extra_file_source = evas_stringshare_add(cfg->view.extra_file_source);
   if (cfg->icon.key_hint) sd->config->icon.key_hint = evas_stringshare_add(cfg->icon.key_hint);
   if (cfg->theme.background) sd->config->theme.background = evas_stringshare_add(cfg->theme.background);
   if (cfg->theme.frame) sd->config->theme.frame = evas_stringshare_add(cfg->theme.frame);
   if (cfg->theme.icons) sd->config->theme.icons = evas_stringshare_add(cfg->theme.icons);
}

EAPI E_Fm2_Config *
e_fm2_config_get(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return NULL; // safety
   if (!evas_object_type_get(obj)) return NULL; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return NULL; // safety
   return sd->config;
}

EAPI Evas_List *
e_fm2_selected_list_get(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   Evas_List *list = NULL, *l;
   E_Fm2_Icon *ic;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return NULL; // safety
   if (!evas_object_type_get(obj)) return NULL; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return NULL; // safety
   for (l = sd->icons; l; l = l->next)
     {
	ic = l->data;
	if (ic->selected)
	  list = evas_list_append(list, &(ic->info));
     }
   return list;
}

EAPI Evas_List *
e_fm2_all_list_get(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   Evas_List *list = NULL, *l;
   E_Fm2_Icon *ic;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return NULL; // safety
   if (!evas_object_type_get(obj)) return NULL; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return NULL; // safety
   for (l = sd->icons; l; l = l->next)
     {
	ic = l->data;
	list = evas_list_append(list, &(ic->info));
     }
   return list;
}

EAPI void
e_fm2_select_set(Evas_Object *obj, const char *file, int select)
{
   E_Fm2_Smart_Data *sd;
   Evas_List *l;
   E_Fm2_Icon *ic;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety
   for (l = sd->icons; l; l = l->next)
     {
	ic = l->data;
	if ((file) && (!strcmp(ic->info.file, file)))
	  {
	     if (select) _e_fm2_icon_select(ic);
	     else _e_fm2_icon_deselect(ic);
	  }
	else
	  {
	     if (ic->sd->config->selection.single)
	       _e_fm2_icon_deselect(ic);
	     ic->last_selected = 0;
	  }
     }
}

EAPI void
e_fm2_file_show(Evas_Object *obj, const char *file)
{
   E_Fm2_Smart_Data *sd;
   Evas_List *l;
   E_Fm2_Icon *ic;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety
   for (l = sd->icons; l; l = l->next)
     {
	ic = l->data;
	if (!strcmp(ic->info.file, file))
	  {
	     _e_fm2_icon_make_visible(ic);
	     return;
	  }
     }
}

EAPI void
e_fm2_icon_menu_replace_callback_set(Evas_Object *obj, void (*func) (void *data, Evas_Object *obj, E_Menu *m, E_Fm2_Icon_Info *info), void *data)
{
   E_Fm2_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety
   sd->icon_menu.replace.func = func;
   sd->icon_menu.replace.data = data;
}

EAPI void
e_fm2_icon_menu_start_extend_callback_set(Evas_Object *obj, void (*func) (void *data, Evas_Object *obj, E_Menu *m, E_Fm2_Icon_Info *info), void *data)
{
   E_Fm2_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety
   sd->icon_menu.start.func = func;
   sd->icon_menu.start.data = data;
}

EAPI void
e_fm2_icon_menu_end_extend_callback_set(Evas_Object *obj, void (*func) (void *data, Evas_Object *obj, E_Menu *m, E_Fm2_Icon_Info *info), void *data)
{
   E_Fm2_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety
   sd->icon_menu.end.func = func;
   sd->icon_menu.end.data = data;
}

EAPI void
e_fm2_icon_menu_flags_set(Evas_Object *obj, E_Fm2_Menu_Flags flags)
{
   E_Fm2_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety
   sd->icon_menu.flags = flags;
}

EAPI E_Fm2_Menu_Flags
e_fm2_icon_menu_flags_get(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return 0; // safety
   if (!evas_object_type_get(obj)) return 0; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return 0; // safety
   return sd->icon_menu.flags;
}

EAPI void
e_fm2_window_object_set(Evas_Object *obj, E_Object *eobj)
{
   E_Fm2_Smart_Data *sd;
   const char *drop[] = { "enlightenment/eapp", "enlightenment/border", "text/uri-list" };
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety
   sd->eobj = eobj;
   if (sd->drop_handler) e_drop_handler_del(sd->drop_handler);
   sd->drop_handler = e_drop_handler_add(sd->eobj,
					 sd, 
					 _e_fm2_cb_dnd_enter,
					 _e_fm2_cb_dnd_move,
					 _e_fm2_cb_dnd_leave,
					 _e_fm2_cb_dnd_drop,
					 drop, 3, sd->x, sd->y, sd->w, sd->h);
}

EAPI void
e_fm2_icons_update(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   Evas_List *l;
   E_Fm2_Icon *ic;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety
   for (l = sd->icons; l; l = l->next)
     {
	ic = l->data;
	if (ic->realized)
	  {
	     _e_fm2_icon_unrealize(ic);
	     _e_fm2_icon_realize(ic);
	  }
     }
}

EAPI void
e_fm2_pan_set(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety
   if (x > (sd->max.w - sd->w)) x = sd->max.w - sd->w;
   if (x < 0) x = 0;
   if (y > (sd->max.h - sd->h)) y = sd->max.h - sd->h;
   if (y < 0) y = 0;
   if ((sd->pos.x == x) && (sd->pos.y == y)) return;
   sd->pos.x = x;
   sd->pos.y = y;
   if (sd->scroll_job) ecore_job_del(sd->scroll_job);
   sd->scroll_job = ecore_job_add(_e_fm2_cb_scroll_job, obj);
}

EAPI void
e_fm2_pan_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety
   if (x) *x = sd->pos.x;
   if (y) *y = sd->pos.y;
}

EAPI void
e_fm2_pan_max_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y)
{
   E_Fm2_Smart_Data *sd;
   Evas_Coord mx, my;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety
   mx = sd->max.w - sd->w;
   if (mx < 0) mx = 0;
   my = sd->max.h - sd->h;
   if (my < 0) my = 0;
   if (x) *x = mx;
   if (y) *y = my;
}

EAPI void
e_fm2_pan_child_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety
   if (w) *w = sd->max.w;
   if (h) *h = sd->max.h;
}

EAPI void
e_fm2_all_icons_update(void)
{
   Evas_List *l;
   
   for (l = _e_fm2_list; l; l = l->next)
     e_fm2_icons_update(l->data);
}

EAPI void
e_fm2_fop_delete_add(Evas_Object *obj, E_Fm2_Icon_Info *ici)
{
   E_Fm2_Smart_Data *sd;
   E_Fm2_Fop *fop;
   E_Fm2_Fop_Item *fi;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety
   /* if no obj fop struct, create fop struct and add idler */
   /* append fop node for delete and this file to the fop */
   
   /* if obj is deleted what to do to fop: */
   /* if obj is deleted, detach fop and let it run on its own till end */
   /* if obj changes dir detach fop obj */
   
   /* on fop queue empty - deete idler and free fop */
   /* in fop idler go to the item on the head of the fop queue. if it is
    * a file or a symlink, delete it directly. if it is a dir then
    * opendir it in the fop entry (fope->dir != NULL now) and if dir is
    * not null, readdir till end (when at end remove item) and per item
    * if it is a file or link - delete it directly, if it is a dir, prepend to
    * fop queue.
    * per fop item processed, if obj is attached to fop - report status usig
    * the status overlay for the fm edj obj.
    */
   fop = _e_fm2_fop_add(sd);
   if (!fop) return;
   
   fi = E_NEW(E_Fm2_Fop_Item, 1);
   fi->fop = fop;
   fi->type = FOP_DELETE;
   fi->file = evas_stringshare_add(ici->file);
   if ((S_ISDIR(ici->statinfo.st_mode)) && (!ici->real_link))
     fi->is_dir = 1;
   fi->fop->items = evas_list_append(fi->fop->items, fi);
//   printf("ADD0: %s\n", fi->file);
}

EAPI void 
e_fm2_fop_move_add(Evas_Object *obj, const char *src, const char *dst, const char *rel, int after, int file_add)
{
   E_Fm2_Smart_Data *sd;
   E_Fm2_Fop *fop;
   E_Fm2_Fop_Item *fi;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety
   fop = _e_fm2_fop_add(sd);
   if (!fop) return;
   fi = E_NEW(E_Fm2_Fop_Item, 1);
   fi->fop = fop;
   fi->type = FOP_MOVE;
   fi->file = evas_stringshare_add(src);
   fi->file2 = evas_stringshare_add(dst);
   if (rel) fi->file3 = evas_stringshare_add(rel);
   fi->after = after;
   fi->file_add = file_add;
   fi->fop->items = evas_list_append(fi->fop->items, fi);
}

EAPI void 
e_fm2_fop_link_add(Evas_Object *obj, const char *src, const char *dst)
{
   E_Fm2_Smart_Data *sd;
   E_Fm2_Fop *fop;
   E_Fm2_Fop_Item *fi;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety
   fop = _e_fm2_fop_add(sd);
   if (!fop) return;
   fi = E_NEW(E_Fm2_Fop_Item, 1);
   fi->fop = fop;
   fi->type = FOP_LINK;
   fi->file = evas_stringshare_add(src);
   fi->file2 = evas_stringshare_add(dst);
   fi->fop->items = evas_list_append(fi->fop->items, fi);
}

EAPI void 
e_fm2_fop_add_add(Evas_Object *obj, const char *file, const char *rel, int after)
{
   E_Fm2_Smart_Data *sd;
   E_Fm2_Fop *fop;
   E_Fm2_Fop_Item *fi;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety
   fop = _e_fm2_fop_add(sd);
   if (!fop) return;
   fi = E_NEW(E_Fm2_Fop_Item, 1);
   fi->fop = fop;
   fi->type = FOP_ADD;
   fi->file = evas_stringshare_add(file);
   if (rel) fi->file3 = evas_stringshare_add(rel);
   fi->after = after;
   fi->fop->items = evas_list_append(fi->fop->items, fi);
}

/* local subsystem functions */
static const char *
_e_fm2_dev_path_map(const char *dev, const char *path)
{
   char buf[4096] = "", *s, *ss;
   int len;
   
   /* map a device name to a mount point/path on the os (and append path) */
   if (!dev) return evas_stringshare_add(path);

   /* FIXME: load mappings from config and use them first - maybe device
    * discovery should be done through config and not the below (except
    * maybe for home directory and root fs and other simple thngs */
   /* FIXME: also add other virtualized dirs like "backgrounds", "themes",
    * "favorites" */
#define CMP(x) (e_util_glob_case_match(dev, x))   
#define PRT(args...) snprintf(buf, sizeof(buf), ##args)
   
   if      (CMP("/")) {
      PRT("%s", path);
   }
   else if (CMP("~/")) {
      s = (char *)e_user_homedir_get();
      PRT("%s%s", s, path);
   }
   else if (dev[0] == '/') {
      /* dev is a full path - consider it a mountpoint device on its own */
      PRT("%s%s", dev, path);
   }
   else if (CMP("favorites")) {
      /* this is a virtual device - it's where your favorites list is 
       * stored - a dir with 
       .desktop files or symlinks (in fact anything
       * you like
       */
      s = (char *)e_user_homedir_get();
      PRT("%s/.e/e/fileman/favorites", s);
   }
   else if (CMP("dvd") || CMP("dvd-*"))  {
      /* FIXME: find dvd mountpoint optionally for dvd no. X */
      /* maybe make part of the device mappings config? */
   }
   else if (CMP("cd") || CMP("cd-*") || CMP("cdrom") || CMP("cdrom-*") ||
	    CMP("dvd") || CMP("dvd-*")) {
      /* FIXME: find cdrom or dvd mountpoint optionally for cd/dvd no. X */
      /* maybe make part of the device mappings config? */
   }
   /* FIXME: add code to find USB devices (multi-card readers or single,
    * usb thumb drives, other usb storage devices (usb hd's etc.)
    */
   /* maybe make part of the device mappings config? */
   /* FIXME: add code for finding nfs shares, smb shares etc. */
   /* maybe make part of the device mappings config? */

   /* strip out excess multiple slashes */
   s = buf;
   while (*s)
     {
	if ((s[0] == '/') && (s[1] == '/'))
	  {
	     ss = s;
	     do
	       {
		  ss[0] = ss[1];
		  ss++;
	       }
	     while (*ss);
	  }
	s++;
     }
   /* strip out slashes at the end - unless its just "/" */
   len = strlen(buf);
   while ((len > 1) && (buf[len - 1] == '/'))
     {
	buf[len - 1] = 0;
	len--;
     }
   return evas_stringshare_add(buf);
}

static void
_e_fm2_file_add(Evas_Object *obj, const char *file, int unique, const char *file_rel, int after)
{
   E_Fm2_Smart_Data *sd;
   E_Fm2_Icon *ic, *ic2;
   Evas_List *l;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   /* if we only want unique icon names - if it's there - ignore */
   if (unique)
     {
	for (l = sd->icons; l; l = l->next)
	  {
	     ic = l->data;
	     if (!strcmp(ic->info.file, file))
	       {
		  sd->tmp.last_insert = NULL;
//		  sd->iconlist_changed = 1;
		  return;
	       }
	  }
	for (l = sd->queue; l; l = l->next)
	  {
	     ic = l->data;
	     if (!strcmp(ic->info.file, file))
	       {
		  sd->tmp.last_insert = NULL;
//		  sd->iconlist_changed = 1;
		  return;
	       }
	  }
     }
   /* create icon obj and append to unsorted list */
   ic = _e_fm2_icon_new(sd, file);
   if (ic)
     {
	if (!file_rel)
	  {
	     /* respekt da ordah! */
	     if (sd->order_file)
	       sd->queue = evas_list_append(sd->queue, ic);
	     else
	       {
		  /* insertion sort it here to spread the sort load into idle time */
		  for (l = sd->queue; l; l = l->next)
		    {
		       ic2 = l->data;
		       if (_e_fm2_cb_icon_sort(ic, ic2) < 0)
			 {
			    sd->queue = evas_list_prepend_relative_list(sd->queue, ic, l);
			    break;
			 }
		    }
		  if (!l) sd->queue = evas_list_append(sd->queue, ic);
	       }
	  }
	else
	  {
	     for (l = sd->icons; l; l = l->next)
	       {
		  ic2 = l->data;
		  if (!strcmp(ic2->info.file, file_rel))
		    {
		       if (after)
			 sd->icons = evas_list_append_relative(sd->icons, ic, ic2);
		       else
			 sd->icons = evas_list_prepend_relative(sd->icons, ic, ic2);
		       break;
		    }
	       }
	     if (!l) sd->icons = evas_list_append(sd->icons, ic);
	  }
	sd->tmp.last_insert = NULL;
	sd->iconlist_changed = 1;
     }
}

static void
_e_fm2_file_del(Evas_Object *obj, const char *file)
{
   E_Fm2_Smart_Data *sd;
   E_Fm2_Icon *ic;
   Evas_List *l;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   for (l = sd->icons; l; l = l->next)
     {
	ic = l->data;
	if (!strcmp(ic->info.file, file))
	  {
	     sd->icons = evas_list_remove_list(sd->icons, l);
	     if (ic->region)
	       ic->region->list = evas_list_remove(ic->region->list, ic);
	     _e_fm2_icon_free(ic);
	     return;
	  }
     }
}

static void
_e_fm2_scan_start(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   /* start scanning the dir in an idler and putting found items on the */
   /* queue as icons - waiting for sorting and region insertion by a */
   /* timer that ticks off taking queued icons and putting them in a */
   /* sorted position in the icon list */
   /* this system is great - it completely does the dir scanning in idle
    * time when e has nothing better to do - BUt it will mean that the dir
    * will first draw with an empty view then slowly fill up as the scan
    * happens - this means e remains interactive, but could mean for more
    * redraws that we want. what we want to do is maybe bring some of the
    * scan forward to here for a short period of time so when the window
    * is drawn - its drawn with contents ... if they didnt take too long
    * to fill
    */
   /* if i add the above pre-scan and it doesnt finish - continue here */
   if ((sd->scan_idler) || (sd->scan_timer)) return;
   sd->order_file = 0;
   ecore_desktop_instrumentation_reset();
   if (sd->monitor.monitor) ecore_file_monitor_del(sd->monitor.monitor);
   sd->monitor.monitor = ecore_file_monitor_add(sd->realpath, _e_fm2_cb_file_monitor, obj);
   sd->scan_idler = ecore_idler_add(_e_fm2_cb_scan_idler, obj);
   sd->scan_timer = ecore_timer_add(0.5, _e_fm2_cb_scan_timer, obj);
   sd->busy_count++;
   if (sd->busy_count == 1)
     edje_object_signal_emit(sd->overlay, "e,state,busy,start", "e");
}

static void
_e_fm2_scan_stop(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if ((sd->dir) || (sd->order))
     {
	sd->busy_count--;
	if (sd->busy_count == 0)
	  edje_object_signal_emit(sd->overlay, "e,state,busy,stop", "e");
     }
   /* stop the scan idler, the sort timer and free the queue */
   if (sd->dir)
     {
	closedir(sd->dir);
	sd->dir = NULL;
     }
   if (sd->order)
     {
	fclose(sd->order);
	sd->order = NULL;
     }
   if (sd->tmp.obj)
     {
	evas_object_del(sd->tmp.obj);
	sd->tmp.obj = NULL;
     }
   if (sd->tmp.obj2)
     {
	evas_object_del(sd->tmp.obj2);
	sd->tmp.obj2 = NULL;
     }
   if (sd->scan_idler)
     {
	ecore_idler_del(sd->scan_idler);
	sd->scan_idler = NULL;
     }
   if (sd->scan_timer)
     {
	ecore_timer_del(sd->scan_timer);
	sd->scan_timer = NULL;
     }
   if (sd->sort_idler)
     {
	ecore_idler_del(sd->sort_idler);
	sd->sort_idler = NULL;
     }
   
   E_FREE(sd->tmp.list_index);
   _e_fm2_queue_free(obj);
   _e_fm2_obj_icons_place(sd);
   ecore_desktop_instrumentation_print();
   
   _e_fm2_live_process_begin(obj);
}

static void
_e_fm2_queue_process(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   E_Fm2_Icon *ic, *ic2;
   Evas_List *l, **ll;
   int added = 0, i, p0, p1, n, v;
   double t;
   char buf[4096];

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (!sd->queue) return;
//   double tt = ecore_time_get();
//   int queued = evas_list_count(sd->queue);
   /* take unsorted and insert into the icon list - reprocess regions */
   t = ecore_time_get();
   if (!sd->tmp.last_insert)
     {
#if 1
	n = evas_list_count(sd->icons);
	E_FREE(sd->tmp.list_index);
	if (n > 0)
	  sd->tmp.list_index = malloc(n * sizeof(Evas_List *));
	if (sd->tmp.list_index)
	  {
	     ll = sd->tmp.list_index;
	     for (l = sd->icons; l; l = l->next)
	       {
		  *ll = l;
		  ll++;
	       }
	     /* binary search first queue */
	     ic = sd->queue->data;
	     p0 = 0; p1 = n;
	     i = (p0 + p1) / 2;
	     ll = sd->tmp.list_index;
	     do
	       {
		  ic2 = ll[i]->data;
		  v = _e_fm2_cb_icon_sort(ic, ic2);
		  if (v < 0) /* ic should go before ic2 */
		    p1 = i;
		  else /* ic should go at or after ic2 */
		    p0 = i;
		  i = (p0 + p1) / 2;
		  l = ll[i];
	       }
	     while ((p1 - p0) > 1);
	  }
	else
#endif	  
	  l = sd->icons;
     }
   else
     l = sd->tmp.last_insert;
   while (sd->queue)
     {
	
	ic = sd->queue->data;
	sd->queue = evas_list_remove_list(sd->queue, sd->queue);
	/* insertion sort - better than qsort for the way we are doing
	 * things - incrimentally scan and sort as we go as we now know
	 * that the queue files are in order, we speed up insertions to
	 * a worst case of O(n) where n is the # of files in the list
	 * so far
	 */
	if (sd->order_file)
	  {
	     l = NULL;
	  }
	else
	  {
	     for (; l; l = l->next)
	       {
		  ic2 = l->data;
		  if (_e_fm2_cb_icon_sort(ic, ic2) < 0)
		    {
		       sd->icons = evas_list_prepend_relative_list(sd->icons, ic, l);
		       sd->tmp.last_insert = l;
		       break;
		    }
	       }
	  }
	if (!l)
	  {
	     sd->icons = evas_list_append(sd->icons, ic);
	     sd->tmp.last_insert = evas_list_last(sd->icons);
	  }
	added++;
	/* if we spent more than 1/20th of a second inserting - give up
	 * for now */
	if ((ecore_time_get() - t) > 0.05) break;
     }
//   printf("FM: SORT %1.3f (%i files) (%i queued, %i added) [%i iter]\n",
//	  ecore_time_get() - tt, evas_list_count(sd->icons), queued, 
//	  added, sd->tmp.iter);
   snprintf(buf, sizeof(buf), _("%i Files"), evas_list_count(sd->icons));
   edje_object_part_text_set(sd->overlay, "e.text.busy_label", buf);
   /* FIXME: this could get a lot faster - avoid it or something. scan
    speed goes from 200-250 files/0.2 sec to 80 or so in my tests */
   if (sd->resize_job) ecore_job_del(sd->resize_job);
   sd->resize_job = ecore_job_add(_e_fm2_cb_resize_job, obj);
   evas_object_smart_callback_call(sd->obj, "changed", NULL);
   sd->tmp.iter++;
}

static void
_e_fm2_queue_free(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   /* just free the icons in the queue  and the queue itself */
   while (sd->queue)
     {
	_e_fm2_icon_free(sd->queue->data);
	sd->queue = evas_list_remove_list(sd->queue, sd->queue);
     }
}

static void
_e_fm2_regions_free(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   /* free up all regions */
   while (sd->regions.list)
     {
	_e_fm2_region_free(sd->regions.list->data);
        sd->regions.list = evas_list_remove_list(sd->regions.list, sd->regions.list);
     }
}

static void
_e_fm2_regions_populate(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   Evas_List *l;
   E_Fm2_Region *rg;
   E_Fm2_Icon *ic;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   /* take the icon list and split into regions */
   rg = NULL;
   evas_event_freeze(evas_object_evas_get(obj));
   edje_freeze();
   for (l = sd->icons; l; l = l->next)
     {
	ic = l->data;
	if (!rg)
	  {
	     rg = _e_fm2_region_new(sd);
	     sd->regions.list = evas_list_append(sd->regions.list, rg);
	  }
	ic->region = rg;
	rg->list = evas_list_append(rg->list, ic);
	if (rg->w == 0)
	  {
	     rg->x = ic->x;
	     rg->y = ic->y;
	     rg->w = ic->w;
	     rg->h = ic->h;
	  }
	else
	  {
	     if (ic->x < rg->x)
	       {
		  rg->w += rg->x - ic->x;
		  rg->x = ic->x;
	       }
	     if ((ic->x + ic->w) > (rg->x + rg->w))
	       {
		  rg->w += (ic->x + ic->w) - (rg->x + rg->w);
	       }
	     if (ic->y < rg->y)
	       {
		  rg->h += rg->y - ic->y;
		  rg->y = ic->y;
	       }
	     if ((ic->y + ic->h) > (rg->y + rg->h))
	       {
		  rg->h += (ic->y + ic->h) - (rg->y + rg->h);
	       }
	  }
	if (evas_list_count(rg->list) > sd->regions.member_max)
	  rg = NULL;
     }
   _e_fm2_regions_eval(obj);
   for (l = sd->icons; l; l = l->next)
     {
        ic = l->data;
	if ((!ic->region->realized) && (ic->realized))
	  _e_fm2_icon_unrealize(ic);
     }
   _e_fm2_obj_icons_place(sd);
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(obj));
}

static void
_e_fm2_icons_place_icons(E_Fm2_Smart_Data *sd)
{
   Evas_List *l;
   E_Fm2_Icon *ic;
   Evas_Coord x, y, rh;

   x = 0; y = 0;
   rh = 0;
   for (l = sd->icons; l; l = l->next)
     {
	ic = l->data;
	if ((x > 0) && ((x + ic->w) > sd->w))
	  {
	     x = 0;
	     y += rh;
	     rh = 0;
	  }
	ic->x = x;
	ic->y = y;
	x += ic->w;
	if (ic->h > rh) rh = ic->h;
	if ((ic->x + ic->w) > sd->max.w) sd->max.w = ic->x + ic->w;
	if ((ic->y + ic->h) > sd->max.h) sd->max.h = ic->y + ic->h;
     }
}

static void
_e_fm2_icons_place_grid_icons(E_Fm2_Smart_Data *sd)
{
   Evas_List *l;
   E_Fm2_Icon *ic;
   Evas_Coord x, y, gw, gh;

   gw = 0; gh = 0;
   for (l = sd->icons; l; l = l->next)
     {
	ic = l->data;
	if (ic->w > gw) gw = ic->w;
	if (ic->h > gh) gh = ic->h;
     }
   x = 0; y = 0;
   for (l = sd->icons; l; l = l->next)
     {
	ic = l->data;
	if ((x > 0) && ((x + ic->w) > sd->w))
	  {
	     x = 0;
	     y += gh;
	  }
	ic->x = x + (gw - ic->w);
	ic->y = y + (gh - ic->h);
	x += gw;
	if ((ic->x + ic->w) > sd->max.w) sd->max.w = ic->x + ic->w;
	if ((ic->y + ic->h) > sd->max.h) sd->max.h = ic->y + ic->h;
     }
}

static void
_e_fm2_icons_place_custom_icons(E_Fm2_Smart_Data *sd)
{
   Evas_List *l;
   E_Fm2_Icon *ic;

   for (l = sd->icons; l; l = l->next)
     {
	ic = l->data;

	if (!ic->saved_pos)
	  {
	     /* FIXME: place using smart place fn */
	  }
	
	if ((ic->x + ic->w) > sd->max.w) sd->max.w = ic->x + ic->w;
	if ((ic->y + ic->h) > sd->max.h) sd->max.h = ic->y + ic->h;
     }
}

#if 0
static void
_e_fm2_icons_place_custom_grid_icons(E_Fm2_Smart_Data *sd)
{
   Evas_List *l;
   E_Fm2_Icon *ic;

   for (l = sd->icons; l; l = l->next)
     {
	ic = l->data;
	
	if (!ic->saved_pos)
	  {
	     /* FIXME: place using grid fn */
	  }
	
	if ((ic->x + ic->w) > sd->max.w) sd->max.w = ic->x + ic->w;
	if ((ic->y + ic->h) > sd->max.h) sd->max.h = ic->y + ic->h;
     }
}
#endif

static void
_e_fm2_icons_place_custom_smart_grid_icons(E_Fm2_Smart_Data *sd)
{
   Evas_List *l;
   E_Fm2_Icon *ic;

   for (l = sd->icons; l; l = l->next)
     {
	ic = l->data;
	
	if (!ic->saved_pos)
	  {
	     /* FIXME: place using smart grid fn */
	  }
	
	if ((ic->x + ic->w) > sd->max.w) sd->max.w = ic->x + ic->w;
	if ((ic->y + ic->h) > sd->max.h) sd->max.h = ic->y + ic->h;
     }
}

static void
_e_fm2_icons_place_list(E_Fm2_Smart_Data *sd)
{
   Evas_List *l;
   E_Fm2_Icon *ic;
   Evas_Coord x, y;
   int i;

   x = y = 0;
   for (i = 0, l = sd->icons; l; l = l->next, i++)
     {
	ic = l->data;
	
	ic->x = x;
	ic->y = y;
	if (sd->w > ic->min_w)
	  ic->w = sd->w;
	else
	  ic->w = ic->min_w;
	y += ic->h;
	ic->odd = (i & 0x01);
	if ((ic->x + ic->w) > sd->max.w) sd->max.w = ic->x + ic->w;
	if ((ic->y + ic->h) > sd->max.h) sd->max.h = ic->y + ic->h;
     }
   for (i = 0, l = sd->icons; l; l = l->next, i++)
     {
	ic = l->data;
	ic->w = sd->max.w;
     }
}

static void
_e_fm2_icons_place(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   /* take the icon list and find a location for them */
   sd->max.w = 0;
   sd->max.h = 0;
   switch (sd->config->view.mode)
     {
      case E_FM2_VIEW_MODE_ICONS:
	_e_fm2_icons_place_icons(sd);
	break;
      case E_FM2_VIEW_MODE_GRID_ICONS:
	_e_fm2_icons_place_grid_icons(sd);
	break;
      case E_FM2_VIEW_MODE_CUSTOM_ICONS:
	_e_fm2_icons_place_custom_icons(sd);
	break;
      case E_FM2_VIEW_MODE_CUSTOM_GRID_ICONS:
	_e_fm2_icons_place_custom_smart_grid_icons(sd);
	break;
      case E_FM2_VIEW_MODE_CUSTOM_SMART_GRID_ICONS:
	_e_fm2_icons_place_custom_smart_grid_icons(sd);
	break;
      case E_FM2_VIEW_MODE_LIST:
	_e_fm2_icons_place_list(sd);
	break;
      default:
	break;
     }
   /* tell our parent scrollview - if any, that we have changed */
   evas_object_smart_callback_call(sd->obj, "changed", NULL);
}

static void
_e_fm2_icons_free(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   _e_fm2_queue_free(obj);
   /* free all icons */
   while (sd->icons)
     {
	_e_fm2_icon_free(sd->icons->data);
        sd->icons = evas_list_remove_list(sd->icons, sd->icons);
     }
   sd->tmp.last_insert = NULL;
   E_FREE(sd->tmp.list_index);
}

static void
_e_fm2_regions_eval(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   Evas_List *l;
   E_Fm2_Region *rg;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_event_freeze(evas_object_evas_get(obj));
   edje_freeze();
   for (l = sd->regions.list; l; l = l->next)
     {
	rg = l->data;
	
	if (_e_fm2_region_visible(rg))
	  _e_fm2_region_realize(rg);
	else
	  _e_fm2_region_unrealize(rg);
     }
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(obj));
}

static void
_e_fm2_config_free(E_Fm2_Config *cfg)
{
   if (cfg->view.extra_file_source) evas_stringshare_del(cfg->view.extra_file_source);
   if (cfg->icon.key_hint) evas_stringshare_del(cfg->icon.key_hint);
   if (cfg->theme.background) evas_stringshare_del(cfg->theme.background);
   if (cfg->theme.frame) evas_stringshare_del(cfg->theme.frame);
   if (cfg->theme.icons) evas_stringshare_del(cfg->theme.icons);
   free(cfg);
}

/**************************/

static E_Fm2_Icon *
_e_fm2_icon_new(E_Fm2_Smart_Data *sd, const char *file)
{
   E_Fm2_Icon *ic;
   
   /* create icon */
   ic = E_NEW(E_Fm2_Icon, 1);
   ic->info.fm = sd->obj;
   ic->info.file = evas_stringshare_add(file);
   ic->sd = sd;
   if (!_e_fm2_icon_fill(ic))
     {
	evas_stringshare_del(ic->info.file);
	free(ic);
	return NULL;
     }
   return ic;
}

static void
_e_fm2_icon_unfill(E_Fm2_Icon *ic)
{
   if (ic->info.mime) evas_stringshare_del(ic->info.mime);
   if (ic->info.label) evas_stringshare_del(ic->info.label);
   if (ic->info.comment) evas_stringshare_del(ic->info.comment);
   if (ic->info.generic) evas_stringshare_del(ic->info.generic);
   if (ic->info.icon) evas_stringshare_del(ic->info.icon);
   if (ic->info.link) evas_stringshare_del(ic->info.link);
   if (ic->info.real_link) evas_stringshare_del(ic->info.real_link);
   if (ic->info.pseudo_dir) evas_stringshare_del(ic->info.pseudo_dir);
   ic->info.mime = NULL;
   ic->info.label = NULL;
   ic->info.comment = NULL;
   ic->info.generic = NULL;
   ic->info.icon = NULL;
   ic->info.link = NULL;
   ic->info.real_link = NULL;
   ic->info.pseudo_dir = NULL;
   ic->info.mount = 0;
   ic->info.pseudo_link = 0;
   ic->info.deleted = 0;
   ic->info.broken_link = 0;
}

static int
_e_fm2_icon_fill(E_Fm2_Icon *ic)
{
   Evas_Coord mw = 0, mh = 0;
   Evas_Object *obj, *obj2;
   char buf[4096], *lnk;
   const char *mime;
   
   snprintf(buf, sizeof(buf), "%s/%s", ic->sd->realpath, ic->info.file);
   lnk = ecore_file_readlink(buf);
   if (stat(buf, &(ic->info.statinfo)) == -1)
     {
	if (lnk)
	  ic->info.broken_link = 1;
	else
	  {
	     if (ic->sd->config->view.extra_file_source)
	       {
		  snprintf(buf, sizeof(buf), "%s/%s",
			   ic->sd->config->view.extra_file_source, ic->info.file);
		  if (stat(buf, &(ic->info.statinfo)) == -1)
		    return 0;
		  ic->info.pseudo_dir = evas_stringshare_add(ic->sd->config->view.extra_file_source);
		  ic->info.pseudo_link = 1;
	       }
	     else
	       return 0;
	  }
     }
   if (lnk)
     {
	if (lnk[0] == '/')
	  {
	     ic->info.link = evas_stringshare_add(lnk);
	     ic->info.real_link = evas_stringshare_add(lnk);
	  }
	else
	  {
	     char *rp;
	     
	     snprintf(buf, sizeof(buf), "%s/%s", ic->sd->realpath, lnk);
	     rp = ecore_file_realpath(buf);
	     if (rp)
	       {
		  ic->info.link = evas_stringshare_add(rp);
		  free(rp);
	       }
	     ic->info.real_link = evas_stringshare_add(lnk);
	  }
	free(lnk);
     }
   
   if (!ic->info.mime)
     {
	mime = e_fm_mime_filename_get(ic->info.file);
	if (mime) ic->info.mime = evas_stringshare_add(mime);
     }
   
   if ((e_util_glob_case_match(ic->info.file, "*.desktop")) ||
       (e_util_glob_case_match(ic->info.file, "*.directory")))
     _e_fm2_icon_desktop_load(ic);
   evas_event_freeze(evas_object_evas_get(ic->sd->obj));
   edje_freeze();
   switch (ic->sd->config->view.mode)
     {
      case E_FM2_VIEW_MODE_ICONS:
      case E_FM2_VIEW_MODE_GRID_ICONS:
      case E_FM2_VIEW_MODE_CUSTOM_ICONS:
      case E_FM2_VIEW_MODE_CUSTOM_GRID_ICONS:
      case E_FM2_VIEW_MODE_CUSTOM_SMART_GRID_ICONS:
	/* FIXME: need to define icon edjes. here goes:
	 * 
	 * fileman/icon/fixed
	 * fileman/icon/variable
	 * fileman/list/fixed
	 * fileman/list/variable
	 * fileman/list_odd/fixed
	 * fileman/list_odd/variable
	 * 
	 * and now list other things i will need
	 * 
	 * fileman/background
	 * fileman/selection
	 * fileman/scrollframe
	 *
	 */
	if ((!ic->sd->config->icon.fixed.w) || (!ic->sd->config->icon.fixed.h))
	  {
	     obj = ic->sd->tmp.obj;
	     if (!obj)
	       {
		  obj = edje_object_add(evas_object_evas_get(ic->sd->obj));
		  e_theme_edje_object_set(obj, "base/theme/fileman",
					  "e/fileman/icon/variable");
                  ic->sd->tmp.obj = obj;
	       }
	     _e_fm2_icon_label_set(ic, obj);
	     edje_object_size_min_calc(obj, &mw, &mh);
	  }
	ic->w = mw;
	ic->h = mh;
	if (ic->sd->config->icon.fixed.w) ic->w = ic->sd->config->icon.icon.w;
	if (ic->sd->config->icon.fixed.h) ic->h = ic->sd->config->icon.icon.h;
	ic->min_w = mw;
	ic->min_h = mh;
	break;
      case E_FM2_VIEW_MODE_LIST:
	  {
	     obj = ic->sd->tmp.obj;
	     if (!obj)
	       {
		  obj = edje_object_add(evas_object_evas_get(ic->sd->obj));
		  if (ic->sd->config->icon.fixed.w)
		    e_theme_edje_object_set(obj, "base/theme/fileman",
					    "e/fileman/list/fixed");
		  else
		    e_theme_edje_object_set(obj, "base/theme/fileman",
					    "e/fileman/list/variable");
		  ic->sd->tmp.obj = obj;
	       }
	     _e_fm2_icon_label_set(ic, obj);
	     obj2 = ic->sd->tmp.obj2;
	     if (!obj2)
	       {
		  obj2 = evas_object_rectangle_add(evas_object_evas_get(ic->sd->obj));
		  ic->sd->tmp.obj2 = obj2;
	       }
	     edje_extern_object_min_size_set(obj2, ic->sd->config->icon.list.w, ic->sd->config->icon.list.h);
	     edje_extern_object_max_size_set(obj2, ic->sd->config->icon.list.w, ic->sd->config->icon.list.h);
	     edje_object_part_swallow(obj, "e.swallow.icon", obj2);
	     edje_object_size_min_calc(obj, &mw, &mh);
	  }
	if (mw < ic->sd->w) ic->w = ic->sd->w;
	else ic->w = mw;
	ic->h = mh;
	ic->min_w = mw;
	ic->min_h = mh;
	break;
      default:
	break;
     }
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(ic->sd->obj));
   return 1;
}

static void
_e_fm2_icon_free(E_Fm2_Icon *ic)
{
   /* free icon, object data etc. etc. */
   if (ic->sd->drop_icon == ic)
     {
	/* FIXME: call hide call */
	ic->sd->drop_icon = NULL;
     }
   _e_fm2_icon_unrealize(ic);
   if (ic->menu)
     {
	e_menu_post_deactivate_callback_set(ic->menu, NULL, NULL);
	e_object_del(E_OBJECT(ic->menu));
	ic->menu = NULL;
     }
   if (ic->dialog)
     {
	e_object_del(E_OBJECT(ic->dialog));
	ic->dialog = NULL;
     }
   if (ic->entry_dialog)
     {
	e_object_del(E_OBJECT(ic->entry_dialog));
	ic->entry_dialog = NULL;
     }
   if (ic->info.file) evas_stringshare_del(ic->info.file);
   if (ic->info.mime) evas_stringshare_del(ic->info.mime);
   if (ic->info.label) evas_stringshare_del(ic->info.label);
   if (ic->info.comment) evas_stringshare_del(ic->info.comment);
   if (ic->info.generic) evas_stringshare_del(ic->info.generic);
   if (ic->info.icon) evas_stringshare_del(ic->info.icon);
   if (ic->info.link) evas_stringshare_del(ic->info.link);
   if (ic->info.real_link) evas_stringshare_del(ic->info.real_link);
   if (ic->info.pseudo_dir) evas_stringshare_del(ic->info.pseudo_dir);
   free(ic);
}

static void
_e_fm2_icon_realize(E_Fm2_Icon *ic)
{
   if (ic->realized) return;
   /* actually create evas objects etc. */
   ic->realized = 1;
   evas_event_freeze(evas_object_evas_get(ic->sd->obj));
   ic->obj = edje_object_add(evas_object_evas_get(ic->sd->obj));
   edje_object_freeze(ic->obj);
   evas_object_smart_member_add(ic->obj, ic->sd->obj);
   evas_object_stack_below(ic->obj, ic->sd->drop);
   if (ic->sd->config->view.mode == E_FM2_VIEW_MODE_LIST)
     {
        if (ic->sd->config->icon.fixed.w)
	  {
	     if (ic->odd)
	       e_theme_edje_object_set(ic->obj, "base/theme/widgets",
				       "e/fileman/list_odd/fixed");
	     else
	       e_theme_edje_object_set(ic->obj, "base/theme/widgets",
				       "e/fileman/list/fixed");
	  }
	else
	  {
	     if (ic->odd)
	       e_theme_edje_object_set(ic->obj, "base/theme/widgets",
				       "e/fileman/list_odd/variable");
	     else
	       e_theme_edje_object_set(ic->obj, "base/theme/widgets",
				       "e/fileman/list/variable");
	  }
     }
   else
     {
        if (ic->sd->config->icon.fixed.w)
	  e_theme_edje_object_set(ic->obj, "base/theme/fileman",
				  "e/fileman/icon/fixed");
	else
	  e_theme_edje_object_set(ic->obj, "base/theme/fileman",
				  "e/fileman/icon/variable");
     }
   _e_fm2_icon_label_set(ic, ic->obj);
   evas_object_clip_set(ic->obj, ic->sd->clip);
   evas_object_move(ic->obj,
		    ic->sd->x + ic->x - ic->sd->pos.x,
		    ic->sd->y + ic->y - ic->sd->pos.y);
   evas_object_resize(ic->obj, ic->w, ic->h);

   evas_object_event_callback_add(ic->obj, EVAS_CALLBACK_MOUSE_DOWN, _e_fm2_cb_icon_mouse_down, ic);
   evas_object_event_callback_add(ic->obj, EVAS_CALLBACK_MOUSE_UP, _e_fm2_cb_icon_mouse_up, ic);
   evas_object_event_callback_add(ic->obj, EVAS_CALLBACK_MOUSE_MOVE, _e_fm2_cb_icon_mouse_move, ic);
   
   _e_fm2_icon_icon_set(ic);
   
   edje_object_thaw(ic->obj);
   evas_event_thaw(evas_object_evas_get(ic->sd->obj));
   evas_object_show(ic->obj);

   if (ic->selected)
     {
	/* FIXME: need new signal to INSTANTLY activate - no anim */
	edje_object_signal_emit(ic->obj, "e,state,selected", "e");
	edje_object_signal_emit(ic->obj_icon, "e,state,selected", "e");
     }
}

static void
_e_fm2_icon_unrealize(E_Fm2_Icon *ic)
{
   if (!ic->realized) return;
   /* delete evas objects */
   ic->realized = 0;
   evas_object_del(ic->obj);
   ic->obj = NULL;
   evas_object_del(ic->obj_icon);
   ic->obj_icon = NULL;
}

static int
_e_fm2_icon_visible(E_Fm2_Icon *ic)
{
   /* return if the icon is visible */
   if (
       ((ic->x - ic->sd->pos.x) < (ic->sd->w + OVERCLIP)) &&
       ((ic->x + ic->w - ic->sd->pos.x) > (-OVERCLIP)) &&
       ((ic->y - ic->sd->pos.y) < (ic->sd->h + OVERCLIP)) &&
       ((ic->y + ic->h - ic->sd->pos.y) > (-OVERCLIP))
       )
     return 1;
   return 0;
}

static void
_e_fm2_icon_label_set(E_Fm2_Icon *ic, Evas_Object *obj)
{
   char buf[4096], *p;
   int len;

   if (ic->info.label)
     {
	edje_object_part_text_set(obj, "e.text.label", ic->info.label);
	return;
     }
   if ((ic->sd->config->icon.extension.show) ||
       (S_ISDIR(ic->info.statinfo.st_mode)))
     edje_object_part_text_set(obj, "e.text.label", ic->info.file);
   else
     {
	/* remove extension. handle double extensions like .tar.gz too
	 * also be fuzzy - up to 4 chars of extn is ok - eg .html but 5 or
	 * more is considered part of the name
	 */
	strncpy(buf, ic->info.file, sizeof(buf) - 2);
	buf[sizeof(buf) - 1] = 0;
	
	len = strlen(buf);
	p = strrchr(buf, '.');
	if ((p) && ((len - (p - buf)) < 6))
	  {
	     *p = 0;
	
	     len = strlen(buf);
	     p = strrchr(buf, '.');
	     if ((p) && ((len - (p - buf)) < 6)) *p = 0;
	  }
	edje_object_part_text_set(obj, "e.text.label", buf);
     }
}

static Evas_Object *
_e_fm2_icon_icon_direct_set(E_Fm2_Icon *ic, Evas_Object *o, void (*gen_func) (void *data, Evas_Object *obj, void *event_info), void *data, int force_gen)
{
   Evas_Object *oic;
   char buf[4096], *p;

   if (ic->info.icon)
     {
	/* custom icon */
	if (ic->info.icon[0] == '/')
	   {
	      /* path to icon file */
	       oic = e_icon_add(evas_object_evas_get(o));
	       e_icon_file_set(oic, ic->info.icon);
	       e_icon_fill_inside_set(oic, 1);
	   }
	else
	   {
	      /* theme icon */
	      oic = edje_object_add(evas_object_evas_get(o));
              e_util_edje_icon_set(oic, ic->info.icon);
	   }
	edje_object_part_swallow(o, "e.swallow.icon", oic);
        evas_object_show(oic);
	return oic;
     }
   if (S_ISDIR(ic->info.statinfo.st_mode))
     {
	oic = edje_object_add(evas_object_evas_get(o));
	e_theme_edje_object_set(oic, "base/theme/fileman",
				"e/icons/fileman/folder");
	edje_object_part_swallow(o, "e.swallow.icon", oic);
	evas_object_show(oic);
     }
   else
     {
	if (ic->info.mime)
	  {
	     const char *icon;
	     
	     icon = e_fm_mime_icon_get(ic->info.mime);
	     /* use mime type to select icon */
	     if (!icon)
	       {
		  oic = edje_object_add(evas_object_evas_get(o));
		  e_theme_edje_object_set(oic, "base/theme/fileman",
					  "e/icons/fileman/file");
	       }
	     else if (!strcmp(icon, "THUMB"))
	       {
		  if (ic->info.pseudo_link)
		    snprintf(buf, sizeof(buf), "%s/%s", ic->info.pseudo_dir, ic->info.file);
		  else
		    snprintf(buf, sizeof(buf), "%s/%s", ic->sd->realpath, ic->info.file);
		  
		  oic = e_thumb_icon_add(evas_object_evas_get(o));
		  e_thumb_icon_file_set(oic, buf, NULL);
		  e_thumb_icon_size_set(oic, 128, 128);
		  evas_object_smart_callback_add(oic, "e_thumb_gen",
						 gen_func, data);
		  _e_fm2_icon_thumb(ic, oic, force_gen);
	       }
	     else if (!strcmp(icon, "DESKTOP"))
	       {
		  E_App *app;
		  
		  if (ic->info.pseudo_link)
		    snprintf(buf, sizeof(buf), "%s/%s", ic->info.pseudo_dir, ic->info.file);
		  else
		    snprintf(buf, sizeof(buf), "%s/%s", ic->sd->realpath, ic->info.file);
		  app = e_app_new(buf, 0);
		  if (app)
		    {
		       oic = e_app_icon_add(app, evas_object_evas_get(o));
		       e_object_unref(E_OBJECT(app));
		    }
	       }
	     else if (!strncmp(icon, "e/icons/fileman/mime/", 21))
	       {
		  oic = edje_object_add(evas_object_evas_get(o));
		  if (!e_theme_edje_object_set(oic, 
					       "base/theme/fileman",
					       icon))
		    e_theme_edje_object_set(oic, "base/theme/fileman",
					    "e/icons/fileman/file");
	       }
	     else
	       {
		  p = strrchr(icon, '.');
		  if ((p) && (!strcmp(p, ".edj")))
		    {
		       oic = edje_object_add(evas_object_evas_get(o));
		       if (!edje_object_file_set(oic, icon, "icon"))
			 e_theme_edje_object_set(oic, "base/theme/fileman",
						 "e/icons/fileman/file");
		    }
		  else
		    {
		       oic = e_icon_add(evas_object_evas_get(o));
		       e_icon_file_set(oic, icon);
		    }
	       }
	  }
	else
	  {
	     if (ic->info.pseudo_link)
	       snprintf(buf, sizeof(buf), "%s/%s", ic->info.pseudo_dir, ic->info.file);
	     else
	       snprintf(buf, sizeof(buf), "%s/%s", ic->sd->realpath, ic->info.file);
	     /* fallback */
	     if (
		 (e_util_glob_case_match(ic->info.file, "*.edj"))
		 )
	       {
		  oic = e_thumb_icon_add(evas_object_evas_get(o));
		  if (ic->sd->config->icon.key_hint)
		    e_thumb_icon_file_set(oic, buf, ic->sd->config->icon.key_hint);
		  else
		    {
		       /* FIXME: There is probably a quicker way of doing this. */
		       if (edje_file_group_exists(buf, "icon"))
			 e_thumb_icon_file_set(oic, buf, "icon");
		       else if (edje_file_group_exists(buf, "e/desktop/background"))
			 e_thumb_icon_file_set(oic, buf, "e/desktop/background");
		       else if (edje_file_group_exists(buf, "e/init/splash"))
			 e_thumb_icon_file_set(oic, buf, "e/init/splash");
		    }
		  e_thumb_icon_size_set(oic, 128, 96);
		  evas_object_smart_callback_add(oic, "e_thumb_gen",
						 gen_func, data);
		  _e_fm2_icon_thumb(ic, oic, force_gen);
	       }
	     else if ((e_util_glob_case_match(ic->info.file, "*.desktop")) || 
		      (e_util_glob_case_match(ic->info.file, "*.directory")))
	       {
		  E_App *app;
		  
		  app = e_app_new(buf, 0);
		  if (app)
		    {
		       oic = e_app_icon_add(app, evas_object_evas_get(o));
		       e_object_unref(E_OBJECT(app));
		    }
	       }
	     else if (S_ISCHR(ic->info.statinfo.st_mode))
	       {
		  oic = edje_object_add(evas_object_evas_get(o));
		  e_theme_edje_object_set(oic, "base/theme/fileman",
					  "e/icons/fileman/file");
	       }
	     else if (S_ISBLK(ic->info.statinfo.st_mode))
	       {
		  oic = edje_object_add(evas_object_evas_get(o));
		  e_theme_edje_object_set(oic, "base/theme/fileman",
					  "e/icons/fileman/file");
	       }
	     else if (S_ISFIFO(ic->info.statinfo.st_mode))
	       {
		  oic = edje_object_add(evas_object_evas_get(o));
		  e_theme_edje_object_set(oic, "base/theme/fileman",
					  "e/icons/fileman/file");
	       }
	     else if (S_ISSOCK(ic->info.statinfo.st_mode))
	       {
		  oic = edje_object_add(evas_object_evas_get(o));
		  e_theme_edje_object_set(oic, "base/theme/fileman",
					  "e/icons/fileman/file");
	       }
	     else if (ecore_file_can_exec(buf))
	       {
		  oic = edje_object_add(evas_object_evas_get(o));
		  e_theme_edje_object_set(oic, "base/theme/fileman",
					  "e/icons/fileman/file");
	       }
	     else
	       {
		  oic = edje_object_add(evas_object_evas_get(o));
		  e_theme_edje_object_set(oic, "base/theme/fileman",
					  "e/icons/fileman/file");
	       }
	  }
	edje_object_part_swallow(o, "e.swallow.icon", oic);
	evas_object_show(oic);
     }
   return oic;
}

static void
_e_fm2_icon_icon_set(E_Fm2_Icon *ic)
{
   if (!ic->realized) return;
   ic->obj_icon = _e_fm2_icon_icon_direct_set(ic, ic->obj,
					      _e_fm2_cb_icon_thumb_gen,
					      ic, 0);
}

static void
_e_fm2_icon_thumb(E_Fm2_Icon *ic, Evas_Object *oic, int force)
{
   if ((force) ||
       ((_e_fm2_icon_visible(ic)) && 
	(!ic->sd->queue) && 
	(!ic->sd->sort_idler) &&
	(!ic->sd->scan_idler)))
     e_thumb_icon_begin(oic);
}

static void
_e_fm2_icon_select(E_Fm2_Icon *ic)
{
   if (ic->selected) return;
   ic->selected = 1;
   ic->last_selected = 1;
   if (ic->realized)
     {
	edje_object_signal_emit(ic->obj, "e,state,selected", "e");
	edje_object_signal_emit(ic->obj_icon, "e,state,selected", "e");
	evas_object_stack_below(ic->obj, ic->sd->drop);
//	evas_object_stack_below(ic->sd->drop, ic->sd->overlay);
//	evas_object_stack_below(ic->sd->drop_in, ic->sd->overlay);
     }
}

static void
_e_fm2_icon_deselect(E_Fm2_Icon *ic)
{
   if (!ic->selected) return;
   ic->selected = 0;
   ic->last_selected = 0;
   if (ic->realized)
     {
	edje_object_signal_emit(ic->obj, "e,state,unselected", "e");
	edje_object_signal_emit(ic->obj_icon, "e,state,unselected", "e");
     }
}

static const char *
_e_fm2_icon_desktop_url_eval(const char *val)
{
   const char *s;
   char *path, *p;
   
   if (strlen(val) < 6) return NULL;
   if (strncmp(val, "file:", 5)) return NULL;
   path = (char *)val + 5;
   p = e_util_shell_env_path_eval(path);
   if (!p) return NULL;
   s = evas_stringshare_add(p);
   free(p);
   return s;
}

static int
_e_fm2_icon_desktop_load(E_Fm2_Icon *ic)
{
   char buf[4096];
   Ecore_Desktop *desktop;
   
   if (ic->info.pseudo_link)
     snprintf(buf, sizeof(buf), "%s/%s", ic->info.pseudo_dir, ic->info.file);
   else
     snprintf(buf, sizeof(buf), "%s/%s", ic->sd->realpath, ic->info.file);

   desktop = ecore_desktop_get(buf, NULL);
   if (desktop)
     {
	if (desktop->name)     ic->info.label   = evas_stringshare_add(desktop->name);
	if (desktop->generic)  ic->info.generic = evas_stringshare_add(desktop->generic);
	if (desktop->comment)  ic->info.comment = evas_stringshare_add(desktop->comment);
	
	if (desktop->icon)
	  {
	     char *v;
	     
	     /* FIXME: Use a real icon size. */
	     v = desktop->icon_path;
// make it consistent and use the same icon everywhere	     
//	     v = ecore_desktop_icon_find(desktop->icon, NULL, e_config->icon_theme);
	     if (v)
	       {
		  ic->info.icon = evas_stringshare_add(v);
//		  free(v);
	       }
	  }
	
	if (desktop->type)
	  {
	     if (!strcmp(desktop->type, "Mount"))
	       {
		  ic->info.mount = 1;
		  if (desktop->URL)
		    ic->info.link = _e_fm2_icon_desktop_url_eval(desktop->URL);
	       }
	     else if (!strcmp(desktop->type, "Link"))
	       {
		  if (desktop->URL)
		    ic->info.link = _e_fm2_icon_desktop_url_eval(desktop->URL);
	       }
	     else if (!strcmp(desktop->type, "Application"))
	       {
	       }
	     else
	       goto error;
	  }
     }
   
   return 1;
   error:
   if (ic->info.label) evas_stringshare_del(ic->info.label);
   if (ic->info.comment) evas_stringshare_del(ic->info.comment);
   if (ic->info.generic) evas_stringshare_del(ic->info.generic);
   if (ic->info.icon) evas_stringshare_del(ic->info.icon);
   if (ic->info.link) evas_stringshare_del(ic->info.link);
   if (ic->info.pseudo_dir) evas_stringshare_del(ic->info.pseudo_dir);
   ic->info.label = NULL;
   ic->info.comment = NULL;
   ic->info.generic = NULL;
   ic->info.icon = NULL;
   ic->info.link = NULL;
   ic->info.pseudo_dir = NULL;
   return 0;
}

/**************************/
static E_Fm2_Region *
_e_fm2_region_new(E_Fm2_Smart_Data *sd)
{
   E_Fm2_Region *rg;
   
   rg = E_NEW(E_Fm2_Region, 1);
   rg->sd = sd;
   return rg;
}

static void
_e_fm2_region_free(E_Fm2_Region *rg)
{
   E_Fm2_Icon *ic;
   
   while (rg->list)
     {
	ic = rg->list->data;
	ic->region = NULL;
	rg->list = evas_list_remove_list(rg->list, rg->list);
     }
   free(rg);
}

static void
_e_fm2_region_realize(E_Fm2_Region *rg)
{
   Evas_List *l;
   E_Fm2_Icon *ic;
   
   if (rg->realized) return;
   /* actually create evas objects etc. */
   rg->realized = 1;
   edje_freeze();
   for (l = rg->list; l; l = l->next) _e_fm2_icon_realize(l->data);
   for (l = rg->list; l; l = l->next)
     {
	ic = l->data;
	if (ic->selected)
	  evas_object_stack_below(ic->obj, ic->sd->drop);
     }
   edje_thaw();
}

static void
_e_fm2_region_unrealize(E_Fm2_Region *rg)
{
   Evas_List *l;
   
   if (!rg->realized) return;
   /* delete evas objects */
   rg->realized = 0;
   edje_freeze();
   for (l = rg->list; l; l = l->next) _e_fm2_icon_unrealize(l->data);
   edje_thaw();
}

static int
_e_fm2_region_visible(E_Fm2_Region *rg)
{
   /* return if the icon is visible */
   if (
       ((rg->x - rg->sd->pos.x) < (rg->sd->w + OVERCLIP)) &&
       ((rg->x + rg->w - rg->sd->pos.x) > (-OVERCLIP)) &&
       ((rg->y - rg->sd->pos.y) < (rg->sd->h + OVERCLIP)) &&
       ((rg->y + rg->h - rg->sd->pos.y) > (-OVERCLIP))
       )
     return 1;
   return 0;
}

static void
_e_fm2_icon_make_visible(E_Fm2_Icon *ic)
{
   if (ic->sd->config->view.mode == E_FM2_VIEW_MODE_LIST)
     {
	if (
	    ((ic->y - ic->sd->pos.y) >= 0) &&
	    ((ic->y + ic->h - ic->sd->pos.y) <= (ic->sd->h))
	    )
	  return;
	if ((ic->y - ic->sd->pos.y) < 0)
	  e_fm2_pan_set(ic->sd->obj, ic->sd->pos.x, ic->y);
	else
	  e_fm2_pan_set(ic->sd->obj, ic->sd->pos.x, ic->y - ic->sd->h + ic->h);
     }
   else
     {
	Evas_Coord x, y;
	
	if (
	    ((ic->y - ic->sd->pos.y) >= 0) &&
	    ((ic->y + ic->h - ic->sd->pos.y) <= (ic->sd->h)) &&
	    ((ic->x - ic->sd->pos.x) >= 0) &&
	    ((ic->x + ic->w - ic->sd->pos.x) <= (ic->sd->w))
	    )
	  return;
	x = ic->sd->pos.x;
	if ((ic->x - ic->sd->pos.x) < 0)
	  x = ic->x;
	else if ((ic->x + ic->w - ic->sd->pos.x) > (ic->sd->w))
	  x = ic->x + ic->w - ic->sd->w;
	y = ic->sd->pos.y;
	if ((ic->y - ic->sd->pos.y) < 0)
	  y = ic->y;
	else if ((ic->y + ic->h - ic->sd->pos.y) > (ic->sd->h))
	  y = ic->y + ic->h - ic->sd->h;
	e_fm2_pan_set(ic->sd->obj, x, y);
     }
   evas_object_smart_callback_call(ic->sd->obj, "pan_changed", NULL);
}

static void
_e_fm2_icon_desel_any(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   Evas_List *l;
   E_Fm2_Icon *ic;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   for (l = sd->icons; l; l = l->next)
     {
	ic = l->data;
	if (ic->selected) _e_fm2_icon_deselect(ic);
     }
}

static E_Fm2_Icon *
_e_fm2_icon_first_selected_find(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   Evas_List *l;
   E_Fm2_Icon *ic;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return NULL;
   for (l = sd->icons; l; l = l->next)
     {
	ic = l->data;
	if (ic->selected) return ic;
     }
   return NULL;
}

static void
_e_fm2_icon_sel_first(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   E_Fm2_Icon *ic;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (!sd->icons) return;
   _e_fm2_icon_desel_any(obj);
   ic = sd->icons->data;
   _e_fm2_icon_select(ic);
   evas_object_smart_callback_call(sd->obj, "selection_change", NULL);
   _e_fm2_icon_make_visible(ic);
}

static void
_e_fm2_icon_sel_last(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   E_Fm2_Icon *ic;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (!sd->icons) return;
   _e_fm2_icon_desel_any(obj);
   ic = evas_list_last(sd->icons)->data;
   _e_fm2_icon_select(ic);
   evas_object_smart_callback_call(sd->obj, "selection_change", NULL);
   _e_fm2_icon_make_visible(ic);
}

static void
_e_fm2_icon_sel_prev(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   Evas_List *l;
   E_Fm2_Icon *ic;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (!sd->icons) return;
   for (l = sd->icons; l; l = l->next)
     {
	ic = l->data;
	if (ic->selected)
	  {
	     if (!l->prev) return;
	     ic = l->prev->data;
	     break;
	  }
	ic = NULL;
     }
   if (!ic)
     {
	_e_fm2_icon_sel_last(obj);
	return;
     }
   _e_fm2_icon_desel_any(obj);
   _e_fm2_icon_select(ic);
   evas_object_smart_callback_call(sd->obj, "selection_change", NULL);
   _e_fm2_icon_make_visible(ic);
}

static void
_e_fm2_icon_sel_next(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   Evas_List *l;
   E_Fm2_Icon *ic;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (!sd->icons) return;
   for (l = sd->icons; l; l = l->next)
     {
	ic = l->data;
	if (ic->selected)
	  {
	     if (!l->next) return;
	     ic = l->next->data;
	     break;
	  }
	ic = NULL;
     }
   if (!ic)
     {
	_e_fm2_icon_sel_first(obj);
	return;
     }
   _e_fm2_icon_desel_any(obj);
   _e_fm2_icon_select(ic);
   evas_object_smart_callback_call(sd->obj, "selection_change", NULL);
   _e_fm2_icon_make_visible(ic);
}

/* FIXME: prototype */
static void
_e_fm2_typebuf_show(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   E_FREE(sd->typebuf.buf);
   sd->typebuf.buf = strdup("");
   edje_object_part_text_set(sd->overlay, "e.text.typebuf_label", sd->typebuf.buf);
   edje_object_signal_emit(sd->overlay, "e,state,typebuf,start", "e");
   sd->typebuf_visible = 1;
}

static void
_e_fm2_typebuf_hide(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   E_FREE(sd->typebuf.buf);
   edje_object_signal_emit(sd->overlay, "e,state,typebuf,stop", "e");
   sd->typebuf_visible = 0;
}

static void
_e_fm2_typebuf_history_prev(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   /* FIXME: do */
}

static void
_e_fm2_typebuf_history_next(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   /* FIXME: do */
}

static void
_e_fm2_typebuf_run(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   E_Fm2_Icon *ic;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   _e_fm2_typebuf_hide(obj);
   ic = _e_fm2_icon_first_selected_find(obj);
   if (ic)
     {
	if ((S_ISDIR(ic->info.statinfo.st_mode)) && 
	    (ic->sd->config->view.open_dirs_in_place) &&
	    (!ic->sd->config->view.no_subdir_jump) &&
	    (!ic->sd->config->view.single_click)
	    )
	  {
	     char buf[4096], *dev = NULL;
	     
	     if (ic->sd->dev) dev = strdup(ic->sd->dev);
	     if (ic->info.pseudo_link)
	       snprintf(buf, sizeof(buf), "%s/%s", ic->info.pseudo_dir, ic->info.file);
	     else
	       snprintf(buf, sizeof(buf), "%s/%s", ic->sd->path, ic->info.file);
	     e_fm2_path_set(ic->sd->obj, dev, buf);
	     E_FREE(dev);
	  }
	else
	  {
	     evas_object_smart_callback_call(ic->sd->obj, "selected", NULL);
	  }
     }
}

static void
_e_fm2_typebuf_match(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   Evas_List *l;
   E_Fm2_Icon *ic;
   char *tb;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (!sd->typebuf.buf) return;
   if (!sd->icons) return;
   _e_fm2_icon_desel_any(obj);
   tb = malloc(strlen(sd->typebuf.buf) + 2);
   if (!tb) return;
   strcpy(tb, sd->typebuf.buf);
   strcat(tb, "*");
   for (l = sd->icons; l; l = l->next)
     {
	ic = l->data;
	if (
	    ((ic->info.label) &&
	     (e_util_glob_case_match(ic->info.label, tb))) ||
	    ((ic->info.file) &&
	     (e_util_glob_case_match(ic->info.file, tb)))
	    )
	  {
	     _e_fm2_icon_select(ic);
	     evas_object_smart_callback_call(sd->obj, "selection_change", NULL);
	     _e_fm2_icon_make_visible(ic);
	     break;
	  }
     }
   free(tb);
}

static void
_e_fm2_typebuf_complete(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   /* FIXME: do */
   _e_fm2_typebuf_match(obj);
}

static void
_e_fm2_typebuf_char_append(Evas_Object *obj, const char *ch)
{
   E_Fm2_Smart_Data *sd;
   char *ts;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (!sd->typebuf.buf) return;
   ts = malloc(strlen(sd->typebuf.buf) + strlen(ch) + 1);
   if (!ts) return;
   strcpy(ts, sd->typebuf.buf);
   strcat(ts, ch);
   free(sd->typebuf.buf);
   sd->typebuf.buf = ts;
   _e_fm2_typebuf_match(obj);
   edje_object_part_text_set(sd->overlay, "e.text.typebuf_label", sd->typebuf.buf);
}

static void
_e_fm2_typebuf_char_backspace(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   char *ts;
   int len, p, dec;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (!sd->typebuf.buf) return;
   len = strlen(sd->typebuf.buf);
   if (len == 0)
     {
	_e_fm2_typebuf_hide(obj);
	return;
     }
   p = evas_string_char_prev_get(sd->typebuf.buf, len, &dec);
   if (p >= 0) sd->typebuf.buf[p] = 0;
   ts = strdup(sd->typebuf.buf);
   if (!ts) return;
   free(sd->typebuf.buf);
   sd->typebuf.buf = ts;
   _e_fm2_typebuf_match(obj);
   edje_object_part_text_set(sd->overlay, "e.text.typebuf_label", sd->typebuf.buf);
}

/**************************/

/* FIXME: prototype + reposition + implement */
static void
_e_fm2_dnd_drop_configure(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (!sd->drop_icon) return;
   if (sd->drop_after == -1)
     {
	evas_object_move(sd->drop_in,
			 sd->x + sd->drop_icon->x - sd->pos.x,
			 sd->y + sd->drop_icon->y - sd->pos.y);
	evas_object_resize(sd->drop_in, sd->drop_icon->w, sd->drop_icon->h);
     }
   else if (sd->drop_after)
     {
	evas_object_move(sd->drop,
			 sd->x + sd->drop_icon->x - sd->pos.x,
			 sd->y + sd->drop_icon->y - sd->pos.y + sd->drop_icon->h - 1);
	evas_object_resize(sd->drop, sd->drop_icon->w, 2);
     }
   else
     {
	evas_object_move(sd->drop,
			 sd->x + sd->drop_icon->x - sd->pos.x,
			 sd->y + sd->drop_icon->y - sd->pos.y - 1);
	evas_object_resize(sd->drop, sd->drop_icon->w, 2);
     }
}

/* FIXME: prototype + reposition + implement */
static void
_e_fm2_dnd_drop_all_show(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (sd->drop_show)
     {
	edje_object_signal_emit(sd->drop, "e,state,unselected", "e");
	sd->drop_show = 0;
     }
   if (sd->drop_in_show)
     {
	edje_object_signal_emit(sd->drop_in, "e,state,unselected", "e");
	sd->drop_in_show = 0;
     }
   if (!sd->drop_all)
     {
	printf("DISP DROP ALL SHOW\n");
	edje_object_signal_emit(sd->overlay, "e,state,drop,start", "e");
	sd->drop_all = 1;
     }
   sd->drop_icon = NULL;
   sd->drop_after = 0;
}

/* FIXME: prototype + reposition + implement */
static void
_e_fm2_dnd_drop_all_hide(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (sd->drop_all)
     {
	printf("DISP DROP ALL HIDE\n");
	edje_object_signal_emit(sd->overlay, "e,state,drop,stop", "e");
	sd->drop_all = 0;
     }
}

/* FIXME: prototype + reposition + implement */
static void
_e_fm2_dnd_drop_show(E_Fm2_Icon *ic, int after)
{
   int emit = 0;
   
   if ((ic->sd->drop_icon == ic) &&
       (ic->sd->drop_after == after)) return;
   if (((ic->sd->drop_icon) && (!ic)) ||
       ((!ic->sd->drop_icon) && (ic)) ||
       ((after < 0) && (ic->sd->drop_after >= 0)) ||
       ((after >= 0) && (ic->sd->drop_after < 0)))
     emit = 1;
   ic->sd->drop_icon = ic;
   ic->sd->drop_after = after;
   if (emit)
     {  
	if (ic->sd->drop_after != -1)
	  {
	     printf("DISP DROP ON drop\n");
	     edje_object_signal_emit(ic->sd->drop_in, "e,state,unselected", "e");
	     edje_object_signal_emit(ic->sd->drop, "e,state,selected", "e");
	     ic->sd->drop_in_show = 0;
	     ic->sd->drop_show = 1;
	  }
	else
	  {
	     printf("DISP DROP ON drop_in\n");
	     edje_object_signal_emit(ic->sd->drop, "e,state,unselected", "e");
	     edje_object_signal_emit(ic->sd->drop_in, "e,state,selected", "e");
	     ic->sd->drop_in_show = 1;
	     ic->sd->drop_show = 0;
	  }
     }
   _e_fm2_dnd_drop_all_hide(ic->sd->obj);
   _e_fm2_dnd_drop_configure(ic->sd->obj);
}

/* FIXME: prototype + reposition + implement */
static void
_e_fm2_dnd_drop_hide(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   printf("DISP DROP OFF BOTH\n");
   if (sd->drop_show)
     {
	edje_object_signal_emit(sd->drop, "e,state,unselected", "e");
	sd->drop_show = 0;
     }
   if (sd->drop_in_show)
     {
	edje_object_signal_emit(sd->drop_in, "e,state,unselected", "e");
	sd->drop_in_show = 0;
     }
   sd->drop_icon = NULL;
   sd->drop_after = 0;
}

/* FIXME: prototype + reposition + implement */
static void
_e_fm2_dnd_finish(Evas_Object *obj, int refresh)
{
   E_Fm2_Smart_Data *sd;
   E_Fm2_Icon *ic;
   Evas_List *l;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (!sd->drag) return;
   sd->drag = 0;
   for (l = sd->icons; l; l = l->next)
     {
	ic = l->data;
	ic->drag.dnd = 0;
     }
   if (refresh) e_fm2_refresh(obj);
}

static void
_e_fm2_cb_dnd_enter(void *data, const char *type, void *event)
{
   E_Fm2_Smart_Data *sd;
   E_Event_Dnd_Enter *ev;
   
   sd = data;
   if (!type) return;
   if (strcmp(type, "text/uri-list")) return;
   ev = (E_Event_Dnd_Enter *)event;
   printf("DND IN %i %i\n", ev->x, ev->y);
}
 
static void
_e_fm2_cb_dnd_move(void *data, const char *type, void *event)
{
   E_Fm2_Smart_Data *sd;
   E_Event_Dnd_Move *ev;
   E_Fm2_Icon *ic;
   Evas_List *l;
   
   sd = data;
   if (!type) return;
   if (strcmp(type, "text/uri-list")) return;
   ev = (E_Event_Dnd_Move *)event;
   printf("DND MOVE %i %i\n", ev->x, ev->y);
   for (l = sd->icons; l; l = l->next)
     {
	ic = l->data;
	if (E_INSIDE(ev->x, ev->y, ic->x - ic->sd->pos.x, ic->y - ic->sd->pos.y, ic->w, ic->h))
	  {
	     printf("OVER %s\n", ic->info.file);
	     if (ic->drag.dnd) return;
	     /* if list view */
	     if (ic->sd->config->view.mode == E_FM2_VIEW_MODE_LIST)
	       {
		  /* if there is a .order file - we can re-order files */
//		  if (ic->sd->order_file)
		  if (1)
		    {
		       /* if dir: */
                       if ((S_ISDIR(ic->info.statinfo.st_mode)) &&
			   (!ic->sd->config->view.no_subdir_drop))
			 {
			    /* if bottom 25% or top 25% then insert between prev or next */
			    /* if in middle 50% then put in dir */
			    if (ev->y <= (ic->y - ic->sd->pos.y + (ic->h / 4)))
			      {
				 _e_fm2_dnd_drop_show(ic, 0);
				 printf("DDD 0\n");
			      }
			    else if (ev->y > (ic->y - ic->sd->pos.y + ((ic->h * 3) / 4)))
			      {
				 _e_fm2_dnd_drop_show(ic, 1);
				 printf("DDD 1\n");
			      }
			    else
			      {
				 _e_fm2_dnd_drop_show(ic, -1);
				 printf("DDD -1\n");
			      }
			 }
		       else
			 {
			    /* if top 50% or bottom 50% then insert between prev or next */
			    if (ev->y <= (ic->y - ic->sd->pos.y + (ic->h / 2)))
			      _e_fm2_dnd_drop_show(ic, 0);
			    else
			      _e_fm2_dnd_drop_show(ic, 1);
			 }
		    }
		  /* we can only drop into subdirs */
		  else
		    {
		       /* if it's over a dir - hilight as it will be dropped in */
                       if ((S_ISDIR(ic->info.statinfo.st_mode)) &&
			   (!ic->sd->config->view.no_subdir_drop))
			 _e_fm2_dnd_drop_show(ic, -1);
		       else
			 _e_fm2_dnd_drop_hide(sd->obj);
		    }
	       }
	     else
	       {
		  /* FIXME: icon view mode */
	       }
	     return;
	  }
     }
   /* FIXME: not over icon - is it within the fm view? if so drop there */
   if (E_INSIDE(ev->x, ev->y, 0, 0, sd->w, sd->h))
     {
	/* if listview - it is now after last file */
	if (sd->config->view.mode == E_FM2_VIEW_MODE_LIST)
	  {
	     /* if there is a .order file - we can re-order files */
	     if (sd->order_file)
	       {
		  ic = evas_list_data(evas_list_last(sd->icons));
		  if (ic)
		    {
		       if (!ic->drag.dnd)
			 _e_fm2_dnd_drop_show(ic, 1);
		       else
			 _e_fm2_dnd_drop_all_show(sd->obj);
		    }
		  else
		    _e_fm2_dnd_drop_all_show(sd->obj);
	       }
	     else
	       _e_fm2_dnd_drop_all_show(sd->obj);
	  }
	else
	  {
	     /* FIXME: icon view mode */
	  }
	return;
     }
   /* outside fm view */
   _e_fm2_dnd_drop_hide(sd->obj);
}

static void
_e_fm2_cb_dnd_leave(void *data, const char *type, void *event)
{
   E_Fm2_Smart_Data *sd;
   E_Event_Dnd_Leave *ev;
   
   sd = data;
   if (!type) return;
   if (strcmp(type, "text/uri-list")) return;
   ev = (E_Event_Dnd_Leave *)event;
   printf("DND LEAVE %i %i\n", ev->x, ev->y);
   _e_fm2_dnd_drop_hide(sd->obj);
   _e_fm2_dnd_drop_all_hide(sd->obj);
}
 
static void
_e_fm2_cb_dnd_drop(void *data, const char *type, void *event)
{
   E_Fm2_Smart_Data *sd;
   E_Event_Dnd_Drop *ev;
   Evas_List *fsel, *l, *ll;
   char buf[4096], *fl, *d;

   sd = data;
   if (!type) return;
   if (strcmp(type, "text/uri-list")) return;
   ev = (E_Event_Dnd_Drop *)event;
   fsel = ev->data;
   printf("DROP: %i %i\n", ev->x, ev->y);
   for (l = fsel; l; l = l->next)
     {
	fl = l->data;
	printf("  %s\n", fl);
     }
   /* note - logic.
    * if drop file prefix path matches extra_file_source then it can be
    * and indirect link - dont MOVE the file just add filename to list.
    * if not literally move the file in. if move can't work - try a copy.
    * on a literal move find any fm views for the dir of the dropped file
    * and refresh those, as well as refresh current target fm dir
    */
   if (sd->drop_all) /* drop arbitarily into the dir */
     {
	printf("drop all\n");
	/* move file into this fm dir */
	for (ll = fsel; ll; ll = ll->next)
	  {
	     d = ecore_file_get_dir(ll->data);
	     /* get the dir of each file */
	     if (d)
	       {
		  /* if the file is not in the target dir */
		  if (strcmp(d, sd->realpath))
		    {
		       /* if the file is not a pseudo-link */
		       if (e_util_strcmp(sd->config->view.extra_file_source, d))
			 {
			    /* move the file into the subdir */
			    snprintf(buf, sizeof(buf), "%s/%s",
				     sd->realpath, ecore_file_get_file(ll->data));
			    e_fm2_fop_move_add(sd->obj,
					       ll->data, buf,
					       NULL, 0, 1);
//			    if (ecore_file_exists(buf))
//			      {
//				 /* FIXME: error - file exists */
//			      }
//			    else
//			      {
/*FOPME*/
//				 if (ecore_file_mv(ll->data, buf))
//				   _e_fm2_live_file_add(sd->obj,
//							ecore_file_get_file(ll->data),
//							NULL, 0);
//			      }
			 }
		       else
			 _e_fm2_live_file_add(sd->obj,
					      ecore_file_get_file(ll->data),
					      NULL, 0);
		    }
		  else
		    {
		       /* file is in target dir - move into subdir */
		       snprintf(buf, sizeof(buf), "%s/%s",
				sd->realpath, ecore_file_get_file(ll->data));
		       e_fm2_fop_move_add(sd->obj,
					  ll->data, buf,
					  NULL, 0, 1);
//		       if (ecore_file_exists(buf))
//			 {
//			    /* FIXME: error - file exists */
//			 }
//		       else
//			 {
/*FOPME*/
//			    if (ecore_file_mv(ll->data, buf))
//			      _e_fm2_live_file_add(sd->obj,
//						   ecore_file_get_file(ll->data),
//						   NULL, 0);
//			 }
		    }
		  free(d);
	       }
	  }
     }
   else if (sd->drop_icon) /* inot or before/after an icon */
     {
	printf("drop icon\n");
	if (sd->drop_after == -1) /* put into subdir in icon */
	  {
	     /* move file into dir that this icon is for */
	     for (ll = fsel; ll; ll = ll->next)
	       {
		  /* move the file into the subdir */
		  snprintf(buf, sizeof(buf), "%s/%s/%s",
			   sd->realpath, sd->drop_icon->info.file, ecore_file_get_file(ll->data));
		  printf("mv %s %s\n", (char *)ll->data, buf);
		  e_fm2_fop_move_add(sd->obj,
				     ll->data, buf,
				     NULL, 0, 0);
//		  if (ecore_file_exists(buf))
//		    {
//		       /* FIXME: error - file exists */
//		    }
//		  else
///*FOPME*/		    ecore_file_mv(ll->data, buf);
	       }
	  }
	else
	  {
	     if (sd->config->view.mode == E_FM2_VIEW_MODE_LIST) /* list */
	       {
		  if (sd->order_file) /* there is an order file */
		    {
		       for (ll = fsel; ll; ll = ll->next)
			 {
			    snprintf(buf, sizeof(buf), "%s/%s",
				     sd->realpath, ecore_file_get_file(ll->data));
			    d = ecore_file_get_dir(ll->data);
			    if (d)
			      {
				 if (!strcmp(sd->realpath, d))
				   {
				      _e_fm2_live_file_del(sd->obj,
							   ecore_file_get_file(ll->data));
				   }
				 else
				   {
				      if (sd->config->view.link_drop)
					e_fm2_fop_link_add(sd->obj, ll->data, buf);
				      else
					e_fm2_fop_move_add(sd->obj,
							   ll->data, buf,
							   NULL, 0, 0);
				   }
				 free(d);
			      }
//			    d = ecore_file_get_dir(ll->data);
//			    if (d)
//			      {
//				 if (!strcmp(d, sd->realpath))
//				   {
//				      printf("listrm %s\n", ecore_file_get_file(ll->data));
//				      _e_fm2_live_file_del(sd->obj, ecore_file_get_file(ll->data));
//				   }
//				 else
//				   {
///*FOPME*/				      ecore_file_symlink(ll->data, buf);
//				   }
//				 free(d);
//			      }
			 }
		       if (sd->drop_after == 0)
			 {
			    for (ll = evas_list_last(fsel); ll; ll = ll->prev)
			      {
				 e_fm2_fop_add_add(sd->obj, ll->data, sd->drop_icon->info.file, 0);
//				 printf("listadd %s, before %s\n", ecore_file_get_file(ll->data), sd->drop_icon->info.file);
//				 _e_fm2_live_file_add(sd->obj,
//						      ecore_file_get_file(ll->data),
//						      sd->drop_icon->info.file, 0);
			      }
			 }
		       else
			 {
			    for (ll = fsel; ll; ll = ll->next)
			      {
				 e_fm2_fop_add_add(sd->obj, ll->data, sd->drop_icon->info.file, 1);
//				 printf("listadd %s, after %s\n", ecore_file_get_file(ll->data), sd->drop_icon->info.file);
//				 _e_fm2_live_file_add(sd->obj,
//						      ecore_file_get_file(ll->data),
//						      sd->drop_icon->info.file, 1);
			      }
			 }
		    }
		  else /* no order file */
		    {
		       for (ll = fsel; ll; ll = ll->next)
			 {
			    /* move the file into the subdir */
			    snprintf(buf, sizeof(buf), "%s/%s",
				     sd->realpath, ecore_file_get_file(ll->data));
			    printf("mv %s %s\n", (char *)ll->data, buf);
			    e_fm2_fop_move_add(sd->obj,
					       ll->data, buf,
					       NULL, 0, 1);
//			    if (ecore_file_exists(buf))
//			      {
//				 /* FIXME: error - file exists */
//			      }
//			    else
//			      {
///*FOPME*/				 if (ecore_file_mv(ll->data, buf))
//				   _e_fm2_live_file_add(sd->obj,
//							ecore_file_get_file(ll->data),
//							NULL, 0);
//			      }
			 }
		    }
	       }
	  }
     }
   _e_fm2_dnd_drop_hide(sd->obj);
   _e_fm2_dnd_drop_all_hide(sd->obj);
   for (l = _e_fm2_list; l; l = l->next)
     _e_fm2_dnd_finish(l->data, 0);
}

/* FIXME: prototype */
static void
_e_fm2_mouse_1_handler(E_Fm2_Icon *ic, int up, Evas_Modifier *modifiers)
{
   Evas_List *l;
   E_Fm2_Icon *ic2;
   int multi_sel = 0, range_sel = 0, seen = 0, sel_change = 0;

   if (ic->sd->config->selection.windows_modifiers)
     {
	if (evas_key_modifier_is_set(modifiers, "Shift"))
	  range_sel = 1;
	else if (evas_key_modifier_is_set(modifiers, "Control"))
	  multi_sel = 1;
     }
   else
     {
	if (evas_key_modifier_is_set(modifiers, "Control"))
	  range_sel = 1;
	else if (evas_key_modifier_is_set(modifiers, "Shift"))
	  multi_sel = 1;
     }
   if (ic->sd->config->selection.single)
     {
	multi_sel = 0;
	range_sel = 0;
     }
   if (range_sel)
     {
	/* find last selected - if any, and select all icons between */
	for (l = ic->sd->icons; l; l = l->next)
	  {
	     ic2 = l->data;
	     if (ic2 == ic) seen = 1;
	     if (ic2->last_selected)
	       {
		  ic2->last_selected = 0;
		  if (seen)
		    {
		       for (; (l) && (l->data != ic); l = l->prev)
			 {
			    ic2 = l->data;
			    if (!ic2->selected) sel_change = 1;
			    _e_fm2_icon_select(ic2);
			    ic2->last_selected = 0;
			 }
		    }
		  else
		    {
		       for (; (l) && (l->data != ic); l = l->next)
			 {
			    ic2 = l->data;
			    if (!ic2->selected) sel_change = 1;
			    _e_fm2_icon_select(ic2);
			    ic2->last_selected = 0;
			 }
		    }
		  break;
	       }
	  }
     }
   else if (!multi_sel)
     {
	/* desel others */
	for (l = ic->sd->icons; l; l = l->next)
	  {
	     ic2 = l->data;
	     if (ic2 != ic)
	       {
		  if (ic2->selected)
		    {
		       _e_fm2_icon_deselect(ic2);
		       sel_change = 1;
		    }
	       }
	  }
     }
   else
     {
	if (!up)
	  {
	     for (l = ic->sd->icons; l; l = l->next)
	       {
		  ic2 = l->data;
		  ic2->last_selected = 0;
	       }
	  }
     }
   if ((multi_sel) && (ic->selected))
     {
	if ((up) && (!ic->drag.dnd) && (!ic->down_sel))
	  {
	     sel_change = 1;
	     _e_fm2_icon_deselect(ic);
	  }
     }
   else
     {
	if (!up)
	  {
	     if (!ic->selected) sel_change = 1;
	     _e_fm2_icon_select(ic);
	     ic->down_sel = 1;
	     ic->last_selected = 1;
	  }

     }
   if (sel_change)
     evas_object_smart_callback_call(ic->sd->obj, "selection_change", NULL);
   if ((!(S_ISDIR(ic->info.statinfo.st_mode)) ||
	(ic->sd->config->view.no_subdir_jump)) &&
       (ic->sd->config->view.single_click)
       )
     {
	evas_object_smart_callback_call(ic->sd->obj, "selected", NULL);
     }
}

static void
_e_fm2_cb_icon_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   E_Fm2_Icon *ic;

   ic = data;
   ev = event_info;
   if ((ev->button == 1) && (ev->flags & EVAS_BUTTON_DOUBLE_CLICK))
     {
	/* if its a directory && open dirs in-place is set then change the dir
	 * to be the dir + file */
	if ((S_ISDIR(ic->info.statinfo.st_mode)) && 
	    (ic->sd->config->view.open_dirs_in_place) &&
	    (!ic->sd->config->view.no_subdir_jump) &&
	    (!ic->sd->config->view.single_click)
	    )
	  {
	     char buf[4096], *dev = NULL;
	     
	     if (ic->sd->dev) dev = strdup(ic->sd->dev);
	     if (ic->info.pseudo_link)
	       snprintf(buf, sizeof(buf), "%s/%s", ic->info.pseudo_dir, ic->info.file);
	     else
	       snprintf(buf, sizeof(buf), "%s/%s", ic->sd->path, ic->info.file);
	     e_fm2_path_set(ic->sd->obj, dev, buf);
	     E_FREE(dev);
	  }
	else
	  {
	     evas_object_smart_callback_call(ic->sd->obj, "selected", NULL);
	  }
	/* if its in file selector mode then signal that a selection has
	 * taken place and dont do anything more */
	
	/* do the below per selected file */
	/* if its a directory and open dirs in-place is not set, then 
	 * signal owner that a new dir should be opened */
	/* if its a normal file - do what the mime type says to do with
	 * that file type */
     }
   else if (ev->button == 1)
     {
	if ((ic->sd->eobj))
	  {
	     ic->drag.x = ev->output.x;
	     ic->drag.y = ev->output.y;
	     ic->drag.start = 1;
	     ic->drag.dnd = 0;
	  }
	_e_fm2_mouse_1_handler(ic, 0, ev->modifiers);
     }
   else if (ev->button == 3)
     {
	_e_fm2_icon_menu(ic, ic->sd->obj, ev->timestamp);
	e_util_evas_fake_mouse_up_later(evas_object_evas_get(ic->sd->obj),
					ev->button);
//	evas_event_feed_mouse_up(evas_object_evas_get(ic->sd->obj), ev->button,
//				 EVAS_BUTTON_NONE, ev->timestamp, NULL);
     }
}
    
static void
_e_fm2_cb_icon_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *ev;
   E_Fm2_Icon *ic;
   
   ic = data;
   ev = event_info;
   if ((ev->button == 1) && (!ic->drag.dnd))
     {
	_e_fm2_mouse_1_handler(ic, 1, ev->modifiers);
        ic->drag.start = 0;
	ic->drag.dnd = 0;
     }
   ic->down_sel = 0;
}

static void
_e_fm2_cb_drag_finished(E_Drag *drag, int dropped)
{
   Evas_List *fsel;
   char *f;
   
   fsel = drag->data;
   while (fsel)
     {
	f = fsel->data;
	free(f);
	fsel = evas_list_remove_list(fsel, fsel);
     }
}

static void
_e_fm2_cb_icon_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Move *ev;
   E_Fm2_Icon *ic;
   E_Fm2_Icon_Info *ici;
   
   ic = data;
   ev = event_info;
   if ((ic->drag.start) && (ic->sd->eobj))
     {     
	int dx, dy;
	
	dx = ev->cur.output.x - ic->drag.x;
	dy = ev->cur.output.y - ic->drag.y;
	if (((dx * dx) + (dy * dy)) >
	    (e_config->drag_resist * e_config->drag_resist))
	  {
	     E_Drag *d;
	     Evas_Object *o, *o2;
	     Evas_Coord x, y, w, h;
	     const char *drag_types[] = { "text/uri-list" }, *realpath;
	     char buf[4096];
	     E_Container *con = NULL;
	     Evas_List *l, *sl, *fsel = NULL;
	     int i;
	     
	     switch (ic->sd->eobj->type)
	       {
		case E_GADCON_TYPE:
		  con = ((E_Gadcon *)(ic->sd->eobj))->zone->container;
		  break;
		case E_WIN_TYPE:
		  con = ((E_Win *)(ic->sd->eobj))->container;
		  break;
		case E_BORDER_TYPE:
		  con = ((E_Border *)(ic->sd->eobj))->zone->container;
		  break;
		case E_POPUP_TYPE:
		  con = ((E_Popup *)(ic->sd->eobj))->zone->container;
		  break;
		  /* FIXME: add mroe types as needed */
		default:
		  break;
	       }
	     if (!con) return;
	     ic->sd->drag = 1;
	     ic->drag.dnd = 1;
	     ic->drag.start = 0;
	     evas_object_geometry_get(ic->obj, &x, &y, &w, &h);
	     realpath = e_fm2_real_path_get(ic->sd->obj);
	     sl = e_fm2_selected_list_get(ic->sd->obj);
	     for (l = sl, i = 0; l; l = l->next, i++)
	       {
		  ici = l->data;
		  /* file:///path is correct: file://<host>/<path> with null <host> */
		  if (ici->pseudo_link)
		    snprintf(buf, sizeof(buf), "file://%s/%s", ici->pseudo_dir, ici->file);
		  else
		    {
		       if (!strcmp(realpath, "/"))
			 snprintf(buf, sizeof(buf), "file:///%s", ici->file);
		       else
			 snprintf(buf, sizeof(buf), "file://%s/%s", realpath, ici->file);
		    }
		  fsel = evas_list_append(fsel, strdup(buf));
	       }
	     evas_list_free(sl);
	     d = e_drag_new(con,
			    x, y, drag_types, 1,
			    fsel, -1, NULL, _e_fm2_cb_drag_finished);
	     o = edje_object_add(e_drag_evas_get(d));
	     if (ic->sd->config->view.mode == E_FM2_VIEW_MODE_LIST)
	       {
		  if (ic->sd->config->icon.fixed.w)
		    {
		       if (ic->odd)
			 e_theme_edje_object_set(o, "base/theme/widgets",
						 "e/fileman/list_odd/fixed");
		       else
			 e_theme_edje_object_set(o, "base/theme/widgets",
						 "e/fileman/list/fixed");
		    }
		  else
		    {
		       if (ic->odd)
			 e_theme_edje_object_set(o, "base/theme/widgets",
						 "e/fileman/list_odd/variable");
		       else
			 e_theme_edje_object_set(o, "base/theme/widgets",
						 "e/fileman/list/variable");
		    }
	       }
	     else
	       {
		  if (ic->sd->config->icon.fixed.w)
		    e_theme_edje_object_set(o, "base/theme/fileman",
					    "e/fileman/icon/fixed");
		  else
		    e_theme_edje_object_set(o, "base/theme/fileman",
					    "e/fileman/icon/variable");
	       }
	     _e_fm2_icon_label_set(ic, o);
	     o2 = _e_fm2_icon_icon_direct_set(ic, o,
					      _e_fm2_cb_icon_thumb_dnd_gen, o,
					      1);
	     edje_object_signal_emit(o, "e,state,selected", "e");
	     edje_object_signal_emit(o2, "e,state,selected", "e");
	     e_drag_object_set(d, o);
	     e_drag_resize(d, w, h);
	     e_drag_start(d, ic->drag.x, ic->drag.y);
	     e_util_evas_fake_mouse_up_later(evas_object_evas_get(ic->sd->obj),
					     1);
//	     evas_event_feed_mouse_up(evas_object_evas_get(ic->sd->obj),
//				      1, EVAS_BUTTON_NONE,
//				      ecore_x_current_time_get(), NULL);
	  }
     }
}

static void
_e_fm2_cb_icon_thumb_dnd_gen(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *o;
   Evas_Coord w = 0, h = 0;
   int have_alpha;
   
   o = data;
   e_icon_size_get(obj, &w, &h);
   have_alpha = e_icon_alpha_get(obj);
//   if (ic->sd->config->view.mode == E_FM2_VIEW_MODE_LIST)
     {
	edje_extern_object_aspect_set(obj,
				      EDJE_ASPECT_CONTROL_BOTH, w, h);
     }
   edje_object_part_swallow(o, "e.swallow.icon", obj);
   if (have_alpha)
     edje_object_signal_emit(o, "e,action,thumb,gen,alpha", "e");
   else
     edje_object_signal_emit(o, "e,action,thumb,gen", "e");
}

static void
_e_fm2_cb_icon_thumb_gen(void *data, Evas_Object *obj, void *event_info)
{
   E_Fm2_Icon *ic;
   
   ic = data;
   if (ic->realized)
     {
	Evas_Coord w = 0, h = 0;
	int have_alpha;
	
	e_icon_size_get(obj, &w, &h);
	have_alpha = e_icon_alpha_get(obj);
	if (ic->sd->config->view.mode == E_FM2_VIEW_MODE_LIST)
	  {
	     edje_extern_object_aspect_set(obj,
					   EDJE_ASPECT_CONTROL_BOTH, w, h);
	  }
	edje_object_part_swallow(ic->obj, "e.swallow.icon", obj);
	if (have_alpha)
	  edje_object_signal_emit(ic->obj, "e,action,thumb,gen,alpha", "e");
	else
	  edje_object_signal_emit(ic->obj, "e,action,thumb,gen", "e");
     }
}

static void
_e_fm2_cb_key_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Key_Down *ev;
   E_Fm2_Smart_Data *sd;
   E_Fm2_Icon *ic;
   
   sd = data;
   ev = event_info;
   if (!strcmp(ev->keyname, "Left"))
     { 
	/* FIXME: icon mode, typebuf extras */
	/* list mode: scroll left n pix
	 * icon mode: prev icon
	 * typebuf mode: cursor left
	 */
	_e_fm2_icon_sel_prev(obj);
    }
   else if (!strcmp(ev->keyname, "Right"))
     {
	/* FIXME: icon mode, typebuf extras */
	/* list mode: scroll right n pix
	 * icon mode: next icon
	 * typebuf mode: cursor right
	 */
	_e_fm2_icon_sel_next(obj);
     }
   else if (!strcmp(ev->keyname, "Up"))
     {
	/* FIXME: icon mode */
	/* list mode: prev icon
	 * icon mode: up an icon
	 * typebuf mode: previous history
	 */
	if (sd->typebuf_visible)
	  _e_fm2_typebuf_history_prev(obj);
	else
	  _e_fm2_icon_sel_prev(obj);
     }
   else if (!strcmp(ev->keyname, "Home"))
     {
	/* FIXME: typebuf extras */
	/* go to first icon
	 * typebuf mode: cursor to start
	 */
	_e_fm2_icon_sel_first(obj);
     }
   else if (!strcmp(ev->keyname, "End"))
     {
	/* FIXME: typebuf extras */
	/* go to last icon
	 * typebuf mode: cursor to end
	 */
	_e_fm2_icon_sel_last(obj);
     }
   else if (!strcmp(ev->keyname, "Down"))
     {
	/* FIXME: icon mode */
	/* list mode: next icon
	 * icon mode: down an icon
	 * typebuf mode: next history
	 */
	if (sd->typebuf_visible)
	  _e_fm2_typebuf_history_next(obj);
	else
	  _e_fm2_icon_sel_next(obj);
     }
   else if (!strcmp(ev->keyname, "Prior"))
     {
	/* up h * n pixels */
	e_fm2_pan_set(obj, sd->pos.x, sd->pos.y - sd->h);
	evas_object_smart_callback_call(sd->obj, "pan_changed", NULL);
     }
   else if (!strcmp(ev->keyname, "Next"))
     {
	/* down h * n pixels */
	e_fm2_pan_set(obj, sd->pos.x, sd->pos.y + sd->h);
	evas_object_smart_callback_call(sd->obj, "pan_changed", NULL);
     }
   else if (!strcmp(ev->keyname, "Escape"))
     {
	/* typebuf mode: end typebuf mode */
	if (sd->typebuf_visible)
	  _e_fm2_typebuf_hide(obj);
	else
	  {
	     ic = _e_fm2_icon_first_selected_find(obj);
	     if (ic)
	       _e_fm2_icon_desel_any(obj);
	     else
	       {
		  if (e_fm2_has_parent_get(obj))
		    e_fm2_parent_go(obj);
	       }
	  }
     }
   else if (!strcmp(ev->keyname, "Return"))
     {
	/* if selected - select callback.
	 * typebuf mode: if nothing selected - run cmd
	 */
	if (sd->typebuf_visible)
	  _e_fm2_typebuf_run(obj);
	else
	  {
	     ic = _e_fm2_icon_first_selected_find(obj);
	     if (ic)
	       {
		  if ((S_ISDIR(ic->info.statinfo.st_mode)) && 
		      (ic->sd->config->view.open_dirs_in_place) &&
		      (!ic->sd->config->view.no_subdir_jump) &&
		      (!ic->sd->config->view.single_click)
		      )
		    {
		       char buf[4096], *dev = NULL;
		       
		       if (ic->sd->dev) dev = strdup(ic->sd->dev);
		       if (ic->info.pseudo_link)
			 snprintf(buf, sizeof(buf), "%s/%s", ic->info.pseudo_dir, ic->info.file);
		       else
			 snprintf(buf, sizeof(buf), "%s/%s", ic->sd->path, ic->info.file);
		       e_fm2_path_set(ic->sd->obj, dev, buf);
		       E_FREE(dev);
		    }
		  else
		    {
		       evas_object_smart_callback_call(ic->sd->obj, "selected", NULL);
		    }
	       }
	  }
     }
   else if (!strcmp(ev->keyname, "Insert"))
     {
	/* dunno what to do with this yet */
     }
   else if (!strcmp(ev->keyname, "Tab"))
     {
	/* typebuf mode: tab complete */
	if (sd->typebuf_visible)
	  _e_fm2_typebuf_complete(obj);
     }
   else if (!strcmp(ev->keyname, "BackSpace"))
     {
	/* typebuf mode: backspace */
	if (sd->typebuf_visible)
	  _e_fm2_typebuf_char_backspace(obj);
     }
   else if (!strcmp(ev->keyname, "Delete"))
     {
	/* FIXME: all */
	/* delete file dialog */
	/* typebuf mode: delete */
     }
   else
     {
	if (ev->string)
	  {
	     if (!sd->typebuf_visible) _e_fm2_typebuf_show(obj);
	     _e_fm2_typebuf_char_append(obj, ev->string);
	  }
     }
}

static void
_e_fm2_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   E_Fm2_Smart_Data *sd;
   
   sd = data;
   ev = event_info;
   if (ev->button == 3)
     {
	_e_fm2_menu(sd->obj, ev->timestamp);
	e_util_evas_fake_mouse_up_later(evas_object_evas_get(sd->obj),
					ev->button);
//	evas_event_feed_mouse_up(evas_object_evas_get(sd->obj), ev->button,
//				 EVAS_BUTTON_NONE, ev->timestamp, NULL);
     }
}
    
static void
_e_fm2_cb_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *ev;
   E_Fm2_Smart_Data *sd;
   
   sd = data;
   ev = event_info;
}

static void
_e_fm2_cb_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Move *ev;
   E_Fm2_Smart_Data *sd;
   
   sd = data;
   ev = event_info;
}
    
static void
_e_fm2_cb_scroll_job(void *data)
{
   E_Fm2_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(data);
   if (!sd) return;
   sd->scroll_job = NULL;
   evas_event_freeze(evas_object_evas_get(sd->obj));
   edje_freeze();
   _e_fm2_regions_eval(sd->obj);
   _e_fm2_obj_icons_place(sd);
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(sd->obj));
}

static void
_e_fm2_cb_resize_job(void *data)
{
   E_Fm2_Smart_Data *sd;
   Evas_List *l;
   
   sd = evas_object_smart_data_get(data);
   if (!sd) return;
   sd->resize_job = NULL;
   evas_event_freeze(evas_object_evas_get(sd->obj));
   edje_freeze();
   switch (sd->config->view.mode)
     {
      case E_FM2_VIEW_MODE_ICONS:
	_e_fm2_regions_free(sd->obj);
	_e_fm2_icons_place(sd->obj);
	_e_fm2_regions_populate(sd->obj);
	break;
      case E_FM2_VIEW_MODE_GRID_ICONS:
	_e_fm2_regions_free(sd->obj);
	_e_fm2_icons_place(sd->obj);
	_e_fm2_regions_populate(sd->obj);
	break;
      case E_FM2_VIEW_MODE_CUSTOM_ICONS:
	_e_fm2_regions_eval(sd->obj);
	_e_fm2_obj_icons_place(sd);
	break;
      case E_FM2_VIEW_MODE_CUSTOM_GRID_ICONS:
	_e_fm2_regions_eval(sd->obj);
	_e_fm2_obj_icons_place(sd);
	break;
      case E_FM2_VIEW_MODE_CUSTOM_SMART_GRID_ICONS:
	_e_fm2_regions_eval(sd->obj);
	_e_fm2_obj_icons_place(sd);
	break;
      case E_FM2_VIEW_MODE_LIST:
	if (sd->iconlist_changed)
	  {
	     for (l = sd->icons; l; l = l->next)
	       _e_fm2_icon_unrealize(l->data);
	  }
        _e_fm2_regions_free(sd->obj);
	_e_fm2_icons_place(sd->obj);
        _e_fm2_regions_populate(sd->obj);
	break;
      default:
	break;
     }
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(sd->obj));
   sd->iconlist_changed = 0;
}

static int
_e_fm2_cb_icon_sort(void *data1, void *data2)
{
   E_Fm2_Icon *ic1, *ic2;
   char *l1, *l2;
   
   ic1 = data1;
   ic2 = data2;
   l1 = (char *)ic1->info.file;
   if (ic1->info.label) l1 = (char *)ic1->info.label;
   l2 = (char *)ic2->info.file;
   if (ic2->info.label) l2 = (char *)ic2->info.label;
   if (ic1->sd->config->list.sort.dirs.first)
     {
	if ((S_ISDIR(ic1->info.statinfo.st_mode)) != 
	    (S_ISDIR(ic2->info.statinfo.st_mode)))
	  {
	     if (S_ISDIR(ic1->info.statinfo.st_mode)) return -1;
	     else return 1;
	  }
     }
   else if (ic1->sd->config->list.sort.dirs.last)
     {
	if ((S_ISDIR(ic1->info.statinfo.st_mode)) != 
	    (S_ISDIR(ic2->info.statinfo.st_mode)))
	  {
	     if (S_ISDIR(ic1->info.statinfo.st_mode)) return 1;
	     else return -1;
	  }
     }
   if (ic1->sd->config->list.sort.no_case)
     {
	char buf1[4096], buf2[4096], *p;
	
	strncpy(buf1, l1, sizeof(buf1) - 2);
	strncpy(buf2, l2, sizeof(buf2) - 2);
	buf1[sizeof(buf1) - 1] = 0;
	buf2[sizeof(buf2) - 1] = 0;
	p = buf1;
	while (*p)
	  {
	     *p = tolower(*p);
	     p++;
	  }
	p = buf2;
	while (*p)
	  {
	     *p = tolower(*p);
	     p++;
	  }
	return strcmp(buf1, buf2);
     }
   return strcmp(l1, l2);
}

static int
_e_fm2_cb_scan_idler(void *data)
{
   E_Fm2_Smart_Data *sd;
   struct dirent *dp;
   int len;
   char buf[4096];
   
   sd = evas_object_smart_data_get(data);
   if (!sd) return 0;
 
   if ((!sd->dir) && (!sd->order))
     {
	if (!sd->order_file)
	  {
	     snprintf(buf, sizeof(buf), "%s/.order", sd->realpath);
	     sd->order = fopen(buf, "rb");
	     if (sd->order) sd->order_file = 1;
	  }
	if (!sd->order)
	  {
//	     sd->order_file = 0;
	     sd->dir = opendir(sd->realpath);
	     if (!sd->dir) goto endscan;
	  }
     }

   if (sd->order)
     {
	if (!fgets(buf, sizeof(buf), sd->order))
	  {
	     /* stop order file read - now pick up real files */
	     fclose(sd->order);
	     sd->order = NULL;
	  }
	else
	  {
	     len = strlen(buf);
	     if ((len > 0) && (buf[len - 1] == '\n')) buf[len - 1] = 0;
	     _e_fm2_file_add(data, buf, 0, NULL, 0);
	  }
     }
   else if (sd->dir)
     {
	dp = readdir(sd->dir);
	if (!dp) goto endscan;
	/* no - you don't want the current and parent dir links listed */
	if ((!strcmp(dp->d_name, ".")) || (!strcmp(dp->d_name, ".."))) return 1;
	/* skip dotfiles if we're not showing hidden files */
	if (dp->d_name[0] == '.' && !sd->show_hidden_files) return 1;
	/* always hide .order files */
	if (!strcmp(dp->d_name, ".order")) return 1;
	_e_fm2_file_add(data, dp->d_name, sd->order_file, NULL, 0);
     }
   return 1;
   
   endscan:
   sd->scan_idler = NULL;
   if (sd->scan_timer)
     {
	ecore_timer_del(sd->scan_timer);
	sd->scan_timer = ecore_timer_add(0.0001, _e_fm2_cb_scan_timer, sd->obj);
     }
   else
     _e_fm2_scan_stop(data);
   return 0;
}

static int
_e_fm2_cb_scan_timer(void *data)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(data);
   if (!sd) return 0;
   _e_fm2_queue_process(data);
   sd->scan_timer = NULL;
   if ((!sd->queue) && (!sd->scan_idler))
     {
	_e_fm2_scan_stop(data);
	return 0;
     }
   if (sd->scan_idler)
     sd->scan_timer = ecore_timer_add(0.2, _e_fm2_cb_scan_timer, sd->obj);
   else
     {
	if (!sd->sort_idler)
	  sd->sort_idler = ecore_idler_add(_e_fm2_cb_sort_idler, data);
     }
   return 0;
}

static int
_e_fm2_cb_sort_idler(void *data)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(data);
   if (!sd) return 0;
   _e_fm2_queue_process(data);
   if (!sd->queue)
     {
	sd->sort_idler = NULL;
	_e_fm2_scan_stop(data);
	return 0;
     }
   return 1;
}

/**************************/
static void
_e_fm2_obj_icons_place(E_Fm2_Smart_Data *sd)
{
   Evas_List *l, *ll;
   E_Fm2_Region *rg;
   E_Fm2_Icon *ic;

   evas_event_freeze(evas_object_evas_get(sd->obj));
   edje_freeze();
   for (l = sd->regions.list; l; l = l->next)
     {
	rg = l->data;
	if (rg->realized)
	  {
	     for (ll = rg->list; ll; ll = ll->next)
	       {
		  ic = ll->data;
		  if (ic->realized)
		    {
		       evas_object_move(ic->obj, 
					sd->x + ic->x - sd->pos.x, 
					sd->y + ic->y - sd->pos.y);
		       evas_object_resize(ic->obj, ic->w, ic->h);
		       _e_fm2_icon_thumb(ic, ic->obj_icon, 0);
		    }
	       }
	  }
     }
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(sd->obj));
}

/**************************/

static void
_e_fm2_smart_add(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = E_NEW(E_Fm2_Smart_Data, 1);
   if (!sd) return;
   sd->obj = obj;
   sd->clip = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_smart_member_add(sd->clip, obj);
   evas_object_color_set(sd->clip, 255, 255, 255, 255);
   
   sd->underlay = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_clip_set(sd->underlay, sd->clip);
   evas_object_smart_member_add(sd->underlay, obj);
   evas_object_color_set(sd->underlay, 0, 0, 0, 0);
   evas_object_show(sd->underlay);

   evas_object_event_callback_add(obj, EVAS_CALLBACK_KEY_DOWN, _e_fm2_cb_key_down, sd);
   evas_object_event_callback_add(sd->underlay, EVAS_CALLBACK_MOUSE_DOWN, _e_fm2_cb_mouse_down, sd);
   evas_object_event_callback_add(sd->underlay, EVAS_CALLBACK_MOUSE_UP, _e_fm2_cb_mouse_up, sd);
   evas_object_event_callback_add(sd->underlay, EVAS_CALLBACK_MOUSE_MOVE, _e_fm2_cb_mouse_move, sd);
   
   sd->drop = edje_object_add(evas_object_evas_get(obj));
   evas_object_clip_set(sd->drop, sd->clip);
   e_theme_edje_object_set(sd->drop, "base/theme/fileman",
			   "e/fileman/list/drop_between");
   evas_object_smart_member_add(sd->drop, obj);
   evas_object_show(sd->drop);
   
   sd->drop_in = edje_object_add(evas_object_evas_get(obj));
   evas_object_clip_set(sd->drop_in, sd->clip);
   e_theme_edje_object_set(sd->drop_in, "base/theme/fileman",
			   "e/fileman/list/drop_in");
   evas_object_smart_member_add(sd->drop_in, obj);
   evas_object_show(sd->drop_in);
   
   sd->overlay = edje_object_add(evas_object_evas_get(obj));
   evas_object_clip_set(sd->overlay, sd->clip);
   e_theme_edje_object_set(sd->overlay, "base/theme/fileman",
			   "e/fileman/overlay");
   evas_object_smart_member_add(sd->overlay, obj);
   evas_object_show(sd->overlay);
   
   evas_object_smart_data_set(obj, sd);
   evas_object_move(obj, 0, 0);
   evas_object_resize(obj, 0, 0);
   _e_fm2_list = evas_list_append(_e_fm2_list, sd->obj);
}

static void
_e_fm2_smart_del(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   _e_fm2_fop_detach(sd);
   
   if (sd->monitor.monitor)
     {
	ecore_file_monitor_del(sd->monitor.monitor);
	sd->monitor.monitor = NULL;
     }
   _e_fm2_live_process_end(obj);
   _e_fm2_scan_stop(obj);
   _e_fm2_queue_free(obj);
   _e_fm2_regions_free(obj);
   _e_fm2_icons_free(obj);
   if (sd->menu)
     {
	e_menu_post_deactivate_callback_set(sd->menu, NULL, NULL);
	e_object_del(E_OBJECT(sd->menu));
	sd->menu = NULL;
     }
   if (sd->entry_dialog)
     {
	e_object_del(E_OBJECT(sd->entry_dialog));
	sd->entry_dialog = NULL;
     }
   if (sd->scroll_job) ecore_job_del(sd->scroll_job);
   if (sd->resize_job) ecore_job_del(sd->resize_job);
   if (sd->refresh_job) ecore_job_del(sd->refresh_job);
   if (sd->dev) evas_stringshare_del(sd->dev);
   if (sd->path) evas_stringshare_del(sd->path);
   if (sd->realpath) evas_stringshare_del(sd->realpath);
   sd->dev = sd->path = sd->realpath = NULL;
   if (sd->config) _e_fm2_config_free(sd->config);
   
   E_FREE(sd->typebuf.buf);

   evas_object_del(sd->underlay);
   evas_object_del(sd->overlay);
   evas_object_del(sd->drop);
   evas_object_del(sd->drop_in);
   evas_object_del(sd->clip);
   if (sd->drop_handler) e_drop_handler_del(sd->drop_handler);
   _e_fm2_list = evas_list_remove(_e_fm2_list, sd->obj);
   free(sd);
}

static void
_e_fm2_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if ((sd->x == x) && (sd->y == y)) return;
   sd->x = x;
   sd->y = y;
   evas_object_move(sd->underlay, sd->x, sd->y);
   evas_object_move(sd->overlay, sd->x, sd->y);
   _e_fm2_dnd_drop_configure(sd->obj);
   evas_object_move(sd->clip, sd->x - OVERCLIP, sd->y - OVERCLIP);
   _e_fm2_obj_icons_place(sd);
   if (sd->drop_handler)
     e_drop_handler_geometry_set(sd->drop_handler, sd->x, sd->y, sd->w, sd->h);
}

static void
_e_fm2_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Fm2_Smart_Data *sd;
   int wch = 0, hch = 0;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if ((sd->w == w) && (sd->h == h)) return;
   if (w != sd->w) wch = 1;
   if (h != sd->h) hch = 1;
   sd->w = w;
   sd->h = h;
   evas_object_resize(sd->underlay, sd->w, sd->h);
   evas_object_resize(sd->overlay, sd->w, sd->h);
   _e_fm2_dnd_drop_configure(sd->obj);
   evas_object_resize(sd->clip, sd->w + (OVERCLIP * 2), sd->h + (OVERCLIP * 2));

   /* for automatic layout - do this - NB; we could put this on a timer delay */
   if (wch)
     {
	if (sd->resize_job) ecore_job_del(sd->resize_job);
	sd->resize_job = ecore_job_add(_e_fm2_cb_resize_job, obj);
     }
   else
     {
	if (sd->scroll_job) ecore_job_del(sd->scroll_job);
	sd->scroll_job = ecore_job_add(_e_fm2_cb_scroll_job, obj);
     }
   if (sd->drop_handler)
     e_drop_handler_geometry_set(sd->drop_handler, sd->x, sd->y, sd->w, sd->h);
}

static void
_e_fm2_smart_show(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_show(sd->clip);
}

static void
_e_fm2_smart_hide(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_hide(sd->clip);
}

static void
_e_fm2_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_color_set(sd->clip, r, g, b, a);
}

static void
_e_fm2_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_set(sd->clip, clip);
}

static void
_e_fm2_smart_clip_unset(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_unset(sd->clip);
}

static void
_e_fm2_order_file_rewrite(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   Evas_List *l;
   E_Fm2_Icon *ic;
   FILE *f;
   char buf[4096];
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   snprintf(buf, sizeof(buf), "%s/.order", sd->realpath);
   f = fopen(buf, "w");
   if (!f) return;

   for (l = sd->icons; l; l = l->next)
     {
	ic = l->data;
	if (!ic->info.deleted)
	  fprintf(f, "%s\n", ic->info.file);
     }
   if (!sd->order_file) sd->order_file = 1;
   fclose(f);
}

static void
_e_fm2_menu(Evas_Object *obj, unsigned int timestamp)
{
   E_Fm2_Smart_Data *sd;
   E_Menu *mn;
   E_Menu_Item *mi;
   E_Manager *man;
   E_Container *con;
   E_Zone *zone;
   int x, y;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   mn = e_menu_new();
   e_menu_category_set(mn, "e/fileman/action");

   if (sd->icon_menu.replace.func)
     {
	sd->icon_menu.replace.func(sd->icon_menu.replace.data, sd->obj, mn, NULL);
     }
   else
     {
	if (sd->icon_menu.start.func)
	  {
	     sd->icon_menu.start.func(sd->icon_menu.start.data, sd->obj, mn, NULL);
	     mi = e_menu_item_new(mn);
	     e_menu_item_separator_set(mi, 1);
	  }

	if (!(sd->icon_menu.flags & E_FM2_MENU_NO_REFRESH))
	  {
	     mi = e_menu_item_new(mn);
	     e_menu_item_label_set(mi, _("Refresh View"));
	     e_menu_item_icon_edje_set(mi,
				       e_theme_edje_file_get("base/theme/fileman",
							     "e/fileman/button/refresh"),
				       "e/fileman/button/refresh");
	     e_menu_item_callback_set(mi, _e_fm2_refresh, sd);
	  }
	
	if (!(sd->icon_menu.flags & E_FM2_MENU_NO_SHOW_HIDDEN))
	  {
	     mi = e_menu_item_new(mn);
	     e_menu_item_label_set(mi, _("Show Hidden Files"));
	     e_menu_item_icon_edje_set(mi,
				       e_theme_edje_file_get("base/theme/fileman",
							     "e/fileman/button/hidden_files"),
				       "e/fileman/button/hidden_files");
	     e_menu_item_check_set(mi, 1);
	     e_menu_item_toggle_set(mi, sd->show_hidden_files);
	     e_menu_item_callback_set(mi, _e_fm2_toggle_hidden_files, sd);
	  }
	
	if (!(sd->icon_menu.flags & E_FM2_MENU_NO_REMEMBER_ORDERING))
	  {
	     if (!sd->config->view.always_order)
	       {
		  mi = e_menu_item_new(mn);
		  e_menu_item_label_set(mi, _("Remember Ordering"));
		  e_menu_item_icon_edje_set(mi,
					    e_theme_edje_file_get("base/theme/fileman",
								  "e/fileman/button/ordering"),
					    "e/fileman/button/ordering");
		  e_menu_item_check_set(mi, 1);
		  e_menu_item_toggle_set(mi, sd->order_file);
		  e_menu_item_callback_set(mi, _e_fm2_toggle_ordering, sd);
	       }
	     if ((sd->order_file) || (sd->config->view.always_order))
	       {
		  mi = e_menu_item_new(mn);
		  e_menu_item_label_set(mi, _("Sort Now"));
		  e_menu_item_icon_edje_set(mi,
					    e_theme_edje_file_get("base/theme/fileman",
								  "e/fileman/button/ordering"),
					    "e/fileman/button/sort");
		  e_menu_item_callback_set(mi, _e_fm2_sort, sd);
	       }
	  }
	
	if (!(sd->icon_menu.flags & E_FM2_MENU_NO_NEW_DIRECTORY))
	  {
	     if (ecore_file_can_write(sd->realpath))
	       {
		  mi = e_menu_item_new(mn);
		  e_menu_item_separator_set(mi, 1);
		  
		  mi = e_menu_item_new(mn);
		  e_menu_item_label_set(mi, _("New Directory"));
		  e_menu_item_icon_edje_set(mi,
					    e_theme_edje_file_get("base/theme/fileman",
								  "e/fileman/button/new_dir"),
					    "e/fileman/button/new_dir");
		  e_menu_item_callback_set(mi, _e_fm2_new_directory, sd);
	       }
	  }
	     
	if (sd->icon_menu.end.func)
	  {
	     mi = e_menu_item_new(mn);
	     e_menu_item_separator_set(mi, 1);
	     sd->icon_menu.end.func(sd->icon_menu.end.data, sd->obj, mn, NULL);
	  }
     }
   
   man = e_manager_current_get();
   if (!man)
     {
	e_object_del(E_OBJECT(mn));
	return;
     }
   con = e_container_current_get(man);
   if (!con)
     {
	e_object_del(E_OBJECT(mn));
	return;
     }
   ecore_x_pointer_xy_get(con->win, &x, &y);
   zone = e_util_zone_current_get(man);
   if (!zone)
     {
	e_object_del(E_OBJECT(mn));
	return;
     }
   sd->menu = mn;
   e_menu_post_deactivate_callback_set(mn, _e_fm2_menu_post_cb, sd);
   e_menu_activate_mouse(mn, zone, 
			 x, y, 1, 1, 
			 E_MENU_POP_DIRECTION_DOWN, timestamp);
}

static void
_e_fm2_menu_post_cb(void *data, E_Menu *m)
{
   E_Fm2_Smart_Data *sd;
   
   sd = data;
   sd->menu = NULL;
}

static void
_e_fm2_icon_menu(E_Fm2_Icon *ic, Evas_Object *obj, unsigned int timestamp)
{
   E_Fm2_Smart_Data *sd;
   E_Menu *mn;
   E_Menu_Item *mi;
   E_Manager *man;
   E_Container *con;
   E_Zone *zone;
   Evas_List *sel;
   int x, y, can_w, can_w2, protect;
   char buf[4096];
   
   sd = ic->sd;

   mn = e_menu_new();
   e_menu_category_set(mn, "e/fileman/action");

   if (sd->icon_menu.replace.func)
     {
	sd->icon_menu.replace.func(sd->icon_menu.replace.data, sd->obj, mn, NULL);
     }
   else
     {
	if (sd->icon_menu.start.func)
	  {
	     sd->icon_menu.start.func(sd->icon_menu.start.data, sd->obj, mn, NULL);
	     mi = e_menu_item_new(mn);
	     e_menu_item_separator_set(mi, 1);
	  }

	if (!(sd->icon_menu.flags & E_FM2_MENU_NO_REFRESH))
	  {
	     mi = e_menu_item_new(mn);
	     e_menu_item_label_set(mi, _("Refresh View"));
	     e_menu_item_icon_edje_set(mi,
				       e_theme_edje_file_get("base/theme/fileman",
							     "e/fileman/button/refresh"),
				       "e/fileman/button/refresh");
	     e_menu_item_callback_set(mi, _e_fm2_refresh, sd);
	  }
	
	if (!(sd->icon_menu.flags & E_FM2_MENU_NO_SHOW_HIDDEN))
	  {
	     mi = e_menu_item_new(mn);
	     e_menu_item_label_set(mi, _("Show Hidden Files"));
	     e_menu_item_icon_edje_set(mi,
				       e_theme_edje_file_get("base/theme/fileman",
							     "e/fileman/button/hidden_files"),
				       "e/fileman/button/hidden_files");
	     e_menu_item_check_set(mi, 1);
	     e_menu_item_toggle_set(mi, sd->show_hidden_files);
	     e_menu_item_callback_set(mi, _e_fm2_toggle_hidden_files, sd);
	  }
	
	if (!(sd->icon_menu.flags & E_FM2_MENU_NO_REMEMBER_ORDERING))
	  {
	     if (!sd->config->view.always_order)
	       {
		  mi = e_menu_item_new(mn);
		  e_menu_item_label_set(mi, _("Remember Ordering"));
		  e_menu_item_icon_edje_set(mi,
					    e_theme_edje_file_get("base/theme/fileman",
								  "e/fileman/button/ordering"),
					    "e/fileman/button/ordering");
		  e_menu_item_check_set(mi, 1);
		  e_menu_item_toggle_set(mi, sd->order_file);
		  e_menu_item_callback_set(mi, _e_fm2_toggle_ordering, sd);
	       }
	     if ((sd->order_file) || (sd->config->view.always_order))
	       {
		  mi = e_menu_item_new(mn);
		  e_menu_item_label_set(mi, _("Sort Now"));
		  e_menu_item_icon_edje_set(mi,
					    e_theme_edje_file_get("base/theme/fileman",
								  "e/fileman/button/ordering"),
					    "e/fileman/button/sort");
		  e_menu_item_callback_set(mi, _e_fm2_sort, sd);
	       }
	  }
	
	if (!(sd->icon_menu.flags & E_FM2_MENU_NO_NEW_DIRECTORY))
	  {
	     if (ecore_file_can_write(sd->realpath))
	       {
		  mi = e_menu_item_new(mn);
		  e_menu_item_separator_set(mi, 1);
		  
		  mi = e_menu_item_new(mn);
		  e_menu_item_label_set(mi, _("New Directory"));
		  e_menu_item_icon_edje_set(mi,
					    e_theme_edje_file_get("base/theme/fileman",
								  "e/fileman/button/new_dir"),
					    "e/fileman/button/new_dir");
		  e_menu_item_callback_set(mi, _e_fm2_new_directory, sd);
	       }
	  }
   
	can_w = 0;
	can_w2 = 1;
	if (ic->sd->order_file)
	  {
	     snprintf(buf, sizeof(buf), "%s/.order", sd->realpath);
	     can_w2 = ecore_file_can_write(buf);
	  }
	if (ic->info.pseudo_link)
	  snprintf(buf, sizeof(buf), "%s/%s", ic->info.pseudo_dir, ic->info.file);
	else
	  snprintf(buf, sizeof(buf), "%s/%s", sd->realpath, ic->info.file);
	if ((ic->info.link) && (!ic->info.pseudo_link))
	  {
	     struct stat st;
	     
	     if (lstat(buf, &st) == 0)
	       {
		  if (st.st_uid == getuid())
		    {
		       if (st.st_mode & S_IWUSR) can_w = 1;
		    }
		  else if (st.st_gid == getgid())
		    {
		       if (st.st_mode & S_IWGRP) can_w = 1;
		    }
		  else
		    {
		       if (st.st_mode & S_IWOTH) can_w = 1;
		    }
	       }
	  }
	else
	  {
	     if (ic->info.pseudo_link)
	       snprintf(buf, sizeof(buf), "%s/.order", sd->realpath);
	     if (ecore_file_can_write(buf)) can_w = 1;
	  }
	
	sel = e_fm2_selected_list_get(ic->sd->obj);
	if ((!sel) || evas_list_count(sel) == 1)
	  {
	     snprintf(buf, sizeof(buf), "%s/%s", sd->realpath, ic->info.file);
	     protect = e_filereg_file_protected(buf);
	  }
	else
	  protect = 0;
	
	if ((can_w) && (can_w2) && !(protect))
	  {
	     mi = e_menu_item_new(mn);
	     e_menu_item_separator_set(mi, 1);
	     
	     if (!(sd->icon_menu.flags & E_FM2_MENU_NO_DELETE))
	       {
		  mi = e_menu_item_new(mn);
		  e_menu_item_label_set(mi, _("Delete"));
		  e_menu_item_icon_edje_set(mi,
					    e_theme_edje_file_get("base/theme/fileman",
								  "e/fileman/button/delete"),
					    "e/fileman/button/delete");
		  e_menu_item_callback_set(mi, _e_fm2_file_delete, ic);
	       }
	     
	     if (!(sd->icon_menu.flags & E_FM2_MENU_NO_RENAME))
	       {
		  mi = e_menu_item_new(mn);
		  e_menu_item_label_set(mi, _("Rename"));
		  e_menu_item_icon_edje_set(mi,
					    e_theme_edje_file_get("base/theme/fileman",
								  "e/fileman/button/rename"),
					    "e/fileman/button/rename");
		  e_menu_item_callback_set(mi, _e_fm2_file_rename, ic);
	       }
	  }
	
	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, _("Properties"));
	e_menu_item_icon_edje_set(mi,
				  e_theme_edje_file_get("base/theme/fileman",
							"e/fileman/button/properties"),
				  "e/fileman/button/properties");
	e_menu_item_callback_set(mi, _e_fm2_file_properties, ic);
	
	if (sd->icon_menu.end.func)
	  {
	     mi = e_menu_item_new(mn);
	     e_menu_item_separator_set(mi, 1);
	     sd->icon_menu.end.func(sd->icon_menu.end.data, sd->obj, mn, &(ic->info));
	  }
     }
   
   man = e_manager_current_get();
   if (!man)
     {
	e_object_del(E_OBJECT(mn));
	return;
     }
   con = e_container_current_get(man);
   if (!con)
     {
	e_object_del(E_OBJECT(mn));
	return;
     }
   ecore_x_pointer_xy_get(con->win, &x, &y);
   zone = e_util_zone_current_get(man);
   if (!zone)
     {
	e_object_del(E_OBJECT(mn));
	return;
     }
   ic->menu = mn;
   e_menu_post_deactivate_callback_set(mn, _e_fm2_icon_menu_post_cb, ic);
   e_menu_activate_mouse(mn, zone, 
			 x, y, 1, 1, 
			 E_MENU_POP_DIRECTION_DOWN, timestamp);
}

static void
_e_fm2_icon_menu_post_cb(void *data, E_Menu *m)
{
   E_Fm2_Icon *ic;
   
   ic = data;
   ic->menu = NULL;
}

static void
_e_fm2_refresh(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Fm2_Smart_Data *sd;
   
   sd = data;
   if (sd->refresh_job) ecore_job_del(sd->refresh_job);
   sd->refresh_job = ecore_job_add(_e_fm2_refresh_job_cb, sd->obj);
}

static void
_e_fm2_toggle_hidden_files(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Fm2_Smart_Data *sd;
   
   sd = data;
   if (sd->show_hidden_files)
     sd->show_hidden_files = 0;
   else
     sd->show_hidden_files = 1;

   _e_fm2_refresh(data, m, mi);
}

static void
_e_fm2_toggle_ordering(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Fm2_Smart_Data *sd;
   char buf[4096];
   
   sd = data;
   if (sd->order_file)
     {
	snprintf(buf, sizeof(buf), "%s/.order", sd->realpath);
	ecore_file_unlink(buf);
     }
   else
     _e_fm2_order_file_rewrite(sd->obj);
   _e_fm2_refresh(data, m, mi);
}

static void
_e_fm2_sort(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Fm2_Smart_Data *sd;
   
   sd = data;
   sd->icons = evas_list_sort(sd->icons, evas_list_count(sd->icons),
			      _e_fm2_cb_icon_sort);
   _e_fm2_order_file_rewrite(sd->obj);
   _e_fm2_refresh(data, m, mi);
}

static void
_e_fm2_new_directory(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Fm2_Smart_Data *sd;
   E_Manager *man;
   E_Container *con;
   
   sd = data;
   if (sd->entry_dialog) return;
   
   man = e_manager_current_get();
   if (!man) return;
   con = e_container_current_get(man);
   if (!con) return;
   
   sd->entry_dialog = e_entry_dialog_show(_("Create a new Directory"), "enlightenment/e",
					  _("New Directory Name:"),
					  "", NULL, NULL, 
					  _e_fm2_new_directory_yes_cb, 
					  _e_fm2_new_directory_no_cb, sd);
   E_OBJECT(sd->entry_dialog)->data = sd;
   e_object_del_attach_func_set(E_OBJECT(sd->entry_dialog), _e_fm2_new_directory_delete_cb);
}

static void
_e_fm2_new_directory_delete_cb(void *obj)
{
   E_Fm2_Smart_Data *sd;
   
   sd = E_OBJECT(obj)->data;
   sd->entry_dialog = NULL;
}

static void
_e_fm2_new_directory_yes_cb(char *text, void *data)
{
   E_Fm2_Smart_Data *sd;
   E_Dialog *dialog;
   E_Manager *man;
   E_Container *con;
   char buf[PATH_MAX];
   char error[PATH_MAX + 256];
   
   sd = data;
   sd->entry_dialog = NULL;
   if ((text) && (text[0]))
     {
	snprintf(buf, sizeof(buf), "%s/%s", sd->realpath, text);

	if (!ecore_file_mkdir(buf))
	  {
	     man = e_manager_current_get();
	     if (!man) return;
	     con = e_container_current_get(man);
	     if (!con) return;
	     
	     dialog = e_dialog_new(con, "E", "_fm_new_dir_error_dialog");
	     e_dialog_button_add(dialog, _("OK"), NULL, NULL, NULL);
	     e_dialog_button_focus_num(dialog, 1);
	     e_dialog_title_set(dialog, _("Error"));
	     snprintf(error, PATH_MAX + 256,
		      _("Could not create directory:<br>"
			"<hilight>%s</hilight>"),
		      text);
	     e_dialog_text_set(dialog, error);
	     e_win_centered_set(dialog->win, 1);
	     e_dialog_show(dialog);
	     return;
	  }
/*	
	if (sd->order_file)
	  {
	     FILE *f;
	     
	     snprintf(buf, sizeof(buf), "%s/.order", sd->realpath);
	     f = fopen(buf, "a");
	     if (f)
	       {
		  fprintf(f, "%s\n", text);
		  fclose(f);
	       }
	  }
 */
	_e_fm2_live_file_add(sd->obj, text, NULL, 0);
	/* FIXME: disable refresh - modify icons in-place */
//	if (sd->refresh_job) ecore_job_del(sd->refresh_job);
//	sd->refresh_job = ecore_job_add(_e_fm2_refresh_job_cb, sd->obj);
     }
}

static void
_e_fm2_new_directory_no_cb(void *data)
{
   E_Fm2_Smart_Data *sd;
   
   sd = data;
   sd->entry_dialog = NULL;
}

static void
_e_fm2_file_rename(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Fm2_Icon *ic;
   E_Manager *man;
   E_Container *con;
   char text[PATH_MAX + 256];
   
   ic = data;
   if (ic->entry_dialog) return;
   
   man = e_manager_current_get();
   if (!man) return;
   con = e_container_current_get(man);
   if (!con) return;
   
   snprintf(text, PATH_MAX + 256,
	    _("Rename %s to:"),
	    ic->info.file);
   ic->entry_dialog = e_entry_dialog_show(_("Rename File"), "enlightenment/e",
					  text, ic->info.file, NULL, NULL, 
					  _e_fm2_file_rename_yes_cb, 
					  _e_fm2_file_rename_no_cb, ic);
   E_OBJECT(ic->entry_dialog)->data = ic;
   e_object_del_attach_func_set(E_OBJECT(ic->entry_dialog), _e_fm2_file_rename_delete_cb);
}

static void
_e_fm2_file_rename_delete_cb(void *obj)
{
   E_Fm2_Icon *ic;
   
   ic = E_OBJECT(obj)->data;
   ic->entry_dialog = NULL;
}

static void
_e_fm2_file_rename_yes_cb(char *text, void *data)
{
   E_Fm2_Icon *ic;
   E_Dialog *dialog;
   E_Manager *man;
   E_Container *con;
   char newpath[4096];
   char oldpath[4096];
   char error[4096 + 256];
   
   ic = data;
   ic->entry_dialog = NULL;
   if ((text) && (strcmp(text, ic->info.file)))
     {
	if (ic->info.pseudo_link)
	  {
	     snprintf(oldpath, sizeof(oldpath), "%s/%s", ic->info.pseudo_dir, ic->info.file);
	     snprintf(newpath, sizeof(newpath), "%s/%s", ic->info.pseudo_dir, text);
	  }
	else
	  {
	     snprintf(oldpath, sizeof(oldpath), "%s/%s", ic->sd->realpath, ic->info.file);
	     snprintf(newpath, sizeof(newpath), "%s/%s", ic->sd->realpath, text);
	  }
	if (e_filereg_file_protected(oldpath)) return;

	if (!ecore_file_mv(oldpath, newpath))
	  {
	     man = e_manager_current_get();
	     if (!man) return;
	     con = e_container_current_get(man);
	     if (!con) return;
	     
	     dialog = e_dialog_new(con, "E", "_fm_file_rename_error_dialog");
	     e_dialog_button_add(dialog, _("OK"), NULL, NULL, NULL);
	     e_dialog_button_focus_num(dialog, 1);
	     e_dialog_title_set(dialog, _("Error"));
	     snprintf(error, sizeof(error),
		      _("Could not rename from <hilight>%s</hilight> to <hilight>%s</hilight>"),
		      ic->info.file, text);
	     e_dialog_text_set(dialog, error);
	     e_win_centered_set(dialog->win, 1);
	     e_dialog_show(dialog);
	     return;
	  }
	_e_fm2_live_file_del(ic->sd->obj, ic->info.file);
	_e_fm2_live_file_add(ic->sd->obj, text, NULL, 0);
//	evas_stringshare_del(ic->info.file);
//	ic->info.file = evas_stringshare_add(text);
//	if (ic->sd->order_file) _e_fm2_order_file_rewrite(ic->sd->obj);
	/* FIXME: disable refresh - modify icons in-place */
//	if (ic->sd->refresh_job) ecore_job_del(ic->sd->refresh_job);
//	ic->sd->refresh_job = ecore_job_add(_e_fm2_refresh_job_cb, ic->sd->obj);
     }
}

static void
_e_fm2_file_rename_no_cb(void *data)
{
   E_Fm2_Icon *ic;
   
   ic = data;
   ic->entry_dialog = NULL;
}

static void
_e_fm2_file_properties(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Fm2_Icon *ic;
   E_Manager *man;
   E_Container *con;
 
   ic = data;
   if (ic->entry_dialog) return;
   
   man = e_manager_current_get();
   if (!man) return;
   con = e_container_current_get(man);
   if (!con) return;

   /* FIXME: get and store properties dialog into icon */
   e_fm_prop_file(con, &(ic->info));
}

static void
_e_fm2_file_delete(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Manager *man;
   E_Container *con;
   E_Dialog *dialog;
   E_Fm2_Icon *ic;
   char text[4096 + 256];
   Evas_List *sel;
   
   man = e_manager_current_get();
   if (!man) return;
   con = e_container_current_get(man);
   if (!con) return;
   
   ic = data;
   if (ic->dialog) return;
   dialog = e_dialog_new(con, "E", "_fm_file_delete_dialog");
   ic->dialog = dialog;
   E_OBJECT(dialog)->data = ic;
   e_object_del_attach_func_set(E_OBJECT(dialog), _e_fm2_file_delete_delete_cb);
   e_dialog_button_add(dialog, _("Yes"), NULL, _e_fm2_file_delete_yes_cb, ic);
   e_dialog_button_add(dialog, _("No"), NULL, _e_fm2_file_delete_no_cb, ic);
   e_dialog_button_focus_num(dialog, 1);
   e_dialog_title_set(dialog, _("Confirm Delete"));
   sel = e_fm2_selected_list_get(ic->sd->obj);
   if ((!sel) || (evas_list_count(sel) == 1))
     snprintf(text, sizeof(text), 
	      _("Are you sure you want to delete<br>"
		"<hilight>%s</hilight> ?"),
	      ic->info.file);
   else
     {
	snprintf(text, sizeof(text), 
		 _("Are you sure you want to delete<br>"
		   "the %d selected files in:<br>"
		   "<hilight>%s</hilight> ?"),
		 evas_list_count(sel),
		 ic->sd->realpath);
     }
   if (sel) evas_list_free(sel);
   e_dialog_text_set(dialog, text);
   e_win_centered_set(dialog->win, 1);
   e_dialog_show(dialog);
}

static void
_e_fm2_file_delete_delete_cb(void *obj)
{
   E_Fm2_Icon *ic;
   
   ic = E_OBJECT(obj)->data;
   ic->dialog = NULL;
}

static void
_e_fm2_file_delete_yes_cb(void *data, E_Dialog *dialog)
{
   E_Fm2_Icon *ic;
   char buf[4096];
   Evas_List *sel, *l;
   E_Fm2_Icon_Info *ici;
   int ok;
   
   ic = data;
   ic->dialog = NULL;
   
   e_object_del(E_OBJECT(dialog));
   sel = e_fm2_selected_list_get(ic->sd->obj);
   if (sel && (evas_list_count(sel) != 1))
     {
	for (l = sel; l; l = l->next)
	  {
	     ok = 1;
	     ici = l->data;
	     if (!ici->pseudo_link)
	       {
		  snprintf(buf, sizeof(buf), "%s/%s", ic->sd->realpath, ici->file);
		  if (e_filereg_file_protected(buf)) continue;
		  
		  /* FIXME: recursive rm might block - need to get smart */
		  e_fm2_fop_delete_add(ic->sd->obj, ici);
/*		  
		  if (!(ecore_file_recursive_rm(buf)))
		    {
		       char text[4096 + 256];
		     
		       ok = 0;
		       man = e_manager_current_get();
		       if (man)
			 {
			    con = e_container_current_get(man);
			    if (con)
			      {
				 dialog = e_dialog_new(con, "E", "_fm_file_delete_error_dialog");
				 e_dialog_button_add(dialog, _("OK"), NULL, NULL, NULL);
				 e_dialog_button_focus_num(dialog, 1);
				 e_dialog_title_set(dialog, _("Error"));
				 snprintf(text, sizeof(text),
					  _("Could not delete <br>"
					    "<hilight>%s</hilight>"), buf);
				 e_dialog_text_set(dialog, text);
				 e_win_centered_set(dialog->win, 1);
				 e_dialog_show(dialog);
				 // e_object_del(E_OBJECT(dialog));
			      }
			 }
		    }
		  else
 */
//		  ici->deleted = 1;
		  ok = 0;
	       }
	     else ici->deleted = 1;
	     if (ok) _e_fm2_live_file_del(ic->sd->obj, ici->file);
	  }
	evas_list_free(sel);
     }
   else
     {
	if (!ic->info.pseudo_link)
	  {
	     snprintf(buf, sizeof(buf), "%s/%s", ic->sd->realpath, ic->info.file);
	     if (e_filereg_file_protected(buf)) return;

	     /* FIXME: recursive rm might block - need to get smart */
	     e_fm2_fop_delete_add(ic->sd->obj, &(ic->info));
/*	     
	     if (!(ecore_file_recursive_rm(buf)))
	       {
		  char text[4096 + 256];
		  
		  man = e_manager_current_get();
		  if (!man) return;
		  con = e_container_current_get(man);
		  if (!con) return;
		  
		  dialog = e_dialog_new(con, "E", "_fm_file_delete_error_dialog");
		  e_dialog_button_add(dialog, _("OK"), NULL, NULL, NULL);
		  e_dialog_button_focus_num(dialog, 1);
		  e_dialog_title_set(dialog, _("Error"));
		  snprintf(text, sizeof(text),
			   _("Could not delete <br>"
			     "<hilight>%s</hilight>"), buf);
		  e_dialog_text_set(dialog, text);
		  e_win_centered_set(dialog->win, 1);
		  e_dialog_show(dialog);
		  // e_object_del(E_OBJECT(dialog));
		  return;
	       }
 */
	  }
	else
	  {
	     ic->info.deleted = 1;
	     _e_fm2_live_file_del(ic->sd->obj, ic->info.file);
	  }
     }
   
//   if (ic->sd->order_file) _e_fm2_order_file_rewrite(ic->sd->obj);
   /* FIXME: disable refresh - modify icons in-place */
//   if (ic->sd->refresh_job) ecore_job_del(ic->sd->refresh_job);
//   ic->sd->refresh_job = ecore_job_add(_e_fm2_refresh_job_cb, ic->sd->obj);
   
   evas_object_smart_callback_call(ic->sd->obj, "files_deleted", NULL);
}

static void
_e_fm2_file_delete_no_cb(void *data, E_Dialog *dialog)
{
   E_Fm2_Icon *ic;
   
   ic = data;
   ic->dialog = NULL;
   e_object_del(E_OBJECT(dialog));
}

static void
_e_fm2_refresh_job_cb(void *data)
{
   E_Fm2_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(data);
   if (!sd) return;
   e_fm2_refresh(data);
   sd->refresh_job = NULL;
}




static void
_e_fm2_live_file_add(Evas_Object *obj, const char *file, const char *file_rel, int after)
{
   E_Fm2_Smart_Data *sd;
   E_Fm2_Action *a;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   a = E_NEW(E_Fm2_Action, 1);
   if (!a) return;
   sd->live.actions = evas_list_append(sd->live.actions, a);
   a->type = FILE_ADD;
   a->file = evas_stringshare_add(file);
   if (file_rel) a->file2 = evas_stringshare_add(file_rel);
   a->flags = after;
   _e_fm2_live_process_begin(obj);
}

static void
_e_fm2_live_file_del(Evas_Object *obj, const char *file)
{
   E_Fm2_Smart_Data *sd;
   E_Fm2_Action *a;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   a = E_NEW(E_Fm2_Action, 1);
   if (!a) return;
   sd->live.actions = evas_list_append(sd->live.actions, a);
   a->type = FILE_DEL;
   a->file = evas_stringshare_add(file);
   _e_fm2_live_process_begin(obj);
}

static void
_e_fm2_live_file_changed(Evas_Object *obj, char *file)
{
   E_Fm2_Smart_Data *sd;
   E_Fm2_Action *a;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   a = E_NEW(E_Fm2_Action, 1);
   if (!a) return;
   sd->live.actions = evas_list_append(sd->live.actions, a);
   a->type = FILE_CHANGE;
   a->file = evas_stringshare_add(file);
   _e_fm2_live_process_begin(obj);
}

static void
_e_fm2_live_process_begin(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if ((sd->order) || (sd->dir)) return;
   if (!sd->live.actions) return;
   if ((sd->live.idler) || (sd->live.timer) ||
       (sd->scan_idler) || (sd->scan_timer)) return;
   sd->live.idler = ecore_idler_add(_e_fm2_cb_live_idler, obj);
   sd->live.timer = ecore_timer_add(0.2, _e_fm2_cb_live_timer, obj);
}

static void
_e_fm2_live_process_end(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   E_Fm2_Action *a;
   
   sd = evas_object_smart_data_get(obj);
   while (sd->live.actions)
     {
	a = sd->live.actions->data;
	sd->live.actions = evas_list_remove_list(sd->live.actions, sd->live.actions);
	if (a->file) evas_stringshare_del(a->file);
	if (a->file2) evas_stringshare_del(a->file2);
	free(a);
     }
   if (sd->live.idler)
     {
	ecore_idler_del(sd->live.idler);
	sd->live.idler = NULL;
     }
   if (sd->live.timer)
     {
	ecore_timer_del(sd->live.timer);
	sd->live.timer = NULL;
     }
}

static void
_e_fm2_live_process(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   E_Fm2_Action *a;
   Evas_List *l;
   E_Fm2_Icon *ic;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd->live.actions) return;
   a = sd->live.actions->data;
   sd->live.actions = evas_list_remove_list(sd->live.actions, sd->live.actions);
   switch (a->type)
     {
      case FILE_ADD:
	/* new file to sort in place */
	if (!((a->file[0] == '.') && (!sd->show_hidden_files)))
	  _e_fm2_file_add(obj, a->file, 1, a->file2, a->flags);
	break;
      case FILE_DEL:
	if (!((a->file[0] == '.') && (!sd->show_hidden_files)))
	  _e_fm2_file_del(obj, a->file);
	sd->live.deletions = 1;
	break;
      case FILE_CHANGE:
	if (!((a->file[0] == '.') && (!sd->show_hidden_files)))
	  {
	     for (l = sd->icons; l; l = l->next)
	       {
		  ic = l->data;
		  if (!strcmp(ic->info.file, a->file))
		    {
		       int realized;
		       
		       realized = ic->realized;
		       if (realized) _e_fm2_icon_unrealize(ic);
		       _e_fm2_icon_unfill(ic);
		       _e_fm2_icon_fill(ic);
		       if (realized) _e_fm2_icon_realize(ic);
		       break;
		    }
	       }
	  }
	break;
      default:
	break;
     }
   if (a->file) evas_stringshare_del(a->file);
   if (a->file2) evas_stringshare_del(a->file2);
   free(a);
}

static int
_e_fm2_cb_live_idler(void *data)
{
   E_Fm2_Smart_Data *sd;
   double t;
   
   sd = evas_object_smart_data_get(data);
   if (!sd) return 0;
   t = ecore_time_get();
   do
     {
	if (!sd->live.actions) break;
	_e_fm2_live_process(data);
     }
   while ((ecore_time_get() - t) > 0.02);
   if (sd->live.actions) return 1;
   _e_fm2_live_process_end(data);
   _e_fm2_cb_live_timer(data);
   if ((sd->order_file) || (sd->config->view.always_order))
     _e_fm2_order_file_rewrite(data);
   sd->live.idler = NULL;
   return 0;
}

static int
_e_fm2_cb_live_timer(void *data)
{
   E_Fm2_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(data);
   if (!sd) return 0;
   if (sd->queue) _e_fm2_queue_process(data);
   else if (sd->iconlist_changed)
     {
	if (sd->resize_job) ecore_job_del(sd->resize_job);
	sd->resize_job = ecore_job_add(_e_fm2_cb_resize_job, sd->obj);
     }
   else
     {
	if (sd->live.deletions)
	  {
	     sd->iconlist_changed = 1;
	     if (sd->resize_job) ecore_job_del(sd->resize_job);
	     sd->resize_job = ecore_job_add(_e_fm2_cb_resize_job, sd->obj);
	  }
     }
   sd->live.deletions = 0;
   sd->live.timer = NULL;
   if ((!sd->queue) && (!sd->live.idler)) return 0;
   sd->live.timer = ecore_timer_add(0.2, _e_fm2_cb_live_timer, data);
   return 0;
}

static void
_e_fm2_cb_file_monitor(void *data, Ecore_File_Monitor *em, Ecore_File_Event event, const char *path)
{
   E_Fm2_Smart_Data *sd;
   char *file;

   sd = evas_object_smart_data_get(data);
   if (!sd) return;
   file = (char *)ecore_file_get_file(path);
   if (!strcmp(file, ".order"))
     {
	/* FIXME: for now, ignore, BUT later load up .order - compare with icon
	 * order in list and fi they differ - use new .order */
     }
   else
     {
	if ((event == ECORE_FILE_EVENT_CREATED_FILE) ||
	    (event == ECORE_FILE_EVENT_CREATED_DIRECTORY))
	  {
	     _e_fm2_live_file_add(sd->obj, file, NULL, 0);
//	     printf("FADD %s\n", file);
	  }
	else if ((event == ECORE_FILE_EVENT_DELETED_FILE) ||
		 (event == ECORE_FILE_EVENT_DELETED_DIRECTORY))
	  {
	     _e_fm2_live_file_del(sd->obj, file);
//	     printf("FDEL %s\n", file);
	  }
	else if (event == ECORE_FILE_EVENT_MODIFIED)
	  {
	     _e_fm2_live_file_changed(sd->obj, file);
//	     printf("FMOD %s\n", file);
	  }
	else if (event == ECORE_FILE_EVENT_DELETED_SELF)
	  {
	     evas_object_smart_callback_call(sd->obj, "dir_deleted", NULL);
	  }
     }
}

static E_Fm2_Fop *
_e_fm2_fop_add(E_Fm2_Smart_Data *sd)
{
   if (!sd->fop)
     {
	sd->fop = E_NEW(E_Fm2_Fop, 1);
	if (!sd->fop) return NULL;
	sd->fop->obj = sd->obj;
	sd->fop->dir = evas_stringshare_add(e_fm2_real_path_get(sd->obj));
	if (!sd->fop->dir)
	  {
	     free(sd->fop);
	     sd->fop = NULL;
	     return NULL;
	  }
	sd->fop->idler = ecore_idler_add(_e_fm2_cb_fop_idler, sd->fop);
	if (!sd->fop->idler)
	  {
	     evas_stringshare_del(sd->fop->dir);
	     free(sd->fop);
	     sd->fop = NULL;
	     return NULL;
	  }
	sd->fop->timer = ecore_timer_add(0.2, _e_fm2_cb_fop_timer, sd->fop);
	sd->busy_count++;
	if (sd->busy_count == 1)
	  edje_object_signal_emit(sd->overlay, "e,state,busy,start", "e");
     }
   _e_fm2_fop_list = evas_list_append(_e_fm2_fop_list, sd->fop);
   return sd->fop;
}

static void
_e_fm2_fop_del(E_Fm2_Fop *fop)
{
   if (fop->idler)
     {
	ecore_idler_del(fop->idler);
	fop->idler = NULL;
     } 
   if (fop->timer)
     {
	_e_fm2_cb_fop_timer(fop);
	ecore_timer_del(fop->timer);
	fop->timer = NULL;
     } 
   if (fop->dir)
     {
	evas_stringshare_del(fop->dir);
	fop->dir = NULL;
     }
   if (fop->obj)
     {
	E_Fm2_Smart_Data *sd;
	
	sd = evas_object_smart_data_get(fop->obj);
	if (sd) sd->fop = NULL;
	fop->obj = NULL;
	sd->busy_count--;
	if (sd->busy_count == 0)
	  edje_object_signal_emit(sd->overlay, "e,state,busy,stop", "e");
     }
   _e_fm2_fop_list = evas_list_remove(_e_fm2_fop_list, fop);
  free(fop);
}

static void
_e_fm2_fop_detach(E_Fm2_Smart_Data *sd)
{
   if (!sd->fop) return;
   sd->fop->obj = NULL;
   sd->fop = NULL;
   sd->busy_count--;
   if (sd->busy_count == 0)
     edje_object_signal_emit(sd->overlay, "e,state,busy,stop", "e");
}

static int
_e_fm2_fop_process(E_Fm2_Fop *fop)
{
   E_Fm2_Fop_Item *fi, *fi2;
   char buf[4096];
   struct dirent *dp;
   E_Manager *man;
   E_Container *con;
   E_Dialog *dialog;
   
   if (!fop->items) return 0;
   fi = fop->items->data;
   switch (fi->type)
     {
      case FOP_DELETE:
	if (fi->dir)
	  {
	     dp = readdir(fi->dir);
	     if (!dp)
	       {
		  snprintf(buf, sizeof(buf), "%s/%s", fi->fop->dir, fi->file);
		  if (ecore_file_rmdir(buf))
		    {
		       if ((fi->fop->obj) && (!strchr(fi->file, '/')))
			 _e_fm2_live_file_del(fi->fop->obj, fi->file);
		    }
		  /* FIXME: if deletign vast dirs and lots of errors happen
		   * we need a central dialog that just collects all of them
		   * and not have dozens or 100's of dialogs */
		  else
		    {
		       char text[4096 + 256];
		     
		       man = e_manager_current_get();
		       if (man)
			 {
			    con = e_container_current_get(man);
			    if (con)
			      {
				 dialog = e_dialog_new(con, "E", "_fm_file_delete_error_dialog");
				 e_dialog_button_add(dialog, _("OK"), NULL, NULL, NULL);
				 e_dialog_button_focus_num(dialog, 1);
				 e_dialog_title_set(dialog, _("Error"));
				 snprintf(text, sizeof(text),
					  _("Could not delete <br>"
					    "<hilight>%s</hilight>"), buf);
				 e_dialog_text_set(dialog, text);
				 e_win_centered_set(dialog->win, 1);
				 e_dialog_show(dialog);
			      }
			 }
		    }
	       }
	     else
	       {
		  if ((!strcmp(dp->d_name, ".")) || (!strcmp(dp->d_name, ".."))) return 1;
		  fi2 = E_NEW(E_Fm2_Fop_Item, 1);
		  fi2->fop = fop;
		  fi2->type = FOP_DELETE;
		  snprintf(buf, sizeof(buf), "%s/%s/%s", fi->fop->dir, fi->file, dp->d_name);
		  if (ecore_file_is_dir(buf))
		    {
		       char *lnk;
		       
		       lnk = ecore_file_readlink(buf);
		       if (lnk) free(lnk);
		       else fi2->is_dir = 1;
		    }
		  snprintf(buf, sizeof(buf), "%s/%s", fi->file, dp->d_name);
		  fi2->file = evas_stringshare_add(buf);
		  fi->fop->items = evas_list_prepend(fi->fop->items, fi2);
//		  printf("ADD: %s\n", fi2->file);
		  return 1;
	       }
	  }
	else if (fi->is_dir)
	  {
	     snprintf(buf, sizeof(buf), "%s/%s", fi->fop->dir, fi->file);
	     fi->dir = opendir(buf);
	     if (!fi->dir)
	       {
		  if (ecore_file_rmdir(buf))
		    {
		       if ((fi->fop->obj) && (!strchr(fi->file, '/')))
			 _e_fm2_live_file_del(fi->fop->obj, fi->file);
		    }
		  /* FIXME: if deletign vast dirs and lots of errors happen
		   * we need a central dialog that just collects all of them
		   * and not have dozens or 100's of dialogs */
		  else
		    {
		       char text[4096 + 256];
		     
		       man = e_manager_current_get();
		       if (man)
			 {
			    con = e_container_current_get(man);
			    if (con)
			      {
				 dialog = e_dialog_new(con, "E", "_fm_file_delete_error_dialog");
				 e_dialog_button_add(dialog, _("OK"), NULL, NULL, NULL);
				 e_dialog_button_focus_num(dialog, 1);
				 e_dialog_title_set(dialog, _("Error"));
				 snprintf(text, sizeof(text),
					  _("Could not delete <br>"
					    "<hilight>%s</hilight>"), buf);
				 e_dialog_text_set(dialog, text);
				 e_win_centered_set(dialog->win, 1);
				 e_dialog_show(dialog);
			      }
			 }
		    }
	       }
	     else
	       return 1;
	  }
	else
	  {
	     snprintf(buf, sizeof(buf), "%s/%s", fi->fop->dir, fi->file);
	     if (ecore_file_unlink(buf))
	       {
		  if ((fi->fop->obj) && (!strchr(fi->file, '/')))
		    _e_fm2_live_file_del(fi->fop->obj, fi->file);
	       }
	     /* FIXME: if deletign vast dirs and lots of errors happen
	      * we need a central dialog that just collects all of them
	      * and not have dozens or 100's of dialogs */
	     else
	       {
		  char text[4096 + 256];
		  
		  man = e_manager_current_get();
		  if (man)
		    {
		       con = e_container_current_get(man);
		       if (con)
			 {
			    dialog = e_dialog_new(con, "E", "_fm_file_delete_error_dialog");
			    e_dialog_button_add(dialog, _("OK"), NULL, NULL, NULL);
			    e_dialog_button_focus_num(dialog, 1);
			    e_dialog_title_set(dialog, _("Error"));
			    snprintf(text, sizeof(text),
				     _("Could not delete <br>"
				       "<hilight>%s</hilight>"), buf);
			    e_dialog_text_set(dialog, text);
			    e_win_centered_set(dialog->win, 1);
			    e_dialog_show(dialog);
			 }
		    }
	       }
	  }
	break;
      case FOP_MOVE:
	/* FIXME: handle moves */
	if (ecore_file_exists(fi->file2))
	  {
	     /* FIXME: error - file exists */
	  }
	else
	  {
	     if (ecore_file_mv(fi->file, fi->file2))
	       {
		  if ((fi->fop->obj) && (fi->file_add))
		    _e_fm2_live_file_add(fi->fop->obj,
					 fi->file, fi->file3, fi->after);
	       }
	     else
	       {
		  /* FIXME: error - move failed */
	       }
	  }
	break;
      case FOP_LINK:
	if (!ecore_file_symlink(fi->file, fi->file2))
	  {
	     /* FIXME: error - symlink failed */
	  }
	break;
      case FOP_ADD:
	if (fi->fop->obj)
	  _e_fm2_live_file_add(fi->fop->obj,
			       ecore_file_get_file(fi->file),
			       fi->file3, fi->after);
	break;
      default:
	break;
     }
   /* remove and free */
   fop->items = evas_list_remove_list(fop->items, fop->items);
   if (fi->file)
     {
	evas_stringshare_del(fi->file);
	fi->file = NULL;
     }
   if (fi->file2)
     {
	evas_stringshare_del(fi->file2);
	fi->file2 = NULL;
     }
   if (fi->file3)
     {
	evas_stringshare_del(fi->file3);
	fi->file3 = NULL;
     }
   if (fi->dir)
     {
	closedir(fi->dir);
	fi->dir = NULL;
     }
   free(fi);
   if (!fop->items) return 0;
   return 1;
}

static int
_e_fm2_cb_fop_idler(void *data)
{
   E_Fm2_Fop *fop;
   
   fop = data;
   if (!_e_fm2_fop_process(fop))
     {
	_e_fm2_fop_del(fop);
	return 0;
     }
   return 1;
}

static int
_e_fm2_cb_fop_timer(void *data)
{
   E_Fm2_Fop *fop;
   E_Fm2_Smart_Data *sd;
   char buf[4096];
   
   fop = data;
   if (!fop->obj) return 1;
   sd = evas_object_smart_data_get(fop->obj);
   snprintf(buf, sizeof(buf), _("%i Queued"), evas_list_count(fop->items));
   edje_object_part_text_set(sd->overlay, "e.text.busy_label", buf);
   return 1;
}
