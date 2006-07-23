/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_Event_Entry_Change E_Event_Entry_Change;

#else
#ifndef E_ENTRY_H
#define E_ENTRY_H

struct _E_Event_Entry_Change
{
   Evas_Object *object;
   char    *keyname;
   char    *key;
   char    *string;
};

EAPI Evas_Object *e_editable_text_add(Evas *evas);
EAPI void         e_editable_text_text_set(Evas_Object *object, const char *text);
EAPI const char  *e_entry_text_get(Evas_Object *object);
EAPI void         e_editable_text_insert(Evas_Object *object, const char *text);
EAPI void         e_editable_text_delete_char_before(Evas_Object *object);
EAPI void         e_editable_text_delete_char_after(Evas_Object *object);
EAPI void         e_editable_text_cursor_move_at_start(Evas_Object *object);
EAPI void         e_editable_text_cursor_move_at_end(Evas_Object *object);
EAPI void         e_editable_text_cursor_move_left(Evas_Object *object);
EAPI void         e_editable_text_cursor_move_right(Evas_Object *object);
EAPI void         e_editable_text_cursor_show(Evas_Object *object);
EAPI void         e_editable_text_cursor_hide(Evas_Object *object);

EAPI int          e_entry_init(void);
EAPI int          e_entry_shutdown(void);
EAPI Evas_Object *e_entry_add(Evas *evas);
EAPI void         e_entry_text_set (Evas_Object *entry, const char *text);
EAPI const char  *e_editable_text_text_get(Evas_Object *object);    
EAPI void         e_entry_text_insert (Evas_Object *entry, const char *text);
EAPI void         e_entry_delete_char_before(Evas_Object *object);
EAPI void         e_entry_delete_char_after(Evas_Object *object);
EAPI void         e_entry_cursor_move_at_start(Evas_Object *object);
EAPI void         e_entry_cursor_move_at_end(Evas_Object *object);
EAPI void         e_entry_cursor_move_left(Evas_Object *object);
EAPI void         e_entry_cursor_move_right(Evas_Object *object);
EAPI void         e_entry_cursor_show(Evas_Object *object);
EAPI void         e_entry_cursor_hide(Evas_Object *object);   
EAPI void         e_entry_change_handler_set(Evas_Object *object, void (*func)(void *data, Evas_Object *entry, char *key), void *data);
EAPI void         e_entry_focus(Evas_Object *object);
EAPI void         e_entry_unfocus(Evas_Object *object);
EAPI void         e_entry_min_size_get(Evas_Object *object, Evas_Coord *mw, Evas_Coord *mh);

#endif
#endif
