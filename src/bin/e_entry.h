/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifndef E_ENTRY_H
#define E_ENTRY_H

#include <Evas.h>

EAPI Evas_Object *e_editable_text_add(Evas *evas);

EAPI void e_editable_text_text_set(Evas_Object *object, const char *text);
EAPI const char *e_entry_text_get(Evas_Object *object);
EAPI void e_editable_text_insert(Evas_Object *object, const char *text);

EAPI void e_editable_text_delete_char_before(Evas_Object *object);
EAPI void e_editable_text_delete_char_after(Evas_Object *object);

EAPI void e_editable_text_cursor_move_at_start(Evas_Object *object);
EAPI void e_editable_text_cursor_move_at_end(Evas_Object *object);
EAPI void e_editable_text_cursor_move_left(Evas_Object *object);
EAPI void e_editable_text_cursor_move_right(Evas_Object *object);

EAPI void e_editable_text_cursor_show(Evas_Object *object);
EAPI void e_editable_text_cursor_hide(Evas_Object *object);

EAPI Evas_Object *e_entry_add(Evas *evas);
EAPI void e_entry_text_set (Evas_Object *entry, const char *text);
EAPI const char* e_editable_text_text_get(Evas_Object *object);    
EAPI void e_entry_text_insert (Evas_Object *entry, const char *text);
EAPI void e_entry_delete_char_before(Evas_Object *object);
EAPI void e_entry_delete_char_after(Evas_Object *object);
EAPI void e_entry_cursor_move_at_start(Evas_Object *object);
EAPI void e_entry_cursor_move_at_end(Evas_Object *object);
EAPI void e_entry_cursor_move_left(Evas_Object *object);
EAPI void e_entry_cursor_move_right(Evas_Object *object);
EAPI void e_entry_cursor_show(Evas_Object *object);
EAPI void e_entry_cursor_hide(Evas_Object *object);

#endif
