/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_Desk E_Desk;
typedef struct _E_Event_Desk_Show E_Event_Desk_Show;
typedef struct _E_Event_Desk_Before_Show E_Event_Desk_Before_Show;
typedef struct _E_Event_Desk_After_Show E_Event_Desk_After_Show;
typedef struct _E_Event_Desk_DeskShow E_Event_Desk_DeskShow;
typedef struct _E_Event_Desk_Name_Change E_Event_Desk_Name_Change;

#else
#ifndef E_DESK_H
#define E_DESK_H

#define E_DESK_TYPE 0xE0b01005

struct _E_Desk
{
   E_Object             e_obj_inherit;

   E_Zone              *zone;
   const char          *name;
   int                  x, y;
   char                 visible : 1;
   unsigned int         deskshow_toggle : 1;
   int                  fullscreen_borders;

   Evas_Object         *bg_object;
   
   Ecore_Animator      *animator;
};

struct _E_Event_Desk_Show
{
   E_Desk   *desk;
};

struct _E_Event_Desk_Before_Show
{
   E_Desk   *desk;
};

struct _E_Event_Desk_After_Show
{
   E_Desk   *desk;
};

struct _E_Event_Desk_Name_Change
{
   E_Desk   *desk;
};

EAPI int          e_desk_init(void);
EAPI int          e_desk_shutdown(void);
EAPI E_Desk      *e_desk_new(E_Zone *zone, int x, int y);
EAPI void         e_desk_name_set(E_Desk *desk, const char *name);
EAPI void         e_desk_name_add(int container, int zone, int desk_x, int desk_y, const char *name);
EAPI void         e_desk_name_del(int container, int zone, int desk_x, int desk_y);
EAPI void         e_desk_name_update(void);
EAPI void         e_desk_show(E_Desk *desk);
EAPI void         e_desk_deskshow(E_Zone *zone);
EAPI void         e_desk_last_focused_focus(E_Desk *desk);
EAPI E_Desk      *e_desk_current_get(E_Zone *zone);
EAPI E_Desk      *e_desk_at_xy_get(E_Zone *zone, int x, int y);
EAPI E_Desk      *e_desk_at_pos_get(E_Zone *zone, int pos);
EAPI void         e_desk_xy_get(E_Desk *desk, int *x, int *y);
EAPI void         e_desk_next(E_Zone *zone);
EAPI void         e_desk_prev(E_Zone *zone);
EAPI void         e_desk_row_add(E_Zone *zone);
EAPI void         e_desk_row_remove(E_Zone *zone);
EAPI void         e_desk_col_add(E_Zone *zone);
EAPI void         e_desk_col_remove(E_Zone *zone);

extern EAPI int E_EVENT_DESK_SHOW;
extern EAPI int E_EVENT_DESK_BEFORE_SHOW;
extern EAPI int E_EVENT_DESK_AFTER_SHOW;
extern EAPI int E_EVENT_DESK_DESKSHOW;
extern EAPI int E_EVENT_DESK_NAME_CHANGE;

#endif
#endif
