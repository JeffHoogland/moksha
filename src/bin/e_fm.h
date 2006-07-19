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

typedef struct _E_Fm2_Config E_Fm2_Config;

#else
#ifndef E_FM_H
#define E_FM_H

struct _E_Fm2_Config
{
   /* general view mode */
   struct {
      E_Fm2_View_Mode mode;
      unsigned char   open_dirs_in_place;
      unsigned char   selector;
      unsigned char   single_click;
      unsigned char   no_subdir_jump;
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
      char            *background;
      char            *frame;
      char            *icons;
      unsigned char    fixed;
   } theme;
   /* used internally only - used to save to disk only and laod from disk */
   /* FIXME: not implemented yet */
   struct {
      int              x, y, w, h;
      struct {
	 int           w, h;
      } res;
      int              screen;
   } geometry;
};

EAPI int                   e_fm2_init(void);
EAPI int                   e_fm2_shutdown(void);
EAPI Evas_Object          *e_fm2_add(Evas *evas);
EAPI void                  e_fm2_path_set(Evas_Object *obj, char *dev, char *path);
EAPI void                  e_fm2_path_get(Evas_Object *obj, const char **dev, const char **path);
EAPI int                   e_fm2_has_parent_get(Evas_Object *obj);
EAPI void                  e_fm2_parent_go(Evas_Object *obj);
    
EAPI void                  e_fm2_pan_set(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
EAPI void                  e_fm2_pan_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);
EAPI void                  e_fm2_pan_max_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);
EAPI void                  e_fm2_pan_child_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h);


#endif
#endif
