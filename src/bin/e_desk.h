#ifdef E_TYPEDEFS

typedef struct _E_Desk E_Desk;
typedef struct _E_Event_Desk_Show E_Event_Desk_Show;
typedef struct _E_Event_Desk_Before_Show E_Event_Desk_Before_Show;
typedef struct _E_Event_Desk_After_Show E_Event_Desk_After_Show;
typedef struct _E_Event_Desk_Name_Change E_Event_Desk_Name_Change;
typedef struct _E_Event_Desk_Window_Profile_Change E_Event_Desk_Window_Profile_Change;

#else
#ifndef E_DESK_H
#define E_DESK_H

#define E_DESK_TYPE 0xE0b01005

struct _E_Desk
{
   E_Object             e_obj_inherit;

   E_Zone              *zone;
   const char          *name;
   const char          *window_profile;
   int                  x, y;
   unsigned char        visible : 1;
   unsigned int         deskshow_toggle : 1;
   int                  fullscreen_borders;

   Evas_Object         *bg_object;

   Ecore_Animator      *animator;
   Eina_Bool            animating;
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

struct _E_Event_Desk_Window_Profile_Change
{
   E_Desk   *desk;
};

EINTERN int          e_desk_init(void);
EINTERN int          e_desk_shutdown(void);
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
#if (ECORE_VERSION_MAJOR > 1) || (ECORE_VERSION_MINOR >= 8)
EAPI void         e_desk_window_profile_set(E_Desk *desk, const char *profile);
EAPI void         e_desk_window_profile_add(int container, int zone, int desk_x, int desk_y, const char *profile);
EAPI void         e_desk_window_profile_del(int container, int zone, int desk_x, int desk_y);
EAPI void         e_desk_window_profile_update(void);
#endif

extern EAPI int E_EVENT_DESK_SHOW;
extern EAPI int E_EVENT_DESK_BEFORE_SHOW;
extern EAPI int E_EVENT_DESK_AFTER_SHOW;
extern EAPI int E_EVENT_DESK_DESKSHOW;
extern EAPI int E_EVENT_DESK_NAME_CHANGE;
#if (ECORE_VERSION_MAJOR > 1) || (ECORE_VERSION_MINOR >= 8)
extern EAPI int E_EVENT_DESK_WINDOW_PROFILE_CHANGE;
#endif

#endif
#endif
