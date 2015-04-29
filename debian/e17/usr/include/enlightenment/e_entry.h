#ifdef E_TYPEDEFS
#else
#ifndef E_ENTRY_H
#define E_ENTRY_H

EAPI Evas_Object *e_entry_add                 (Evas *evas);
EAPI void         e_entry_text_set            (Evas_Object *entry, const char *text);
EAPI const char  *e_entry_text_get            (Evas_Object *entry);
EAPI void         e_entry_clear               (Evas_Object *entry);
EAPI void         e_entry_select_all          (Evas_Object *entry);

EAPI void         e_entry_password_set        (Evas_Object *entry, int password_mode);
EAPI void         e_entry_size_min_get        (Evas_Object *entry, Evas_Coord *minw, Evas_Coord *minh);
EAPI void         e_entry_focus               (Evas_Object *entry);
EAPI void         e_entry_unfocus             (Evas_Object *entry);
EAPI void         e_entry_enable              (Evas_Object *entry);
EAPI void         e_entry_disable             (Evas_Object *entry);
EAPI void         e_entry_edit                (Evas_Object *entry);
EAPI void         e_entry_noedit              (Evas_Object *entry);
EAPI void         e_entry_nomenu(Evas_Object *entry);
#endif
#endif
