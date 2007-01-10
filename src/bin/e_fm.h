/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

/* IGNORE this code for now! */

typedef enum _E_Fm2_View_Mode
{
   E_FM2_VIEW_MODE_ICONS, /* regular layout row by row like text */
   E_FM2_VIEW_MODE_GRID_ICONS, /* regular grid layout */
   E_FM2_VIEW_MODE_CUSTOM_ICONS, /* icons go anywhere u drop them (desktop) */
   E_FM2_VIEW_MODE_CUSTOM_GRID_ICONS, /* icons go anywhere u drop them but align to a grid */
   E_FM2_VIEW_MODE_CUSTOM_SMART_GRID_ICONS, /* icons go anywhere u drop them but try align to icons nearby */
   E_FM2_VIEW_MODE_LIST /* vertical fileselector list */
} E_Fm2_View_Mode;

typedef enum _E_Fm2_Menu_Flags
{
   E_FM2_MENU_NO_REFRESH           = (1 << 0),
   E_FM2_MENU_NO_SHOW_HIDDEN       = (1 << 1),
   E_FM2_MENU_NO_REMEMBER_ORDERING = (1 << 2),
   E_FM2_MENU_NO_NEW_DIRECTORY     = (1 << 3),
   E_FM2_MENU_NO_DELETE            = (1 << 4),
   E_FM2_MENU_NO_RENAME            = (1 << 5)
} E_Fm2_Menu_Flags;

typedef struct _E_Fm2_Config      E_Fm2_Config;
typedef struct _E_Fm2_Icon        E_Fm2_Icon;
typedef struct _E_Fm2_Icon_Info   E_Fm2_Icon_Info;
typedef struct _E_Fm2_Custom_File E_Fm2_Custom_File;

#else
#ifndef E_FM_H
#define E_FM_H

struct _E_Fm2_Config
{
   /* general view mode */
   struct {
      E_Fm2_View_Mode mode;
      const char     *extra_file_source;
      unsigned char   open_dirs_in_place;
      unsigned char   selector;
      unsigned char   single_click;
      unsigned char   no_subdir_jump;
      unsigned char   no_subdir_drop;
      unsigned char   always_order;
      unsigned char   link_drop;
   } view;
   /* display of icons */
   struct {
      struct {
	 int           w, h;
      } icon;
      struct {
	 int           w, h;
      } list;
      struct {
	 unsigned char w;
	 unsigned char h;
      } fixed;
      struct {
	 unsigned char show;
      } extension;
      const char      *key_hint;
   } icon;
   /* how to sort files */
   struct {
      struct {
	 unsigned char    no_case;
	 struct {
	    unsigned char first;
	    unsigned char last;
	 } dirs;
      } sort;
   } list;
   /* control how you can select files */
   struct {
      unsigned char    single;
      unsigned char    windows_modifiers;
   } selection;
   /* the background - if any, and how to handle it */
   /* FIXME: not implemented yet */
   struct {
      const char      *background;
      const char      *frame;
      const char      *icons;
      unsigned char    fixed;
   } theme;
};

struct _E_Fm2_Icon_Info
{
   Evas_Object      *fm;
   const char       *file;
   const char       *mime;
   const char       *label;
   const char       *comment;
   const char       *generic;
   const char       *icon;
   const char       *link;
   const char       *real_link;
   const char       *pseudo_dir;
   struct stat       statinfo;
   unsigned char     icon_type;
   unsigned char     mount : 1;
   unsigned char     pseudo_link : 1;
   unsigned char     deleted : 1;
   unsigned char     broken_link : 1;
};

struct _E_Fm2_Custom_File
{
   struct {
      int            x, y, w, h;
      int            res_w, res_h;
      unsigned char  valid;
   } geom;
   struct {
      int            type;
      const char    *icon;
      unsigned char  valid;
   } icon;
   const char       *label;
   /* FIXME: this will have more added */
};

EAPI E_Fm2_Custom_File    *e_fm2_custom_file_get(const char *path);
EAPI void                  e_fm2_custom_file_set(const char *path, E_Fm2_Custom_File *cf);
EAPI void                  e_fm2_custom_file_del(const char *path);
EAPI void                  e_fm2_custom_file_rename(const char *path, const char *new_path);
EAPI void                  e_fm2_custom_file_flush(void);

EAPI int                   e_fm2_init(void);
EAPI int                   e_fm2_shutdown(void);
EAPI Evas_Object          *e_fm2_add(Evas *evas);
EAPI void                  e_fm2_path_set(Evas_Object *obj, const char *dev, const char *path);
EAPI void                  e_fm2_path_get(Evas_Object *obj, const char **dev, const char **path);
EAPI void                  e_fm2_refresh(Evas_Object *obj);
EAPI const char           *e_fm2_real_path_get(Evas_Object *obj);
EAPI int                   e_fm2_has_parent_get(Evas_Object *obj);
EAPI void                  e_fm2_parent_go(Evas_Object *obj);
EAPI void                  e_fm2_config_set(Evas_Object *obj, E_Fm2_Config *cfg);
EAPI E_Fm2_Config         *e_fm2_config_get(Evas_Object *obj);
EAPI Evas_List            *e_fm2_selected_list_get(Evas_Object *obj);
EAPI Evas_List            *e_fm2_all_list_get(Evas_Object *obj);
EAPI void                  e_fm2_select_set(Evas_Object *obj, const char *file, int select);
EAPI void                  e_fm2_file_show(Evas_Object *obj, const char *file);
EAPI void                  e_fm2_icon_menu_replace_callback_set(Evas_Object *obj, void (*func) (void *data, Evas_Object *obj, E_Menu *m, E_Fm2_Icon_Info *info), void *data);
EAPI void                  e_fm2_icon_menu_start_extend_callback_set(Evas_Object *obj, void (*func) (void *data, Evas_Object *obj, E_Menu *m, E_Fm2_Icon_Info *info), void *data);
EAPI void                  e_fm2_icon_menu_end_extend_callback_set(Evas_Object *obj, void (*func) (void *data, Evas_Object *obj, E_Menu *m, E_Fm2_Icon_Info *info), void *data);
EAPI void                  e_fm2_icon_menu_flags_set(Evas_Object *obj, E_Fm2_Menu_Flags flags);
EAPI E_Fm2_Menu_Flags      e_fm2_icon_menu_flags_get(Evas_Object *obj);
EAPI void                  e_fm2_window_object_set(Evas_Object *obj, E_Object *eobj);
EAPI void                  e_fm2_icons_update(Evas_Object *obj);

EAPI void                  e_fm2_pan_set(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
EAPI void                  e_fm2_pan_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);
EAPI void                  e_fm2_pan_max_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);
EAPI void                  e_fm2_pan_child_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h);

EAPI void                  e_fm2_all_icons_update(void);

EAPI void                  e_fm2_fop_delete_add(Evas_Object *obj, E_Fm2_Icon_Info *ici);
EAPI void                  e_fm2_fop_move_add(Evas_Object *obj, const char *src, const char *dst, const char *rel, int after, int file_add);
EAPI void                  e_fm2_fop_link_add(Evas_Object *obj, const char *src, const char *dst);
EAPI void                  e_fm2_fop_add_add(Evas_Object *obj, const char *file, const char *rel, int after);

EAPI Evas_Object *
  e_fm2_icon_get(Evas *evas, const char *realpath,
		 E_Fm2_Icon *ic, E_Fm2_Icon_Info *ici,
		 const char *keyhint,
		 void (*gen_func) (void *data, Evas_Object *obj, void *event_info),
		 void *data, int force_gen, const char **type_ret);

#endif
#endif
