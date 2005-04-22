/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_Zone     E_Zone;

typedef struct _E_Event_Zone_Desk_Count_Set     E_Event_Zone_Desk_Count_Set;

#else
#ifndef E_ZONE_H
#define E_ZONE_H

#define E_ZONE_TYPE 0xE0b0100d

struct _E_Zone
{
   E_Object             e_obj_inherit;

   int                  x, y, w, h;
   char                *name;
   /* num matches the id of the xinerama screen
    * this zone belongs to. */
   int                  num;
   E_Container         *container;

   Evas_Object         *bg_object;
   Evas_Object         *bg_event_object;
   Evas_Object         *bg_clip_object;
   
   int                  desk_x_count, desk_y_count;
   int                  desk_x_current, desk_y_current;
   E_Desk             **desks;

   Evas_List           *handlers;

   struct {
	Ecore_X_Window top, right, bottom, left;
	Ecore_Timer *timer;
	int x, y;
	E_Direction direction;
   } flip;
};

struct _E_Event_Zone_Desk_Count_Set
{
   E_Zone *zone;
};

EAPI int        e_zone_init(void);
EAPI int        e_zone_shutdown(void);
EAPI E_Zone    *e_zone_new(E_Container *con, int num, int x, int y, int w, int h);
EAPI void       e_zone_move(E_Zone *zone, int x, int y);
EAPI void       e_zone_resize(E_Zone *zone, int w, int h);
EAPI void       e_zone_move_resize(E_Zone *zone, int x, int y, int w, int h);
EAPI E_Zone    *e_zone_current_get(E_Container *con);
EAPI void       e_zone_bg_reconfigure(E_Zone *zone);
EAPI void       e_zone_desk_count_set(E_Zone *zone, int x_count, int y_count);
EAPI void       e_zone_desk_count_get(E_Zone *zone, int *x_count, int *y_count);

extern EAPI int E_EVENT_ZONE_DESK_COUNT_SET;

#endif
#endif
