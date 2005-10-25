/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_WIDGET_ENTRY_H
#define E_WIDGET_ENTRY_H

EAPI Evas_Object     *e_widget_entry_add(Evas *evas, char **val);
EAPI void             e_widget_entry_text_set (Evas_Object *entry, const char *text);
EAPI const char      *e_editable_text_text_get(Evas_Object *entry);
EAPI void             e_widget_entry_text_insert (Evas_Object *entry, const char *text);
EAPI void             e_widget_entry_delete_char_before(Evas_Object *entry);
EAPI void             e_widget_entry_delete_char_after(Evas_Object *entry);
EAPI void             e_widget_entry_cursor_move_at_start(Evas_Object *entry);
EAPI void             e_widget_entry_cursor_move_at_end(Evas_Object *entry);
EAPI void             e_widget_entry_cursor_move_left(Evas_Object *entry);
EAPI void             e_widget_entry_cursor_move_right(Evas_Object *entry);
EAPI void             e_widget_entry_cursor_show(Evas_Object *entry);
EAPI void             e_widget_entry_cursor_hide(Evas_Object *entry);

#endif
#endif
