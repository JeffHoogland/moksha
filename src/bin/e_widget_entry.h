/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_WIDGET_ENTRY_H
#define E_WIDGET_ENTRY_H

EAPI Evas_Object *e_widget_entry_add                    (Evas *evas, char **text_location);
EAPI void         e_widget_entry_text_set               (Evas_Object *entry, const char *text);
EAPI const char  *e_widget_entry_text_get               (Evas_Object *entry);
EAPI void         e_widget_entry_clear                  (Evas_Object *entry);
EAPI void         e_widget_entry_password_set           (Evas_Object *entry, int password);

#endif
#endif
