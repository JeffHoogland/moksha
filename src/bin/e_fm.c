/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_fm_hal.h"
#include "e_fm_op.h"

#define OVERCLIP 128

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

typedef struct _E_Fm2_Smart_Data         E_Fm2_Smart_Data;
typedef struct _E_Fm2_Region             E_Fm2_Region;
typedef struct _E_Fm2_Finfo              E_Fm2_Finfo;
typedef struct _E_Fm2_Action             E_Fm2_Action;
typedef struct _E_Fm2_Client             E_Fm2_Client;
typedef struct _E_Fm2_Uri                E_Fm2_Uri;
typedef struct _E_Fm2_Context_Menu_Data  E_Fm2_Context_Menu_Data;

struct _E_Fm2_Smart_Data
{
   int               id;
   Evas_Coord        x, y, w, h, pw, ph;
   Evas_Object      *obj;
   Evas_Object      *clip;
   Evas_Object      *underlay;
   Evas_Object      *overlay;
   Evas_Object      *drop;
   Evas_Object      *drop_in;
   Evas_Object      *sel_rect;
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
      Eina_List     *list;
      int            member_max;
   } regions;
   struct {
      struct {
	 void (*func) (void *data, Evas_Object *obj, E_Menu *m, E_Fm2_Icon_Info *info);
	 void *data;
      } start, end, replace;
      E_Fm2_Menu_Flags flags;
   } icon_menu;
   
   Eina_List        *icons;
   Eina_List        *icons_place;
   Eina_List        *queue;
   Ecore_Timer      *scan_timer;
   Ecore_Timer      *sort_idler;
   Ecore_Job        *scroll_job;
   Ecore_Job        *resize_job;
   Ecore_Job        *refresh_job;
   E_Menu           *menu;
   E_Entry_Dialog   *entry_dialog;
   unsigned char     iconlist_changed : 1;
   unsigned char     order_file : 1;
   unsigned char     typebuf_visible : 1;
   unsigned char     show_hidden_files : 1;
   unsigned char     listing : 1;
   unsigned char     inherited_dir_props : 1;
   signed char       view_mode; /* -1 = unset */
   signed short      icon_size; /* -1 = unset */
   
   E_Fm2_Config     *config;
   const char       *custom_theme;
   const char       *custom_theme_content;

   struct {
      Evas_Object      *obj, *obj2;
      Eina_List        *last_insert;
      Eina_List        **list_index;
      int              iter;
   } tmp;
   
   struct {
      Eina_List       *actions;
      Ecore_Idler     *idler;
      Ecore_Timer     *timer;
      unsigned char    deletions : 1;
   } live;
   
   struct {
      char            *buf;
      Ecore_Timer     *timer;
   } typebuf;
   
   int                 busy_count;
   
   E_Object           *eobj;
   E_Drop_Handler     *drop_handler;
   E_Fm2_Icon         *drop_icon;
   E_Fm2_Mount        *mount;
   signed char         drop_after;
   unsigned char       drop_show : 1;
   unsigned char       drop_in_show : 1;
   unsigned char       drop_all : 1;
   unsigned char       drag : 1;
   unsigned char       selecting : 1;
   struct {
      int ox, oy;
      int x, y, w, h;
   } selrect;
};
 
struct _E_Fm2_Region
{
   E_Fm2_Smart_Data *sd;
   Evas_Coord        x, y, w, h;
   Eina_List        *list;
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
   E_Config_Dialog  *prop_dialog;
   E_Dialog         *dialog;

   E_Fm2_Icon_Info   info;
   
   struct {
      Evas_Coord     x, y;
      unsigned char  start : 1;
      unsigned char  dnd : 1;
      unsigned char  src : 1;
   } drag;
   
   unsigned char     realized : 1;
   unsigned char     selected : 1;
   unsigned char     last_selected : 1;
   unsigned char     saved_pos : 1;
   unsigned char     odd : 1;
   unsigned char     down_sel : 1;
   unsigned char     removable_state_change : 1;
};

struct _E_Fm2_Finfo
{
   struct stat st;
   int broken_link;
   const char *lnk;
   const char *rlnk;
};

struct _E_Fm2_Action
{
   E_Fm2_Action_Type  type;
   const char        *file;
   const char        *file2;
   int                flags;
   E_Fm2_Finfo        finf;
};

struct _E_Fm2_Client
{
   Ecore_Ipc_Client *cl;
   int req;
};

struct _E_Fm2_Uri
{
   const char *hostname;
   const char *path;
};

struct _E_Fm2_Context_Menu_Data 
{
   E_Fm2_Icon *icon;
   E_Fm2_Mime_Handler *handler;
};

static const char *_e_fm2_dev_path_map(const char *dev, const char *path);
static void _e_fm2_file_add(Evas_Object *obj, const char *file, int unique, const char *file_rel, int after, E_Fm2_Finfo *finf);
static void _e_fm2_file_del(Evas_Object *obj, const char *file);
static void _e_fm2_queue_process(Evas_Object *obj); 
static void _e_fm2_queue_free(Evas_Object *obj);
static void _e_fm2_regions_free(Evas_Object *obj);
static void _e_fm2_regions_populate(Evas_Object *obj);
static void _e_fm2_icons_place(Evas_Object *obj);
static void _e_fm2_icons_free(Evas_Object *obj);
static void _e_fm2_regions_eval(Evas_Object *obj);
static void _e_fm2_config_free(E_Fm2_Config *cfg);

static void _e_fm2_dir_load_props(E_Fm2_Smart_Data *sd);
static void _e_fm2_dir_save_props(E_Fm2_Smart_Data *sd);

static Evas_Object *_e_fm2_file_fm2_find(const char *file);
static E_Fm2_Icon *_e_fm2_icon_find(Evas_Object *obj, const char *file);
static const char *_e_fm2_uri_escape(const char *path);
static Eina_List *_e_fm2_uri_path_list_get(Eina_List *uri_list);
static Eina_List *_e_fm2_uri_icon_list_get(Eina_List *uri);
	   
static E_Fm2_Icon *_e_fm2_icon_new(E_Fm2_Smart_Data *sd, const char *file, E_Fm2_Finfo *finf);
static void _e_fm2_icon_unfill(E_Fm2_Icon *ic);
static int _e_fm2_icon_fill(E_Fm2_Icon *ic, E_Fm2_Finfo *finf);
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
static int _e_fm2_cb_icon_sort(const void *data1, const void *data2);
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

static void _e_fm2_menu(Evas_Object *obj, unsigned int timestamp);
static void _e_fm2_menu_post_cb(void *data, E_Menu *m);
static void _e_fm2_icon_menu(E_Fm2_Icon *ic, Evas_Object *obj, unsigned int timestamp);
static void _e_fm2_icon_menu_post_cb(void *data, E_Menu *m);
static void _e_fm2_icon_menu_item_cb(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_fm2_toggle_inherit_dir_props(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_fm2_view_menu_pre(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_fm2_view_menu_grid_icons_cb(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_fm2_view_menu_custom_icons_cb(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_fm2_view_menu_list_cb(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_fm2_view_menu_use_default_cb(void *data, E_Menu *m, E_Menu_Item *mi);
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
static void _e_fm2_file_properties_delete_cb(void *obj);

static void _e_fm_retry_abort_dialog(int pid, const char *str);
static void _e_fm_retry_abort_delete_cb(void *obj);
static void _e_fm_retry_abort_retry_cb(void *data, E_Dialog *dialog);
static void _e_fm_retry_abort_abort_cb(void *data, E_Dialog *dialog);

static void _e_fm_overwrite_dialog(int pid, const char *str);
static void _e_fm_overwrite_delete_cb(void *obj);
static void _e_fm_overwrite_no_cb(void *data, E_Dialog *dialog);
static void _e_fm_overwrite_no_all_cb(void *data, E_Dialog *dialog);
static void _e_fm_overwrite_yes_cb(void *data, E_Dialog *dialog);
static void _e_fm_overwrite_yes_all_cb(void *data, E_Dialog *dialog);

static void _e_fm_error_dialog(int pid, const char *str);
static void _e_fm_error_delete_cb(void *obj);
static void _e_fm_error_retry_cb(void *data, E_Dialog *dialog);
static void _e_fm_error_abort_cb(void *data, E_Dialog *dialog);
static void _e_fm_error_ignore_this_cb(void *data, E_Dialog *dialog);
static void _e_fm_error_ignore_all_cb(void *data, E_Dialog *dialog);

static void _e_fm2_file_delete(Evas_Object *obj);
static void _e_fm2_file_delete_menu(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_fm2_file_delete_delete_cb(void *obj);
static void _e_fm2_file_delete_yes_cb(void *data, E_Dialog *dialog);
static void _e_fm2_file_delete_no_cb(void *data, E_Dialog *dialog);
static void _e_fm2_refresh_job_cb(void *data);
static void _e_fm_file_buffer_clear(void);
static void _e_fm2_file_cut(Evas_Object *obj);
static void _e_fm2_file_copy(Evas_Object *obj);
static void _e_fm2_file_paste(Evas_Object *obj);
static void _e_fm2_file_cut_menu(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_fm2_file_copy_menu(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_fm2_file_paste_menu(void *data, E_Menu *m, E_Menu_Item *mi);

static void _e_fm2_live_file_add(Evas_Object *obj, const char *file, const char *file_rel, int after, E_Fm2_Finfo *finf);
static void _e_fm2_live_file_del(Evas_Object *obj, const char *file);
static void _e_fm2_live_file_changed(Evas_Object *obj, const char *file, E_Fm2_Finfo *finf);
static void _e_fm2_live_process_begin(Evas_Object *obj);
static void _e_fm2_live_process_end(Evas_Object *obj);
static void _e_fm2_live_process(Evas_Object *obj);
static int _e_fm2_cb_live_idler(void *data);
static int _e_fm2_cb_live_timer(void *data);

static int _e_fm2_theme_edje_object_set(E_Fm2_Smart_Data *sd, Evas_Object *o, const char *category, const char *group);
static int _e_fm2_theme_edje_icon_object_set(E_Fm2_Smart_Data *sd, Evas_Object *o, const char *category, const char *group);

static void _e_fm2_mouse_1_handler(E_Fm2_Icon *ic, int up, Evas_Modifier *modifiers);

static void _e_fm2_client_spawn(void);
static E_Fm2_Client *_e_fm2_client_get(void);
static int _e_fm2_client_monitor_add(const char *path);
static void _e_fm2_client_monitor_del(int id, const char *path);
static int _e_fm_client_file_del(const char *args);
static int _e_fm2_client_file_trash(const char *path);
static int _e_fm2_client_file_mkdir(const char *path, const char *rel, int rel_to, int x, int y, int res_w, int res_h);
static int _e_fm_client_file_move(const char *args);
static int _e_fm2_client_file_symlink(const char *path, const char *dest, const char *rel, int rel_to, int x, int y, int res_w, int res_h);
static int _e_fm_client_file_copy(const char *args);

static void _e_fm2_sel_rect_update(void *data);
static inline void _e_fm2_context_menu_append(Evas_Object *obj, const char *path, Eina_List *l, E_Menu *mn, E_Fm2_Icon *ic);
static int _e_fm2_context_list_sort(const void *data1, const void *data2);

static char *_e_fm_string_append_char(char *str, size_t *size, size_t *len, char c);
static char *_e_fm_string_append_quoted(char *str, size_t *size, size_t *len, const char *src);

static char *_e_fm2_meta_path = NULL;
static Evas_Smart *_e_fm2_smart = NULL;
static Eina_List *_e_fm2_list = NULL;
static Eina_List *_e_fm2_list_remove = NULL;
static int _e_fm2_list_walking = 0;
static Eina_List *_e_fm2_client_list = NULL;
static Eina_List *_e_fm2_menu_contexts = NULL;
static Eina_List *_e_fm_file_buffer = NULL; /* Files for copy&paste are saved here. */
static int _e_fm_file_buffer_cutting = 0;
static int _e_fm_file_buffer_copying = 0;

/* contains:
 * _e_volume_edd
 * _e_storage_edd
 * _e_volume_free()
 * _e_storage_free()
 * _e_volume_edd_new()
 * _e_storage_edd_new()
 * _e_storage_volume_edd_init()
 * _e_storage_volume_edd_shutdown()
 */
#define E_FM_SHARED_CODEC
#include "e_fm_shared.h"
#undef E_FM_SHARED_CODEC

static inline void
_eina_stringshare_replace(const char **p, const char *str)
{
   str = eina_stringshare_add(str);
   eina_stringshare_del(*p);
   if (*p == str)
     return;
   *p = str;
}

/***/

EAPI int
e_fm2_init(void)
{
   const char *homedir;
   char  path[PATH_MAX];

   eina_stringshare_init();
   ecore_job_init();
   _e_storage_volume_edd_init();
   homedir = e_user_homedir_get();
   snprintf(path, sizeof(path), "%s/.e/e/fileman/metadata", homedir);
   ecore_file_mkpath(path);
   _e_fm2_meta_path = strdup(path);

     {
	static const Evas_Smart_Class sc =
	  {
	     "e_fm",
	       EVAS_SMART_CLASS_VERSION,
	       _e_fm2_smart_add, /* add */
	       _e_fm2_smart_del, /* del */
	       _e_fm2_smart_move, /* move */
	       _e_fm2_smart_resize, /* resize */
	       _e_fm2_smart_show,/* show */
	       _e_fm2_smart_hide,/* hide */
	       _e_fm2_smart_color_set, /* color_set */
	       _e_fm2_smart_clip_set, /* clip_set */
	       _e_fm2_smart_clip_unset, /* clip_unset */
	       NULL,
	       NULL,
	       NULL,
	       NULL
	  };
	_e_fm2_smart = evas_smart_class_new(&sc);
     }
//   _e_fm2_client_spawn();
   e_fm2_custom_file_init();
   efreet_mime_init();
   return 1;
}

EAPI int
e_fm2_shutdown(void)
{
   evas_smart_free(_e_fm2_smart);
   _e_fm2_smart = NULL;
   E_FREE(_e_fm2_meta_path);
   e_fm2_custom_file_shutdown();
   _e_storage_volume_edd_shutdown();
   efreet_mime_shutdown();
   ecore_job_shutdown();
   eina_stringshare_shutdown();
   return 1;
}

EAPI Evas_Object *
e_fm2_add(Evas *evas)
{
   return evas_object_smart_add(evas, _e_fm2_smart);
}

static void
_e_fm2_cb_mount_ok(void *data)
{
   E_Fm2_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(data);
   if (!sd) return; // safety
   
   if (strcmp(sd->mount->mount_point, sd->realpath))
     {
	e_fm2_path_set(sd->obj, "/", sd->mount->mount_point);
     }
   else
     {
	sd->id = _e_fm2_client_monitor_add(sd->mount->mount_point);
	sd->listing = 1;
	evas_object_smart_callback_call(data, "dir_changed", NULL);
	sd->tmp.iter = 0;
     }
}

static void
_e_fm2_cb_mount_fail(void *data)
{
   E_Fm2_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(data);
   if (!sd) return; // safety
   /* FIXME; some dialog */
   if (sd->mount)
     {
	e_fm2_hal_unmount(sd->mount);
	sd->mount = NULL;
	evas_object_smart_callback_call(data, "dir_deleted", NULL);
     }
}

EAPI void
e_fm2_path_set(Evas_Object *obj, const char *dev, const char *path)
{
   E_Fm2_Smart_Data *sd;
   const char *realpath;

   sd = evas_object_smart_data_get(obj);
   if (!sd || !path) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety

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
   
   realpath = _e_fm2_dev_path_map(dev, path);
   /* If the path doesn't exist, popup a dialog */
   if (dev && strncmp(dev, "removable:", 10)
       && !ecore_file_exists(realpath))
     {
	E_Manager *man;
	E_Container *con;
	E_Dialog *dialog;
	char text[4096 + 256];

	man = e_manager_current_get();
	if (!man) return;
	con = e_container_current_get(man);
	if (!con) return;

	dialog = e_dialog_new(con, "E", "_fm_file_unexisting_path_dialog");
	e_dialog_button_add(dialog, _("Close"), NULL, NULL, dialog);
	e_dialog_button_focus_num(dialog, 0);
	e_dialog_title_set(dialog, _("Nonexistent path"));

	snprintf(text, sizeof(text), 
		 _("%s doesn't exist."),
		 realpath);

	e_dialog_text_set(dialog, text);
	e_win_centered_set(dialog->win, 1);
	e_dialog_show(dialog);
	return;
     }

   if (sd->realpath) _e_fm2_client_monitor_del(sd->id, sd->realpath);
   sd->listing = 0;

   _eina_stringshare_replace(&sd->dev, dev);
   _eina_stringshare_replace(&sd->path, path);
   eina_stringshare_del(sd->realpath);
   sd->realpath = realpath;
   _e_fm2_queue_free(obj);
   _e_fm2_regions_free(obj);
   _e_fm2_icons_free(obj);
   edje_object_part_text_set(sd->overlay, "e.text.busy_label", "");

   _e_fm2_dir_load_props(sd);

   /* If the path change from a mountpoint to something else, we fake-unmount */
   if (sd->mount && sd->mount->mount_point 
       && strncmp(sd->mount->mount_point, sd->realpath, 
		  strlen(sd->mount->mount_point)))
     {
	e_fm2_hal_unmount(sd->mount);
	sd->mount = NULL;
     }

   /* If the path is of type removable: we add a new mountpoint */
   if (sd->dev && !sd->mount && !strncmp(sd->dev, "removable:", 10))
     {
	E_Volume *v = NULL;
	
	v = e_fm2_hal_volume_find(sd->dev + strlen("removable:"));
	if (v) 
	  sd->mount = e_fm2_hal_mount(v, 
				     _e_fm2_cb_mount_ok, _e_fm2_cb_mount_fail,
				     NULL, NULL, obj);
     }
   else if (sd->config->view.open_dirs_in_place == 0)
     {
	E_Fm2_Mount *m;
	m = e_fm2_hal_mount_find(sd->realpath);
	if (m) 
	  sd->mount = e_fm2_hal_mount(m->volume, 
				     _e_fm2_cb_mount_ok, _e_fm2_cb_mount_fail,
				     NULL, NULL, obj);
     }
   
   if (!sd->mount || sd->mount->mounted)
     {
	sd->id = _e_fm2_client_monitor_add(sd->realpath);
	sd->listing = 1;
     }

   /* Clean up typebuf. */
   _e_fm2_typebuf_hide(obj);
   
   evas_object_smart_callback_call(obj, "dir_changed", NULL);
   sd->tmp.iter = 0;
}

EAPI void
e_fm2_underlay_show(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety
   evas_object_show(sd->underlay);
}

EAPI void
e_fm2_underlay_hide(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety
   evas_object_hide(sd->underlay);
}

EAPI void
e_fm2_all_unsel(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety
   _e_fm2_icon_desel_any(obj);
}

EAPI void
e_fm2_custom_theme_set(Evas_Object *obj, const char *path)
{
   E_Fm2_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety
   _eina_stringshare_replace(&sd->custom_theme, path);
   _e_fm2_theme_edje_object_set(sd, sd->drop, "base/theme/fileman",
				"list/drop_between");
   _e_fm2_theme_edje_object_set(sd, sd->drop_in, "base/theme/fileman",
				"list/drop_in");
   _e_fm2_theme_edje_object_set(sd, sd->overlay, "base/theme/fileman",
				"overlay");
}

EAPI void
e_fm2_custom_theme_content_set(Evas_Object *obj, const char *content)
{
   E_Fm2_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety
   _eina_stringshare_replace(&sd->custom_theme_content, content);
   _e_fm2_theme_edje_object_set(sd, sd->drop, "base/theme/fileman",
				"list/drop_between");
   _e_fm2_theme_edje_object_set(sd, sd->drop_in, "base/theme/fileman",
				"list/drop_in");
   _e_fm2_theme_edje_object_set(sd, sd->overlay, "base/theme/fileman",
				"overlay");
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

static E_Fm2_Custom_File *
_e_fm2_dir_load_props_from_parent(const char *path)
{
   E_Fm2_Custom_File *cf;
   char *parent;

   if ((!path) || (path[0] == '\0') || (strcmp(path, "/") == 0))
     return NULL;

   parent = ecore_file_dir_get(path);
   cf = e_fm2_custom_file_get(parent);
   if ((cf) && (cf->dir) && (cf->dir->prop.in_use))
     {
	free(parent);
	return cf;
     }

   cf = _e_fm2_dir_load_props_from_parent(parent);
   free(parent);
   return cf;
}

static void
_e_fm2_dir_load_props(E_Fm2_Smart_Data *sd)
{
   E_Fm2_Custom_File *cf;

   cf = e_fm2_custom_file_get(sd->realpath);
   if ((cf) && (cf->dir))
     {
	Evas_Coord x, y;

	if (sd->max.w - sd->w > 0)
	  x = (sd->max.w - sd->w) * cf->dir->pos.x;
	else
	  x = 0;

	if (sd->max.h - sd->h > 0)
	  y = (sd->max.h - sd->h) * cf->dir->pos.y;
	else
	  y = 0;

	e_fm2_pan_set(sd->obj, x, y);

	if (cf->dir->prop.in_use)
	  {
	     sd->view_mode = cf->dir->prop.view_mode;
	     sd->icon_size = cf->dir->prop.icon_size;
	     sd->order_file = !!cf->dir->prop.order_file;
	     sd->show_hidden_files = !!cf->dir->prop.show_hidden_files;
	     sd->inherited_dir_props = 0;
	     return;
	  }
     }
   else
     {
	sd->pos.x = 0;
	sd->pos.y = 0;
     }

   sd->inherited_dir_props = 1;

   cf = _e_fm2_dir_load_props_from_parent(sd->realpath);
   if ((cf) && (cf->dir) && (cf->dir->prop.in_use))
     {
	sd->view_mode = cf->dir->prop.view_mode;
	sd->icon_size = cf->dir->prop.icon_size;
	sd->order_file = !!cf->dir->prop.order_file;
	sd->show_hidden_files = !!cf->dir->prop.show_hidden_files;
     }
   else
     {
	sd->view_mode = -1;
	sd->icon_size = -1;
	sd->order_file = 0;
	sd->show_hidden_files = 0;
     }
}

static void
_e_fm2_dir_save_props(E_Fm2_Smart_Data *sd)
{
   E_Fm2_Custom_File *cf, cf0;
   E_Fm2_Custom_Dir dir0;

   cf = e_fm2_custom_file_get(sd->realpath);
   if (!cf)
     {
	cf = &cf0;
	memset(cf, 0, sizeof(*cf));
	cf->dir = &dir0;
     }
   else if (!cf->dir)
     {
	E_Fm2_Custom_File *cf2 = cf;
	cf = &cf0;
	memcpy(cf, cf2, sizeof(*cf2));
	cf->dir = &dir0;
     }

   if (sd->max.w - sd->w > 0)
     cf->dir->pos.x = sd->pos.x / (double)(sd->max.w - sd->w);
   else
     cf->dir->pos.x = 0.0;

   if (sd->max.h - sd->h)
     cf->dir->pos.y = sd->pos.y / (double)(sd->max.h - sd->h);
   else
     cf->dir->pos.y = 0.0;

   cf->dir->prop.icon_size = sd->icon_size;
   cf->dir->prop.view_mode = sd->view_mode;
   cf->dir->prop.order_file = sd->order_file;
   cf->dir->prop.show_hidden_files = sd->show_hidden_files;
   cf->dir->prop.in_use = !sd->inherited_dir_props;

   e_fm2_custom_file_set(sd->realpath, cf);
   e_fm2_custom_file_flush();
}

EAPI void
e_fm2_refresh(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety

   _e_fm2_dir_save_props(sd);

   _e_fm2_queue_free(obj);
   _e_fm2_regions_free(obj);
   _e_fm2_icons_free(obj);
   
   sd->order_file = 0;
   
   if (sd->realpath)
     {
	sd->listing = 0;
	_e_fm2_client_monitor_del(sd->id, sd->realpath);
	sd->id = _e_fm2_client_monitor_add(sd->realpath);
	sd->listing = 1;
     }
   
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
   char *path, *p;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety
   if (!sd->path) return;
   path = strdup(sd->path);
   if (!path) return;
   if ((p = strrchr(path, '/'))) *p = 0;
   if (*path == 0)
     e_fm2_path_set(obj, sd->dev, "/");
   else
     e_fm2_path_set(obj, sd->dev, path);

   free(path);
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
   sd->config->icon.key_hint = eina_stringshare_add(cfg->icon.key_hint);
   sd->config->theme.background = eina_stringshare_add(cfg->theme.background);
   sd->config->theme.frame = eina_stringshare_add(cfg->theme.frame);
   sd->config->theme.icons = eina_stringshare_add(cfg->theme.icons);
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

EAPI Eina_List *
e_fm2_selected_list_get(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   Eina_List *list = NULL, *l;
   E_Fm2_Icon *ic;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return NULL; // safety
   if (!evas_object_type_get(obj)) return NULL; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return NULL; // safety
   for (l = sd->icons; l; l = l->next)
     {
	ic = l->data;
	if (ic->selected)
	  list = eina_list_append(list, &(ic->info));
     }
   return list;
}

EAPI Eina_List *
e_fm2_all_list_get(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   Eina_List *list = NULL, *l;
   E_Fm2_Icon *ic;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return NULL; // safety
   if (!evas_object_type_get(obj)) return NULL; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return NULL; // safety
   for (l = sd->icons; l; l = l->next)
     {
	ic = l->data;
	list = eina_list_append(list, &(ic->info));
     }
   return list;
}

EAPI void
e_fm2_select_set(Evas_Object *obj, const char *file, int select)
{
   E_Fm2_Smart_Data *sd;
   Eina_List *l;
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
   Eina_List *l;
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
   const char *drop[] = { "enlightenment/desktop", "enlightenment/border", "text/uri-list" };
   
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
					 drop, 3, 
					 sd->x, sd->y, sd->w, sd->h);
   e_drop_handler_responsive_set(sd->drop_handler);
}

EAPI void
e_fm2_icons_update(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   Eina_List *l;
   E_Fm2_Icon *ic;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety
   for (l = sd->icons; l; l = l->next)
     {
	E_Fm2_Custom_File *cf;
	char buf[PATH_MAX];
	
	ic = l->data;
	
	eina_stringshare_del(ic->info.icon);
	ic->info.icon = NULL;
	ic->info.icon_type = 0;
	
	snprintf(buf, sizeof(buf), "%s/%s", ic->sd->realpath, ic->info.file);
	if ((e_util_glob_case_match(ic->info.file, "*.desktop")) ||
	    (e_util_glob_case_match(ic->info.file, "*.directory")))
	  _e_fm2_icon_desktop_load(ic);
	
	cf = e_fm2_custom_file_get(buf);
	if (cf)
	  {
	     if (cf->icon.valid)
	       {
		  _eina_stringshare_replace(&ic->info.icon, cf->icon.icon);
		  ic->info.icon_type = cf->icon.type;
	       }
	  }
	
	if (ic->realized)
	  {
	     _e_fm2_icon_unrealize(ic);
	     _e_fm2_icon_realize(ic);
	  }
     }
   e_fm2_custom_file_flush();
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
   Eina_List *l;
   
   _e_fm2_list_walking++;
   for (l = _e_fm2_list; l; l = l->next)
     {
	if ((_e_fm2_list_walking > 0) &&
	    (eina_list_data_find(_e_fm2_list_remove, l->data))) continue;
	e_fm2_icons_update(l->data);
     }
   _e_fm2_list_walking--;
   if (_e_fm2_list_walking == 0)
     {
	while (_e_fm2_list_remove)
	  {
	     _e_fm2_list = eina_list_remove(_e_fm2_list, _e_fm2_list_remove->data);
	     _e_fm2_list_remove = eina_list_remove_list(_e_fm2_list_remove, _e_fm2_list_remove);
	  }
     }
}

EAPI Evas_Object *
e_fm2_icon_get(Evas *evas, E_Fm2_Icon *ic,
	       void (*gen_func) (void *data, Evas_Object *obj, void *event_info),
	       void *data, int force_gen, const char **type_ret)
{
   Evas_Object *oic = NULL;
   char buf[PATH_MAX], *p;

   if (ic->info.icon)
     {
	/* custom icon */
	if ((ic->info.icon[0] == '/') || 
	    (!strncmp(ic->info.icon, "./", 2)) ||
	    (!strncmp(ic->info.icon, "../", 3)))
	  {
	     const char *icfile;
	     
	     /* path to icon file */
	     icfile = ic->info.icon;
	     if (!strncmp(ic->info.icon, "./", 2))
	       {
		  /* relative path */
		  snprintf(buf, sizeof(buf), "%s/%s",
			   e_fm2_real_path_get(ic->info.fm),
			   ic->info.icon + 2);
		  icfile = buf;
	       }
	     else if (!strncmp(ic->info.icon, "../", 3))
	       {
		  char *dirpath;
		  
		  /* relative path - parent */
		  dirpath = strdup(e_fm2_real_path_get(ic->info.fm));
		  if (dirpath)
		    {
		       p = strrchr(dirpath, '/');
		       if (p)
			 {
			    *p = 0;
			    snprintf(buf, sizeof(buf), "%s/%s", dirpath, 
				     ic->info.icon + 3);
			    icfile = buf;
			 }
		       free(dirpath);
		    }
	       }
	     p = strrchr(ic->info.icon, '.');
	     if ((p) && (!strcasecmp(p, ".edj")))
	       {
		  oic = edje_object_add(evas);
		  if (!edje_object_file_set(oic, icfile, "icon"))
		    _e_fm2_theme_edje_icon_object_set(ic->sd, oic,
						      "base/theme/fileman",
						      "file");
	       }
	     else
	       {
		  oic = e_icon_add(evas);
		  e_icon_file_set(oic, icfile);
		  e_icon_fill_inside_set(oic, 1);
	       }
	     if (type_ret) *type_ret = "CUSTOM";
	  }
	else
	  {
	     if ((ic->info.mime) && (ic->info.file))
	       {
		  const char *icon;
		  
		  icon = e_fm_mime_icon_get(ic->info.mime);
		  if (!strcmp(icon, "DESKTOP"))
		    {
		       Efreet_Desktop *ef;

		       snprintf(buf, sizeof(buf), "%s/%s", 
				e_fm2_real_path_get(ic->info.fm), ic->info.file);
		       ef = efreet_desktop_new(buf);
		       if (ef)
			 {
			    if (ef->icon)
			      {
				 oic = edje_object_add(evas);
				 if (!e_util_edje_icon_set(oic, ef->icon))
				   {
				      evas_object_del(oic);
				      oic = NULL;
				   }
				 else
				   {
				      if (type_ret) *type_ret = "THEME_ICON";
				   }
			      }
			    if (!oic)
			      {
				 oic = e_util_desktop_icon_add(ef, 48, evas);
				 if (type_ret) *type_ret = "DESKTOP";
			      }
			    efreet_desktop_free(ef);
			 }
		    }
	       }
	     if (!oic)
	       {
		  /* theme icon */
		  oic = edje_object_add(evas);
		  if (ic->info.icon) 
		    e_util_edje_icon_set(oic, ic->info.icon);
		  if (type_ret) *type_ret = "THEME_ICON";
	       }
	  }
	return oic;
     }
   if (S_ISDIR(ic->info.statinfo.st_mode))
     {
	oic = edje_object_add(evas);
	_e_fm2_theme_edje_icon_object_set(ic->sd, oic,
					  "base/theme/fileman",
					  "folder");
     }
   else
     {
	if (ic->info.icon_type == 1)
	  {
	     snprintf(buf, sizeof(buf), "%s/%s", ic->sd->realpath, ic->info.file);
	     oic = e_thumb_icon_add(evas);
	     e_thumb_icon_file_set(oic, buf, NULL);
	     e_thumb_icon_size_set(oic, 128, 128);
	     if (gen_func) evas_object_smart_callback_add(oic, 
							  "e_thumb_gen",
							  gen_func, data);
	     if (!ic)
	       e_thumb_icon_begin(oic);
	     else
	       _e_fm2_icon_thumb(ic, oic, force_gen);
	     if (type_ret) *type_ret = "THUMB";
	  }
	else if (ic->info.mime)
	  {
	     const char *icon;
	     
	     icon = e_fm_mime_icon_get(ic->info.mime);
	     /* use mime type to select icon */
	     if (!icon)
	       {
		  oic = edje_object_add(evas);
		  _e_fm2_theme_edje_icon_object_set(ic->sd, oic,
						    "base/theme/fileman",
						    "file");
		  if (type_ret) *type_ret = "FILE_TYPE";
	       }
	     else if (!strcmp(icon, "THUMB"))
	       {
		  snprintf(buf, sizeof(buf), "%s/%s", ic->sd->realpath, ic->info.file);
		  
		  oic = e_thumb_icon_add(evas);
		  e_thumb_icon_file_set(oic, buf, NULL);
		  e_thumb_icon_size_set(oic, 128, 128);
		  if (gen_func) evas_object_smart_callback_add(oic, 
							       "e_thumb_gen",
							       gen_func, data);
		  if (!ic)
		    e_thumb_icon_begin(oic);
		  else
		    _e_fm2_icon_thumb(ic, oic, force_gen);
		  if (type_ret) *type_ret = "THUMB";
	       }
	     else if (!strcmp(icon, "DESKTOP"))
	       {
		  Efreet_Desktop *ef;
		 
		  oic = NULL; 
		  snprintf(buf, sizeof(buf), "%s/%s", ic->sd->realpath, ic->info.file);
		  ef = efreet_desktop_new(buf);
		  if (ef) oic = e_util_desktop_icon_add(ef, 48, evas);
		  if (type_ret) *type_ret = "DESKTOP";
		  if (ef) efreet_desktop_free(ef);
	       }
	     else if (!strncmp(icon, "e/icons/fileman/mime/", 21))
	       {
		  oic = edje_object_add(evas);
		  if (!_e_fm2_theme_edje_icon_object_set(ic->sd, oic, 
							 "base/theme/fileman",
							 icon + 21 - 5))
		    _e_fm2_theme_edje_icon_object_set(ic->sd, oic,
						      "base/theme/fileman",
						      "file");
		  if (type_ret) *type_ret = "THEME";
	       }
	     else
	       {
		  p = strrchr(icon, '.');
		  if ((p) && (!strcmp(p, ".edj")))
		    {
		       oic = edje_object_add(evas);
		       if (!edje_object_file_set(oic, icon, "icon"))
			 _e_fm2_theme_edje_icon_object_set(ic->sd, oic,
							   "base/theme/fileman",
							   "file");
		    }
		  else
		    {
		       oic = e_icon_add(evas);
		       e_icon_file_set(oic, icon);
		       e_icon_fill_inside_set(oic, 1);
		    }
		  if (type_ret) *type_ret = "CUSTOM";
	       }
	  }
	else
	  {
	     snprintf(buf, sizeof(buf), "%s/%s", ic->sd->realpath, ic->info.file);
	     /* fallback */
	     if ((e_util_glob_case_match(ic->info.file, "*.edj")))
	       {
		  oic = e_thumb_icon_add(evas);
		  if (ic->sd->config->icon.key_hint)
		    e_thumb_icon_file_set(oic, buf,
					  ic->sd->config->icon.key_hint);
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
		  if (gen_func) evas_object_smart_callback_add(oic,
							       "e_thumb_gen",
							       gen_func, data);
		  if (!ic)
		    e_thumb_icon_begin(oic);
		  else
		    _e_fm2_icon_thumb(ic, oic, force_gen);
		  if (type_ret) *type_ret = "THUMB";
	       }
	     else if ((e_util_glob_case_match(ic->info.file, "*.desktop")) || 
		      (e_util_glob_case_match(ic->info.file, "*.directory")))
	       {
		  Efreet_Desktop *ef;

		  oic = NULL; 
		  ef = efreet_desktop_new(buf);
		  if (ef) oic = e_util_desktop_icon_add(ef, 48, evas);
		  if (type_ret) *type_ret = "DESKTOP";
		  if (ef) efreet_desktop_free(ef);
	       }
	     else if (e_util_glob_case_match(ic->info.file, "*.imc"))
	       {	  
		  E_Input_Method_Config *imc;
		  Eet_File *imc_ef;
	
		  oic = NULL;
		  imc_ef = eet_open(buf, EET_FILE_MODE_READ);	     
	
		  if (imc_ef)
		    {
		       imc = e_intl_input_method_config_read(imc_ef);
		       eet_close(imc_ef);
	
		       if (imc->e_im_setup_exec) 
			 {
			    Efreet_Desktop *desktop;
			    desktop = efreet_util_desktop_exec_find(imc->e_im_setup_exec);
			    if (desktop) 
			      oic = e_util_desktop_icon_add(desktop, 24, evas);
			 }
		       e_intl_input_method_config_free(imc);
		    }
		  
		  if (oic == NULL) 
		    {
		       oic = edje_object_add(evas);	    
		       _e_fm2_theme_edje_icon_object_set(ic->sd, oic,
							 "base/theme/fileman",
							 "file");
		       if (type_ret) *type_ret = "FILE_TYPE";
		    }
		  else
		    {
		       if (type_ret) *type_ret = "IMC";
		    }
	       }
	     else if (S_ISCHR(ic->info.statinfo.st_mode))
	       {
		  oic = edje_object_add(evas);
		  _e_fm2_theme_edje_icon_object_set(ic->sd, oic,
						    "base/theme/fileman",
						    "file");
		  if (type_ret) *type_ret = "FILE_TYPE";
	       }
	     else if (S_ISBLK(ic->info.statinfo.st_mode))
	       {
		  oic = edje_object_add(evas);
		  _e_fm2_theme_edje_icon_object_set(ic->sd, oic,
						    "base/theme/fileman",
						    "file");
		  if (type_ret) *type_ret = "FILE_TYPE";
	       }
	     else if (S_ISFIFO(ic->info.statinfo.st_mode))
	       {
		  oic = edje_object_add(evas);
		  _e_fm2_theme_edje_icon_object_set(ic->sd, oic,
						    "base/theme/fileman",
						    "file");
		  if (type_ret) *type_ret = "FILE_TYPE";
	       }
	     else if (S_ISSOCK(ic->info.statinfo.st_mode))
	       {
		  oic = edje_object_add(evas);
		  _e_fm2_theme_edje_icon_object_set(ic->sd, oic,
						    "base/theme/fileman",
						    "file");
		  if (type_ret) *type_ret = "FILE_TYPE";
	       }
	     else if (ecore_file_can_exec(buf))
	       {
		  oic = edje_object_add(evas);
		  _e_fm2_theme_edje_icon_object_set(ic->sd, oic,
						    "base/theme/fileman",
						    "file");
		  if (type_ret) *type_ret = "FILE_TYPE";
	       }
	     else
	       {
		  oic = edje_object_add(evas);
		  _e_fm2_theme_edje_icon_object_set(ic->sd, oic,
						    "base/theme/fileman",
						    "file");
		  if (type_ret) *type_ret = "FILE_TYPE";
	       }
	  }
     }
   return oic;
}

EAPI E_Fm2_Icon_Info *
e_fm2_icon_file_info_get(E_Fm2_Icon *ic)
{
   if (!ic) return NULL;
   return &(ic->info);
}

EAPI void 
e_fm2_icon_geometry_get(E_Fm2_Icon *ic, int *x, int *y, int *w, int *h) 
{
   int xx, yy, ww, hh;
   
   if (x) *x = 0; if (y) *y = 0; if (w) *w = 0; if (h) *h = 0;
   if (ic) 
     {
	evas_object_geometry_get(ic->obj, &xx, &yy, &ww, &hh);
	if (x) *x = xx;
	if (y) *y = yy;
	if (w) *w = ww;
	if (h) *h = hh;
     }
}

/* FIXME: track real exe with exe del events etc. */
static int _e_fm2_client_spawning = 0;

static void
_e_fm2_client_spawn(void)
{
   Ecore_Exe *exe;
   char buf[4096];
   
   if (_e_fm2_client_spawning) return;
   snprintf(buf, sizeof(buf), "%s/enlightenment/utils/enlightenment_fm", e_prefix_lib_get());
   exe = ecore_exe_run(buf, NULL);
   _e_fm2_client_spawning = 1;
}

static E_Fm2_Client *
_e_fm2_client_get(void)
{
   Eina_List *l;
   E_Fm2_Client *cl, *cl_chosen = NULL;
   int min_req = 0x7fffffff;

   /* if we don't have a slave - spane one */
   if (!_e_fm2_client_list)
     {
	_e_fm2_client_spawn();
	return NULL;
     }
   for (l = _e_fm2_client_list; l; l = l->next)
     {
	cl = l->data;
	if (cl->req < min_req)
	  {
	     min_req = cl->req;
	     cl_chosen = cl;
	  }
     }
   return cl_chosen;
}

typedef struct _E_Fm2_Message E_Fm2_Message;

struct _E_Fm2_Message
{
   int major, minor, ref, ref_to, response;
   void *data;
   int size;
};

static Eina_List *_e_fm2_messages = NULL;

static void
_e_fm2_client_message_queue(int major, int minor, int ref, int ref_to, int response, const void *data, int size)
{
   E_Fm2_Message *msg;
   
   msg = E_NEW(E_Fm2_Message, 1);
   if (!msg) return;
   msg->major = major;
   msg->minor = minor;
   msg->ref = ref;
   msg->ref_to = ref_to;
   msg->response = response;
   if (data)
     {
	msg->size = size;
	msg->data = malloc(size);
	if (msg->data)
	  memcpy(msg->data, data, size);
	else
	  {
	     free(msg);
	     return;
	  }
     }
   _e_fm2_messages = eina_list_append(_e_fm2_messages, msg);
}

static void
_e_fm2_client_message_flush(E_Fm2_Client *cl, E_Fm2_Message *msg)
{
   _e_fm2_messages = eina_list_remove(_e_fm2_messages, msg);
   ecore_ipc_client_send(cl->cl, msg->major, msg->minor, 
			 msg->ref, msg->ref_to, msg->response, 
			 msg->data, msg->size);
   cl->req++;
   free(msg->data);
   free(msg);
}

static void
_e_fm2_client_messages_flush(void)
{
   while (_e_fm2_messages)
     {
	E_Fm2_Client *cl;
	
	cl = _e_fm2_client_get();
	if (!cl) break;
	_e_fm2_client_message_flush(cl, _e_fm2_messages->data);
     }
}

static int
_e_fm_client_send_new(int minor, void *data, int size)
{
   static int id = 0;
   E_Fm2_Client *cl;
   
   id ++;
   cl = _e_fm2_client_get();
   if (!cl)
     {
	_e_fm2_client_message_queue(E_IPC_DOMAIN_FM, minor, 
				    id, 0, 0, 
				    data, size);
     }
   else
     {
	ecore_ipc_client_send(cl->cl, E_IPC_DOMAIN_FM, minor, 
			      id, 0, 0, 
			      data, size);
	cl->req++;
     }

   return id;
}

static int
_e_fm_client_send(int minor, int id, void *data, int size)
{
   E_Fm2_Client *cl;
   
   cl = _e_fm2_client_get();
   if (!cl)
     {
	_e_fm2_client_message_queue(E_IPC_DOMAIN_FM, minor, 
				    id, 0, 0, 
				    data, size);
     }
   else
     {
	ecore_ipc_client_send(cl->cl, E_IPC_DOMAIN_FM, minor, 
			      id, 0, 0, 
			      data, size);
	cl->req++;
     }

   return id;
}

static int
_e_fm2_client_monitor_add(const char *path)
{
   return _e_fm_client_send_new(E_FM_OP_MONITOR_START, (void *)path, strlen(path) + 1);
}

static void
_e_fm2_client_monitor_del(int id, const char *path)
{
   _e_fm_client_send(E_FM_OP_MONITOR_END, id, (void *)path, strlen(path) + 1);
}

static int
_e_fm_client_file_del(const char *files)
{
   return _e_fm_client_send_new(E_FM_OP_REMOVE, (void *)files, strlen(files) + 1);
}

static int
_e_fm2_client_file_trash(const char *path)
{
   return _e_fm_client_send_new(E_FM_OP_TRASH, (void *)path, strlen(path) + 1);
}

static int
_e_fm2_client_file_mkdir(const char *path, const char *rel, int rel_to, int x, int y, int res_w, int res_h)
{
   char *d;
   int l1, l2, l;
   
   l1 = strlen(path);
   l2 = strlen(rel);
   l = l1 + 1 + l2 + 1 + (sizeof(int) * 3);
   d = alloca(l);
   strcpy(d, path);
   strcpy(d + l1 + 1, rel);
   memcpy(d + l1 + 1 + l2 + 1, &rel_to, sizeof(int));
   memcpy(d + l1 + 1 + l2 + 1 + sizeof(int), &x, sizeof(int));
   memcpy(d + l1 + 1 + l2 + 1 + (2 * sizeof(int)), &y, sizeof(int));

   return _e_fm_client_send_new(E_FM_OP_MKDIR, (void *)d, l);
}

static int
_e_fm_client_file_move(const char *args)
{
   return _e_fm_client_send_new(E_FM_OP_MOVE, (void *)args, strlen(args) + 1);
}

static int
_e_fm2_client_file_symlink(const char *path, const char *dest, const char *rel, int rel_to, int x, int y, int res_w, int res_h)
{
   char *d;
   int l1, l2, l3, l;
   
   l1 = strlen(path);
   l2 = strlen(dest);
   l3 = strlen(rel);
   l = l1 + 1 + l2 + 1 + l3 + 1 + (sizeof(int) * 3);
   d = alloca(l);
   strcpy(d, path);
   strcpy(d + l1 + 1, dest);
   strcpy(d + l1 + 1 + l2 + 1, rel);
   memcpy(d + l1 + 1 + l2 + 1 + l3 + 1, &rel_to, sizeof(int));
   memcpy(d + l1 + 1 + l2 + 1 + l3 + 1 + sizeof(int), &x, sizeof(int));
   memcpy(d + l1 + 1 + l2 + 1 + l3 + 1 + (2 * sizeof(int)), &y, sizeof(int));
   
   if ((x != -9999) && (y != -9999))
     {
	E_Fm2_Custom_File *cf, cf0;
	
	cf = e_fm2_custom_file_get(dest);
	if (!cf)
	  {
	     memset(&cf0, 0, sizeof(E_Fm2_Custom_File));
	     cf = &cf0;
	  }
	cf->geom.x = x;
	cf->geom.y = y;
	cf->geom.res_w = res_w;
	cf->geom.res_h = res_h;
	cf->geom.valid = 1;
	e_fm2_custom_file_set(dest, cf);
	e_fm2_custom_file_flush();
     }

   return _e_fm_client_send_new(E_FM_OP_SYMLINK, (void *)d, l);
}

static int
_e_fm_client_file_copy(const char *args)
{
   return _e_fm_client_send_new(E_FM_OP_COPY, (void *)args, strlen(args) + 1);
}

EAPI int
_e_fm2_client_mount(const char *udi, const char *mountpoint)
{
   char *d;
   int l, l1, l2;
   
   l1 = strlen(udi);
   l2 = strlen(mountpoint);
   l = l1 + 1 + l2 + 1;
   d = alloca(l);
   strcpy(d, udi);
   strcpy(d + l1 + 1, mountpoint);

   return _e_fm_client_send_new(E_FM_OP_MOUNT, (void *)d, l);
}

EAPI int
_e_fm2_client_unmount(const char *udi)
{
   E_Fm2_Client *cl;
   char *d;
   int l, l1;
   
   l1 = strlen(udi);
   l = l1 + 1;
   d = alloca(l);
   strcpy(d, udi);

   cl = _e_fm2_client_get();
   
   return _e_fm_client_send_new(E_FM_OP_UNMOUNT, (void *)d, l);
}

static void
_e_fm2_client_monitor_list_end(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   sd->busy_count--;
   if (sd->busy_count == 0)
     {
	edje_object_signal_emit(sd->overlay, "e,state,busy,stop", "e");
	e_fm2_custom_file_flush();
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
   _e_fm2_live_process_begin(obj);
}

EAPI void
_e_fm2_file_force_update(const char *path)
{
   char *dir;
   Eina_List *l;
   
   dir = ecore_file_dir_get(path);
   if (!dir) return;
   for (l = _e_fm2_list; l; l = l->next)
     {
	if ((_e_fm2_list_walking > 0) &&
	    (eina_list_data_find(_e_fm2_list_remove, l->data))) continue;
	if (!strcmp(e_fm2_real_path_get(l->data), dir))
	  {
	     E_Fm2_Icon *ic;
	     
	     ic = _e_fm2_icon_find(l->data, ecore_file_file_get(path));
	     if (ic)
	       {
		  E_Fm2_Finfo finf;
		  
		  memset(&finf, 0, sizeof(E_Fm2_Finfo));
		  memcpy(&(finf.st), &(ic->info.statinfo),
			 sizeof(struct stat));
		  finf.broken_link = ic->info.broken_link;
		  finf.lnk = ic->info.link;
		  finf.rlnk = ic->info.real_link;
		  ic->removable_state_change = 1;
		  _e_fm2_live_file_changed(l->data, ecore_file_file_get(path),
					   &finf);
	       }
	  }
     }
   free(dir);
}

EAPI void
e_fm2_client_data(Ecore_Ipc_Event_Client_Data *e)
{
   Eina_List *l, *dels = NULL;
   E_Fm2_Client *cl;
   
   if (e->major != 6/*E_IPC_DOMAIN_FM*/) return;
   for (l = _e_fm2_client_list; l; l = l->next)
     {
	cl = l->data;
	if (cl->cl == e->client) break;
     }
   if (!l)
     {
	cl = E_NEW(E_Fm2_Client, 1);
	cl->cl = e->client;
	_e_fm2_client_list = eina_list_prepend(_e_fm2_client_list, cl);
	/* FIXME: new client - send queued msgs */
	_e_fm2_client_spawning = 0;
	_e_fm2_client_messages_flush();
     }
   
   _e_fm2_list_walking++;
   for (l = _e_fm2_list; l; l = l->next)
     {
	unsigned char *p;
	char *evdir;
	const char *dir, *path, *lnk, *rlnk, *file;
	struct stat st;
	int broken_link;
	E_Fm2_Smart_Data *sd;
	Evas_Object *obj;
	
	obj = l->data;
	if ((_e_fm2_list_walking > 0) &&
	    (eina_list_data_find(_e_fm2_list_remove, l->data))) continue;
	dir = e_fm2_real_path_get(obj);
	sd = evas_object_smart_data_get(obj);
	switch (e->minor)
	  {  
	   case E_FM_OP_HELLO:/*hello*/
//             printf("E_FM_OP_HELLO\n");
	     break;
	   case E_FM_OP_OK:/*req ok*/
//             printf("E_FM_OP_OK\n");
	     cl->req--;
	     break;
	   case E_FM_OP_FILE_ADD:/*file add*/
//             printf("E_FM_OP_FILE_ADD\n");
	   case E_FM_OP_FILE_CHANGE:/*file change*/
//             printf("E_FM_OP_FILE_CHANGE\n");
	       {
		  E_Fm2_Finfo finf;
                  
                  p = e->data;
		  /* NOTE: i am NOT converting this data to portable arch/os independant
		   * format. i am ASSUMING e_fm_main and e are local and built together
		   * and thus this will work. if this ever changes this here needs to
		   * change */
		  memcpy(&st, p, sizeof(struct stat));
		  p += sizeof(struct stat);
		  
		  broken_link = p[0];
		  p += 1;
		  
		  path = (char *)p;
		  p += strlen(path) + 1;

		  lnk = (char *)p;
		  p += strlen(lnk) + 1;
		  
		  rlnk = (char *)p;
		  p += strlen(rlnk) + 1;
		  
		  memcpy(&(finf.st), &st, sizeof(struct stat));
		  finf.broken_link = broken_link;
		  finf.lnk = lnk;
		  finf.rlnk = rlnk;
		  
		  evdir = ecore_file_dir_get(path);
		  if ((evdir) && (sd->id == e->ref_to) && 
		      ((!strcmp(evdir, "") || (!strcmp(dir, evdir)))))
		    {
//                       printf(" ch/add response = %i\n", e->response);
		       if (e->response == 0)/*live changes*/
			 {
			    if (e->minor == E_FM_OP_FILE_ADD)/*file add*/
			      {
				 _e_fm2_live_file_add
				   (obj, ecore_file_file_get(path),
				    NULL, 0, &finf);
			      }
			    else if (e->minor == E_FM_OP_FILE_CHANGE)/*file change*/
			      {
				 _e_fm2_live_file_changed
				   (obj, (char *)ecore_file_file_get(path),
				    &finf);
			      }
			 }
		       else/*file add - listing*/
			 {
			    if (e->minor == E_FM_OP_FILE_ADD)/*file add*/
			      {
				 if (!sd->scan_timer)
				   {
				      sd->scan_timer = 
					ecore_timer_add(0.5,
							_e_fm2_cb_scan_timer, 
							sd->obj);
				      sd->busy_count++;
				      if (sd->busy_count == 1)
					edje_object_signal_emit(sd->overlay, "e,state,busy,start", "e");
				   }
				 if (path[0] != 0)
				   {			   
				      file = ecore_file_file_get(path);
				      if ((!strcmp(file, ".order")))
					sd->order_file = 1;
				      else
					{
					   if (!((file[0] == '.') && 
						 (!sd->show_hidden_files)))
					     _e_fm2_file_add(obj, file,
							     sd->order_file, 
							     NULL, 0, &finf);
					}
				   }
				 if (e->response == 2)/* end of scan */
				   {
				      sd->listing = 0;
				      if (sd->scan_timer)
					{
					   ecore_timer_del(sd->scan_timer);
					   sd->scan_timer =
					     ecore_timer_add(0.0001, 
							     _e_fm2_cb_scan_timer,
							     sd->obj);
					}
				      else
					{
					   _e_fm2_client_monitor_list_end(obj);
					}
				   }
			      }
			 }
		    }
		  else
		    {
//                       printf(" ...\n");
		       if ((sd->id == e->ref_to) && (path[0] == 0))
			 {
//                            printf(" end response = %i\n", e->response);
			    if (e->response == 2)/* end of scan */
			      {
				 sd->listing = 0;
				 if (sd->scan_timer)
				   {
				      ecore_timer_del(sd->scan_timer);
				      sd->scan_timer =
					ecore_timer_add(0.0001, 
							_e_fm2_cb_scan_timer,
							sd->obj);
				   }
				 else
				   {
				      _e_fm2_client_monitor_list_end(obj);
				   }
			      }
			 }
		   }
		  if (evdir) free(evdir);
	       }
	     break;
	   case E_FM_OP_FILE_DEL:/*file del*/
//             printf("E_FM_OP_FILE_DEL\n");
	     path = e->data;
	     evdir = ecore_file_dir_get(path);
	     if ((sd->id == e->ref_to) && (!strcmp(dir, evdir)))
	       {
		  _e_fm2_live_file_del
		    (obj, ecore_file_file_get(path));
	       }
	     free(evdir);
	     break;
	   case E_FM_OP_MONITOR_END:/*mon dir del*/
//             printf("E_FM_OP_MONITOR_END\n");
	     path = e->data;
	     if ((sd->id == e->ref_to) && (!strcmp(dir, path)))
	       {
		  dels = eina_list_append(dels, obj);
	       }
	     break;
	   default:
	     break;
	  }
     }
   while (dels)
     {
	Evas_Object *obj;
	
	obj = dels->data;
	dels = eina_list_remove_list(dels, dels);
	if ((_e_fm2_list_walking > 0) &&
	    (eina_list_data_find(_e_fm2_list_remove, obj))) continue;
	evas_object_smart_callback_call(obj, "dir_deleted", NULL);
     }
   _e_fm2_list_walking--;
   if (_e_fm2_list_walking == 0)
     {
	while (_e_fm2_list_remove)
	  {
	     _e_fm2_list = eina_list_remove(_e_fm2_list, _e_fm2_list_remove->data);
	     _e_fm2_list_remove = eina_list_remove_list(_e_fm2_list_remove, _e_fm2_list_remove);
	  }
     }
   switch (e->minor)
     {
      case E_FM_OP_MONITOR_SYNC:/*mon list sync*/
	ecore_ipc_client_send(cl->cl, E_IPC_DOMAIN_FM, E_FM_OP_MONITOR_SYNC,
			      0, 0, e->response, 
			      NULL, 0);
	break;
	
      case E_FM_OP_STORAGE_ADD:/*storage add*/
	if ((e->data) && (e->size > 0))
	  {
	     E_Storage *s;
	     
	     s = eet_data_descriptor_decode(_e_storage_edd, e->data, e->size);
	     if (s) e_fm2_hal_storage_add(s);
	  }
	break;
	
      case E_FM_OP_STORAGE_DEL:/*storage del*/
	if ((e->data) && (e->size > 0))
	  {
	     char *udi;
	     E_Storage *s;
	     
	     udi = e->data;
	     s = e_fm2_hal_storage_find(udi);
	     if (s) e_fm2_hal_storage_del(s);
	  }
	break;
	
      case E_FM_OP_VOLUME_ADD:/*volume add*/
	if ((e->data) && (e->size > 0))
	  {
	     E_Volume *v;
	     
	     v = eet_data_descriptor_decode(_e_volume_edd, e->data, e->size);
	     if (v) e_fm2_hal_volume_add(v);
	  }
	break;

      case E_FM_OP_VOLUME_DEL:/*volume del*/
	if ((e->data) && (e->size > 0))
	  {
	     char *udi;
             E_Volume *v;
	     
	     udi = e->data;
	     v = e_fm2_hal_volume_find(udi);
	     if (v) e_fm2_hal_volume_del(v);
	  }
	break;
	
      case E_FM_OP_MOUNT_DONE:/*mount done*/
	if ((e->data) && (e->size > 1))
	  {
	     E_Volume *v;
	     char *udi, *mountpoint;
	     
	     udi = e->data;
	     mountpoint = udi + strlen(udi) + 1;
	     v = e_fm2_hal_volume_find(udi);
	     if (v) e_fm2_hal_mount_add(v, mountpoint);
	  }
	break;

      case E_FM_OP_UNMOUNT_DONE:/*unmount done*/
	if ((e->data) && (e->size > 1))
	  {
	     E_Volume *v;
	     char *udi;
	     
	     udi = e->data;
	     v = e_fm2_hal_volume_find(udi);
	     if (v)
	       {
		  v->mounted = 0;
		  if (v->mount_point) free(v->mount_point);
		  v->mount_point = NULL;
	       }
	  }
	break;

      case E_FM_OP_ERROR:/*error*/
	printf("%s:%s(%d) Error from slave #%d: %s\n", __FILE__, __FUNCTION__, __LINE__, e->ref, (char *)e->data);
	_e_fm_error_dialog(e->ref, e->data);
	break;

      case E_FM_OP_ERROR_RETRY_ABORT:/*error*/
	printf("%s:%s(%d) Error from slave #%d: %s\n", __FILE__, __FUNCTION__, __LINE__, e->ref, (char *)e->data);
	_e_fm_retry_abort_dialog(e->ref, (char *)e->data);
	break;

      case E_FM_OP_OVERWRITE:/*overwrite*/
	printf("%s:%s(%d) Overwrite from slave #%d: %s\n", __FILE__, __FUNCTION__, __LINE__, e->ref, (char *)e->data);
	_e_fm_overwrite_dialog(e->ref, (char *)e->data);
	break;

      case E_FM_OP_PROGRESS:/*progress*/
	  {
	     int percent, seconds;
	     size_t done, total;
	     char *src = NULL;
	     char *dst = NULL;
	     void *p = e->data;

	     if (!e->data) return;

#define UP(value, type) (value) = *(type *)p; p += sizeof(type)
	     UP(percent, int);
	     UP(seconds, int);
	     UP(done, size_t);
	     UP(total, size_t);
#undef UP
	     src = p;
	     dst = p + strlen(src) + 1;
	     printf("%s:%s(%d) Progress from slave #%d:\n\t%d%% done,\n\t%d seconds left,\n\t%d done,\n\t%d total,\n\tsrc = %s,\n\tdst = %s.\n", __FILE__, __FUNCTION__, __LINE__, e->ref, percent, seconds, done, total, src, dst);
	  }
	break;

     default:
	break;
     }
}

EAPI void
e_fm2_client_del(Ecore_Ipc_Event_Client_Del *e)
{
   Eina_List *l;
   E_Fm2_Client *cl;

   for (l = _e_fm2_client_list; l; l = l->next)
     {
	cl = l->data;
	if (cl->cl == e->client)
	  {
	     _e_fm2_client_list = eina_list_remove_list(_e_fm2_client_list, l);
	     free(cl);
	     break;
	  }
     }
}

/* local subsystem functions */
static const char *
_e_fm2_dev_path_map(const char *dev, const char *path)
{
   char buf[4096] = "", *s, *ss;
   int len;
   
   /* map a device name to a mount point/path on the os (and append path) */

   /* FIXME: load mappings from config and use them first - maybe device
    * discovery should be done through config and not the below (except
    * maybe for home directory and root fs and other simple thngs */
   /* FIXME: also add other virtualized dirs like "backgrounds", "themes",
    * "favorites" */
#define CMP(x) ((dev) && (e_util_glob_case_match(dev, x)))
#define PRT(args...) snprintf(buf, sizeof(buf), ##args)
   
   if (CMP("/")) 
     {
	PRT("%s", path);
     }
   else if (CMP("~/")) 
     {
	s = (char *)e_user_homedir_get();
	PRT("%s%s", s, path);
     }
   else if ((dev) && (dev[0] == '/'))
     {
	/* dev is a full path - consider it a mountpoint device on its own */
	PRT("%s%s", dev, path);
     }
   else if (CMP("favorites")) 
     {
	/* this is a virtual device - it's where your favorites list is 
	 * stored - a dir with 
	 .desktop files or symlinks (in fact anything
	 * you like
	 */
	s = (char *)e_user_homedir_get();
	PRT("%s/.e/e/fileman/favorites", s);
     }
   else if (CMP("desktop")) 
     {
	/* this is a virtual device - it's where your favorites list is 
	 * stored - a dir with 
	 .desktop files or symlinks (in fact anything
	 * you like
	 */
	s = (char *)e_user_homedir_get();
	if (!strcmp(path, "/"))
	  {
	     PRT("%s/Desktop", s);
	     ecore_file_mkpath(buf);
	  }
	else
	  {
	     PRT("%s/Desktop-%s", s, path);
	     ecore_file_mkpath(buf);
	  }
     }
   else if (CMP("removable:*")) 
     {
	E_Volume *v;

	v = e_fm2_hal_volume_find(dev + strlen("removable:"));
	if (v) 
	  {
	     if (!v->mount_point)
	       v->mount_point = e_fm2_hal_volume_mountpoint_get(v);;
	     snprintf(buf, sizeof(buf), "%s%s", v->mount_point, path);
	  }	
     }
   else if (CMP("dvd") || CMP("dvd-*"))  
     {
	/* FIXME: find dvd mountpoint optionally for dvd no. X */
	/* maybe make part of the device mappings config? */
     }
   else if (CMP("cd") || CMP("cd-*") || CMP("cdrom") || CMP("cdrom-*") ||
	    CMP("dvd") || CMP("dvd-*")) 
     {
	/* FIXME: find cdrom or dvd mountpoint optionally for cd/dvd no. X */
	/* maybe make part of the device mappings config? */
     }
   /* FIXME: add code to find USB devices (multi-card readers or single,
    * usb thumb drives, other usb storage devices (usb hd's etc.)
    */
   /* maybe make part of the device mappings config? */
   /* FIXME: add code for finding nfs shares, smb shares etc. */
   /* maybe make part of the device mappings config? */

   if (!strlen(buf)) snprintf(buf, sizeof(buf), "%s", path);

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
   return eina_stringshare_add(buf);
}

static void
_e_fm2_file_add(Evas_Object *obj, const char *file, int unique, const char *file_rel, int after, E_Fm2_Finfo *finf)
{
   E_Fm2_Smart_Data *sd;
   E_Fm2_Icon *ic, *ic2;
   Eina_List *l;

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
		  return;
	       }
	  }
	for (l = sd->queue; l; l = l->next)
	  {
	     ic = l->data;
	     if (!strcmp(ic->info.file, file))
	       {
		  sd->tmp.last_insert = NULL;
		  return;
	       }
	  }
     }
   /* create icon obj and append to unsorted list */
   ic = _e_fm2_icon_new(sd, file, finf);
   if (ic)
     {
	if (!file_rel)
	  {
	     /* respekt da ordah! */
	     if (sd->order_file)
	       sd->queue = eina_list_append(sd->queue, ic);
	     else
	       {
		  /* insertion sort it here to spread the sort load into idle time */
		  for (l = sd->queue; l; l = l->next)
		    {
		       ic2 = l->data;
		       if (_e_fm2_cb_icon_sort(ic, ic2) < 0)
			 {
			    sd->queue = eina_list_prepend_relative_list(sd->queue, ic, l);
			    break;
			 }
		    }
		  if (!l) sd->queue = eina_list_append(sd->queue, ic);
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
			 sd->icons = eina_list_append_relative(sd->icons, ic, ic2);
		       else
			 sd->icons = eina_list_prepend_relative(sd->icons, ic, ic2);
		       break;
		    }
	       }
	     if (!l)
	       sd->icons = eina_list_append(sd->icons, ic);
	     sd->icons_place = eina_list_append(sd->icons_place, ic);
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
   Eina_List *l;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   for (l = sd->icons; l; l = l->next)
     {
	ic = l->data;
	if (!strcmp(ic->info.file, file))
	  {
	     sd->icons = eina_list_remove_list(sd->icons, l);
	     sd->icons_place = eina_list_remove(sd->icons_place, ic);
	     if (ic->region)
	       {
		  ic->region->list = eina_list_remove(ic->region->list, ic);
		  ic->region = NULL;
	       }
	     _e_fm2_icon_free(ic);
	     return;
	  }
     }
}

static void
_e_fm_file_buffer_clear(void)
{
   while (_e_fm_file_buffer)
     {
	eina_stringshare_del(eina_list_data_get(_e_fm_file_buffer));
	_e_fm_file_buffer = eina_list_remove_list(_e_fm_file_buffer, _e_fm_file_buffer);
     }

   _e_fm_file_buffer_cutting = 0;
   _e_fm_file_buffer_copying = 0;
}

static void
_e_fm2_file_cut(Evas_Object *obj)
{
   Eina_List *sel = NULL, *l = NULL;
   const char *realpath;

   sel = e_fm2_selected_list_get(obj);
   if (!sel) return;

   _e_fm_file_buffer_clear();

   realpath = e_fm2_real_path_get(obj);
   _e_fm_file_buffer_cutting = 1;
   for (l = sel; l; l = l->next) 
     {
	E_Fm2_Icon_Info *ici;
	char buf[PATH_MAX];

	ici = l->data;
	if (!ici) continue;
	snprintf(buf, sizeof(buf), "%s/%s", realpath, ici->file);
	_e_fm_file_buffer = eina_list_append(_e_fm_file_buffer, _e_fm2_uri_escape(buf));
     }
}

static void
_e_fm2_file_copy(Evas_Object *obj)
{
   Eina_List *sel = NULL, *l = NULL;
   const char *realpath;

   sel = e_fm2_selected_list_get(obj);
   if (!sel) return;

   _e_fm_file_buffer_clear();

   realpath = e_fm2_real_path_get(obj);
   _e_fm_file_buffer_copying = 1;
   for (l = sel; l; l = l->next) 
     {
	E_Fm2_Icon_Info *ici;
	char buf[PATH_MAX];

	ici = l->data;
	if (!ici) continue;
	snprintf(buf, sizeof(buf), "%s/%s", realpath, ici->file);
	_e_fm_file_buffer = eina_list_append(_e_fm_file_buffer, _e_fm2_uri_escape(buf));
     }
}

static void
_e_fm2_file_paste(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   Eina_List *paths;
   const char *filepath;
   size_t length = 0;
   size_t size = 0;
   char *args = NULL;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   /* Convert URI list to a list of real paths. */
   paths = _e_fm2_uri_path_list_get(_e_fm_file_buffer);

   while (paths) 
     {
	/* Get file's full path. */
	filepath = eina_list_data_get(paths);
	if (!filepath) 
	  {
	     paths = eina_list_remove_list(paths, paths);
	     continue;
	  }

	/* Check if file is protected. */
	if (e_filereg_file_protected(filepath)) 
	  {
	     eina_stringshare_del(filepath);
	     paths = eina_list_remove_list(paths, paths);
	     continue;
	  }

	/* Put filepath into a string of args.
	 * If there are more files, put an additional space.
	 */
	args = _e_fm_string_append_quoted(args, &size, &length, filepath);
	args = _e_fm_string_append_char(args, &size, &length, ' ');
	
	eina_stringshare_del(filepath);
	paths = eina_list_remove_list(paths, paths);
     }

   /* Add destination to the arguments. */
   args = _e_fm_string_append_quoted(args, &size, &length, sd->realpath);

   /* Roll the operation! */
   if (_e_fm_file_buffer_copying)
     {
	_e_fm_client_file_copy(args);
     }
   else
     {
	_e_fm_client_file_move(args);
     }

   free(args);
}

static void
_e_fm2_file_cut_menu(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   E_Fm2_Smart_Data *sd = data;
   if (!sd) return;
   _e_fm2_file_cut(sd->obj);
}

static void
_e_fm2_file_copy_menu(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   E_Fm2_Smart_Data *sd = data;
   if (!sd) return;
   _e_fm2_file_copy(sd->obj);
}

static void
_e_fm2_file_paste_menu(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   E_Fm2_Smart_Data *sd;
   if (!sd) return;
   _e_fm2_file_paste(sd->obj);
}

static void
_e_fm2_queue_process(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   E_Fm2_Icon *ic, *ic2;
   Eina_List *l, **ll;
   int added = 0, i, p0, p1, n, v;
   double t;
   char buf[4096];

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (!sd->queue)
     {
	if (sd->resize_job) ecore_job_del(sd->resize_job);
	sd->resize_job = ecore_job_add(_e_fm2_cb_resize_job, obj);
	evas_object_smart_callback_call(sd->obj, "changed", NULL);
	sd->tmp.last_insert = NULL;
	return;
     }
//   double tt = ecore_time_get();
//   int queued = eina_list_count(sd->queue);
   /* take unsorted and insert into the icon list - reprocess regions */
   t = ecore_time_get();
   if (!sd->tmp.last_insert)
     {
#if 1
	n = eina_list_count(sd->icons);
	E_FREE(sd->tmp.list_index);
	if (n > 0)
	  sd->tmp.list_index = malloc(n * sizeof(Eina_List *));
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
	sd->queue = eina_list_remove_list(sd->queue, sd->queue);
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
		       if (l == sd->icons)
			 sd->icons = eina_list_prepend(sd->icons, ic);
		       else
			 sd->icons = eina_list_prepend_relative_list(sd->icons, 
								     ic, l);
		       sd->tmp.last_insert = l;
		       break;
		    }
	       }
	  }
	if (!l)
	  {
	     sd->icons = eina_list_append(sd->icons, ic);
	     sd->tmp.last_insert = eina_list_last(sd->icons);
	  }
	sd->icons_place = eina_list_append(sd->icons_place, ic);
	added++;
	/* if we spent more than 1/20th of a second inserting - give up
	 * for now */
	if ((ecore_time_get() - t) > 0.05) break;
     }
//   printf("FM: SORT %1.3f (%i files) (%i queued, %i added) [%i iter]\n",
//	  ecore_time_get() - tt, eina_list_count(sd->icons), queued, 
//	  added, sd->tmp.iter);
   snprintf(buf, sizeof(buf), _("%i Files"), eina_list_count(sd->icons));
   edje_object_part_text_set(sd->overlay, "e.text.busy_label", buf);
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
	sd->queue = eina_list_remove_list(sd->queue, sd->queue);
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
        sd->regions.list = eina_list_remove_list(sd->regions.list, sd->regions.list);
     }
}

static void
_e_fm2_regions_populate(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   Eina_List *l;
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
	     sd->regions.list = eina_list_append(sd->regions.list, rg);
	  }
	ic->region = rg;
	rg->list = eina_list_append(rg->list, ic);
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
	if (eina_list_count(rg->list) > sd->regions.member_max)
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
   Eina_List *l;
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
   Eina_List *l;
   E_Fm2_Icon *ic;
   Evas_Coord x, y, gw, gh;
   int cols = 1, col;
   
   gw = 0; gh = 0;
   for (l = sd->icons; l; l = l->next)
     {
	ic = l->data;
	if (ic->w > gw) gw = ic->w;
	if (ic->h > gh) gh = ic->h;
     }
   if (gw > 0) cols = sd->w / gw;
   if (cols < 1) cols = 1;
   x = 0; y = 0; col = 0;
   for (l = sd->icons; l; l = l->next)
     {
	ic = l->data;
	ic->x = x + ((gw - ic->w) / 2);
	ic->y = y + (gh - ic->h);
	x += gw;
	col++;
	if (col >= cols)
	  {
	     col = 0;
	     x = 0;
	     y += gh;
	  }
	if ((ic->x + ic->w) > sd->max.w) sd->max.w = ic->x + ic->w;
	if ((ic->y + ic->h) > sd->max.h) sd->max.h = ic->y + ic->h;
     }
}

static int
_e_fm2_icons_icon_overlaps(E_Fm2_Icon *ic)
{
   Eina_List *l;
   E_Fm2_Icon *ic2;

   /* this is really slow... when we have a lot of icons */
   for (l = ic->sd->icons; l; l = l->next)
     {
	ic2 = l->data;
	if ((ic2 != ic) && (ic2->saved_pos))
	  {
	     if (E_INTERSECTS(ic2->x, ic2->y, ic2->w, ic2->h,
			      ic->x, ic->y, ic->w, ic->h))
	       return 1;
	  }
     }
   return 0;
}

static int
_e_fm2_icons_icon_row_ok(E_Fm2_Icon *ic)
{
   if (ic->x + ic->w > ic->sd->w) return 0;
   if (ic->x < 0) return 0;
   if (ic->y < 0) return 0;
   return 1;
}

static void
_e_fm2_icon_place_relative(E_Fm2_Icon *ic, E_Fm2_Icon *icr, int xrel, int yrel, int xa, int ya)
{
   ic->x = icr->x;
   ic->y = icr->y;
   
   if      (xrel > 0) ic->x += icr->w;
   else if (xrel < 0) ic->x -= ic->w;
   else if (xa == 1)  ic->x += (icr->w - ic->w) / 2;
   else if (xa == 2)  ic->x += icr->w - ic->w;
   
   if      (yrel > 0) ic->y += icr->h;
   else if (yrel < 0) ic->y -= ic->h;
   else if (ya == 1)  ic->y += (icr->h - ic->h) / 2;
   else if (ya == 2)  ic->y += icr->h - ic->h;
}

static void
_e_fm2_icons_place_icon(E_Fm2_Icon *ic)
{
   Eina_List *l, *pl;
   E_Fm2_Icon *ic2;
   
   ic->x = 0;
   ic->y = 0;
   ic->saved_pos = 1;
   /* ### BLAH ### */
//   if (!_e_fm2_icons_icon_overlaps(ic)) return;
/*	     
 _e_fm2_icon_place_relative(ic, ic2, 1, 0, 0, 2);
 if (_e_fm2_icons_icon_row_ok(ic) && !_e_fm2_icons_icon_overlaps(ic)) return;
 _e_fm2_icon_place_relative(ic, ic2, 0, -1, 0, 0);
 if (_e_fm2_icons_icon_row_ok(ic) && !_e_fm2_icons_icon_overlaps(ic)) return;
 _e_fm2_icon_place_relative(ic, ic2, 0, -1, 1, 0);
 if (_e_fm2_icons_icon_row_ok(ic) && !_e_fm2_icons_icon_overlaps(ic)) return;
 _e_fm2_icon_place_relative(ic, ic2, 1, 0, 0, 0);
 if (_e_fm2_icons_icon_row_ok(ic) && !_e_fm2_icons_icon_overlaps(ic)) return;
 _e_fm2_icon_place_relative(ic, ic2, 1, 0, 0, 1);
 if (_e_fm2_icons_icon_row_ok(ic) && !_e_fm2_icons_icon_overlaps(ic)) return;
 _e_fm2_icon_place_relative(ic, ic2, 0, 1, 0, 0);
 if (_e_fm2_icons_icon_row_ok(ic) && !_e_fm2_icons_icon_overlaps(ic)) return;
 _e_fm2_icon_place_relative(ic, ic2, 0, 1, 1, 0);
 if (_e_fm2_icons_icon_row_ok(ic) && !_e_fm2_icons_icon_overlaps(ic)) return;
 */
//   for (l = ic->sd->icons_place; l; l = l->next)
   for (l = ic->sd->icons; l; l = l->next)
     {
	ic2 = l->data;
	
	if ((ic2 != ic) && (ic2->saved_pos))
	  {
	     // ###_
	     _e_fm2_icon_place_relative(ic, ic2, 1, 0, 0, 2);
	     if (_e_fm2_icons_icon_row_ok(ic) && 
		 !_e_fm2_icons_icon_overlaps(ic)) goto done;
	     // _###
	     _e_fm2_icon_place_relative(ic, ic2, -1, 0, 0, 2);
	     if (_e_fm2_icons_icon_row_ok(ic) && 
		 !_e_fm2_icons_icon_overlaps(ic)) goto done;
	  }
     }
   
//   for (l = ic->sd->icons_place; l;)
   for (l = ic->sd->icons; l;)
     {
	ic2 = l->data;
	
	if ((ic2 != ic) && (ic2->saved_pos))
	  {
	     // ###
	     //  |
	     _e_fm2_icon_place_relative(ic, ic2, 0, 1, 1, 0);
	     if (_e_fm2_icons_icon_row_ok(ic) && 
		 !_e_fm2_icons_icon_overlaps(ic)) goto done;
	     //  |
	     // ###
	     _e_fm2_icon_place_relative(ic, ic2, 0, -1, 1, 0);
	     if (_e_fm2_icons_icon_row_ok(ic) && 
		 !_e_fm2_icons_icon_overlaps(ic)) goto done;
	  }
	pl = l;
	l = l->next;
        if ((ic2 != ic) && (ic2->saved_pos))
	  {
//	     ic->sd->icons_place = eina_list_remove_list(ic->sd->icons_place, pl);
	  }
     }
   done:
   return;
}

static void
_e_fm2_icons_place_custom_icons(E_Fm2_Smart_Data *sd)
{
   Eina_List *l;
   E_Fm2_Icon *ic;

   for (l = sd->icons; l; l = l->next)
     {
	ic = l->data;

	if (!ic->saved_pos)
	  {
	     /* FIXME: place using smart place fn */
	     _e_fm2_icons_place_icon(ic);
	  }
	
	if ((ic->x + ic->w) > sd->max.w) sd->max.w = ic->x + ic->w;
	if ((ic->y + ic->h) > sd->max.h) sd->max.h = ic->y + ic->h;
     }
}

static void
_e_fm2_icons_place_custom_grid_icons(E_Fm2_Smart_Data *sd)
{
   /* FIXME: not going to implement this at this stage */
   Eina_List *l;
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

static void
_e_fm2_icons_place_custom_smart_grid_icons(E_Fm2_Smart_Data *sd)
{
   /* FIXME: not going to implement this at this stage */
   Eina_List *l;
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
   Eina_List *l;
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

static inline char
_e_fm2_view_mode_get(const E_Fm2_Smart_Data *sd)
{
   if (sd->view_mode > -1)
     return sd->view_mode;
   return sd->config->view.mode;
}

static inline Evas_Coord
_e_fm2_icon_w_get(const E_Fm2_Smart_Data *sd)
{
   if (sd->icon_size > -1)
     return sd->icon_size * e_scale;
   return sd->config->icon.icon.w;
}

static inline Evas_Coord
_e_fm2_icon_h_get(const E_Fm2_Smart_Data *sd)
{
   if (sd->icon_size > -1)
     return sd->icon_size * e_scale;
   return sd->config->icon.icon.h;
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
   switch (_e_fm2_view_mode_get(sd))
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
	/* FIXME: not going to implement this at this stage */
	_e_fm2_icons_place_custom_grid_icons(sd);
	break;
      case E_FM2_VIEW_MODE_CUSTOM_SMART_GRID_ICONS:
	/* FIXME: not going to implement this at this stage */
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
        sd->icons = eina_list_remove_list(sd->icons, sd->icons);
     }
   eina_list_free(sd->icons_place);
   sd->icons_place = NULL;
   sd->tmp.last_insert = NULL;
   E_FREE(sd->tmp.list_index);
}

static void
_e_fm2_regions_eval(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   Eina_List *l;
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
   if (cfg->icon.key_hint) eina_stringshare_del(cfg->icon.key_hint);
   if (cfg->theme.background) eina_stringshare_del(cfg->theme.background);
   if (cfg->theme.frame) eina_stringshare_del(cfg->theme.frame);
   if (cfg->theme.icons) eina_stringshare_del(cfg->theme.icons);
   free(cfg);
}

static Evas_Object *
_e_fm2_file_fm2_find(const char *file)
{
   char *dir;
   Eina_List *l;
   
   dir = ecore_file_dir_get(file);
   if (!dir) return NULL;
   for (l = _e_fm2_list; l; l = l->next)
     {
	if ((_e_fm2_list_walking > 0) &&
	    (eina_list_data_find(_e_fm2_list_remove, l->data))) continue;
	if (!strcmp(e_fm2_real_path_get(l->data), dir))
	  {
	     free(dir);
	     return l->data;
	  }
     }
   free(dir);
   return NULL;
}

static E_Fm2_Icon *
_e_fm2_icon_find(Evas_Object *obj, const char *file)
{
   E_Fm2_Smart_Data *sd;
   Eina_List *l;
   E_Fm2_Icon *ic;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return NULL;
   for (l = sd->icons; l; l = l->next)
     {
	ic = l->data;
	if (!strcmp(ic->info.file, file)) return ic;
     }
   return NULL;
}

/* Escape illegal caracters within an uri and return an eina_stringshare */
static const char *
_e_fm2_uri_escape(const char *path)
{
   char dest[PATH_MAX * 3 + 7];
   const char *p;
   int i;

   if (!path) return NULL;
   memset(dest, 0, PATH_MAX * 3 + 7);

   snprintf(dest, 8, "file://");

   /* Most app doesn't handle the hostname in the uri so it's put to NULL */
   for (i = 7, p = path; *p != '\0'; p++, i++)
     {
	if (isalnum(*p) || strchr("/$-_.+!*'()", *p))
	  dest[i] = *p;
	else
	  {
	     snprintf(&(dest[i]), 4, "%%%02X", (unsigned char)*p);
	     i += 2;
	  }
     }

   return eina_stringshare_add(dest);
}

/* Parse a single uri and return an E_Fm2_Uri struct.
 * If the parsing have failed it return NULL.
 * The E_Fm2_Uri may have hostname parameter and always a path.
 * If there's no hostname in the uri then the hostname parameter is NULL
 */
static E_Fm2_Uri *
_e_fm2_uri_parse(const char *val)
{
   E_Fm2_Uri *uri;
   const char *p;
   char hostname[PATH_MAX], path[PATH_MAX];
   int i = 0;

   /* The shortest possible path is file:/// 
    * anything smaller than that can't be a valid uri 
    */
   if (strlen(val) <= 7 && strncmp(val, "file://", 7)) return NULL;
   memset(path, 0, PATH_MAX);

   /* An uri should be in a form file://<hostname>/<path> */
   p = val + 7;
   if (*p != '/')
     {
	for (i = 0; *p != '/' && *p != '\0' && i < _POSIX_HOST_NAME_MAX; p++, i++)
	  hostname[i] = *p;
     }
   hostname[i] = '\0';

   /* See http://www.faqs.org/rfcs/rfc1738.html for the escaped chars */
   for (i = 0; *p != '\0' && i < PATH_MAX; i++, p++)
     {
	if (*p == '%')
	  {
	     path[i] = *(++p);
	     path[i+1] = *(++p);
	     path[i] = (char)strtol(&(path[i]), NULL, 16);
	     path[i+1] = '\0';
	  }
	else
	  path[i] = *p;
     }

   uri = E_NEW(E_Fm2_Uri, 1);
   if (hostname[0]) uri->hostname = eina_stringshare_add(hostname);
   else uri->hostname = NULL;
   uri->path = eina_stringshare_add(path);

   return uri;
}

/* Takes an Eina_List of uri and return an Eina_List of real paths */
static Eina_List *
_e_fm2_uri_path_list_get(Eina_List *uri_list)
{
   E_Fm2_Uri *uri;
   Eina_List *l, *path_list = NULL;
   char current_hostname[_POSIX_HOST_NAME_MAX];
   
   if (gethostname(current_hostname, _POSIX_HOST_NAME_MAX) == -1)
     current_hostname[0] = '\0';

   for (l = uri_list; l; l = l->next)
     {
	if (!(uri = _e_fm2_uri_parse(l->data)))
	  continue;

	if (!uri->hostname || !strcmp(uri->hostname, "localhost") 
	    || !strcmp(uri->hostname, current_hostname))
	  {
	     path_list = eina_list_append(path_list, uri->path);
	  }
	else
	  eina_stringshare_del(uri->path);

	if (uri->hostname) eina_stringshare_del(uri->hostname);
	E_FREE(uri);
     }

   return path_list;
}

static Eina_List *
_e_fm2_uri_icon_list_get(Eina_List *uri)
{
   Eina_List *icons = NULL;
   Eina_List *l;
   
   for (l = uri; l; l = l->next)
     {
	const char *path, *file;
	Evas_Object *fm;
	E_Fm2_Icon *ic;
	
	path = l->data;
	ic = NULL;

	fm = _e_fm2_file_fm2_find(path);
	if (fm)
	  {
	     file = ecore_file_file_get(path);
	     ic = _e_fm2_icon_find(fm, file);
	  }
	icons = eina_list_append(icons, ic);
     }
   return icons;
}

/**************************/

static E_Fm2_Icon *
_e_fm2_icon_new(E_Fm2_Smart_Data *sd, const char *file, E_Fm2_Finfo *finf)
{
   E_Fm2_Icon *ic;
   
   /* create icon */
   ic = E_NEW(E_Fm2_Icon, 1);
   ic->info.fm = sd->obj;
   ic->info.ic = ic;
   ic->info.file = eina_stringshare_add(file);
   ic->sd = sd;
   if (!_e_fm2_icon_fill(ic, finf))
     {
	eina_stringshare_del(ic->info.file);
	free(ic);
	return NULL;
     }
   return ic;
}

static void
_e_fm2_icon_unfill(E_Fm2_Icon *ic)
{
   eina_stringshare_del(ic->info.mime);
   eina_stringshare_del(ic->info.label);
   eina_stringshare_del(ic->info.comment);
   eina_stringshare_del(ic->info.generic);
   eina_stringshare_del(ic->info.icon);
   eina_stringshare_del(ic->info.link);
   eina_stringshare_del(ic->info.real_link);
   eina_stringshare_del(ic->info.category);
   ic->info.mime = NULL;
   ic->info.label = NULL;
   ic->info.comment = NULL;
   ic->info.generic = NULL;
   ic->info.icon = NULL;
   ic->info.link = NULL;
   ic->info.real_link = NULL;
   ic->info.mount = 0;
   ic->info.removable = 0;
   ic->info.removable_full = 0;
   ic->info.deleted = 0;
   ic->info.broken_link = 0;
}

static void
_e_fm2_icon_geom_adjust(E_Fm2_Icon *ic, int saved_x, int saved_y, int saved_w, int saved_h, int saved_res_w, int saved_res_h)
{
   int qx, qy, rx, ry, x, y;
   
   if (!((_e_fm2_view_mode_get(ic->sd) == E_FM2_VIEW_MODE_CUSTOM_ICONS) &&
	 (ic->sd->config->view.fit_custom_pos) &&
	 (saved_res_w > 0) &&
	 (saved_res_h > 0)))
     return;
   if (saved_res_w >= 3) qx = saved_x / (saved_res_w / 3);
   else qx = 0;
   rx = saved_x - ((saved_res_w / 2) * qx);
   x = ((ic->sd->w / 2) * qx) + rx;
   ic->x = x;
   
   if (saved_res_h >= 3) qy = saved_y / (saved_res_h / 3);
   else qy = 0;
   ry = saved_y - ((saved_res_h / 2) * qy);
   y = ((ic->sd->h / 2) * qy) + ry;
   ic->y = y;
}

static int
_e_fm2_icon_fill(E_Fm2_Icon *ic, E_Fm2_Finfo *finf)
{
   Evas_Coord mw = 0, mh = 0;
   Evas_Object *obj, *obj2;
   char buf[4096], *lnk;
   const char *mime;
   E_Fm2_Custom_File *cf;

   snprintf(buf, sizeof(buf), "%s/%s", ic->sd->realpath, ic->info.file);
   cf = e_fm2_custom_file_get(buf);
   if (finf)
     {
	memcpy(&(ic->info.statinfo), &(finf->st), sizeof(struct stat));
	if ((finf->lnk) && (finf->lnk[0]))
	  ic->info.link = eina_stringshare_add(finf->lnk);
	else
	  ic->info.link = NULL;
	if ((finf->rlnk) && (finf->rlnk[0]))
	  ic->info.real_link = eina_stringshare_add(finf->rlnk);
	else
	  ic->info.real_link = NULL;
	ic->info.broken_link = finf->broken_link;
     }
   else
     {
	printf("FIXME: remove old non finf icon fill code\n");
	/* FIXME: this should go away... get this from the fm slave proc above */
	lnk = ecore_file_readlink(buf);
	if (stat(buf, &(ic->info.statinfo)) == -1)
	  {
	     if (lnk)
	       ic->info.broken_link = 1;
	     else
	       {
		  return 0;
	       }
	  }
	if (lnk)
	  {
	     if (lnk[0] == '/')
	       {
		  ic->info.link = eina_stringshare_add(lnk);
		  ic->info.real_link = eina_stringshare_add(lnk);
	       }
	     else
	       {
		  char *rp;
		  
		  snprintf(buf, sizeof(buf), "%s/%s", ic->sd->realpath, lnk);
		  rp = ecore_file_realpath(buf);
		  if (rp)
		    {
		       ic->info.link = eina_stringshare_add(rp);
		       free(rp);
		    }
		  ic->info.real_link = eina_stringshare_add(lnk);
	       }
	     free(lnk);
	  }
	/* FIXME: end go away chunk */
     }
   
   if (S_ISDIR(ic->info.statinfo.st_mode))
     {
       ic->info.mime = eina_stringshare_add("inode/directory");
     }
   if (!ic->info.mime)
     {
	mime = e_fm_mime_filename_get(ic->info.file);
	if (mime) ic->info.mime = eina_stringshare_add(mime);
     }
   
   if ((e_util_glob_case_match(ic->info.file, "*.desktop")) ||
       (e_util_glob_case_match(ic->info.file, "*.directory")))
     _e_fm2_icon_desktop_load(ic);

   if (cf)
     {
	if (cf->icon.valid)
	  {
	     if (cf->icon.icon)
	       {
		  _eina_stringshare_replace(&ic->info.icon, cf->icon.icon);
	       }
	     ic->info.icon_type = cf->icon.type;
	  }
	if (cf->geom.valid)
	  {
	     ic->saved_pos = 1;
	     ic->x = cf->geom.x;
	     ic->y = cf->geom.y;
	     if (cf->geom.w > 0) ic->w = cf->geom.w;
	     if (cf->geom.h > 0) ic->h = cf->geom.h;
	     _e_fm2_icon_geom_adjust(ic, cf->geom.x, cf->geom.y, cf->geom.w, cf->geom.h, cf->geom.res_w, cf->geom.res_h);
	  }
     }
   
   evas_event_freeze(evas_object_evas_get(ic->sd->obj));
   edje_freeze();

   switch (_e_fm2_view_mode_get(ic->sd))
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
	 */
	if ((!ic->sd->config->icon.fixed.w) || (!ic->sd->config->icon.fixed.h))
	  {
	     obj = ic->sd->tmp.obj;
	     if (!obj)
	       {
		  obj = edje_object_add(evas_object_evas_get(ic->sd->obj));
		  if ((ic->sd->config->icon.fixed.w) && (ic->sd->config->icon.fixed.h))
		    _e_fm2_theme_edje_object_set(ic->sd, obj,
						 "base/theme/fileman",
						 "icon/fixed");
		  else
		    _e_fm2_theme_edje_object_set(ic->sd, obj,
						 "base/theme/fileman",
						 "icon/variable");
                  ic->sd->tmp.obj = obj;
	       }
	     _e_fm2_icon_label_set(ic, obj);
	     obj2 = ic->sd->tmp.obj2;
	     if (!obj2)
	       {
		  obj2 = evas_object_rectangle_add(evas_object_evas_get(ic->sd->obj));
		  ic->sd->tmp.obj2 = obj2;
	       }
	     /* FIXME: if icons are allowed to have their own size - use it */
	     edje_extern_object_min_size_set(obj2, _e_fm2_icon_w_get(ic->sd), _e_fm2_icon_h_get(ic->sd));
	     edje_extern_object_max_size_set(obj2, _e_fm2_icon_w_get(ic->sd), _e_fm2_icon_h_get(ic->sd));
	     edje_object_part_swallow(obj, "e.swallow.icon", obj2);
	     edje_object_size_min_calc(obj, &mw, &mh);
	  }
	ic->w = mw;
	ic->h = mh;
	if (ic->sd->config->icon.fixed.w) ic->w = _e_fm2_icon_w_get(ic->sd);
	if (ic->sd->config->icon.fixed.h) ic->h = _e_fm2_icon_h_get(ic->sd);
	ic->min_w = mw;
	ic->min_h = mh;
	break;
      case E_FM2_VIEW_MODE_LIST:
	  {
	     obj = ic->sd->tmp.obj;
	     if (!obj)
	       {
		  obj = edje_object_add(evas_object_evas_get(ic->sd->obj));
// vairable sized list items are pretty usless - ignore.		  
//		  if (ic->sd->config->icon.fixed.w)
		    _e_fm2_theme_edje_object_set(ic->sd, obj,
						 "base/theme/fileman",
						 "list/fixed");
//		  else
//		    _e_fm2_theme_edje_object_set(ic->sd, obj, "base/theme/fileman",
//					    "list/variable");
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
   if (ic->prop_dialog)
     {
	e_object_del(E_OBJECT(ic->prop_dialog));
	ic->prop_dialog = NULL;
     }
   eina_stringshare_del(ic->info.file);
   eina_stringshare_del(ic->info.mime);
   eina_stringshare_del(ic->info.label);
   eina_stringshare_del(ic->info.comment);
   eina_stringshare_del(ic->info.generic);
   eina_stringshare_del(ic->info.icon);
   eina_stringshare_del(ic->info.link);
   eina_stringshare_del(ic->info.real_link);
   eina_stringshare_del(ic->info.category);
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
   if (_e_fm2_view_mode_get(ic->sd) == E_FM2_VIEW_MODE_LIST)
     {
	const char *stacking;
	
//        if (ic->sd->config->icon.fixed.w)
//	  {
	if (ic->odd)
	  _e_fm2_theme_edje_object_set(ic->sd, ic->obj,
				       "base/theme/widgets",
				       "list_odd/fixed");
	else
	  _e_fm2_theme_edje_object_set(ic->sd, ic->obj,
				       "base/theme/widgets",
				       "list/fixed");
	stacking = edje_object_data_get(ic->obj, "stacking");
	if (stacking)
	  {
	     if (!strcmp(stacking, "below"))
	       evas_object_stack_above(ic->obj, ic->sd->underlay);
	     else if (!strcmp(stacking, "above"))
	       evas_object_stack_below(ic->obj, ic->sd->drop);
	  }
	
//	  }
//	else
//	  {
//	     if (ic->odd)
//	       _e_fm2_theme_edje_object_set(ic->sd, ic->obj, "base/theme/widgets",
//				       "list_odd/variable");
//	     else
//	       _e_fm2_theme_edje_object_set(ic->sd, ic->obj, "base/theme/widgets",
//				       "list/variable");
//	  }
     }
   else
     {
        if (ic->sd->config->icon.fixed.w)
	  _e_fm2_theme_edje_object_set(ic->sd, ic->obj,
				       "base/theme/fileman",
				       "icon/fixed");
	else
	  _e_fm2_theme_edje_object_set(ic->sd, ic->obj,
				       "base/theme/fileman",
				       "icon/variable");
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
	const char *selectraise;
	
	/* FIXME: need new signal to INSTANTLY activate - no anim */
	/* FIXME: while listing dirs need to use icons in-place and not
	 * unrealize and re-realize */
	edje_object_signal_emit(ic->obj, "e,state,selected", "e");
	edje_object_signal_emit(ic->obj_icon, "e,state,selected", "e");
	selectraise = edje_object_data_get(ic->obj, "selectraise");
        if ((selectraise) && (!strcmp(selectraise, "on")))
	  evas_object_stack_below(ic->obj, ic->sd->drop);
     }
   if (ic->info.removable_full)
     edje_object_signal_emit(ic->obj_icon, "e,state,removable,full", "e");
//   else
//     edje_object_signal_emit(ic->obj_icon, "e,state,removable,empty", "e");
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
	ecore_strlcpy(buf, ic->info.file, sizeof(buf));

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

   oic = e_fm2_icon_get(evas_object_evas_get(o), ic,
			gen_func, data, force_gen, NULL);
   if (oic)
     {
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
	(!ic->sd->listing)))
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
	const char *selectraise;
	
	edje_object_signal_emit(ic->obj, "e,state,selected", "e");
	edje_object_signal_emit(ic->obj_icon, "e,state,selected", "e");
	evas_object_stack_below(ic->obj, ic->sd->drop);
	selectraise = edje_object_data_get(ic->obj, "selectraise");
        if ((selectraise) && (!strcmp(selectraise, "on")))
	  evas_object_stack_below(ic->obj, ic->sd->drop);
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
	const char *stacking, *selectraise;
	
	edje_object_signal_emit(ic->obj, "e,state,unselected", "e");
	edje_object_signal_emit(ic->obj_icon, "e,state,unselected", "e");
        stacking = edje_object_data_get(ic->obj, "stacking");
	selectraise = edje_object_data_get(ic->obj, "selectraise");
	if ((selectraise) && (!strcmp(selectraise, "on")))
	  {
	     if ((stacking) && (!strcmp(stacking, "below")))
	       evas_object_stack_above(ic->obj, ic->sd->underlay);
	  }
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
   path = p;
   while (*path == '/') path++;
   path--;
   s = eina_stringshare_add(path);
   free(p);
   return s;
}

static int
_e_fm2_icon_desktop_load(E_Fm2_Icon *ic)
{
   char buf[4096];
   Efreet_Desktop *desktop;
   
   snprintf(buf, sizeof(buf), "%s/%s", ic->sd->realpath, ic->info.file);

   desktop = efreet_desktop_new(buf);
//   printf("efreet_desktop_new(%s) = %p\n", buf, desktop);
   if (!desktop) goto error;
//   if (desktop->type != EFREET_DESKTOP_TYPE_LINK) goto error;

   ic->info.removable = 0;
   ic->info.removable_full = 0;
   ic->info.label   = eina_stringshare_add(desktop->name);
   ic->info.generic = eina_stringshare_add(desktop->generic_name);
   ic->info.comment = eina_stringshare_add(desktop->comment);
   ic->info.icon    = eina_stringshare_add(desktop->icon);
   if (desktop->url)
     ic->info.link = _e_fm2_icon_desktop_url_eval(desktop->url);
   if (desktop->x)
     {
	const char *type;

	type = eina_hash_find(desktop->x, "X-Enlightenment-Type");
	if (type)
	  {
	     if (!strcmp(type, "Mount")) ic->info.mount = 1;
	     else if (!strcmp(type, "Removable"))
	       {
		  ic->info.removable = 1;
		  if ((!e_fm2_hal_storage_find(ic->info.link)) &&
		       (!e_fm2_hal_volume_find(ic->info.link)))
		    {
		       _e_fm2_live_file_del(ic->sd->obj, ic->info.file);
		       efreet_desktop_free(desktop);
		       goto error;
		    }
	       }
	     type = eina_hash_find(desktop->x, "X-Enlightenment-Removable-State");
	     if (type)
	       {
		  if (!strcmp(type, "Full"))
		    ic->info.removable_full = 1;
	       }
	  }
     }
   /* FIXME: get category */
   ic->info.category = NULL;
   efreet_desktop_free(desktop);

   return 1;
   error:
   eina_stringshare_del(ic->info.label);
   eina_stringshare_del(ic->info.comment);
   eina_stringshare_del(ic->info.generic);
   eina_stringshare_del(ic->info.icon);
   eina_stringshare_del(ic->info.link);
   eina_stringshare_del(ic->info.category);
   ic->info.label = NULL;
   ic->info.comment = NULL;
   ic->info.generic = NULL;
   ic->info.icon = NULL;
   ic->info.link = NULL;
   ic->info.category = NULL;
   //Hack
   if (!strncmp(ic->info.file, "|storage_", 9)) ecore_file_unlink(buf);
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
	rg->list = eina_list_remove_list(rg->list, rg->list);
     }
   free(rg);
}

static void
_e_fm2_region_realize(E_Fm2_Region *rg)
{
   Eina_List *l;
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
   Eina_List *l;
   
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
   if (_e_fm2_view_mode_get(ic->sd) == E_FM2_VIEW_MODE_LIST)
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
   Eina_List *l;
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
   Eina_List *l;
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
   ic = eina_list_last(sd->icons)->data;
   _e_fm2_icon_select(ic);
   evas_object_smart_callback_call(sd->obj, "selection_change", NULL);
   _e_fm2_icon_make_visible(ic);
}

static void
_e_fm2_icon_sel_prev(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   Eina_List *l;
   E_Fm2_Icon *ic, *ic_prev;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (!sd->icons) return;

   ic_prev = NULL;
   EINA_LIST_FOREACH(sd->icons, l, ic)
     {
	if (ic->selected)
	  {
	     if (!l->prev) return;
	     ic_prev = l->prev->data;
	     break;
	  }
     }
   if (!ic_prev)
     {
	_e_fm2_icon_sel_last(obj);
	return;
     }
   _e_fm2_icon_desel_any(obj);
   _e_fm2_icon_select(ic_prev);
   evas_object_smart_callback_call(sd->obj, "selection_change", NULL);
   _e_fm2_icon_make_visible(ic_prev);
}

static void
_e_fm2_icon_sel_next(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   Eina_List *l;
   E_Fm2_Icon *ic, *ic_next;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (!sd->icons) return;

   ic_next = NULL;
   EINA_LIST_FOREACH(sd->icons, l, ic)
     {
	if (ic->selected)
	  {
	     if (!l->next) return;
	     ic_next = l->next->data;
	     break;
	  }
     }
   if (!ic_next)
     {
	_e_fm2_icon_sel_first(obj);
	return;
     }
   _e_fm2_icon_desel_any(obj);
   _e_fm2_icon_select(ic_next);
   evas_object_smart_callback_call(sd->obj, "selection_change", NULL);
   _e_fm2_icon_make_visible(ic_next);
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
	     char buf[4096];
	     snprintf(buf, sizeof(buf), "%s/%s", ic->sd->path, ic->info.file);
	     e_fm2_path_set(ic->sd->obj, ic->sd->dev, buf);
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
   Eina_List *l;
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

static int
_e_fm_typebuf_timer_cb(void *data)
{
   Evas_Object *obj = data;
   E_Fm2_Smart_Data *sd;

   if (!data) return 0;
   sd = evas_object_smart_data_get(obj);
   if (!sd) return 0;

   if (!sd->typebuf_visible) return 0;

   _e_fm2_typebuf_hide(obj);
   sd->typebuf.timer = NULL;

   return 0;
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

   if (sd->typebuf.timer) 
     {
	ecore_timer_del(sd->typebuf.timer);
     }

   sd->typebuf.timer = ecore_timer_add(5.0, _e_fm_typebuf_timer_cb, obj);
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
	     edje_object_signal_emit(ic->sd->drop_in, "e,state,unselected", "e");
	     edje_object_signal_emit(ic->sd->drop, "e,state,selected", "e");
	     ic->sd->drop_in_show = 0;
	     ic->sd->drop_show = 1;
	  }
	else
	  {
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
   Eina_List *l;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (!sd->drag) return;
   sd->drag = 0;
   for (l = sd->icons; l; l = l->next)
     {
	ic = l->data;
	ic->drag.dnd = 0;
	ic->drag.src = 0;
	if (ic->obj) evas_object_show(ic->obj);
	if (ic->obj_icon) evas_object_show(ic->obj_icon);
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
   e_drop_handler_action_set(ev->action);
}
 
static void
_e_fm2_cb_dnd_move(void *data, const char *type, void *event)
{
   E_Fm2_Smart_Data *sd;
   E_Event_Dnd_Move *ev;
   E_Fm2_Icon *ic;
   Eina_List *l;
   
   sd = data;
   if (!type) return;
   if (strcmp(type, "text/uri-list")) return;
   ev = (E_Event_Dnd_Move *)event;
   e_drop_handler_action_set(ev->action);
   for (l = sd->icons; l; l = l->next) /* FIXME: should only walk regions and skip non-visible ones */
     {
	ic = l->data;
	if (E_INSIDE(ev->x, ev->y, ic->x - ic->sd->pos.x, ic->y - ic->sd->pos.y, ic->w, ic->h))
	  {
	     if (ic->drag.dnd) continue;
	     /* if list view */
	     if (_e_fm2_view_mode_get(ic->sd) == E_FM2_VIEW_MODE_LIST)
	       {
		  /* if there is a .order file - we can re-order files */
		  if (ic->sd->order_file)
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
			      }
			    else if (ev->y > (ic->y - ic->sd->pos.y + ((ic->h * 3) / 4)))
			      {
				 _e_fm2_dnd_drop_show(ic, 1);
			      }
			    else
			      {
				 _e_fm2_dnd_drop_show(ic, -1);
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
		  /* if it's over a dir - hilight as it will be dropped in */
		  if ((S_ISDIR(ic->info.statinfo.st_mode)) &&
		      (!ic->sd->config->view.no_subdir_drop))
		    _e_fm2_dnd_drop_show(ic, -1);
		  else
		    _e_fm2_dnd_drop_all_show(sd->obj);
	       }
	     return;
	  }
     }
   /* FIXME: not over icon - is it within the fm view? if so drop there */
   if (E_INSIDE(ev->x, ev->y, 0, 0, sd->w, sd->h))
     {
	/* if listview - it is now after last file */
	if (_e_fm2_view_mode_get(sd) == E_FM2_VIEW_MODE_LIST)
	  {
	     /* if there is a .order file - we can re-order files */
	     if (sd->order_file)
	       {
		  ic = eina_list_data_get(eina_list_last(sd->icons));
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
	  _e_fm2_dnd_drop_all_show(sd->obj);
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
   _e_fm2_dnd_drop_hide(sd->obj);
   _e_fm2_dnd_drop_all_hide(sd->obj);
}

static void
_e_fm_file_reorder(const char *file, const char *dst, const char *relative, int after)
{
   unsigned int length = strlen(file) + 1 + strlen(dst) + 1 + strlen(relative) + 1 + sizeof(after);
   void *data, *p;

   data = alloca(length);
   if (!data) return;

   p = data;

#define P(s) memcpy(p, s, strlen(s) + 1); p += strlen(s) + 1
   P(file);
   P(dst);
   P(relative);
#undef P

   memcpy(p, &after, sizeof(int));

   _e_fm_client_send_new(E_FM_OP_REORDER, data, length);
}

static void
_e_fm_icon_save_position(const char *file, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h)
{
   E_Fm2_Custom_File *cf, new;

   if (!file) return;

   cf = e_fm2_custom_file_get(file);
   if (!cf)
     {
	memset(&new, 0, sizeof(E_Fm2_Custom_File));
	cf = &new;
     }

   cf->geom.x = x;
   cf->geom.y = y;
   cf->geom.res_w = w;
   cf->geom.res_h = h;

   cf->geom.valid = 1;
   e_fm2_custom_file_set(file, cf);
   e_fm2_custom_file_flush();
}

static void
_e_fm_drop_menu_copy_cb(void *data, E_Menu *m, E_Menu_Item *mi)
{
   char *args = data;

   if (!data) return;

   _e_fm_client_file_copy(args);
}

static void
_e_fm_drop_menu_move_cb(void *data, E_Menu *m, E_Menu_Item *mi)
{
   char *args = data;

   if (!data) return;

   _e_fm_client_file_move(args);
}

static void
_e_fm_drop_menu_abort_cb(void *data, E_Menu *m, E_Menu_Item *mi)
{
   if (!data) return;
}

static void
_e_fm_drop_menu_post_cb(void *data, E_Menu *m)
{
   char *args = data;

   if (!data) return;

   free(args);
}

static void
_e_fm_drop_menu(char *args)
{
   E_Menu *menu = e_menu_new();
   E_Menu_Item *item = NULL;
   E_Manager *man = NULL;
   E_Container *con = NULL;
   E_Zone *zone = NULL;
   int x, y;

   if (!menu) return;

   item = e_menu_item_new(menu);
   e_menu_item_label_set(item, _("Copy"));
   e_menu_item_callback_set(item, _e_fm_drop_menu_copy_cb, args);
   e_menu_item_icon_edje_set(item,
	 e_theme_edje_file_get("base/theme/fileman",
	    "e/fileman/default/button/copy"),
	 "e/fileman/default/button/copy");

   item = e_menu_item_new(menu);
   e_menu_item_label_set(item, _("Move"));
   e_menu_item_callback_set(item, _e_fm_drop_menu_move_cb, args);
   e_menu_item_icon_edje_set(item,
	 e_theme_edje_file_get("base/theme/fileman",
	    "e/fileman/default/button/move"),
	 "e/fileman/default/button/move");

   item = e_menu_item_new(menu);
   e_menu_item_label_set(item, _("Abort"));
   e_menu_item_callback_set(item, _e_fm_drop_menu_abort_cb, args);
   e_menu_item_icon_edje_set(item,
	 e_theme_edje_file_get("base/theme/fileman",
	    "e/fileman/default/button/abort"),
	 "e/fileman/default/button/abort");

   man = e_manager_current_get();
   if (!man)
     {
	e_object_del(E_OBJECT(menu));
	return;
     }
   con = e_container_current_get(man);
   if (!con)
     {
	e_object_del(E_OBJECT(menu));
	return;
     }
   ecore_x_pointer_xy_get(con->win, &x, &y);
   zone = e_util_zone_current_get(man);
   if (!zone)
     {
	e_object_del(E_OBJECT(menu));
	return;
     }
   e_menu_post_deactivate_callback_set(menu, _e_fm_drop_menu_post_cb, args);
   e_menu_activate_mouse(menu, zone, 
			 x, y, 1, 1, 
			 E_MENU_POP_DIRECTION_DOWN, 0);
}

static void
_e_fm2_cb_dnd_drop(void *data, const char *type, void *event)
{
   E_Fm2_Smart_Data *sd;
   E_Event_Dnd_Drop *ev;
   E_Fm2_Icon *ic;
   Eina_List *fsel, *l, *ll, *il, *isel;
   char buf[4096];
   const char *fp;
   Evas_Coord dx, dy, ox, oy, x, y;
   int adjust_icons = 0;

   char dirpath[PATH_MAX];
   char *args = NULL;
   size_t size = 0;
   size_t length = 0;
   
   sd = data;
   if (!type) return;
   if (strcmp(type, "text/uri-list")) return;
   ev = (E_Event_Dnd_Drop *)event;

   fsel = _e_fm2_uri_path_list_get(ev->data);
   isel = _e_fm2_uri_icon_list_get(fsel);
   if (!isel) return;
   dx = 0; dy = 0;
   ox = 0; oy = 0;
   for (l = isel; l; l = l->next)
     {
	ic = l->data;
	if (ic && ic->drag.src)
	  {
	     ox = ic->x;
	     oy = ic->y;
	     dx = (ic->drag.x + ic->x - ic->sd->pos.x);
	     dy = (ic->drag.y + ic->y - ic->sd->pos.y);
	     break;
	  }
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
	/* move file into this fm dir */
	for (ll = fsel, il = isel; ll && il; ll = ll->next, il = il->next)
	  {
	     ic = il->data;
	     fp = ll->data;
	     if (!fp) continue;

	     if ((ic) && (_e_fm2_view_mode_get(sd) == E_FM2_VIEW_MODE_CUSTOM_ICONS))
	       {
		  /* dnd doesnt tell me all the co-ords of the icons being dragged so i can't place them accurately.
		   * need to fix this. ev->data probably needs to become more compelx than a list of url's
		   */
		  x = ev->x + (ic->x - ox) - ic->drag.x + sd->pos.x;
		  y = ev->y + (ic->y - oy) - ic->drag.y + sd->pos.y;

		  if (x < 0) x = 0;
		  if (y < 0) y = 0;

		  if (sd->config->view.fit_custom_pos)
		    {
		       if ((x + ic->w) > sd->w) x = (sd->w - ic->w);
		       if ((y + ic->h) > sd->h) y = (sd->h - ic->h);
		    }

		  if (ic->sd == sd)
		    {
		       ic->x = x;
		       ic->y = y;
		       ic->saved_pos = 1;
		       adjust_icons = 1;
		    }

		  snprintf(buf, sizeof(buf), "%s/%s",
			sd->realpath, ecore_file_file_get(fp));
		  _e_fm_icon_save_position(buf, x, y, sd->w, sd->h);
	       }

	     args = _e_fm_string_append_quoted(args, &size, &length, fp);
	     args = _e_fm_string_append_char(args, &size, &length, ' ');

	     eina_stringshare_del(fp);
	  }
	if (adjust_icons)
	  {
	     sd->max.w = 0;
	     sd->max.h = 0;
	     for (l = sd->icons; l; l = l->next)
	       {
		  ic = l->data;
		  if ((ic->x + ic->w) > sd->max.w) sd->max.w = ic->x + ic->w;
		  if ((ic->y + ic->h) > sd->max.h) sd->max.h = ic->y + ic->h;
	       }
	     _e_fm2_obj_icons_place(sd);
	     evas_object_smart_callback_call(sd->obj, "changed", NULL);
	  }
	
	args = _e_fm_string_append_quoted(args, &size, &length, sd->realpath);
     }
   else if (sd->drop_icon) /* inot or before/after an icon */
     {
	if (sd->drop_after == -1) /* put into subdir in icon */
	  {
	     /* move file into dir that this icon is for */
	     for (ll = fsel, il = isel; ll && il; ll = ll->next, il = il->next)
	       {
		  ic = il->data;
		  fp = ll->data;
		  if (!fp) continue;
		  /* move the file into the subdir */
		  snprintf(buf, sizeof(buf), "%s/%s/%s",
			   sd->realpath, sd->drop_icon->info.file, ecore_file_file_get(fp));

		  args = _e_fm_string_append_quoted(args, &size, &length, fp);
		  args = _e_fm_string_append_char(args, &size, &length, ' ');

		  eina_stringshare_del(fp);
	       }

	     snprintf(dirpath, sizeof(dirpath), "%s/%s", sd->realpath, sd->drop_icon->info.file);
	     args = _e_fm_string_append_quoted(args, &size, &length, dirpath);
	  }
	else
	  {
	     if (_e_fm2_view_mode_get(sd) == E_FM2_VIEW_MODE_LIST && sd->order_file) /* list */
	       {
		  for (ll = fsel, il = isel; ll && il; ll = ll->next, il = il->next)
		    {
		       ic = il->data;
		       fp = ll->data;
		       if (!fp) continue;
		       snprintf(buf, sizeof(buf), "%s/%s",
			     sd->realpath, ecore_file_file_get(fp));
		       if (sd->config->view.link_drop)
			 {
			    _e_fm2_client_file_symlink(buf, fp, sd->drop_icon->info.file, sd->drop_after, -9999, -9999, sd->h, sd->h);
			 }
		       else
			 {
			    args = _e_fm_string_append_quoted(args, &size, &length, fp);
			    args = _e_fm_string_append_char(args, &size, &length, ' ');
			 }

		       _e_fm_file_reorder(ecore_file_file_get(fp), sd->realpath, sd->drop_icon->info.file, sd->drop_after);

		       eina_stringshare_del(fp);
		    }

		  args = _e_fm_string_append_quoted(args, &size, &length, sd->realpath);
	       }
	     else
	       {
		  for (ll = fsel, il = isel; ll && il; ll = ll->next, il = il->next)
		    {
		       ic = il->data;
		       fp = ll->data;
		       if (!fp) continue;

		       args = _e_fm_string_append_quoted(args, &size, &length, fp);
		       args = _e_fm_string_append_char(args, &size, &length, ' ');

		       eina_stringshare_del(fp);
		    }
		  args = _e_fm_string_append_quoted(args, &size, &length, sd->realpath);
	       }
	  }
     }

   if (e_drop_handler_action_get() == ECORE_X_ATOM_XDND_ACTION_COPY)
     {
	_e_fm_client_file_copy(args);
	free(args);
     }
   else if (e_drop_handler_action_get() == ECORE_X_ATOM_XDND_ACTION_MOVE)
     {
	_e_fm_client_file_move(args);
	free(args);
     }
   else if (e_drop_handler_action_get() == ECORE_X_ATOM_XDND_ACTION_ASK)
     {
	_e_fm_drop_menu(args);
     }

   _e_fm2_dnd_drop_hide(sd->obj);
   _e_fm2_dnd_drop_all_hide(sd->obj);
   _e_fm2_list_walking++;
   for (l = _e_fm2_list; l; l = l->next)
     {
	if ((_e_fm2_list_walking > 0) &&
	    (eina_list_data_find(_e_fm2_list_remove, l->data))) continue;
	_e_fm2_dnd_finish(l->data, 0);
     }
   _e_fm2_list_walking--;
   if (_e_fm2_list_walking == 0)
     {
	while (_e_fm2_list_remove)
	  {
	     _e_fm2_list = eina_list_remove(_e_fm2_list, _e_fm2_list_remove->data);
	     _e_fm2_list_remove = eina_list_remove_list(_e_fm2_list_remove, _e_fm2_list_remove);
	  }
     }
   eina_list_free(fsel);
   eina_list_free(isel);
}

/* FIXME: prototype */
static void
_e_fm2_mouse_1_handler(E_Fm2_Icon *ic, int up, Evas_Modifier *modifiers)
{
   Eina_List *l;
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
	if (up)
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
	if (
	    (S_ISDIR(ic->info.statinfo.st_mode)) && 
	    (ic->sd->config->view.open_dirs_in_place) &&
	    (!ic->sd->config->view.no_subdir_jump) &&
	    (!ic->sd->config->view.single_click)
	    )
	  {
	     char buf[4096];
	     snprintf(buf, sizeof(buf), "%s/%s", ic->sd->path, ic->info.file);
	     e_fm2_path_set(ic->sd->obj, ic->sd->dev, buf);
	  }
	else
	  evas_object_smart_callback_call(ic->sd->obj, "selected", NULL);
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
	     ic->drag.x = ev->output.x - ic->x - ic->sd->x + ic->sd->pos.x;
	     ic->drag.y = ev->output.y - ic->y - ic->sd->y + ic->sd->pos.y;
	     ic->drag.start = 1;
	     ic->drag.dnd = 0;
	     ic->drag.src = 1;
	  }
	  _e_fm2_mouse_1_handler(ic, 0, ev->modifiers);
     }
   else if (ev->button == 3)
     {
	if (!ic->selected) _e_fm2_mouse_1_handler(ic, 0, ev->modifiers);
	_e_fm2_icon_menu(ic, ic->sd->obj, ev->timestamp);
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
	if (!(ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD))
	  _e_fm2_mouse_1_handler(ic, 1, ev->modifiers);
        ic->drag.start = 0;
	ic->drag.dnd = 0;
	ic->drag.src = 0;

        if (
	    (S_ISDIR(ic->info.statinfo.st_mode)) &&
            (ic->sd->config->view.open_dirs_in_place) &&
            (!ic->sd->config->view.no_subdir_jump) &&
            (ic->sd->config->view.single_click)
            )
          {
             char buf[4096];
             snprintf(buf, sizeof(buf), "%s/%s", ic->sd->path, ic->info.file);
             e_fm2_path_set(ic->sd->obj, ic->sd->dev, buf);
          }
        else if ((S_ISDIR(ic->info.statinfo.st_mode)) && (ic->sd->config->view.single_click))
          evas_object_smart_callback_call(ic->sd->obj, "selected", NULL);



     }
   ic->down_sel = 0;
}

static void
_e_fm2_cb_drag_finished(E_Drag *drag, int dropped)
{
   E_Fm2_Uri *uri;
   const char *p;
   char buf[PATH_MAX * 3 + 7];
   Evas_Object *fm;
   int i;
   
   memset(buf, 0, sizeof(buf));
   for (p = drag->data, i = 0; p && *p != '\0'; p++, i++) 
     {
	if (*p == '\r')
	  {
	     p++;
	     i = -1;
	     uri = _e_fm2_uri_parse(buf);
	     memset(buf, 0, sizeof(buf));
	     if (!uri) continue;

	     fm = _e_fm2_file_fm2_find(uri->path);
	     if (fm)
	       {
		  const char *file;
		  E_Fm2_Icon *ic;

		  file = ecore_file_file_get(uri->path);
		  ic = _e_fm2_icon_find(fm, file);
		  ic->drag.dnd = 0;
		  if (ic->obj) evas_object_show(ic->obj);
		  if (ic->obj_icon) evas_object_show(ic->obj_icon);
	       }

	     if (uri->hostname) eina_stringshare_del(uri->hostname);
	     eina_stringshare_del(uri->path);
	     E_FREE(uri);
	  }
	else
	  buf[i] = *p;
     }
   free(drag->data);
}

void
_e_fm_drag_key_down_cb(E_Drag *drag, Ecore_X_Event_Key_Down *e)
{
   if (!strncmp(e->keyname, "Alt", 3))
     {
	ecore_x_dnd_source_action_set(ECORE_X_ATOM_XDND_ACTION_ASK);
	edje_object_signal_emit(drag->object, "e,state,ask", "e");
     }
   else if (!strncmp(e->keyname, "Shift", 5))
     {
	ecore_x_dnd_source_action_set(ECORE_X_ATOM_XDND_ACTION_MOVE);
	edje_object_signal_emit(drag->object, "e,state,move", "e");
     }
   else if (!strncmp(e->keyname, "Control", 7))
     {
	ecore_x_dnd_source_action_set(ECORE_X_ATOM_XDND_ACTION_COPY);	
	edje_object_signal_emit(drag->object, "e,state,copy", "e");
     }
}

void
_e_fm_drag_key_up_cb(E_Drag *drag, Ecore_X_Event_Key_Up *e)
{
   /* Default action would be move. ;) */

   if (!strncmp(e->keyname, "Alt", 3))
     {
	ecore_x_dnd_source_action_set(ECORE_X_ATOM_XDND_ACTION_MOVE);
     }
   else if (!strncmp(e->keyname, "Shift", 5))
     {
	ecore_x_dnd_source_action_set(ECORE_X_ATOM_XDND_ACTION_MOVE);
     }
   else if (!strncmp(e->keyname, "Control", 7))
     {
	ecore_x_dnd_source_action_set(ECORE_X_ATOM_XDND_ACTION_MOVE);
     }

   edje_object_signal_emit(drag->object, "e,state,move", "e");
}

static void
_e_fm2_cb_icon_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Move *ev;
   E_Fm2_Icon *ic;
   E_Fm2_Icon_Info *ici;
   
   ic = data;
   ev = event_info;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;
   if ((ic->drag.start) && (ic->sd->eobj))
     {     
	int dx, dy;
	
	dx = ev->cur.output.x - (ic->drag.x + ic->x + ic->sd->x - ic->sd->pos.x);
	dy = ev->cur.output.y - (ic->drag.y + ic->y + ic->sd->y - ic->sd->pos.y);
	if (((dx * dx) + (dy * dy)) >
	    (e_config->drag_resist * e_config->drag_resist))
	  {
	     E_Drag *d;
	     Evas_Object *o, *o2;
	     Evas_Coord x, y, w, h;
	     const char *drag_types[] = { "text/uri-list" }, *realpath;
	     char buf[PATH_MAX + 8], *sel = NULL;
	     E_Container *con = NULL;
	     Eina_List *l, *sl;
	     int i, sel_length = 0;
	     
	     switch (ic->sd->eobj->type)
	       {
		case E_GADCON_TYPE:
		  con = ((E_Gadcon *)(ic->sd->eobj))->zone->container;
		  break;
		case E_WIN_TYPE:
		  con = ((E_Win *)(ic->sd->eobj))->container;
		  break;
		case E_ZONE_TYPE:
		  con = ((E_Zone *)(ic->sd->eobj))->container;
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
	     if (ic->obj) evas_object_hide(ic->obj);
	     if (ic->obj_icon) evas_object_hide(ic->obj_icon);
	     ic->drag.start = 0;
	     evas_object_geometry_get(ic->obj, &x, &y, &w, &h);
	     realpath = e_fm2_real_path_get(ic->sd->obj);
	     sl = e_fm2_selected_list_get(ic->sd->obj);
	     for (l = sl, i = 0; l; l = l->next, i++)
	       {
		  const char *s;
		  ici = l->data;
		  snprintf(buf, sizeof(buf), "%s/%s", realpath, ici->file);

		  s = _e_fm2_uri_escape(buf);
		  if (!s) continue;
		  if (sel_length == 0)
		    {
		       sel_length = strlen(s) + 2;
		       sel = malloc(sel_length + 1);
		       if (!sel) break;
		       sel[0] = '\0';
		    }
		  else
		    {
		       sel_length += strlen(s) + 2;
		       sel = realloc(sel, sel_length+1);
		       if (!sel) break;
		    }
		  sel = strcat(sel, s);
		  sel = strcat(sel, "\r\n");
		  eina_stringshare_del(s);

		  ici->ic->drag.dnd = 1;
		  if (ici->ic->obj) evas_object_hide(ici->ic->obj);
		  if (ici->ic->obj_icon) evas_object_hide(ici->ic->obj_icon);
	       }
	     eina_list_free(sl);
	     if (!sel) return;
	     
	     d = e_drag_new(con,
			    x, y, drag_types, 1,
			    sel, strlen(sel), NULL, _e_fm2_cb_drag_finished);
	     o = edje_object_add(e_drag_evas_get(d));
	     if (_e_fm2_view_mode_get(ic->sd) == E_FM2_VIEW_MODE_LIST)
	       {
		  if (ic->sd->config->icon.fixed.w)
		    {
		       if (ic->odd)
			 _e_fm2_theme_edje_object_set(ic->sd, o,
						      "base/theme/widgets",
						      "list_odd/fixed");
		       else
			 _e_fm2_theme_edje_object_set(ic->sd, o,
						      "base/theme/widgets",
						      "list/fixed");
		    }
		  else
		    {
		       if (ic->odd)
			 _e_fm2_theme_edje_object_set(ic->sd, o,
						      "base/theme/widgets",
						      "list_odd/variable");
		       else
			 _e_fm2_theme_edje_object_set(ic->sd, o,
						      "base/theme/widgets",
						      "list/variable");
		    }
	       }
	     else
	       {
		  if (ic->sd->config->icon.fixed.w)
		    _e_fm2_theme_edje_object_set(ic->sd, o,
						 "base/theme/fileman",
						 "icon/fixed");
		  else
		    _e_fm2_theme_edje_object_set(ic->sd, o,
						 "base/theme/fileman",
						 "icon/variable");
	       }
	     _e_fm2_icon_label_set(ic, o);
	     o2 = _e_fm2_icon_icon_direct_set(ic, o,
					      _e_fm2_cb_icon_thumb_dnd_gen, o,
					      1);
	     edje_object_signal_emit(o, "e,state,selected", "e");
	     edje_object_signal_emit(o2, "e,state,selected", "e");
	     e_drag_object_set(d, o);
	     edje_object_signal_emit(o, "e,state,move", "e");
	     e_drag_resize(d, w, h);

	     e_drag_key_down_cb_set(d, _e_fm_drag_key_down_cb);
	     e_drag_key_up_cb_set(d, _e_fm_drag_key_up_cb);

	     e_drag_xdnd_start(d,
			       ic->drag.x + ic->x + ic->sd->x - ic->sd->pos.x,
			       ic->drag.y + ic->y + ic->sd->y - ic->sd->pos.y);
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
//   if (_e_fm2_view_mode_get(ic->sd) == E_FM2_VIEW_MODE_LIST)
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
//	if (_e_fm2_view_mode_get(ic->sd) == E_FM2_VIEW_MODE_LIST)
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

   if (evas_key_modifier_is_set(ev->modifiers, "Control"))
     {
	if (!strcmp(ev->keyname, "x"))
	  {
	     _e_fm2_file_cut(obj);
	     return;
	  }
	else if (!strcmp(ev->keyname, "c"))
	  {
	     _e_fm2_file_copy(obj);
	     return;
	  }
	else if (!strcmp(ev->keyname, "v"))
	  {
	     _e_fm2_file_paste(obj);
	     return;
	  }
     }

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
		       char buf[4096];
		       snprintf(buf, sizeof(buf), "%s/%s", ic->sd->path, ic->info.file);
		       e_fm2_path_set(ic->sd->obj, ic->sd->dev, buf);
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
	/* FIXME: typebuf extras */
	if (sd->typebuf_visible)
	  { /* typebuf mode: delete */ }
	else
	  _e_fm2_file_delete(obj);
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
   if (ev->button == 1)
     {
	Eina_List *l;
	int multi_sel = 0, range_sel = 0, sel_change = 0;
	
	if (sd->config->selection.windows_modifiers)
	  {
	     if (evas_key_modifier_is_set(ev->modifiers, "Shift"))
	       range_sel = 1;
	     else if (evas_key_modifier_is_set(ev->modifiers, "Control"))
	       multi_sel = 1;
	  }
	else
	  {
	     if (evas_key_modifier_is_set(ev->modifiers, "Control"))
	       range_sel = 1;
	     else if (evas_key_modifier_is_set(ev->modifiers, "Shift"))
	       multi_sel = 1;
	  }
	if (sd->config->selection.single)
	  {
	     multi_sel = 0;
	     range_sel = 0;
	  }
	if ((!multi_sel) && (!range_sel))
	  {
	     for (l = sd->icons; l; l = l->next)
	       {
		  E_Fm2_Icon *ic;
		  
		  ic = l->data;
		  if (ic->selected)
		    {
		       _e_fm2_icon_deselect(ic);
		       sel_change = 1;
		    }
	       }
	  }
	if (sel_change)
	  evas_object_smart_callback_call(sd->obj, "selection_change", NULL);

	if (!(ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD))
	  {
	     if (!sd->config->selection.single) 
	       {
		  sd->selrect.ox = ev->canvas.x;
		  sd->selrect.oy = ev->canvas.y;
		  sd->selecting = 1;
	       }
	  }
     }
   else if (ev->button == 3)
     {
	_e_fm2_menu(sd->obj, ev->timestamp);
     }
}
    
static void
_e_fm2_cb_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Fm2_Smart_Data *sd;
   
   sd = data;
   sd->selecting = 0;
   sd->selrect.ox = 0;
   sd->selrect.oy = 0;
   evas_object_hide(sd->sel_rect);
}

static void
_e_fm2_cb_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Move *ev;
   E_Fm2_Smart_Data *sd;
   Eina_List *l = NULL;
   int x, y, w, h;
   int sel_change = 0;

   sd = data;
   ev = event_info;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
     {
	if (sd->selecting)
	  {
	     sd->selecting = 0;
	     sd->selrect.ox = 0;
	     sd->selrect.oy = 0;
	     evas_object_hide(sd->sel_rect);
	  }
	return;
     }
   if (!sd->selecting) return;

   if (ev->cur.canvas.x < sd->selrect.ox) 
     {
	sd->selrect.x = ev->cur.canvas.x;
	sd->selrect.w = (sd->selrect.ox - sd->selrect.x);
     }
   else 
     {
	sd->selrect.x = MIN(sd->selrect.ox, ev->cur.canvas.x);
	sd->selrect.w = abs(sd->selrect.x - ev->cur.canvas.x);
     }
   if (ev->cur.canvas.y < sd->selrect.oy) 
     {
	sd->selrect.y = ev->cur.canvas.y;
	sd->selrect.h = (sd->selrect.oy - sd->selrect.y);
     }
   else 
     {
	sd->selrect.y = MIN(sd->selrect.oy, ev->cur.canvas.y);
	sd->selrect.h = abs(sd->selrect.y - ev->cur.canvas.y);
     }
   _e_fm2_sel_rect_update(sd);

   evas_object_geometry_get(sd->sel_rect, &x, &y, &w, &h);

/*
 * Leave commented for now. Start of scrolling the sel_rect
 * 
   int nx, ny, nw, nh;

   nx = sd->pos.x;
   if ((x - sd->pos.x) < 0)
     nx = x;
   else if ((x + w - sd->pos.x) > (sd->w))
     nx = x + w - sd->w;
   ny = sd->pos.y;
   if ((y - sd->pos.y) < 0)
     ny = y;
   else if ((y + h - sd->pos.y) > (sd->h))
     ny = y + h - sd->h;
   e_fm2_pan_set(sd->obj, nx, ny);
   evas_object_smart_callback_call(sd->obj, "pan_changed", NULL);
*/

   for (l = sd->icons; l; l = l->next)
     {
	E_Fm2_Icon *ic;
	int ix, iy, iw, ih;
	int ix_t, iy_t, iw_t, ih_t;
	
	ic = l->data;
	if (!ic) continue;
	evas_object_geometry_get(ic->obj_icon, &ix, &iy, &iw, &ih);
	evas_object_geometry_get(edje_object_part_object_get(ic->obj,
							     "e.text.label"),
				 &ix_t, &iy_t, &iw_t, &ih_t);
	if (E_INTERSECTS(x, y, w, h, ix, iy, iw, ih) ||
	    E_INTERSECTS(x, y, w, h, ix_t, iy_t, iw_t, ih_t))
	  {
	     if (!ic->selected)
	       {
		  _e_fm2_icon_select(ic);
		  sel_change = 1;
	       }
	  }
	else
	  {
	     if (ic->selected)
	       {
		  _e_fm2_icon_deselect(ic);
		  sel_change = 1;
	       }
	  }
     }
   if (sel_change)
     evas_object_smart_callback_call(sd->obj, "selection_change", NULL);
}

static void 
_e_fm2_sel_rect_update(void *data) 
{
   E_Fm2_Smart_Data *sd;
   
   sd = data;
   evas_object_move(sd->sel_rect, sd->selrect.x, sd->selrect.y);
   evas_object_resize(sd->sel_rect, sd->selrect.w, sd->selrect.h);
   evas_object_show(sd->sel_rect);
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
   _e_fm2_dir_save_props(sd);
}

static void
_e_fm2_cb_resize_job(void *data)
{
   E_Fm2_Smart_Data *sd;
   Eina_List *l;
   
   sd = evas_object_smart_data_get(data);
   if (!sd) return;
   sd->resize_job = NULL;
   evas_event_freeze(evas_object_evas_get(sd->obj));
   edje_freeze();
   switch (_e_fm2_view_mode_get(sd))
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
	if (sd->config->view.fit_custom_pos)
	  {
	     for (l = sd->icons; l; l = l->next)
	       {
		  E_Fm2_Icon *ic;
		  
		  ic = l->data;
		  ic->region = NULL;
		  _e_fm2_icon_geom_adjust(ic, ic->x, ic->y, ic->w, ic->h, sd->pw, sd->ph);
	       }
	  }
        _e_fm2_regions_free(sd->obj);
//	_e_fm2_regions_eval(sd->obj);
	_e_fm2_icons_place(sd->obj);
	_e_fm2_regions_populate(sd->obj);
	break;
      case E_FM2_VIEW_MODE_CUSTOM_GRID_ICONS:
	/* FIXME: not going to implement this at this stage */
        _e_fm2_regions_free(sd->obj);
//	_e_fm2_regions_eval(sd->obj);
	_e_fm2_icons_place(sd->obj);
	_e_fm2_regions_populate(sd->obj);
	break;
      case E_FM2_VIEW_MODE_CUSTOM_SMART_GRID_ICONS:
	/* FIXME: not going to implement this at this stage */
        _e_fm2_regions_free(sd->obj);
//	_e_fm2_regions_eval(sd->obj);
	_e_fm2_icons_place(sd->obj);
	_e_fm2_regions_populate(sd->obj);
	break;
      case E_FM2_VIEW_MODE_LIST:
	if (sd->iconlist_changed)
	  {
	     for (l = sd->icons; l; l = l->next)
	       {
		  E_Fm2_Icon *ic;
		  
		  ic = l->data;
		  ic->region = NULL;
//		  _e_fm2_icon_unrealize(ic);
	       }
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
   sd->pw = sd->w;
   sd->ph = sd->h;

   if ((sd->max.w > 0) && (sd->max.h > 0) && (sd->w > 0) && (sd->h > 0))
     {
	E_Fm2_Custom_File *cf = e_fm2_custom_file_get(sd->realpath);
	if ((cf) && (cf->dir))
	  {
	     sd->pos.x = cf->dir->pos.x * (sd->max.w - sd->w);
	     sd->pos.y = cf->dir->pos.y * (sd->max.h - sd->h);
	     evas_object_smart_callback_call(sd->obj, "pan_changed", NULL);
	  }
     }
}

static int
_e_fm2_cb_icon_sort(const void *data1, const void *data2)
{
   const E_Fm2_Icon *ic1, *ic2;
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
	
/*	if (ic1->sd->config->list.sort.category)
	  {
 * FIXME: implement category sorting
	  }
	else
*/	  {
	     ecore_strlcpy(buf1, l1, sizeof(buf1));
	     ecore_strlcpy(buf2, l2, sizeof(buf2));
	  }
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
_e_fm2_cb_scan_timer(void *data)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(data);
   if (!sd) return 0;
   _e_fm2_queue_process(data);
   sd->scan_timer = NULL;
   if (!sd->listing)
     {
	_e_fm2_client_monitor_list_end(data);
	return 0;
     }
   if (sd->busy_count > 0)
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
   if (!sd->listing)
     {
	sd->sort_idler = NULL;
        _e_fm2_client_monitor_list_end(data);
	return 0;
     }
   return 1;
}

/**************************/
static void
_e_fm2_obj_icons_place(E_Fm2_Smart_Data *sd)
{
   Eina_List *l, *ll;
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
		       if (!_e_fm2_icon_visible(ic))
			 {
			    e_thumb_icon_end(ic->obj_icon);
			 }
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

   sd->view_mode = -1; /* unset */
   sd->icon_size = -1; /* unset */
   
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
   _e_fm2_theme_edje_object_set(sd, sd->drop,
				"base/theme/fileman",
				"list/drop_between");
   evas_object_smart_member_add(sd->drop, obj);
   evas_object_show(sd->drop);
   
   sd->drop_in = edje_object_add(evas_object_evas_get(obj));
   evas_object_clip_set(sd->drop_in, sd->clip);
   _e_fm2_theme_edje_object_set(sd, sd->drop_in,
				"base/theme/fileman",
				"list/drop_in");
   evas_object_smart_member_add(sd->drop_in, obj);
   evas_object_show(sd->drop_in);
   
   sd->overlay = edje_object_add(evas_object_evas_get(obj));
   evas_object_clip_set(sd->overlay, sd->clip);
   _e_fm2_theme_edje_object_set(sd, sd->overlay,
				"base/theme/fileman",
				"overlay");
   evas_object_smart_member_add(sd->overlay, obj);
   evas_object_show(sd->overlay);

   sd->sel_rect = edje_object_add(evas_object_evas_get(obj));
   evas_object_clip_set(sd->sel_rect, sd->clip);
   _e_fm2_theme_edje_object_set(sd, sd->sel_rect, "base/theme/fileman",
				"rubberband");
   evas_object_smart_member_add(sd->sel_rect, obj);
   
   evas_object_smart_data_set(obj, sd);
   evas_object_move(obj, 0, 0);
   evas_object_resize(obj, 0, 0);
   _e_fm2_list = eina_list_append(_e_fm2_list, sd->obj);
}

static void
_e_fm2_smart_del(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   _e_fm2_client_monitor_list_end(obj);
   if (sd->realpath) _e_fm2_client_monitor_del(sd->id, sd->realpath);
   _e_fm2_live_process_end(obj);
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
   eina_stringshare_del(sd->custom_theme);
   eina_stringshare_del(sd->custom_theme_content);
   sd->custom_theme = sd->custom_theme_content = NULL;
   eina_stringshare_del(sd->dev);
   eina_stringshare_del(sd->path);
   eina_stringshare_del(sd->realpath);
   sd->dev = sd->path = sd->realpath = NULL;
   if (sd->mount)
     {
	e_fm2_hal_unmount(sd->mount);
	sd->mount = NULL;
     }
   if (sd->config) _e_fm2_config_free(sd->config);
   
   E_FREE(sd->typebuf.buf);

   evas_object_del(sd->underlay);
   evas_object_del(sd->overlay);
   evas_object_del(sd->drop);
   evas_object_del(sd->drop_in);
   evas_object_del(sd->sel_rect);
   evas_object_del(sd->clip);
   if (sd->drop_handler) e_drop_handler_del(sd->drop_handler);
   if (_e_fm2_list_walking == 0)
     _e_fm2_list = eina_list_remove(_e_fm2_list, sd->obj);
   else
     _e_fm2_list_remove = eina_list_append(_e_fm2_list_remove, sd->obj);
   free(sd);
   e_fm2_custom_file_flush();
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
   if ((_e_fm2_view_mode_get(sd) == E_FM2_VIEW_MODE_LIST) ||
       (_e_fm2_view_mode_get(sd) == E_FM2_VIEW_MODE_GRID_ICONS))
     {
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
     }
   else if (_e_fm2_view_mode_get(sd) == E_FM2_VIEW_MODE_CUSTOM_ICONS)
     {
	if (sd->config->view.fit_custom_pos)
	  {
	     if (sd->resize_job) ecore_job_del(sd->resize_job);
	     sd->resize_job = ecore_job_add(_e_fm2_cb_resize_job, obj);
	  }
	else
	  {
	     if (sd->scroll_job) ecore_job_del(sd->scroll_job);
	     sd->scroll_job = ecore_job_add(_e_fm2_cb_scroll_job, obj);
	  }
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
     sd->icon_menu.replace.func(sd->icon_menu.replace.data, sd->obj, mn, NULL);
   else
     {
	if (sd->icon_menu.start.func)
	  {
	     sd->icon_menu.start.func(sd->icon_menu.start.data, sd->obj, mn, NULL);
	     mi = e_menu_item_new(mn);
	     e_menu_item_separator_set(mi, 1);
	  }
	if (!(sd->icon_menu.flags & E_FM2_MENU_NO_INHERIT_PARENT))
	  {
	     mi = e_menu_item_new(mn);
	     e_menu_item_label_set(mi, _("Inherit parent settings"));
	     e_menu_item_icon_edje_set(mi,
				       e_theme_edje_file_get("base/theme/fileman",
							     "e/fileman/default/button/inherit"),
				       "e/fileman/default/button/inherit");
	     e_menu_item_check_set(mi, 1);
	     e_menu_item_toggle_set(mi, sd->inherited_dir_props);
	     e_menu_item_callback_set(mi, _e_fm2_toggle_inherit_dir_props, sd);
	  }
	if (!(sd->icon_menu.flags & E_FM2_MENU_NO_VIEW_MENU)) 
	  {
	     mi = e_menu_item_new(mn);
	     e_menu_item_label_set(mi, _("View Mode"));
	     e_util_menu_item_edje_icon_set(mi, "enlightenment/appearance");
	     e_menu_item_submenu_pre_callback_set(mi, _e_fm2_view_menu_pre, sd);
	  }
	if (!(sd->icon_menu.flags & E_FM2_MENU_NO_REFRESH))
	  {
	     mi = e_menu_item_new(mn);
	     e_menu_item_label_set(mi, _("Refresh View"));
	     e_menu_item_icon_edje_set(mi,
				       e_theme_edje_file_get("base/theme/fileman",
							     "e/fileman/default/button/refresh"),
				       "e/fileman/default/button/refresh");
	     e_menu_item_callback_set(mi, _e_fm2_refresh, sd);
	  }
	
	if (!(sd->icon_menu.flags & E_FM2_MENU_NO_SHOW_HIDDEN))
	  {
	     mi = e_menu_item_new(mn);
	     e_menu_item_label_set(mi, _("Show Hidden Files"));
	     e_menu_item_icon_edje_set(mi,
				       e_theme_edje_file_get("base/theme/fileman",
							     "e/fileman/default/button/hidden_files"),
				       "e/fileman/default/button/hidden_files");
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
								  "e/fileman/default/button/ordering"),
					    "e/fileman/default/button/ordering");
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
								  "e/fileman/default/button/ordering"),
					    "e/fileman/default/button/sort");
		  e_menu_item_callback_set(mi, _e_fm2_sort, sd);
	       }
	  }
	
	if (!(sd->icon_menu.flags & E_FM2_MENU_NO_NEW_DIRECTORY))
	  {
	     mi = e_menu_item_new(mn);
	     e_menu_item_separator_set(mi, 1);
	     
	     mi = e_menu_item_new(mn);
	     e_menu_item_label_set(mi, _("New Directory"));
	     e_menu_item_icon_edje_set(mi,
		   e_theme_edje_file_get("base/theme/fileman",
		      "e/fileman/default/button/new_dir"),
		   "e/fileman/default/button/new_dir");
	     e_menu_item_callback_set(mi, _e_fm2_new_directory, sd);
	  }

	if ((!(sd->icon_menu.flags & E_FM2_MENU_NO_PASTE)) && 
	    (eina_list_count(_e_fm_file_buffer) > 0))
	  {
	     if (ecore_file_can_write(sd->realpath))
	       {
		  mi = e_menu_item_new(mn);
		  e_menu_item_separator_set(mi, 1);
		  
		  mi = e_menu_item_new(mn);
		  e_menu_item_label_set(mi, _("Paste"));
		  e_menu_item_icon_edje_set(mi,
					    e_theme_edje_file_get("base/theme/fileman",
								  "e/fileman/default/button/paste"),
					    "e/fileman/default/button/paste");
		  e_menu_item_callback_set(mi, _e_fm2_file_paste_menu, sd);
	       }
	  }
	
	if (sd->icon_menu.end.func)
	  sd->icon_menu.end.func(sd->icon_menu.end.data, sd->obj, mn, NULL);
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
   Eina_List *sel, *l = NULL;
   int x, y, can_w, can_w2, protect;
   char buf[4096];
   
   sd = ic->sd;

   mn = e_menu_new();
   e_menu_category_set(mn, "e/fileman/action");

   if (sd->icon_menu.replace.func)
     sd->icon_menu.replace.func(sd->icon_menu.replace.data, sd->obj, mn, NULL);
   else
     {
	if (sd->icon_menu.start.func)
	  {
	     sd->icon_menu.start.func(sd->icon_menu.start.data, sd->obj, mn, NULL);
	     mi = e_menu_item_new(mn);
	     e_menu_item_separator_set(mi, 1);
	  }

	if (!(sd->icon_menu.flags & E_FM2_MENU_NO_INHERIT_PARENT))
	  {
	     mi = e_menu_item_new(mn);
	     e_menu_item_label_set(mi, _("Inherit parent settings"));
	     e_menu_item_icon_edje_set(mi,
				       e_theme_edje_file_get("base/theme/fileman",
							     "e/fileman/default/button/inherit"),
				       "e/fileman/default/button/inherit");
	     e_menu_item_check_set(mi, 1);
	     e_menu_item_toggle_set(mi, sd->inherited_dir_props);
	     e_menu_item_callback_set(mi, _e_fm2_toggle_inherit_dir_props, sd);
	  }
	if (!(sd->icon_menu.flags & E_FM2_MENU_NO_VIEW_MENU)) 
	  {
	     mi = e_menu_item_new(mn);
	     e_menu_item_label_set(mi, _("View Mode"));
	     e_util_menu_item_edje_icon_set(mi, "enlightenment/appearance");
	     e_menu_item_submenu_pre_callback_set(mi, _e_fm2_view_menu_pre, sd);
	  }
	if (!(sd->icon_menu.flags & E_FM2_MENU_NO_REFRESH))
	  {
	     mi = e_menu_item_new(mn);
	     e_menu_item_label_set(mi, _("Refresh View"));
	     e_menu_item_icon_edje_set(mi,
				       e_theme_edje_file_get("base/theme/fileman",
							     "e/fileman/default/button/refresh"),
				       "e/fileman/default/button/refresh");
	     e_menu_item_callback_set(mi, _e_fm2_refresh, sd);
	  }
	
	if (!(sd->icon_menu.flags & E_FM2_MENU_NO_SHOW_HIDDEN))
	  {
	     mi = e_menu_item_new(mn);
	     e_menu_item_label_set(mi, _("Show Hidden Files"));
	     e_menu_item_icon_edje_set(mi,
				       e_theme_edje_file_get("base/theme/fileman",
							     "e/fileman/default/button/hidden_files"),
				       "e/fileman/default/button/hidden_files");
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
								  "e/fileman/default/button/ordering"),
					    "e/fileman/default/button/ordering");
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
								  "e/fileman/default/button/ordering"),
					    "e/fileman/default/button/sort");
		  e_menu_item_callback_set(mi, _e_fm2_sort, sd);
	       }
	  }
	
	if (!(sd->icon_menu.flags & E_FM2_MENU_NO_NEW_DIRECTORY))
	  {
	     /* FIXME: stat the dir itself - move to e_fm_main */
	     if (ecore_file_can_write(sd->realpath))
	       {
		  mi = e_menu_item_new(mn);
		  e_menu_item_separator_set(mi, 1);
		  
		  mi = e_menu_item_new(mn);
		  e_menu_item_label_set(mi, _("New Directory"));
		  e_menu_item_icon_edje_set(mi,
					    e_theme_edje_file_get("base/theme/fileman",
								  "e/fileman/default/button/new_dir"),
					    "e/fileman/default/button/new_dir");
		  e_menu_item_callback_set(mi, _e_fm2_new_directory, sd);
	       }
	  }
	if (!(sd->icon_menu.flags & E_FM2_MENU_NO_CUT)) 
	  {
	     if (ecore_file_can_write(sd->realpath))
	       {
		  mi = e_menu_item_new(mn);
		  e_menu_item_separator_set(mi, 1);
		  
		  mi = e_menu_item_new(mn);
		  e_menu_item_label_set(mi, _("Cut"));
		  e_menu_item_icon_edje_set(mi,
					    e_theme_edje_file_get("base/theme/fileman",
								  "e/fileman/default/button/cut"),
					    "e/fileman/default/button/cut");
		  e_menu_item_callback_set(mi, _e_fm2_file_cut_menu, sd);
	       }
	  }
	if (!(sd->icon_menu.flags & E_FM2_MENU_NO_COPY)) 
	  {
	     if (!ecore_file_can_write(sd->realpath))
	       {
		  mi = e_menu_item_new(mn);
		  e_menu_item_separator_set(mi, 1);
	       }
	     
	     mi = e_menu_item_new(mn);
	     e_menu_item_label_set(mi, _("Copy"));
	     e_menu_item_icon_edje_set(mi,
				       e_theme_edje_file_get("base/theme/fileman",
							     "e/fileman/default/button/copy"),
				       "e/fileman/default/button/copy");
	     e_menu_item_callback_set(mi, _e_fm2_file_copy_menu, sd);
	  }
	
	if ((!(sd->icon_menu.flags & E_FM2_MENU_NO_PASTE)) && 
	    (eina_list_count(_e_fm_file_buffer) > 0))
	  {
	     if (ecore_file_can_write(sd->realpath))
	       {
		  mi = e_menu_item_new(mn);
		  e_menu_item_label_set(mi, _("Paste"));
		  e_menu_item_icon_edje_set(mi,
					    e_theme_edje_file_get("base/theme/fileman",
								  "e/fileman/default/button/paste"),
					    "e/fileman/default/button/paste");
		  e_menu_item_callback_set(mi, _e_fm2_file_paste_menu, sd);
	       }
	  }
	
	can_w = 0;
	can_w2 = 1;
	if (ic->sd->order_file)
	  {
	     snprintf(buf, sizeof(buf), "%s/.order", sd->realpath);
	     /* FIXME: stat the .order itself - move to e_fm_main */
//	     can_w2 = ecore_file_can_write(buf);
	  }
	snprintf(buf, sizeof(buf), "%s/%s", sd->realpath, ic->info.file);
	if (ic->info.link)
	  {
	     can_w = 1;
/*	     struct stat st;
	     
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
*/	  }
	else
	  can_w = 1;
	
	sel = e_fm2_selected_list_get(ic->sd->obj);
	if ((!sel) || eina_list_count(sel) == 1)
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
								  "e/fileman/default/button/delete"),
					    "e/fileman/default/button/delete");
		  e_menu_item_callback_set(mi, _e_fm2_file_delete_menu, ic);
	       }
	     
	     if (!(sd->icon_menu.flags & E_FM2_MENU_NO_RENAME))
	       {
		  mi = e_menu_item_new(mn);
		  e_menu_item_label_set(mi, _("Rename"));
		  e_menu_item_icon_edje_set(mi,
					    e_theme_edje_file_get("base/theme/fileman",
								  "e/fileman/default/button/rename"),
					    "e/fileman/default/button/rename");
		  e_menu_item_callback_set(mi, _e_fm2_file_rename, ic);
	       }
	  }
	
	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, _("Properties"));
	e_menu_item_icon_edje_set(mi,
				  e_theme_edje_file_get("base/theme/fileman",
							"e/fileman/default/button/properties"),
				  "e/fileman/default/button/properties");
	e_menu_item_callback_set(mi, _e_fm2_file_properties, ic);

	if (ic->info.mime) 
	  {
	     /* see if we have any mime handlers registered for this file */
	     l = e_fm2_mime_handler_mime_handlers_get(ic->info.mime);
	     if (l) 
	       {
		  snprintf(buf, sizeof(buf), "%s/%s", sd->realpath, ic->info.file);
		  _e_fm2_context_menu_append(obj, buf, l, mn, ic);
	       }
	  }

	/* see if we have any glob handlers registered for this file */
	snprintf(buf, sizeof(buf), "*%s", strrchr(ic->info.file, '.'));
	l = e_fm2_mime_handler_glob_handlers_get(buf);
	if (l) 
	  {
	     snprintf(buf, sizeof(buf), "%s/%s", sd->realpath, ic->info.file);
	     _e_fm2_context_menu_append(obj, buf, l, mn, ic);
	     eina_list_free(l);
	  }
	
	if (sd->icon_menu.end.func)
	  sd->icon_menu.end.func(sd->icon_menu.end.data, sd->obj, mn, &(ic->info));
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

static inline void 
_e_fm2_context_menu_append(Evas_Object *obj, const char *path, Eina_List *l, E_Menu *mn, E_Fm2_Icon *ic) 
{
   Eina_List *ll = NULL;

   if (!l) return;

   l = eina_list_sort(l, -1, _e_fm2_context_list_sort);

   for (ll = l; ll; ll = ll->next) 
     {
	E_Fm2_Mime_Handler *handler = NULL;
	E_Fm2_Context_Menu_Data *md = NULL;
	E_Menu_Item *mi;

	handler = ll->data;
	if ((!handler) || (!e_fm2_mime_handler_test(handler, obj, path)) || 
	    (!handler->label)) continue;
	if (ll == l)
	  {
	     /* only append the separator if this is the first item */
	     /* we do this in here because we dont want to add a separator
	      * when we have no context entries */
	     mi = e_menu_item_new(mn);
	     e_menu_item_separator_set(mi, 1);
	  }

	md = E_NEW(E_Fm2_Context_Menu_Data, 1);
	if (!md) continue;
	md->icon = ic;
	md->handler = handler;
	_e_fm2_menu_contexts = eina_list_append(_e_fm2_menu_contexts, md);

	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, handler->label);
	if (handler->icon_group)
	  e_util_menu_item_edje_icon_set(mi, handler->icon_group);
	e_menu_item_callback_set(mi, _e_fm2_icon_menu_item_cb, md);
     }
}

static int 
_e_fm2_context_list_sort(const void *data1, const void *data2) 
{
   const E_Fm2_Mime_Handler *d1, *d2;
   
   if (!data1) return 1;
   if (!data2) return -1;
   d1 = data1;
   if (!d1->label) return 1;
   d2 = data2;
   if (!d2->label) return -1;
   return (strcmp(d1->label, d2->label));
}

static void
_e_fm2_icon_menu_post_cb(void *data, E_Menu *m)
{
   E_Fm2_Icon *ic;
   
   ic = data;
   ic->menu = NULL;
   while (_e_fm2_menu_contexts) 
     {
	E_Fm2_Context_Menu_Data *md;
	
	md = _e_fm2_menu_contexts->data;
	_e_fm2_menu_contexts = eina_list_remove_list(_e_fm2_menu_contexts, 
						     _e_fm2_menu_contexts);
	E_FREE(md);
     }
}

static void 
_e_fm2_icon_menu_item_cb(void *data, E_Menu *m, E_Menu_Item *mi) 
{
   E_Fm2_Context_Menu_Data *md = NULL;
   Evas_Object *obj = NULL;
   char buf[4096];
   
   md = data;
   if (!md) return;
   obj = md->icon->info.fm;
   if (!obj) return;
   snprintf(buf, sizeof(buf), "%s/%s", e_fm2_real_path_get(obj), md->icon->info.file);
   e_fm2_mime_handler_call(md->handler, obj, buf);
}

struct e_fm2_view_menu_icon_size_data
{
   E_Fm2_Smart_Data *sd;
   short size;
};

static void
_e_fm2_view_menu_icon_size_data_free(void *obj)
{
   struct e_fm2_view_menu_icon_size_data *d = e_object_data_get(obj);
   free(d);
}

static void
_e_fm2_view_menu_icon_size_change(void *data, E_Menu *m, E_Menu_Item *mi)
{
   struct e_fm2_view_menu_icon_size_data *d = data;
   short current_size = _e_fm2_icon_w_get(d->sd);
   d->sd->icon_size = d->size;
   d->sd->inherited_dir_props = 0;
   if (current_size == d->size)
     return;
   _e_fm2_refresh(d->sd, m, mi);
}

static void
_e_fm2_view_menu_icon_size_use_default(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Fm2_Smart_Data *sd = data;
   short old, new;

   old = _e_fm2_icon_w_get(sd);

   if (sd->icon_size == -1)
     sd->icon_size = sd->config->icon.icon.w;
   else
     sd->icon_size = -1;

   new = _e_fm2_icon_w_get(sd);
   sd->inherited_dir_props = 0;

   if (new == old)
     return;

   _e_fm2_refresh(sd, m, mi);
}

static void
_e_fm2_view_menu_icon_size_pre(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Fm2_Smart_Data *sd = data;
   E_Menu *subm;
   const short *itr, sizes[] = {
     22, 32, 48, 64, 96, 128, 256, -1
   };
   short current_size = _e_fm2_icon_w_get(sd);

   if (e_scale > 0.0)
     current_size /= e_scale;

   subm = e_menu_new();
   e_menu_item_submenu_set(mi, subm);

   for (itr = sizes; *itr > -1; itr++)
     {
	char buf[32];
	struct e_fm2_view_menu_icon_size_data *d;

	d = malloc(sizeof(*d));
	if (!d)
	  continue;
	d->sd = sd;
	d->size = *itr;

	snprintf(buf, sizeof(buf), "%hd", *itr);

	mi = e_menu_item_new(subm);
	e_object_data_set(E_OBJECT(mi), d);
	e_object_del_attach_func_set
	  (E_OBJECT(mi), _e_fm2_view_menu_icon_size_data_free);

	e_menu_item_label_set(mi, buf);
	e_menu_item_radio_group_set(mi, 1);
	e_menu_item_radio_set(mi, 1);

	if (current_size == *itr)
	  e_menu_item_toggle_set(mi, 1);

	e_menu_item_callback_set(mi, _e_fm2_view_menu_icon_size_change, d);
     }

   mi = e_menu_item_new(subm);
   e_menu_item_separator_set(mi, 1);

   mi = e_menu_item_new(subm);
   e_menu_item_label_set(mi, _("Use default"));
   e_menu_item_check_set(mi, 1);
   e_menu_item_toggle_set(mi, sd->icon_size == -1);
   e_menu_item_callback_set(mi, _e_fm2_view_menu_icon_size_use_default, sd);
}

static void
_e_fm2_toggle_inherit_dir_props(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Fm2_Smart_Data *sd = data;

   sd->inherited_dir_props = !sd->inherited_dir_props;
   _e_fm2_dir_save_props(sd);
   _e_fm2_dir_load_props(sd);
   _e_fm2_refresh(sd, m, mi);
}

static void 
_e_fm2_view_menu_pre(void *data, E_Menu *m, E_Menu_Item *mi) 
{
   E_Menu *subm;
   E_Fm2_Smart_Data *sd;
   E_Fm2_Config *cfg;
   char view_mode;

   sd = data;
   cfg = e_fm2_config_get(sd->obj);

   subm = e_menu_new();
   e_object_data_set(E_OBJECT(subm), sd);
   e_menu_item_submenu_set(mi, subm);

   view_mode = _e_fm2_view_mode_get(sd);

   mi = e_menu_item_new(subm);
   e_menu_item_label_set(mi, _("Grid Icons"));
   e_menu_item_radio_group_set(mi, 1);
   e_menu_item_radio_set(mi, 1);
   if (view_mode == E_FM2_VIEW_MODE_GRID_ICONS)
     e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _e_fm2_view_menu_grid_icons_cb, sd);

   mi = e_menu_item_new(subm);
   e_menu_item_label_set(mi, _("Custom Icons"));
   e_menu_item_radio_group_set(mi, 1);
   e_menu_item_radio_set(mi, 1);
   if (view_mode == E_FM2_VIEW_MODE_CUSTOM_ICONS)
     e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _e_fm2_view_menu_custom_icons_cb, sd);

   mi = e_menu_item_new(subm);
   e_menu_item_label_set(mi, _("List"));
   e_menu_item_radio_group_set(mi, 1);
   e_menu_item_radio_set(mi, 1);
   if (view_mode == E_FM2_VIEW_MODE_LIST)
     e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _e_fm2_view_menu_list_cb, sd);

   mi = e_menu_item_new(subm);
   e_menu_item_separator_set(mi, 1);

   mi = e_menu_item_new(subm);
   e_menu_item_label_set(mi, _("Use default"));
   e_menu_item_check_set(mi, 1);
   e_menu_item_toggle_set(mi, sd->view_mode == -1);
   e_menu_item_callback_set(mi, _e_fm2_view_menu_use_default_cb, sd);

   if (view_mode == E_FM2_VIEW_MODE_LIST)
     return;

   char buf[64];
   int icon_size = _e_fm2_icon_w_get(sd);

   if (e_scale > 0.0)
     icon_size /= e_scale;

   snprintf(buf, sizeof(buf), _("Icon Size (%d)"), icon_size);

   mi = e_menu_item_new(subm);
   e_menu_item_label_set(mi, buf);
   e_menu_item_submenu_pre_callback_set(mi, _e_fm2_view_menu_icon_size_pre, sd);
}

static void
_e_fm2_view_menu_grid_icons_cb(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Fm2_Smart_Data *sd = data;
   char old;

   old = _e_fm2_view_mode_get(sd);
   sd->view_mode = E_FM2_VIEW_MODE_GRID_ICONS;
   sd->inherited_dir_props = 0;
   if (old == E_FM2_VIEW_MODE_GRID_ICONS)
     return;

   _e_fm2_refresh(sd, m, mi);
}

static void
_e_fm2_view_menu_custom_icons_cb(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Fm2_Smart_Data *sd = data;
   char old;

   old = _e_fm2_view_mode_get(sd);
   sd->view_mode = E_FM2_VIEW_MODE_CUSTOM_ICONS;
   sd->inherited_dir_props = 0;
   if (old == E_FM2_VIEW_MODE_CUSTOM_ICONS)
     return;

   _e_fm2_refresh(sd, m, mi);
}

static void
_e_fm2_view_menu_list_cb(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Fm2_Smart_Data *sd = data;
   char old;

   old = _e_fm2_view_mode_get(sd);
   sd->view_mode = E_FM2_VIEW_MODE_LIST;
   sd->inherited_dir_props = 0;
   if (old == E_FM2_VIEW_MODE_LIST)
     return;

   _e_fm2_refresh(sd, m, mi);
}

static void
_e_fm2_view_menu_use_default_cb(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Fm2_Smart_Data *sd = data;
   char old, new;

   old = _e_fm2_view_mode_get(sd);

   if (sd->view_mode == -1)
     sd->view_mode = sd->config->view.mode;
   else
     sd->view_mode = -1;

   new = _e_fm2_view_mode_get(sd);
   sd->inherited_dir_props = 0;

   if (new == old)
     return;

   _e_fm2_refresh(sd, m, mi);
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

   sd->inherited_dir_props = 0;
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
	/* FIXME: move to e_fm_main */
	ecore_file_unlink(buf);
     }
   else
     {
	FILE *f;
	
	snprintf(buf, sizeof(buf), "%s/.order", sd->realpath);
	f = fopen(buf, "w");
	if (f) fclose(f);
     }
   sd->inherited_dir_props = 0;
   _e_fm2_refresh(data, m, mi);
}

static void
_e_fm2_sort(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Fm2_Smart_Data *sd;
   
   sd = data;
   sd->icons = eina_list_sort(sd->icons, eina_list_count(sd->icons),
			      _e_fm2_cb_icon_sort);
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
   char buf[PATH_MAX];
   
   sd = data;
   sd->entry_dialog = NULL;
   if ((text) && (text[0]))
     {
	snprintf(buf, sizeof(buf), "%s/%s", sd->realpath, text);

	_e_fm2_client_file_mkdir(buf, "", 0, 0, 0, sd->w, sd->h);
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
   char oldpath[PATH_MAX];
   char newpath[PATH_MAX];
   char *args = NULL;
   size_t size = 0;
   size_t length = 0;
   
   ic = data;
   ic->entry_dialog = NULL;
   if ((text) && (strcmp(text, ic->info.file)))
     {
	snprintf(oldpath, sizeof(oldpath), "%s/%s", ic->sd->realpath, ic->info.file);
	snprintf(newpath, sizeof(newpath), "%s/%s", ic->sd->realpath, text);
	if (e_filereg_file_protected(oldpath)) return;

	args = _e_fm_string_append_quoted(args, &size, &length, oldpath);
	args = _e_fm_string_append_char(args, &size, &length, ' ');
	args = _e_fm_string_append_quoted(args, &size, &length, newpath);

	_e_fm_client_file_move(args);
	free(args);
     }
}

static void
_e_fm2_file_rename_no_cb(void *data)
{
   E_Fm2_Icon *ic;
   
   ic = data;
   ic->entry_dialog = NULL;
}

static void _e_fm_retry_abort_dialog(int pid, const char *str)
{
   E_Manager *man;
   E_Container *con;
   E_Dialog *dialog;
   int *id;
   char text[4096 + PATH_MAX];
   
   man = e_manager_current_get();
   if (!man) return;
   con = e_container_current_get(man);
   if (!con) return;
   
   id = malloc(sizeof(int));
   *id = pid;
   
   dialog = e_dialog_new(con, "E", "_fm_overwrite_dialog");
   E_OBJECT(dialog)->data = id;
   e_object_del_attach_func_set(E_OBJECT(dialog), _e_fm_retry_abort_delete_cb);
   e_dialog_button_add(dialog, _("Retry"), NULL, _e_fm_retry_abort_retry_cb, NULL);
   e_dialog_button_add(dialog, _("Abort"), NULL, _e_fm_retry_abort_abort_cb, NULL);

   e_dialog_button_focus_num(dialog, 0);
   e_dialog_title_set(dialog, _("Error"));
   snprintf(text, sizeof(text), 
	 _("%s"),
	 str);
   
   e_dialog_text_set(dialog, text);
   e_win_centered_set(dialog->win, 1);
   e_dialog_show(dialog);

}

static void _e_fm_retry_abort_delete_cb(void *obj)
{
   int *id = E_OBJECT(obj)->data;
   free(id);
}

static void _e_fm_retry_abort_retry_cb(void *data, E_Dialog *dialog)
{
   int *id = E_OBJECT(dialog)->data;
   _e_fm_client_send(E_FM_OP_ERROR_RESPONSE_RETRY, *id, NULL, 0);
   e_object_del(E_OBJECT(dialog));
}

static void _e_fm_retry_abort_abort_cb(void *data, E_Dialog *dialog)
{
   int *id = E_OBJECT(dialog)->data;
   _e_fm_client_send(E_FM_OP_ERROR_RESPONSE_ABORT, *id, NULL, 0);
   e_object_del(E_OBJECT(dialog));
}

static void
_e_fm_overwrite_dialog(int pid, const char *str)
{
   E_Manager *man;
   E_Container *con;
   E_Dialog *dialog;
   int *id;
   char text[4096 + PATH_MAX];
   
   man = e_manager_current_get();
   if (!man) return;
   con = e_container_current_get(man);
   if (!con) return;
   
   id = malloc(sizeof(int));
   *id = pid;
   
   dialog = e_dialog_new(con, "E", "_fm_overwrite_dialog");
   E_OBJECT(dialog)->data = id;
   e_object_del_attach_func_set(E_OBJECT(dialog), _e_fm_overwrite_delete_cb);
   e_dialog_button_add(dialog, _("No"), NULL, _e_fm_overwrite_no_cb, NULL);
   e_dialog_button_add(dialog, _("No to all"), NULL, _e_fm_overwrite_no_all_cb, NULL);
   e_dialog_button_add(dialog, _("Yes"), NULL, _e_fm_overwrite_yes_cb, NULL);
   e_dialog_button_add(dialog, _("Yes to all"), NULL, _e_fm_overwrite_yes_all_cb, NULL);

   e_dialog_button_focus_num(dialog, 0);
   e_dialog_title_set(dialog, _("Error"));
   snprintf(text, sizeof(text), 
	 _("%s"),
	 str);
   
   e_dialog_text_set(dialog, text);
   e_win_centered_set(dialog->win, 1);
   e_dialog_show(dialog);
}

static void
_e_fm_overwrite_delete_cb(void *obj)
{
   int *id = E_OBJECT(obj)->data;
   free(id);
}

static void
_e_fm_overwrite_no_cb(void *data, E_Dialog *dialog)
{
   int *id = E_OBJECT(dialog)->data;
   _e_fm_client_send(E_FM_OP_OVERWRITE_RESPONSE_NO, *id, NULL, 0);
   e_object_del(E_OBJECT(dialog));
}

static void
_e_fm_overwrite_no_all_cb(void *data, E_Dialog *dialog)
{
   int *id = E_OBJECT(dialog)->data;
   _e_fm_client_send(E_FM_OP_OVERWRITE_RESPONSE_NO_ALL, *id, NULL, 0);
   e_object_del(E_OBJECT(dialog));
}

static void
_e_fm_overwrite_yes_cb(void *data, E_Dialog *dialog)
{
   int *id = E_OBJECT(dialog)->data;
   _e_fm_client_send(E_FM_OP_OVERWRITE_RESPONSE_YES, *id, NULL, 0);
   e_object_del(E_OBJECT(dialog));
}

static void
_e_fm_overwrite_yes_all_cb(void *data, E_Dialog *dialog)
{
   int *id = E_OBJECT(dialog)->data;
   _e_fm_client_send(E_FM_OP_OVERWRITE_RESPONSE_YES_ALL, *id, NULL, 0);
   e_object_del(E_OBJECT(dialog));
}

static void
_e_fm_error_dialog(int pid, const char *str)
{
   E_Manager *man;
   E_Container *con;
   E_Dialog *dialog;
   int *id;
   char text[4096 + PATH_MAX];
   
   man = e_manager_current_get();
   if (!man) return;
   con = e_container_current_get(man);
   if (!con) return;
   
   id = malloc(sizeof(int));
   *id = pid;
   
   dialog = e_dialog_new(con, "E", "_fm_error_dialog");
   E_OBJECT(dialog)->data = id;
   e_object_del_attach_func_set(E_OBJECT(dialog), _e_fm_error_delete_cb);
   e_dialog_button_add(dialog, _("Retry"), NULL, _e_fm_error_retry_cb, NULL);
   e_dialog_button_add(dialog, _("Abort"), NULL, _e_fm_error_abort_cb, NULL);
   e_dialog_button_add(dialog, _("Ignore this"), NULL, _e_fm_error_ignore_this_cb, NULL);
   e_dialog_button_add(dialog, _("Ignore all"), NULL, _e_fm_error_ignore_all_cb, NULL);

   e_dialog_button_focus_num(dialog, 0);
   e_dialog_title_set(dialog, _("Error"));
   snprintf(text, sizeof(text), 
	 _("An error occured while performing an operation.<br>"
	    "%s"),
	 str);
   
   e_dialog_text_set(dialog, text);
   e_win_centered_set(dialog->win, 1);
   e_dialog_show(dialog);
}

static void
_e_fm_error_delete_cb(void *obj)
{
   int *id = E_OBJECT(obj)->data;
   free(id);
}

static void
_e_fm_error_retry_cb(void *data, E_Dialog *dialog)
{
   int *id = E_OBJECT(dialog)->data;
   _e_fm_client_send(E_FM_OP_ERROR_RESPONSE_RETRY, *id, NULL, 0);
   e_object_del(E_OBJECT(dialog));
}

static void
_e_fm_error_abort_cb(void *data, E_Dialog *dialog)
{
   int *id = E_OBJECT(dialog)->data;
   _e_fm_client_send(E_FM_OP_ERROR_RESPONSE_ABORT, *id, NULL, 0);
   e_object_del(E_OBJECT(dialog));
}

static void
_e_fm_error_ignore_this_cb(void *data, E_Dialog *dialog)
{
   int *id = E_OBJECT(dialog)->data;
   _e_fm_client_send(E_FM_OP_ERROR_RESPONSE_IGNORE_THIS, *id, NULL, 0);
   e_object_del(E_OBJECT(dialog));
}

static void
_e_fm_error_ignore_all_cb(void *data, E_Dialog *dialog)
{
   int *id = E_OBJECT(dialog)->data;
   _e_fm_client_send(E_FM_OP_ERROR_RESPONSE_IGNORE_ALL, *id, NULL, 0);
   e_object_del(E_OBJECT(dialog));
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

   if (ic->prop_dialog) e_object_del(E_OBJECT(ic->prop_dialog));
   ic->prop_dialog = e_fm_prop_file(con, ic);
   E_OBJECT(ic->prop_dialog)->data = ic;
   e_object_del_attach_func_set(E_OBJECT(ic->prop_dialog), _e_fm2_file_properties_delete_cb);
}

static void
_e_fm2_file_properties_delete_cb(void *obj)
{
   E_Fm2_Icon *ic;
   
   ic = E_OBJECT(obj)->data;
   ic->prop_dialog = NULL;
}

static void
_e_fm2_file_delete_menu(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   E_Fm2_Icon *ic = data;
   if (!ic) return;
   _e_fm2_file_delete(ic->obj);
}

static void
_e_fm2_file_delete(Evas_Object *obj)
{
   E_Manager *man;
   E_Container *con;
   E_Dialog *dialog;
   E_Fm2_Icon *ic;
   char text[4096 + 256];
   Eina_List *sel;

   man = e_manager_current_get();
   if (!man) return;
   con = e_container_current_get(man);
   if (!con) return;
   ic = _e_fm2_icon_first_selected_find(obj);
   if (!ic) return;
   if (ic->dialog) return;
   dialog = e_dialog_new(con, "E", "_fm_file_delete_dialog");
   ic->dialog = dialog;
   E_OBJECT(dialog)->data = ic;
   e_object_del_attach_func_set(E_OBJECT(dialog), _e_fm2_file_delete_delete_cb);
   e_dialog_button_add(dialog, _("Yes"), NULL, _e_fm2_file_delete_yes_cb, ic);
   e_dialog_button_add(dialog, _("No"), NULL, _e_fm2_file_delete_no_cb, ic);
   e_dialog_button_focus_num(dialog, 1);
   e_dialog_title_set(dialog, _("Confirm Delete"));
   sel = e_fm2_selected_list_get(obj);
   if ((!sel) || (eina_list_count(sel) == 1))
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
		 eina_list_count(sel),
		 ic->sd->realpath);
     }
   if (sel) eina_list_free(sel);
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

static char *
_e_fm_string_append_char(char *str, size_t *size, size_t *len, char c)
{
   if (str == NULL)
     {
	str = malloc(4096);
	str[0] = '\x00';
	*size = 4096;
	*len = 0;
     }

   if (*len >= *size - 1)
     {
	*size += 1024;
	str = realloc(str, *size);
     }

   str[(*len)++] = c;
   str[*len] = '\x00';

   return str;
}


static char *
_e_fm_string_append_quoted(char *str, size_t *size, size_t *len, const char *src)
{
   str = _e_fm_string_append_char(str, size, len, '\'');

   while (*src)
     {
	if (*src == '\'')
	  {
	     str = _e_fm_string_append_char(str, size, len, '\'');
	     str = _e_fm_string_append_char(str, size, len, '\\');
	     str = _e_fm_string_append_char(str, size, len, '\'');
	     str = _e_fm_string_append_char(str, size, len, '\'');
	  }
	else
	  str = _e_fm_string_append_char(str, size, len, *src);

	src++;
     }

   str = _e_fm_string_append_char(str, size, len, '\'');

   return str;
}

static void
_e_fm2_file_delete_yes_cb(void *data, E_Dialog *dialog)
{
   E_Fm2_Icon *ic;
   char buf[PATH_MAX];
   char *files = NULL;
   size_t size = 0;
   size_t len = 0;
   Eina_List *sel, *l;
   E_Fm2_Icon_Info *ici;
   
   ic = data;
   ic->dialog = NULL;
   
   e_object_del(E_OBJECT(dialog));
   sel = e_fm2_selected_list_get(ic->sd->obj);
   if (sel && (eina_list_count(sel) != 1))
     {
	for (l = sel; l; l = l->next)
	  {
	     ici = l->data;
	     snprintf(buf, sizeof(buf), "%s/%s", ic->sd->realpath, ici->file);
	     if (e_filereg_file_protected(buf)) continue;

	     files = _e_fm_string_append_quoted(files, &size, &len, buf);
	     if (l->next) files = _e_fm_string_append_char(files, &size, &len, ' ');
	  }

	eina_list_free(sel);
     }
   else
     {
	snprintf(buf, sizeof(buf), "%s/%s", ic->sd->realpath, ic->info.file);
	if (e_filereg_file_protected(buf)) return;
	files = _e_fm_string_append_quoted(files, &size, &len, buf);
     }

   _e_fm_client_file_del(files);

   free(files);
   
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
_e_fm2_live_file_add(Evas_Object *obj, const char *file, const char *file_rel, int after, E_Fm2_Finfo *finf)
{
   E_Fm2_Smart_Data *sd;
   E_Fm2_Action *a;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   a = E_NEW(E_Fm2_Action, 1);
   if (!a) return;
   sd->live.actions = eina_list_append(sd->live.actions, a);
   a->type = FILE_ADD;
   a->file = eina_stringshare_add(file);
   a->file2 = eina_stringshare_add(file_rel);
   a->flags = after;
   if (finf) memcpy(&(a->finf), finf, sizeof(E_Fm2_Finfo));
   a->finf.lnk = eina_stringshare_add(a->finf.lnk);
   a->finf.rlnk = eina_stringshare_add(a->finf.rlnk);
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
   sd->live.actions = eina_list_append(sd->live.actions, a);
   a->type = FILE_DEL;
   a->file = eina_stringshare_add(file);
   _e_fm2_live_process_begin(obj);
}

static void
_e_fm2_live_file_changed(Evas_Object *obj, const char *file, E_Fm2_Finfo *finf)
{
   E_Fm2_Smart_Data *sd;
   E_Fm2_Action *a;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   a = E_NEW(E_Fm2_Action, 1);
   if (!a) return;
   sd->live.actions = eina_list_append(sd->live.actions, a);
   a->type = FILE_CHANGE;
   a->file = eina_stringshare_add(file);
   if (finf) memcpy(&(a->finf), finf, sizeof(E_Fm2_Finfo));
   a->finf.lnk = eina_stringshare_add(a->finf.lnk);
   a->finf.rlnk = eina_stringshare_add(a->finf.rlnk);
   _e_fm2_live_process_begin(obj);
}

static void
_e_fm2_live_process_begin(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd->live.actions) return;
   if ((sd->live.idler) || (sd->live.timer) ||
       (sd->listing) || (sd->scan_timer)) return;
   sd->live.idler = ecore_idler_add(_e_fm2_cb_live_idler, obj);
   sd->live.timer = ecore_timer_add(0.2, _e_fm2_cb_live_timer, obj);
   sd->tmp.last_insert = NULL;
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
	sd->live.actions = eina_list_remove_list(sd->live.actions, sd->live.actions);
	eina_stringshare_del(a->file);
	eina_stringshare_del(a->file2);
	eina_stringshare_del(a->finf.lnk);
	eina_stringshare_del(a->finf.rlnk);
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
   sd->tmp.last_insert = NULL;
}

static void
_e_fm2_live_process(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   E_Fm2_Action *a;
   Eina_List *l;
   E_Fm2_Icon *ic;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd->live.actions) return;
   a = sd->live.actions->data;
   sd->live.actions = eina_list_remove_list(sd->live.actions, sd->live.actions);
   switch (a->type)
     {
      case FILE_ADD:
	/* new file to sort in place */
	if (!strcmp(a->file, ".order"))
	  {
	     sd->order_file = 1;
	     /* FIXME: reload fm view */
	  }
	else
	  {
	     if (!((a->file[0] == '.') && (!sd->show_hidden_files)))
	       _e_fm2_file_add(obj, a->file, 1, a->file2, a->flags, &(a->finf));
	  }
	break;
      case FILE_DEL:
	if (!strcmp(a->file, ".order"))
	  {
	     sd->order_file = 0;
	     /* FIXME: reload fm view */
	  }
	else
	  {
	     if (!((a->file[0] == '.') && (!sd->show_hidden_files)))
	       _e_fm2_file_del(obj, a->file);
	     sd->live.deletions = 1;
	  }
	break;
      case FILE_CHANGE:
	if (!strcmp(a->file, ".order"))
	  {
	     /* FIXME: reload fm view - ignore for now */
	  }
	else
	  {
	     if (!((a->file[0] == '.') && (!sd->show_hidden_files)))
	       {
		  for (l = sd->icons; l; l = l->next)
		    {
		       ic = l->data;
		       if (!strcmp(ic->info.file, a->file))
			 {
			    if (ic->removable_state_change)
			      {
				 _e_fm2_icon_unfill(ic);
				 _e_fm2_icon_fill(ic, &(a->finf));
				 ic->removable_state_change = 0;
				 if ((ic->realized) && (ic->obj_icon))
				   {
				      if (ic->info.removable_full)
					edje_object_signal_emit(ic->obj_icon, "e,state,removable,full", "e");
				      else
					edje_object_signal_emit(ic->obj_icon, "e,state,removable,empty", "e");
				      _e_fm2_icon_label_set(ic, ic->obj);
				   }
			      }
			    else
			      {
				 int realized;
				 
				 realized = ic->realized;
				 if (realized) _e_fm2_icon_unrealize(ic);
				 _e_fm2_icon_unfill(ic);
				 _e_fm2_icon_fill(ic, &(a->finf));
				 if (realized) _e_fm2_icon_realize(ic);
			      }
			    break;
			 }
		    }
	       }
	  }
	break;
      default:
	break;
     }
   eina_stringshare_del(a->file);
   eina_stringshare_del(a->file2);
   eina_stringshare_del(a->finf.lnk);
   eina_stringshare_del(a->finf.rlnk);
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
     {
	e_fm2_refresh(data);
     }
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

static int
_e_fm2_theme_edje_object_set(E_Fm2_Smart_Data *sd, Evas_Object *o, const char *category, const char *group)
{
   char buf[1024];
   int ret;
   
   if (sd->custom_theme_content)
     snprintf(buf, sizeof(buf), "e/fileman/%s/%s", sd->custom_theme_content, group);
   else
     snprintf(buf, sizeof(buf), "e/fileman/default/%s", group);
   
   if (sd->custom_theme)
     {
	if (edje_object_file_set(o, sd->custom_theme, buf)) return 1;
     }
   if (sd->custom_theme)
     {
	if (!ecore_file_exists(sd->custom_theme))
	  {
	     eina_stringshare_del(sd->custom_theme);
	     sd->custom_theme = NULL;
	  }
     }
   ret = e_theme_edje_object_set(o, category, buf);
   return ret;
}

static int
_e_fm2_theme_edje_icon_object_set(E_Fm2_Smart_Data *sd, Evas_Object *o, const char *category, const char *group)
{
   char buf[1024];
   int ret;
   
//   if (sd->custom_theme_content)
//     snprintf(buf, sizeof(buf), "e/icons/fileman/%s/%s", sd->custom_theme_content, group);
//   else
     snprintf(buf, sizeof(buf), "e/icons/fileman/%s", group);
   
   if (sd->custom_theme)
     {
	if (edje_object_file_set(o, sd->custom_theme, buf)) return 1;
     }
   if (sd->custom_theme)
     {
	if (!ecore_file_exists(sd->custom_theme))
	  {
	     eina_stringshare_del(sd->custom_theme);
	     sd->custom_theme = NULL;
	  }
     }
   ret = e_theme_edje_object_set(o, category, buf);
   return ret;
}
