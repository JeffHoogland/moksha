#ifndef E_DESK_H
#define E_DESK_H

typedef struct _E_Desk E_Desk;

struct _E_Desk
{
   E_Object             e_obj_inherit;

   E_Zone              *zone;
   char                *name;
   int                  num;
   char                 visible : 1;

   Evas_Object         *bg_object;

   Evas_List           *clients;
};

EAPI int          e_desk_init(void);
EAPI int          e_desk_shutdown(void);
EAPI E_Desk      *e_desk_new(E_Zone *zone, int x, int y);
EAPI void         e_desk_name_set(E_Desk *desk, const char *name);
EAPI void         e_desk_show(E_Desk *desk);
EAPI E_Desk      *e_desk_current_get(E_Zone *zone);
EAPI void         e_desk_next(E_Zone *zone);
EAPI void         e_desk_prev(E_Zone *zone);
EAPI void         e_desk_row_add(E_Zone *zone);
EAPI void         e_desk_row_remove(E_Zone *zone);
EAPI void         e_desk_col_add(E_Zone *zone);
EAPI void         e_desk_col_remove(E_Zone *zone);

#endif

