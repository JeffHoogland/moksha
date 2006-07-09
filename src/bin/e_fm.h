/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

/* IGNORE this code for now! */

typedef enum _E_Fm2_View_Mode
{
   E_FM_VIEW_MODE_ICONS,
   E_FM_VIEW_MODE_LIST
} E_Fm2_View_Mode;

#else
#ifndef E_FM_H
#define E_FM_H

EAPI int                   e_fm2_init(void);
EAPI int                   e_fm2_shutdown(void);
EAPI Evas_Object          *e_fm2_add(Evas *evas);
EAPI void                  e_fm2_path_set(Evas_Object *obj, char *dev, char *path);
EAPI void                  e_fm2_path_get(Evas_Object *obj, char **dev, char **path);
    
#endif
#endif
