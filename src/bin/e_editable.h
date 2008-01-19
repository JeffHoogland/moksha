/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_EDITABLE_H
#define E_EDITABLE_H

EAPI Evas_Object *e_editable_add                     (Evas *evas);
EAPI void         e_editable_theme_set               (Evas_Object *editable, const char *category, const char *group);
EAPI void         e_editable_password_set            (Evas_Object *editable, int password_mode);
EAPI int          e_editable_password_get            (Evas_Object *editable);

EAPI void         e_editable_text_set                (Evas_Object *editable, const char *text);
EAPI const char  *e_editable_text_get                (Evas_Object *editable);
EAPI char        *e_editable_text_range_get          (Evas_Object *editable, int start, int end);
EAPI int          e_editable_text_length_get         (Evas_Object *editable);
EAPI int          e_editable_insert                  (Evas_Object *editable, int pos, const char *text);
EAPI int          e_editable_delete                  (Evas_Object *editable, int start, int end);

EAPI void         e_editable_cursor_pos_set          (Evas_Object *editable, int pos);
EAPI int          e_editable_cursor_pos_get          (Evas_Object *editable);
EAPI void         e_editable_cursor_move_to_start    (Evas_Object *editable);
EAPI void         e_editable_cursor_move_to_end      (Evas_Object *editable);
EAPI void         e_editable_cursor_move_left        (Evas_Object *editable);
EAPI void         e_editable_cursor_move_right       (Evas_Object *editable);
EAPI void         e_editable_cursor_show             (Evas_Object *editable);
EAPI void         e_editable_cursor_hide             (Evas_Object *editable);

EAPI void         e_editable_selection_pos_set       (Evas_Object *editable, int pos);
EAPI int          e_editable_selection_pos_get       (Evas_Object *editable);
EAPI void         e_editable_selection_move_to_start (Evas_Object *editable);
EAPI void         e_editable_selection_move_to_end   (Evas_Object *editable);
EAPI void         e_editable_selection_move_left     (Evas_Object *editable);
EAPI void         e_editable_selection_move_right    (Evas_Object *editable);
EAPI void         e_editable_select_all              (Evas_Object *editable);
EAPI void         e_editable_unselect_all            (Evas_Object *editable);
EAPI void         e_editable_select_word             (Evas_Object *editable, int index);
EAPI void         e_editable_selection_show          (Evas_Object *editable);
EAPI void         e_editable_selection_hide          (Evas_Object *editable);

EAPI int          e_editable_pos_get_from_coords     (Evas_Object *editable, Evas_Coord x, Evas_Coord y);
EAPI void         e_editable_char_size_get           (Evas_Object *editable, int *w, int *h);

EAPI void         e_editable_enable                  (Evas_Object *entry);
EAPI void         e_editable_disable                 (Evas_Object *entry);

#endif
#endif
