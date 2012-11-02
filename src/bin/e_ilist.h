#ifdef E_TYPEDEFS

typedef struct _E_Ilist_Item E_Ilist_Item;

#else
#ifndef E_ILIST_H
#define E_ILIST_H

struct _E_Ilist_Item
{
   void *sd;
   const char *label;
   Evas_Object *o_base;
   Evas_Object *o_icon;
   Evas_Object *o_end;
   unsigned char header : 1;
   unsigned char selected : 1;
   unsigned char queued : 1;

   void (*func) (void *data, void *data2);
   void (*func_hilight) (void *data, void *data2);
   void *data, *data2;
};

EAPI Evas_Object *e_ilist_add                   (Evas *evas);
EAPI void         e_ilist_append                (Evas_Object *obj, Evas_Object *icon, Evas_Object *end, const char *label, int header, void (*func) (void *data, void *data2), void (*func_hilight) (void *data, void *data2), void *data, void *data2);
EAPI void         e_ilist_append_relative       (Evas_Object *obj, Evas_Object *icon, Evas_Object *end, const char *label, int header, void (*func) (void *data, void *data2), void (*func_hilight) (void *data, void *data2), void *data, void *data2, int relative);
EAPI void         e_ilist_prepend               (Evas_Object *obj, Evas_Object *icon, Evas_Object *end, const char *label, int header, void (*func) (void *data, void *data2), void (*func_hilight) (void *data, void *data2), void *data, void *data2);
EAPI void         e_ilist_prepend_relative      (Evas_Object *obj, Evas_Object *icon, Evas_Object *end, const char *label, int header, void (*func) (void *data, void *data2), void (*func_hilight) (void *data, void *data2), void *data, void *data2, int relative);
EAPI void         e_ilist_clear                 (Evas_Object *obj);
EAPI void         e_ilist_freeze                (Evas_Object *obj);
EAPI void         e_ilist_thaw                  (Evas_Object *obj);
EAPI int          e_ilist_count                 (Evas_Object *obj);
EAPI int          e_ilist_selector_get          (Evas_Object *obj);
EAPI void         e_ilist_selector_set          (Evas_Object *obj, int selector);
EAPI Eina_Bool    e_ilist_multi_select_get      (Evas_Object *obj);
EAPI void         e_ilist_multi_select_set      (Evas_Object *obj, Eina_Bool multi);
EAPI void         e_ilist_size_min_get          (Evas_Object *obj, Evas_Coord *w, Evas_Coord *h);
EAPI void         e_ilist_unselect              (Evas_Object *obj);
EAPI void         e_ilist_selected_set          (Evas_Object *obj, int n);
EAPI int          e_ilist_selected_get          (Evas_Object *obj);
EAPI const char  *e_ilist_selected_label_get    (Evas_Object *obj);
EAPI void        *e_ilist_selected_data_get     (Evas_Object *obj);
EAPI void        *e_ilist_selected_data2_get    (Evas_Object *obj);
EAPI Evas_Object *e_ilist_selected_icon_get     (Evas_Object *obj);
EAPI Evas_Object *e_ilist_selected_end_get      (Evas_Object *obj);
EAPI void         e_ilist_selected_geometry_get (Evas_Object *obj, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h);
EAPI const Eina_List *e_ilist_selected_items_get(Evas_Object *obj);
EAPI int          e_ilist_selected_count_get    (Evas_Object *obj);
EAPI void         e_ilist_remove_num            (Evas_Object *obj, int n);
EAPI const char  *e_ilist_nth_label_get         (Evas_Object *obj, int n);
EAPI void         e_ilist_nth_label_set         (Evas_Object *obj, int n, const char *label);
EAPI Evas_Object *e_ilist_nth_icon_get          (Evas_Object *obj, int n);
EAPI void         e_ilist_nth_icon_set          (Evas_Object *obj, int n, Evas_Object *icon);
EAPI Evas_Object *e_ilist_nth_end_get           (Evas_Object *obj, int n);
EAPI void         e_ilist_nth_end_set           (Evas_Object *obj, int n, Evas_Object *end);
EAPI Eina_Bool    e_ilist_nth_is_header         (Evas_Object *obj, int n);
EAPI void         e_ilist_nth_geometry_get      (Evas_Object *obj, int n, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h);
EAPI void         e_ilist_icon_size_set         (Evas_Object *obj, Evas_Coord w, Evas_Coord h);
EAPI const Eina_List   *e_ilist_items_get             (Evas_Object *obj);
EAPI void         e_ilist_multi_select          (Evas_Object *obj, int n);
EAPI void         e_ilist_range_select          (Evas_Object *obj, int n);
EAPI void         e_ilist_item_label_set(E_Ilist_Item *si, const char *label);
EAPI Eina_Bool  e_ilist_custom_edje_file_set(Evas_Object *obj, const char *file, const char *group);
#endif
#endif
