/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_ENTRY_H
#define E_ENTRY_H

EAPI Evas_Object *e_entry_add                 (Evas *evas);
EAPI void         e_entry_text_set            (Evas_Object *entry, const char *text);
EAPI const char  *e_entry_text_get            (Evas_Object *entry);
EAPI void         e_entry_clear               (Evas_Object *entry);
EAPI Evas_Object *e_entry_editable_object_get (Evas_Object *entry);

EAPI void         e_entry_password_set        (Evas_Object *entry, int password_mode);
EAPI void         e_entry_min_size_get        (Evas_Object *entry, Evas_Coord *minw, Evas_Coord *minh);
EAPI void         e_entry_focus               (Evas_Object *entry);
EAPI void         e_entry_unfocus             (Evas_Object *entry);
EAPI void         e_entry_enable              (Evas_Object *entry);
EAPI void         e_entry_disable             (Evas_Object *entry);

#endif
#endif
