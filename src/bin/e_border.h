/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef enum _E_Icon_Preference
{
   E_ICON_PREF_E_DEFAULT,
   E_ICON_PREF_NETWM,
   E_ICON_PREF_USER
} E_Icon_Preference;

typedef enum _E_Direction
{
   E_DIRECTION_UP,
   E_DIRECTION_DOWN,
   E_DIRECTION_LEFT,
   E_DIRECTION_RIGHT
} E_Direction;

typedef enum _E_Transition
{
   E_TRANSITION_LINEAR,
   E_TRANSITION_SINUSOIDAL,
   E_TRANSITION_ACCELERATE,
   E_TRANSITION_DECELERATE
} E_Transition;

typedef enum _E_Stacking
{
   E_STACKING_NONE,
   E_STACKING_ABOVE,
   E_STACKING_BELOW
} E_Stacking;

typedef enum _E_Focus_Policy
{
   E_FOCUS_CLICK,
   E_FOCUS_MOUSE,
   E_FOCUS_SLOPPY
} E_Focus_Policy;

typedef enum _E_Focus_Setting
{
   E_FOCUS_NONE,
   E_FOCUS_NEW_WINDOW,
   E_FOCUS_NEW_DIALOG,
   E_FOCUS_NEW_DIALOG_IF_OWNER_FOCUSED
} E_Focus_Setting;

typedef enum _E_Maximize
{
   E_MAXIMIZE_NONE       = 0x00000000,
   E_MAXIMIZE_FULLSCREEN = 0x00000001,
   E_MAXIMIZE_SMART      = 0x00000002,
   E_MAXIMIZE_EXPAND     = 0x00000003,
   E_MAXIMIZE_FILL       = 0x00000004,
   E_MAXIMIZE_TYPE       = 0x0000000f,
   E_MAXIMIZE_VERTICAL   = 0x00000010,
   E_MAXIMIZE_HORIZONTAL = 0x00000020,
   E_MAXIMIZE_BOTH       = 0x00000030,
   E_MAXIMIZE_DIRECTION  = 0x000000f0
} E_Maximize;

typedef enum _E_Fullscreen
{
   /* Resize window */
   E_FULLSCREEN_RESIZE,
   /* Change screen resoultion and resize window */
   E_FULLSCREEN_ZOOM
} E_Fullscreen;

typedef enum _E_Window_Placement
{
   E_WINDOW_PLACEMENT_SMART,
   E_WINDOW_PLACEMENT_ANTIGADGET,
   E_WINDOW_PLACEMENT_CURSOR,
   E_WINDOW_PLACEMENT_MANUAL
} E_Window_Placement;

typedef enum _E_Border_Hook_Point
{
   E_BORDER_HOOK_EVAL_PRE_FETCH,
   E_BORDER_HOOK_EVAL_PRE_POST_FETCH,
   E_BORDER_HOOK_EVAL_POST_FETCH,
   E_BORDER_HOOK_EVAL_PRE_BORDER_ASSIGN,
   E_BORDER_HOOK_EVAL_POST_BORDER_ASSIGN,
   E_BORDER_HOOK_EVAL_PRE_NEW_BORDER,
   E_BORDER_HOOK_EVAL_POST_NEW_BORDER,
   E_BORDER_HOOK_EVAL_END,
} E_Border_Hook_Point;

typedef struct _E_Border                     E_Border;
typedef struct _E_Border_Pending_Move_Resize E_Border_Pending_Move_Resize;
typedef struct _E_Border_Hook                E_Border_Hook;
typedef struct _E_Event_Border_Simple        E_Event_Border_Resize;
typedef struct _E_Event_Border_Simple        E_Event_Border_Move;
typedef struct _E_Event_Border_Simple        E_Event_Border_Add;
typedef struct _E_Event_Border_Simple        E_Event_Border_Remove;
typedef struct _E_Event_Border_Simple        E_Event_Border_Show;
typedef struct _E_Event_Border_Simple        E_Event_Border_Hide;
typedef struct _E_Event_Border_Simple        E_Event_Border_Iconify;
typedef struct _E_Event_Border_Simple        E_Event_Border_Uniconify;
typedef struct _E_Event_Border_Simple        E_Event_Border_Stick;
typedef struct _E_Event_Border_Simple        E_Event_Border_Unstick;
typedef struct _E_Event_Border_Zone_Set      E_Event_Border_Zone_Set;
typedef struct _E_Event_Border_Desk_Set      E_Event_Border_Desk_Set;
typedef struct _E_Event_Border_Stack         E_Event_Border_Stack;
typedef struct _E_Event_Border_Simple        E_Event_Border_Icon_Change;
typedef struct _E_Event_Border_Simple        E_Event_Border_Urgent_Change;
typedef struct _E_Event_Border_Simple	     E_Event_Border_Focus_In;
typedef struct _E_Event_Border_Simple        E_Event_Border_Focus_Out;
typedef struct _E_Event_Border_Simple        E_Event_Border_Property;

#else
#ifndef E_BORDER_H
#define E_BORDER_H

#define E_BORDER_TYPE 0xE0b01002

struct _E_Border
{
   E_Object             e_obj_inherit;

   struct {
      struct {
	 int x, y, w, h;
	 int mx, my;
      } current, last_down[3], last_up[3];
   } mouse;
   
   struct {
      struct {
	 int x, y, w, h;
	 int mx, my;
	 int button;
      } down;
   } moveinfo;

   Ecore_X_Window  win;
   int             x, y, w, h;
   int             ref;
   E_Zone         *zone;
   E_Desk         *desk;
   Evas_List      *handlers;

   struct {
      int          x, y;
      struct {
	 int       x, y;
	 double    t;
      } start;
   } fx;
   
   struct {
      int          l, r, t, b;
   } client_inset;

   Ecore_Evas     *bg_ecore_evas;
   Evas           *bg_evas;
   Ecore_X_Window  bg_win;
   Ecore_X_Window  bg_subwin;
   Evas_Object    *bg_object;
   Evas_Object    *icon_object;
   Ecore_X_Window  event_win;
   const char     *internal_icon;
   const char     *internal_icon_key;

   struct {
      Ecore_X_Window shell_win;
      Ecore_X_Window win;
      
      int x, y, w, h;
      
      struct {
	 unsigned char changed : 1;
	 unsigned char user_selected : 1;
	 const char *name;
      } border;
      
      unsigned char shaped : 1;
      unsigned char argb : 1;
      
      /* ICCCM */
      struct {
	 char *title;
	 char *name;
	 char *class;
	 char *icon_name;
	 char *machine;
	 int min_w, min_h;
	 int max_w, max_h;
	 int base_w, base_h;
	 int step_w, step_h;
	 int start_x, start_y;
	 double min_aspect, max_aspect;
	 Ecore_X_Window_State_Hint initial_state;
	 Ecore_X_Window_State_Hint state;
	 Ecore_X_Pixmap icon_pixmap;
	 Ecore_X_Pixmap icon_mask;
	 Ecore_X_Window icon_window;
	 Ecore_X_Window window_group;
	 Ecore_X_Window transient_for;
	 Ecore_X_Window client_leader;
	 Ecore_X_Gravity gravity;
	 char *window_role;
	 unsigned char take_focus : 1;
	 unsigned char accepts_focus : 1;
	 unsigned char urgent : 1;
	 unsigned char delete_request : 1;
	 unsigned char request_pos : 1;
	 struct {
	    int argc;
	    char **argv;
	 } command;
	 struct {
	    unsigned char title : 1;
	    unsigned char name_class : 1;
	    unsigned char icon_name : 1;
	    unsigned char machine : 1;
	    unsigned char hints : 1;
	    unsigned char size_pos_hints : 1;
	    unsigned char protocol : 1;
	    unsigned char transient_for : 1;
	    unsigned char client_leader : 1;
	    unsigned char window_role : 1;
	    unsigned char state : 1;
	    unsigned char command : 1;
	 } fetch;
      } icccm;

      /* MWM */
      struct {
	 Ecore_X_MWM_Hint_Func func;
	 Ecore_X_MWM_Hint_Decor decor;
	 Ecore_X_MWM_Hint_Input input;
	 unsigned char exists : 1;
	 unsigned char borderless : 1;
	 struct {
	    unsigned char hints : 1;
	 } fetch;
      } mwm;

     /* NetWM */
      struct {
	 pid_t         pid;
	 unsigned int  desktop;
	 char         *name;
	 char         *icon_name;
	 Ecore_X_Icon *icons;
	 int           num_icons;
	 unsigned int  user_time;
	 struct {
	    int left;
	    int right;
	    int top;
	    int bottom;
	    int left_start_y;
	    int left_end_y;
	    int right_start_y;
	    int right_end_y;
	    int top_start_x;
	    int top_end_x;
	    int bottom_start_x;
	    int bottom_end_x;
	 } strut;
	 unsigned char ping : 1;
	 struct {
	    unsigned char        request : 1;
	    unsigned int         wait;
	    Ecore_X_Sync_Alarm   alarm;
	    Ecore_X_Sync_Counter counter;
	    unsigned int         serial;
	    double               send_time;
	 } sync;

	 /* NetWM Window state */
	 struct {
	    unsigned char modal : 1;
	    unsigned char sticky : 1;
	    unsigned char maximized_v : 1;
	    unsigned char maximized_h : 1;
	    unsigned char shaded : 1;
	    unsigned char skip_taskbar : 1;
	    unsigned char skip_pager : 1;
	    unsigned char hidden : 1;
	    unsigned char fullscreen : 1;
	    E_Stacking    stacking;
	 } state;

	 /* NetWM Window allowed actions */
         struct {
            unsigned char move : 1;
            unsigned char resize : 1;
            unsigned char minimize : 1;
            unsigned char shade : 1;
            unsigned char stick : 1;
            unsigned char maximized_h : 1;
            unsigned char maximized_v : 1;
            unsigned char fullscreen : 1;
            unsigned char change_desktop : 1;
            unsigned char close : 1;
         } action;

	 Ecore_X_Window_Type type;
	 Ecore_X_Window_Type *extra_types;
	 int extra_types_num;
	 int startup_id;

	 struct {
	    unsigned char name : 1;
	    unsigned char icon_name : 1;
	    unsigned char icon : 1;
	    unsigned char user_time : 1;
	    unsigned char strut : 1;
	    unsigned char type : 1;
	    unsigned char state : 1;
	    /* No, fetch on new_client, shouldn't be changed after map.
	    unsigned char pid : 1;
	    */
	    /* No, ignore this
	    unsigned char desktop : 1;
	    */
	 } fetch;

	 struct {
	    unsigned char state : 1;
	 } update;
      } netwm;

      /* Extra e stuff */
      struct {
	 struct {
	    unsigned char centered : 1;
	 } state;

	 struct {
	    unsigned char state : 1;
	 } fetch;
      } e;
      
      struct {
	 struct {
	    unsigned char soft_menu : 1;
	    unsigned char soft_menus : 1;
	 } fetch;
	 
	 unsigned char soft_menu : 1;
	 unsigned char soft_menus : 1;
      } qtopia;
      struct {
	 struct {
	    unsigned char state : 1;
	    unsigned char vkbd : 1;
	 } fetch;
	 Ecore_X_Atom state;
	 unsigned char vkbd : 1;
      } vkbd;

      Ecore_X_Window_Attributes initial_attributes;
   } client;
   
   E_Container_Shape *shape;
   
   unsigned int    visible : 1;
   unsigned int    await_hide_event;
   unsigned int    moving : 1;
   unsigned int    focused : 1;
   unsigned int    new_client : 1;
   unsigned int    re_manage : 1;
   unsigned int    placed : 1;
   unsigned int    shading : 1;
   unsigned int    shaded : 1;
   unsigned int    iconic : 1;
   unsigned int    deskshow : 1;
   unsigned int    sticky : 1;
   unsigned int    shaped : 1;
   unsigned int    need_shape_merge : 1;
   unsigned int    need_shape_export : 1;
   unsigned int    fullscreen : 1;
   unsigned int    need_fullscreen : 1;
   unsigned int    already_unparented : 1;
   unsigned int    need_reparent : 1;
   unsigned int    button_grabbed : 1;
   unsigned int    delete_requested : 1;
   unsigned int    ping_ok : 1;
   unsigned int    hung : 1;
   unsigned int    take_focus : 1;
   unsigned int    want_focus : 1;
   unsigned int    user_skip_winlist : 1;
   unsigned int    need_maximize : 1;
   E_Maximize      maximized;
   unsigned int    borderless : 1;
   const char     *bordername;

   unsigned int    lock_user_location : 1; /*DONE*/
   unsigned int    lock_client_location : 1; /*DONE*/
   unsigned int    lock_user_size : 1; /*DONE*/
   unsigned int    lock_client_size : 1; /*DONE*/
   unsigned int    lock_user_stacking : 1; /*DONE*/
   unsigned int    lock_client_stacking : 1; /*DONE*/
   unsigned int    lock_user_iconify : 1; /*DONE*/
   unsigned int    lock_client_iconify : 1; /*DONE*/
   unsigned int    lock_user_desk : 1;
   unsigned int    lock_client_desk : 1;
   unsigned int    lock_user_sticky : 1; /*DONE*/
   unsigned int    lock_client_sticky : 1; /*DONE*/
   unsigned int    lock_user_shade : 1; /*DONE*/
   unsigned int    lock_client_shade : 1; /*DONE*/
   unsigned int    lock_user_maximize : 1; /*DONE*/
   unsigned int    lock_client_maximize : 1; /*DONE*/
   unsigned int    lock_user_fullscreen : 1; /*DONE*/
   unsigned int    lock_client_fullscreen : 1; /*DONE*/
   unsigned int    lock_border : 1; /*DONE*/
   unsigned int    lock_close : 1; /*DONE*/
   unsigned int    lock_focus_in : 1; /*DONE*/
   unsigned int    lock_focus_out : 1; /*DONE*/
   unsigned int    lock_life : 1; /*DONE*/

   unsigned int    internal : 1;
   unsigned int    internal_no_remember : 1;
   unsigned int    stolen : 1;
   
   Ecore_Evas     *internal_ecore_evas;
   
   double          ping;
 
   unsigned char   changed : 1;
  
   unsigned char   icon_preference; 
   unsigned char   ignore_first_unmap;
   unsigned char   resize_mode;
   
   struct {
      int x, y, w, h;
      unsigned int layer;
      int zone;
   } saved;

   struct {
      unsigned char valid : 1;
      int x, y, w, h;
      struct {
	 int x, y, w, h;
      } saved;
   } pre_res_change;
   
   struct {
      double start;
      double val;
      int x, y;
      E_Direction dir;
      Ecore_Animator *anim;
   } shade;
   
   struct {
      int x, y;
      int modified;
   } shelf_fix;

   Evas_List *stick_desks;
   E_Menu *border_menu;
   E_Config_Dialog *border_locks_dialog;
   E_Config_Dialog *border_remember_dialog;
   E_Config_Dialog *border_border_dialog;
   E_Dialog *border_prop_dialog;
   Evas_List *pending_move_resize;
   
   struct {
      unsigned char visible : 1;
      unsigned char pos : 1;
      unsigned char size : 1;
      unsigned char stack : 1;
      unsigned char prop : 1;
      unsigned char border : 1;
      unsigned char reset_gravity : 1;
      unsigned char shading : 1;
      unsigned char shaded : 1;
      unsigned char shape : 1;
      unsigned char icon : 1;
   } changes;

   struct {
      unsigned char start : 1;
      int x, y;
   } drag;

   unsigned int layer;
   E_Action *cur_mouse_action;
   Ecore_Timer *raise_timer;
   Ecore_Timer *ping_poller;
   Ecore_Timer *kill_timer;
   int shape_rects_num;
   Ecore_X_Rectangle *shape_rects;
   E_Remember *remember;

   E_Border *modal;

   E_Border  *leader;
   Evas_List *group;

   E_Border  *parent;
   Evas_List *transients;

   Efreet_Desktop *desktop;
   E_Pointer *pointer;
   
   unsigned char post_move   : 1;
   unsigned char post_resize : 1;
   unsigned char post_show : 1;
   Ecore_Idle_Enterer *post_job;
};

struct _E_Border_Pending_Move_Resize 
{
   int x, y, w, h;
   unsigned char move : 1;
   unsigned char resize : 1;
};

struct _E_Border_Hook
{
   E_Border_Hook_Point   hookpoint;
   void                (*func) (void *data, E_Border *bd);
   void                 *data;
   unsigned char         delete_me : 1;
};

struct _E_Event_Border_Simple
{
   E_Border *border;
};

struct _E_Event_Border_Zone_Set
{
   E_Border *border;
   E_Zone   *zone;
};

struct _E_Event_Border_Desk_Set
{
   E_Border *border;
   E_Desk   *desk;
};

struct _E_Event_Border_Stack
{
   E_Border *border, *stack;
   E_Stacking type;
};

EAPI int       e_border_init(void);
EAPI int       e_border_shutdown(void);

EAPI E_Border *e_border_new(E_Container *con, Ecore_X_Window win, int first_map, int internal);
EAPI void      e_border_free(E_Border *bd);
EAPI void      e_border_ref(E_Border *bd);
EAPI void      e_border_unref(E_Border *bd);
EAPI void      e_border_res_change_geometry_save(E_Border *bd);
EAPI void      e_border_res_change_geometry_restore(E_Border *bd);
    
EAPI void      e_border_zone_set(E_Border *bd, E_Zone *zone);
EAPI void      e_border_desk_set(E_Border *bd, E_Desk *desk);
EAPI void      e_border_show(E_Border *bd);
EAPI void      e_border_hide(E_Border *bd, int manage);
EAPI void      e_border_move(E_Border *bd, int x, int y);
EAPI void      e_border_fx_offset(E_Border *bd, int x, int y);
EAPI void      e_border_resize(E_Border *bd, int w, int h);
EAPI void      e_border_move_resize(E_Border *bd, int x, int y, int w, int h);
EAPI void      e_border_layer_set(E_Border *bd, int layer);
EAPI void      e_border_raise(E_Border *bd);
EAPI void      e_border_lower(E_Border *bd);
EAPI void      e_border_stack_above(E_Border *bd, E_Border *above);
EAPI void      e_border_stack_below(E_Border *bd, E_Border *below);
EAPI void      e_border_focus_latest_set(E_Border *bd);
EAPI void      e_border_raise_latest_set(E_Border *bd);
EAPI void      e_border_focus_set_with_pointer(E_Border *bd);
EAPI void      e_border_focus_set(E_Border *bd, int focus, int set);
EAPI void      e_border_shade(E_Border *bd, E_Direction dir);
EAPI void      e_border_unshade(E_Border *bd, E_Direction dir);
EAPI void      e_border_maximize(E_Border *bd, E_Maximize max);
EAPI void      e_border_unmaximize(E_Border *bd, E_Maximize max);
EAPI void      e_border_fullscreen(E_Border *bd, E_Fullscreen policy);
EAPI void      e_border_unfullscreen(E_Border *bd);
EAPI void      e_border_iconify(E_Border *bd);
EAPI void      e_border_uniconify(E_Border *bd);
EAPI void      e_border_stick(E_Border *bd);
EAPI void      e_border_unstick(E_Border *bd);
EAPI void      e_border_pinned_set(E_Border *bd, int set);

EAPI E_Border *e_border_find_by_client_window(Ecore_X_Window win);
EAPI E_Border *e_border_find_by_frame_window(Ecore_X_Window win);
EAPI E_Border *e_border_find_by_window(Ecore_X_Window win);
EAPI E_Border *e_border_find_by_alarm(Ecore_X_Sync_Alarm alarm);
EAPI E_Border *e_border_focused_get(void);

EAPI void      e_border_idler_before(void);

EAPI Evas_List *e_border_client_list(void);

EAPI void e_border_act_move_begin(E_Border *bd, Ecore_X_Event_Mouse_Button_Down *ev);
EAPI void e_border_act_move_end(E_Border *bd, Ecore_X_Event_Mouse_Button_Up *ev);
EAPI void e_border_act_resize_begin(E_Border *bd, Ecore_X_Event_Mouse_Button_Down *ev);
EAPI void e_border_act_resize_end(E_Border *bd, Ecore_X_Event_Mouse_Button_Up *ev);
EAPI void e_border_act_menu_begin(E_Border *bd, Ecore_X_Event_Mouse_Button_Down *ev, int key);
EAPI void e_border_act_close_begin(E_Border *bd);
EAPI void e_border_act_kill_begin(E_Border *bd);

EAPI Evas_Object *e_border_icon_add(E_Border *bd, Evas *evas);

EAPI void e_border_button_bindings_ungrab_all(void);
EAPI void e_border_button_bindings_grab_all(void);

EAPI Evas_List *e_border_focus_stack_get(void);
EAPI Evas_List *e_border_lost_windows_get(E_Zone *zone);

EAPI void e_border_ping(E_Border *bd);
EAPI void e_border_move_cancel(void);
EAPI void e_border_resize_cancel(void);
EAPI void e_border_frame_recalc(E_Border *bd);
EAPI Evas_List *e_border_immortal_windows_get(void);

EAPI const char *e_border_name_get(E_Border *bd);

EAPI void e_border_signal_move_begin(E_Border *bd, const char *sig, const char *src);
EAPI void e_border_signal_move_end(E_Border *bd, const char *sig, const char *src);
EAPI int  e_border_resizing_get(E_Border *bd);
EAPI void e_border_signal_resize_begin(E_Border *bd, const char *dir, const char *sig, const char *src);
EAPI void e_border_signal_resize_end(E_Border *bd, const char *dir, const char *sig, const char *src);
EAPI void e_border_resize_limit(E_Border *bd, int *w, int *h);

EAPI E_Border_Hook *e_border_hook_add(E_Border_Hook_Point hookpoint, void (*func) (void *data, E_Border *bd), void *data);
EAPI void e_border_hook_del(E_Border_Hook *bh);
EAPI void e_border_focus_track_freeze(void);
EAPI void e_border_focus_track_thaw(void);

EAPI E_Border *e_border_under_pointer_get(E_Desk *desk, E_Border *exclude);
EAPI int e_border_pointer_warp_to_center(E_Border *bd);

extern EAPI int E_EVENT_BORDER_RESIZE;
extern EAPI int E_EVENT_BORDER_MOVE;
extern EAPI int E_EVENT_BORDER_ADD;
extern EAPI int E_EVENT_BORDER_SHOW;
extern EAPI int E_EVENT_BORDER_HIDE;
extern EAPI int E_EVENT_BORDER_REMOVE;
extern EAPI int E_EVENT_BORDER_ICONIFY;
extern EAPI int E_EVENT_BORDER_UNICONIFY;
extern EAPI int E_EVENT_BORDER_STICK;
extern EAPI int E_EVENT_BORDER_UNSTICK;
extern EAPI int E_EVENT_BORDER_ZONE_SET;
extern EAPI int E_EVENT_BORDER_DESK_SET;
extern EAPI int E_EVENT_BORDER_STACK;
extern EAPI int E_EVENT_BORDER_ICON_CHANGE;
extern EAPI int E_EVENT_BORDER_URGENT_CHANGE;
extern EAPI int E_EVENT_BORDER_FOCUS_IN;
extern EAPI int E_EVENT_BORDER_FOCUS_OUT;
extern EAPI int E_EVENT_BORDER_PROPERTY;

#endif
#endif
