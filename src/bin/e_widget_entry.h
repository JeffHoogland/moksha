#ifdef E_TYPEDEFS
#else
#ifndef E_WIDGET_ENTRY_H
#define E_WIDGET_ENTRY_H

EAPI Evas_Object *e_widget_entry_add                 (Evas *evas, char **text_location, void (*func) (void *data, void *data2), void *data, void *data2);
EAPI void         e_widget_entry_text_set            (Evas_Object *entry, const char *text);
EAPI const char  *e_widget_entry_text_get            (Evas_Object *entry);
EAPI void         e_widget_entry_clear               (Evas_Object *entry);
EAPI void         e_widget_entry_password_set        (Evas_Object *entry, int password_mode);
EAPI void	  e_widget_entry_readonly_set        (Evas_Object *entry, int readonly_mode);
EAPI void         e_widget_entry_select_all          (Evas_Object *entry);

#endif
#endif
