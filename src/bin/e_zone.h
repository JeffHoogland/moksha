#ifndef E_ZONE_H
#define E_ZONE_H

typedef struct _E_Zone     E_Zone;

struct _E_Zone
{
   E_Object             e_obj_inherit;

   int                  x, y, w, h;
   char                *name;
   int                  num;
   E_Container         *container;

   Evas_Object         *bg_object;
   Evas_Object         *bg_event_object;
   
   int                  desk_x_count, desk_y_count;
   int                  desk_x_current, desk_y_current;
   E_Object           **desks; /* FIXME: why can this not be E_Desk? */
   Evas_List           *clients;

};

EAPI int        e_zone_init(void);
EAPI int        e_zone_shutdown(void);
EAPI E_Zone    *e_zone_new(E_Container *con, int x, int y, int w, int h);
EAPI void       e_zone_move(E_Zone *zone, int x, int y);
EAPI void       e_zone_resize(E_Zone *zone, int w, int h);
EAPI void       e_zone_move_resize(E_Zone *zone, int x, int y, int w, int h);
EAPI E_Zone    *e_zone_current_get(E_Container *con);
EAPI void       e_zone_bg_reconfigure(E_Zone *zone);
EAPI Evas_List *e_zone_clients_list_get(E_Zone *zone);
EAPI void       e_zone_desks_set(E_Zone *zone, int x_count, int y_count);

#endif

