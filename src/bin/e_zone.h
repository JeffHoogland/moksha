/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_Zone     E_Zone;

typedef struct _E_Event_Zone_Desk_Count_Set     E_Event_Zone_Desk_Count_Set;
/* TODO: Move this to a general place? */
typedef struct _E_Event_Pointer_Warp            E_Event_Pointer_Warp;

#else
#ifndef E_ZONE_H
#define E_ZONE_H

#define E_ZONE_TYPE 0xE0b0100d

struct _E_Zone
{
   E_Object             e_obj_inherit;

   int                  x, y, w, h;
   const char          *name;
   /* num matches the id of the xinerama screen
    * this zone belongs to. */
   unsigned int         num;
   E_Container         *container;
   int                  fullscreen;
   unsigned int         deskshow_toggle : 1;

   Evas_Object         *bg_object;
   Evas_Object         *bg_event_object;
   Evas_Object         *bg_clip_object;
   Evas_Object         *prev_bg_object;
   Evas_Object         *transition_object;
   
   int                  desk_x_count, desk_y_count;
   int                  desk_x_current, desk_y_current;
   E_Desk             **desks;

   Evas_List           *handlers;

   struct {
	Ecore_X_Window top, right, bottom, left;
	Ecore_Timer *timer;
	E_Direction direction;
   } flip;
   
   E_Action *cur_mouse_action;
   Evas_List *popups;

   Ecore_Evas          *black_ecore_evas;
   Evas                *black_evas;
   Ecore_X_Window       black_win;
};

struct _E_Event_Zone_Desk_Count_Set
{
   E_Zone *zone;
};

struct _E_Event_Pointer_Warp
{
   struct {
	int x, y;
   } prev;
   struct {
	int x, y;
   } curr;
};

EAPI int        e_zone_init(void);
EAPI int        e_zone_shutdown(void);
EAPI E_Zone    *e_zone_new(E_Container *con, int num, int x, int y, int w, int h);
EAPI void       e_zone_name_set(E_Zone *zone, const char *name);
EAPI void       e_zone_move(E_Zone *zone, int x, int y);
EAPI void       e_zone_resize(E_Zone *zone, int w, int h);
EAPI void       e_zone_move_resize(E_Zone *zone, int x, int y, int w, int h);
EAPI void       e_zone_fullscreen_set(E_Zone *zone, int on);
EAPI E_Zone    *e_zone_current_get(E_Container *con);
EAPI void       e_zone_bg_reconfigure(E_Zone *zone);
EAPI void       e_zone_flip_coords_handle(E_Zone *zone, int x, int y);
EAPI void       e_zone_desk_count_set(E_Zone *zone, int x_count, int y_count);
EAPI void       e_zone_desk_count_get(E_Zone *zone, int *x_count, int *y_count);
EAPI void       e_zone_update_flip(E_Zone *zone);
EAPI void       e_zone_update_flip_all(void);
EAPI void       e_zone_desk_flip_by(E_Zone *zone, int dx, int dy);
EAPI void       e_zone_desk_flip_to(E_Zone *zone, int x, int y);
EAPI void       e_zone_desk_linear_flip_by(E_Zone *zone, int dx);
EAPI void       e_zone_desk_linear_flip_to(E_Zone *zone, int x);
EAPI void       e_zone_flip_win_disable(void);
EAPI void       e_zone_flip_win_restore(void);

EAPI int        e_zone_exec(E_Zone *zone, char *exe);
EAPI int        e_zone_app_exec(E_Zone *zone, E_App *a);

extern EAPI int E_EVENT_ZONE_DESK_COUNT_SET;
extern EAPI int E_EVENT_POINTER_WARP;

#endif
#endif
