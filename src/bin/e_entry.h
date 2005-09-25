#include <Evas.h>

Evas_Object *e_editable_text_add(Evas *evas);

void e_editable_text_text_set(Evas_Object *object, const char *text);
void e_editable_text_insert(Evas_Object *object, const char *text);

void e_editable_text_delete_char_before(Evas_Object *object);
void e_editable_text_delete_char_after(Evas_Object *object);

void e_editable_text_cursor_move_at_start(Evas_Object *object);
void e_editable_text_cursor_move_at_end(Evas_Object *object);
void e_editable_text_cursor_move_left(Evas_Object *object);
void e_editable_text_cursor_move_right(Evas_Object *object);

void e_editable_text_cursor_show(Evas_Object *object);
void e_editable_text_cursor_hide(Evas_Object *object);

Evas_Object *e_entry_add(Evas *evas);
