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

#else
#ifndef E_FM_H
#define E_FM_H

EAPI int                   e_fm2_init(void);
EAPI int                   e_fm2_shutdown(void);
EAPI Evas_Object          *e_fm2_add(Evas *evas);
EAPI void                  e_fm2_path_set(Evas_Object *obj, char *dev, char *path);
EAPI void                  e_fm2_path_get(Evas_Object *obj, char **dev, char **path);

EAPI void                  e_fm2_pan_set(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
EAPI void                  e_fm2_pan_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);
EAPI void                  e_fm2_pan_max_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);
EAPI void                  e_fm2_pan_child_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h);


#endif
#endif
