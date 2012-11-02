#ifdef E_TYPEDEFS

/* IGNORE this code for now! */

typedef enum _E_Fm2_View_Mode
{
   E_FM2_VIEW_MODE_ICONS, /* regular layout row by row like text */
   E_FM2_VIEW_MODE_GRID_ICONS, /* regular grid layout */
   E_FM2_VIEW_MODE_CUSTOM_ICONS, /* icons go anywhere u drop them (desktop) */

   /* FIXME: not going to implement this at this stage */
   E_FM2_VIEW_MODE_CUSTOM_GRID_ICONS, /* icons go anywhere u drop them but align to a grid */
   /* FIXME: not going to implement this at this stage */
   E_FM2_VIEW_MODE_CUSTOM_SMART_GRID_ICONS, /* icons go anywhere u drop them but try align to icons nearby */
   /* FIXME: not going to implement this at this stage */
   E_FM2_VIEW_MODE_LIST /* vertical fileselector list */
} E_Fm2_View_Mode;

typedef enum _E_Fm2_Menu_Flags
{
   E_FM2_MENU_NO_REFRESH = (1 << 0),
   E_FM2_MENU_NO_SHOW_HIDDEN = (1 << 1),
   E_FM2_MENU_NO_REMEMBER_ORDERING = (1 << 2),
   E_FM2_MENU_NO_NEW = (1 << 3),
   E_FM2_MENU_NO_DELETE = (1 << 4),
   E_FM2_MENU_NO_RENAME = (1 << 5),
   E_FM2_MENU_NO_CUT = (1 << 6),
   E_FM2_MENU_NO_COPY = (1 << 7),
   E_FM2_MENU_NO_PASTE = (1 << 8),
   E_FM2_MENU_NO_SYMLINK = (1 << 9),
   E_FM2_MENU_NO_VIEW_MENU = (1 << 10),
   E_FM2_MENU_NO_INHERIT_PARENT = (1 << 11),
   E_FM2_MENU_NO_ACTIVATE_CHANGE = (1 << 12),
   E_FM2_MENU_NO_VIEW_CHANGE = (1 << 13),
} E_Fm2_Menu_Flags;

typedef enum _E_Fm2_View_Flags
{
   E_FM2_VIEW_NO_FLAGS = 0,
   E_FM2_VIEW_LOAD_DIR_CUSTOM = (1 << 0),
   E_FM2_VIEW_SAVE_DIR_CUSTOM = (1 << 1),
   E_FM2_VIEW_INHERIT_DIR_CUSTOM = (1 << 2),
   E_FM2_VIEW_DIR_CUSTOM = (E_FM2_VIEW_LOAD_DIR_CUSTOM |
                            E_FM2_VIEW_SAVE_DIR_CUSTOM |
                            E_FM2_VIEW_INHERIT_DIR_CUSTOM)
} E_Fm2_View_Flags;

typedef struct _E_Fm2_Config    E_Fm2_Config;
typedef struct _E_Fm2_Icon      E_Fm2_Icon;
typedef struct _E_Fm2_Icon_Info E_Fm2_Icon_Info;

#include "e_fm_shared_types.h"

#else
#ifndef E_FM_H
#define E_FM_H

struct _E_Fm2_Config
{
   /* general view mode */
   struct
   {
      E_Fm2_View_Mode mode;
      Eina_Bool       open_dirs_in_place : 1;
      Eina_Bool       selector : 1;
      Eina_Bool       single_click : 1;
      Eina_Bool       no_subdir_jump : 1;
      Eina_Bool       no_subdir_drop : 1;
      Eina_Bool       always_order : 1;
      Eina_Bool       link_drop : 1;
      Eina_Bool       fit_custom_pos : 1;
      Eina_Bool       no_typebuf_set : 1;
      Eina_Bool       no_click_rename : 1;
      unsigned int    single_click_delay;
   } view;
   /* display of icons */
   struct
   {
      struct
      {
         int w, h;
      } icon, list;
      struct
      {
         unsigned char w;
         unsigned char h;
      } fixed;
      struct
      {
         Eina_Bool show : 1;
      } extension;
      const char *key_hint;
      unsigned int max_thumb_size;
   } icon;
   /* how to sort files */
   struct
   {
      struct
      {
         Eina_Bool no_case : 1;
         Eina_Bool size : 1;
         Eina_Bool extension : 1;
         Eina_Bool mtime : 1;
         struct
         {
            Eina_Bool first : 1;
            Eina_Bool last : 1;
         } dirs;
      } sort;
   } list;
   /* control how you can select files */
   struct
   {
      Eina_Bool single : 1;
      Eina_Bool windows_modifiers : 1;
   } selection;
   /* the background - if any, and how to handle it */
   /* FIXME: not implemented yet */
   struct
   {
      const char *background, *frame, *icons;
      Eina_Bool   fixed : 1;
   } theme;
   Eina_Bool secure_rm : 1;
};

struct _E_Fm2_Icon_Info
{
   Evas_Object  *fm;
   E_Fm2_Icon   *ic;
   const char   *file;
   const char   *mime;
   const char   *label;
   const char   *comment;
   const char   *generic;
   const char   *icon;
   const char   *link;
   const char   *real_link;
   const char   *category;
   struct stat   statinfo;
   unsigned char icon_type;
   Eina_Bool     mount : 1;
   Eina_Bool     removable : 1;
   Eina_Bool     removable_full : 1;
   Eina_Bool     deleted : 1;
   Eina_Bool     broken_link : 1;
};

typedef void (*E_Fm_Cb)(void *data, Evas_Object *obj, E_Menu *m, E_Fm2_Icon_Info *info);

EINTERN int           e_fm2_init(void);
EINTERN int           e_fm2_shutdown(void);
EAPI Evas_Object     *e_fm2_add(Evas *evas);
EAPI void             e_fm2_path_set(Evas_Object *obj, const char *dev, const char *path);
EAPI void             e_fm2_custom_theme_set(Evas_Object *obj, const char *path);
EAPI void             e_fm2_custom_theme_content_set(Evas_Object *obj, const char *content);
EAPI void             e_fm2_underlay_show(Evas_Object *obj);
EAPI void             e_fm2_underlay_hide(Evas_Object *obj);
EAPI void             e_fm2_all_unsel(Evas_Object *obj);
EAPI void             e_fm2_all_sel(Evas_Object *obj);
EAPI void             e_fm2_first_sel(Evas_Object *obj);
EAPI void             e_fm2_last_sel(Evas_Object *obj);
EAPI void             e_fm2_path_get(Evas_Object *obj, const char **dev, const char **path);
EAPI void             e_fm2_refresh(Evas_Object *obj);
EAPI const char      *e_fm2_real_path_get(Evas_Object *obj);
EAPI int              e_fm2_has_parent_get(Evas_Object *obj);
EAPI void             e_fm2_parent_go(Evas_Object *obj);
EAPI void             e_fm2_config_set(Evas_Object *obj, E_Fm2_Config *cfg);
EAPI E_Fm2_Config    *e_fm2_config_get(Evas_Object *obj);
EAPI Eina_List       *e_fm2_selected_list_get(Evas_Object *obj);
EAPI Eina_List       *e_fm2_all_list_get(Evas_Object *obj);
EAPI void             e_fm2_select_set(Evas_Object *obj, const char *file, int select);
EAPI void             e_fm2_deselect_all(Evas_Object *obj);
EAPI void             e_fm2_file_show(Evas_Object *obj, const char *file);
EAPI void             e_fm2_icon_menu_replace_callback_set(Evas_Object *obj, E_Fm_Cb func, void *data);
EAPI void             e_fm2_icon_menu_start_extend_callback_set(Evas_Object *obj, E_Fm_Cb func, void *data);
EAPI void             e_fm2_icon_menu_end_extend_callback_set(Evas_Object *obj, E_Fm_Cb func, void *data);
EAPI void             e_fm2_icon_menu_flags_set(Evas_Object *obj, E_Fm2_Menu_Flags flags);
EAPI E_Fm2_Menu_Flags e_fm2_icon_menu_flags_get(Evas_Object *obj);
EAPI void             e_fm2_view_flags_set(Evas_Object *obj, E_Fm2_View_Flags flags);
EAPI E_Fm2_View_Flags e_fm2_view_flags_get(Evas_Object *obj);
EAPI E_Object         *e_fm2_window_object_get(Evas_Object *obj);
EAPI void             e_fm2_window_object_set(Evas_Object *obj, E_Object *eobj);
EAPI void             e_fm2_icons_update(Evas_Object *obj);

EAPI void             e_fm2_pan_set(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
EAPI void             e_fm2_pan_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);
EAPI void             e_fm2_pan_max_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);
EAPI void             e_fm2_pan_child_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h);

EAPI void             e_fm2_all_icons_update(void);

EAPI void             e_fm2_operation_abort(int id);

EAPI Evas_Object     *e_fm2_icon_get(Evas *evas, E_Fm2_Icon *ic, Evas_Smart_Cb gen_func,
                                     void *data, int force_gen, const char **type_ret);
EAPI E_Fm2_Icon_Info *e_fm2_icon_file_info_get(E_Fm2_Icon *ic);
EAPI void             e_fm2_icon_geometry_get(E_Fm2_Icon *ic, int *x, int *y, int *w, int *h);
EAPI Eina_Bool        e_fm2_typebuf_visible_get(Evas_Object *obj);
EAPI void             e_fm2_typebuf_clear(Evas_Object *obj);
EAPI void             e_fm2_overlay_clip_to(Evas_Object *fm, Evas_Object *clip);

EAPI void             e_fm2_client_data(Ecore_Ipc_Event_Client_Data *e);
EAPI void             e_fm2_client_del(Ecore_Ipc_Event_Client_Del *e);
EAPI E_Fm2_View_Mode  e_fm2_view_mode_get(Evas_Object *obj);
EAPI void             e_fm2_optimal_size_calc(Evas_Object *obj, int maxw, int maxh, int *w, int *h);
EAPI const char      *e_fm2_real_path_map(const char *dev, const char *path);
EAPI void             e_fm2_favorites_init(void);
EAPI unsigned int     e_fm2_selected_count(Evas_Object *obj);
EAPI const char      *e_fm2_desktop_url_eval(const char *val);
EAPI E_Fm2_Icon_Info *e_fm2_drop_icon_get(Evas_Object *obj);
EAPI void             e_fm2_drop_menu(Evas_Object *e_fm, char *args);
EAPI Eina_List       *e_fm2_uri_path_list_get(const Eina_List *uri_list);
EAPI Efreet_Desktop *e_fm2_desktop_get(Evas_Object *obj);

EAPI int e_fm2_client_file_move(Evas_Object *e_fm, const char *args);
EAPI int e_fm2_client_file_copy(Evas_Object *e_fm, const char *args);
EAPI int e_fm2_client_file_symlink(Evas_Object *e_fm, const char *args);

EAPI int              _e_fm2_client_mount(const char *udi, const char *mountpoint);
EAPI int              _e_fm2_client_unmount(const char *udi);
EAPI void             _e_fm2_file_force_update(const char *path);

#endif
#endif
