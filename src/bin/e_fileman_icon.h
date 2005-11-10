/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef enum   _E_Fm_Icon_Type E_Fm_Icon_Type;
typedef struct _E_Fm_Icon_Metadata E_Fm_Icon_Metadata;

#else
#ifndef E_FILEMAN_FILE_SMART_H
#define E_FILEMAN_FILE_SMART_H

enum E_Fm_Icon_Type
{
   E_FM_ICON_NORMAL,
   E_FM_ICON_LIST
};

struct _E_Fm_Icon_Metadata
{
   Evas_Coord  x, y, w, h; /* geometry */
   char       *name;       /* filename */
};

EAPI int                 e_fm_icon_init(void);
EAPI int                 e_fm_icon_shutdown(void);
EAPI Evas_Object        *e_fm_icon_add(Evas *evas);
EAPI E_Fm_File          *e_fm_icon_file_get(Evas_Object *obj);    
EAPI void                e_fm_icon_file_set(Evas_Object *obj, E_Fm_File *file);
EAPI void                e_fm_icon_appear_cb(Evas_Object *obj, void *data);
EAPI void                e_fm_icon_disappear_cb(Evas_Object *obj, void *data);    
EAPI void                e_fm_icon_title_set(Evas_Object *obj, const char *title);
EAPI void                e_fm_icon_type_set(Evas_Object *obj, int type);
EAPI void                e_fm_icon_edit_entry_set(Evas_Object *obj, Evas_Object *entry);
EAPI void                e_fm_icon_signal_emit(Evas_Object *obj, const char *source, const char *emission);
EAPI E_Fm_Icon_Metadata *e_fm_icon_meta_generate(Evas_Object *obj);
EAPI void                e_fm_icon_meta_free(E_Fm_Icon_Metadata *m);

#endif
#endif

