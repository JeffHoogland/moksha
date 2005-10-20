/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

#else
#ifndef E_FILEMAN_FILE_SMART_H
#define E_FILEMAN_FILE_SMART_H

EAPI int          e_fm_icon_init(void);
EAPI int          e_fm_icon_shutdown(void);
EAPI Evas_Object *e_fm_icon_add(Evas *evas);
EAPI void         e_fm_icon_file_set(Evas_Object *obj, E_Fm_File *file);
EAPI void         e_fm_icon_title_set(Evas_Object *obj, const char *title);
EAPI void         e_fm_icon_edit_entry_set(Evas_Object *obj, Evas_Object *entry);
EAPI void         e_fm_icon_signal_emit(Evas_Object *obj, const char *source, const char *emission);

#endif
#endif

