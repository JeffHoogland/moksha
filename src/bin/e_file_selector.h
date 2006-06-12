/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef enum _E_File_Selector_View E_File_Selector_View;

#else
#ifndef E_FILE_SELECTOR_H
#define E_FILE_SELECTOR_H

enum E_File_Selector_View
{
   E_FILE_SELECTOR_ICONVIEW,
   E_FILE_SELECTOR_LISTVIEW
};

EAPI Evas_Object *e_file_selector_add(Evas *evas);
EAPI void         e_file_selector_view_set(Evas_Object *object, int view);
EAPI int          e_file_selector_view_get(Evas_Object *object);
EAPI void         e_file_selector_callback_add(Evas_Object *obj, void (*func) (Evas_Object *obj, char *file, void *data), void (*hilite_func) (Evas_Object *obj, char *file, void *data), void *data);
EAPI void         e_file_selector_dir_set(Evas_Object *obj, const char *dir);
    
#endif
#endif
