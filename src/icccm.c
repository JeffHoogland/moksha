#include "debug.h"
#include "e.h"
#include "border.h"
#include "icccm.h"
#include "util.h"

/* Motif window hints */
#define MWM_HINTS_FUNCTIONS           (1L << 0)
#define MWM_HINTS_DECORATIONS         (1L << 1)
#define MWM_HINTS_INPUT_MODE          (1L << 2)
#define MWM_HINTS_STATUS              (1L << 3)

/* bit definitions for MwmHints.functions */
#define MWM_FUNC_ALL            (1L << 0)
#define MWM_FUNC_RESIZE         (1L << 1)
#define MWM_FUNC_MOVE           (1L << 2)
#define MWM_FUNC_MINIMIZE       (1L << 3)
#define MWM_FUNC_MAXIMIZE       (1L << 4)
#define MWM_FUNC_CLOSE          (1L << 5)

/* bit definitions for MwmHints.decorations */
#define MWM_DECOR_ALL                 (1L << 0)
#define MWM_DECOR_BORDER              (1L << 1)
#define MWM_DECOR_RESIZEH             (1L << 2)
#define MWM_DECOR_TITLE               (1L << 3)
#define MWM_DECOR_MENU                (1L << 4)
#define MWM_DECOR_MINIMIZE            (1L << 5)
#define MWM_DECOR_MAXIMIZE            (1L << 6)

/* bit definitions for MwmHints.inputMode */
#define MWM_INPUT_MODELESS                  0
#define MWM_INPUT_PRIMARY_APPLICATION_MODAL 1
#define MWM_INPUT_SYSTEM_MODAL              2
#define MWM_INPUT_FULL_APPLICATION_MODAL    3

#define PROP_MWM_HINTS_ELEMENTS             5

/* Motif window hints */
typedef struct _mwmhints
{
   int flags;
   int functions;
   int decorations;
   int inputMode;
   int status;
}
MWMHints;

void
e_icccm_move_resize(Window win, int x, int y, int w, int h)
{
   D_ENTER;

   ecore_window_send_event_move_resize(win, x, y, w, h);

   D_RETURN;
}

void
e_icccm_send_focus_to(Window win, int takes_focus)
{
   static Atom a_wm_take_focus = 0;
   static Atom a_wm_protocols = 0;
   int msg_focus = 0;
   int *props;
   int size;

   D_ENTER;

   ECORE_ATOM(a_wm_take_focus, "WM_TAKE_FOCUS");
   ECORE_ATOM(a_wm_protocols, "WM_PROTOCOLS");
   
   props = ecore_window_property_get(win, a_wm_protocols, XA_ATOM, &size);
   if (props)
     {
	int i, num;
	
	num = size / sizeof(int);
	for (i = 0; i < num; i++)
	  {
	     if (props[i] == (int)a_wm_take_focus) msg_focus = 1;
	  }
	FREE(props);
     }
   if (takes_focus)
     ecore_focus_to_window(win);
   if (msg_focus)
     {
	unsigned int data[5];
	
	data[0] = a_wm_take_focus;
	data[1] = CurrentTime;
	ecore_window_send_client_message(win, a_wm_protocols, 32, data);
     }

   D_RETURN;
}

void
e_icccm_delete(Window win)
{
   static Atom a_wm_delete_window = 0;
   static Atom a_wm_protocols = 0;
   int *props;
   int size;
   int del_win = 0;
   
   D_ENTER;

   ECORE_ATOM(a_wm_delete_window, "WM_DELETE_WINDOW");
   ECORE_ATOM(a_wm_protocols, "WM_PROTOCOLS");
   
   props = ecore_window_property_get(win, a_wm_protocols, XA_ATOM, &size);
   if (props)
     {
	int i, num;
	
	num = size / sizeof(int);
	for (i = 0; i < num; i++)
	  {
	     if (props[i] == (int)a_wm_delete_window) del_win = 1;
	  }
	FREE(props);
     }
   if (del_win)
     {
	unsigned int data[5];
	
	data[0] = a_wm_delete_window;
	data[1] = CurrentTime;
	ecore_window_send_client_message(win, a_wm_protocols, 32, data);
     }
   else
     {
	ecore_window_kill_client(win);
     }

   D_RETURN;
}

void
e_icccm_state_mapped(Window win)
{
   static Atom a_wm_state = 0;
   unsigned int data[2];
   
   D_ENTER;

   ECORE_ATOM(a_wm_state, "WM_STATE");
   data[0] = NormalState;
   data[1] = 0;
   ecore_window_property_set(win, a_wm_state, a_wm_state, 32, data, 2);

   D_RETURN;
}

void
e_icccm_state_iconified(Window win)
{
   static Atom a_wm_state = 0;
   unsigned int data[2];
   
   D_ENTER;

   ECORE_ATOM(a_wm_state, "WM_STATE");
   data[0] = IconicState;
   data[1] = 0;
   ecore_window_property_set(win, a_wm_state, a_wm_state, 32, data, 2);

   D_RETURN;
}

void
e_icccm_state_withdrawn(Window win)
{
   static Atom a_wm_state = 0;
   unsigned int data[2];
   
   D_ENTER;

   ECORE_ATOM(a_wm_state, "WM_STATE");
   data[0] = WithdrawnState;
   data[1] = 0;
   ecore_window_property_set(win, a_wm_state, a_wm_state, 32, data, 2);

   D_RETURN;
}

void
e_icccm_adopt(Window win)
{
   D_ENTER;

   ecore_window_add_to_save_set(win);

   D_RETURN;
}

void
e_icccm_release(Window win)
{
   D_ENTER;

   ecore_window_del_from_save_set(win);

   D_RETURN;
}

void
e_icccm_get_pos_info(Window win, E_Border *b)
{
   XSizeHints hint;
   int mask;
   
   D_ENTER;

   if (ecore_window_get_wm_size_hints(win, &hint, &mask))
     {
	if ((hint.flags & USPosition) || ((hint.flags & PPosition)))
	  {
	     int x, y, w, h;
	     
	     D("%li %li\n", hint.flags & USPosition, hint.flags & PPosition);
	     b->client.pos.requested = 1;
	     b->client.pos.gravity = NorthWestGravity;
	     if (hint.flags & PWinGravity) 
	       b->client.pos.gravity = hint.win_gravity;
	     x = y = w = h = 0;
	     ecore_window_get_geometry(win, &x, &y, &w, &h);
	     b->client.pos.x = x;
	     b->client.pos.y = y;
	  }
	else
	  {
	     b->client.pos.requested = 0;
	  }
     }

   D_RETURN;
}

void
e_icccm_get_size_info(Window win, E_Border *b)
{
   int base_w, base_h, min_w, min_h, max_w, max_h, grav, step_w, step_h;
   double aspect_min, aspect_max;
   int mask;
   XSizeHints hint;
   
   D_ENTER;

   grav = NorthWestGravity;
   mask = 0;
   min_w = 0;
   min_h = 0;
   max_w = 65535;
   max_h = 65535;
   aspect_min = 0.0;
   aspect_max = 999999.0;
   step_w = 1;
   step_h = 1;
   base_w = 0;
   base_h = 0;
   if (ecore_window_get_wm_size_hints(win, &hint, &mask))
     {
	if (hint.flags & PMinSize)
	  {
	     min_w = hint.min_width;
	     min_h = hint.min_height;
	  }
        if (hint.flags & PMaxSize)
	  {
	     max_w = hint.max_width;
	     max_h = hint.max_height;
	     if (max_w < min_w) max_w = min_w;
	     if (max_h < min_h) max_h = min_h;
	  }
        if (hint.flags & PResizeInc)
	  {
	     step_w = hint.width_inc;
	     step_h = hint.height_inc;
	     if (step_w < 1) step_w = 1;
	     if (step_h < 1) step_h = 1;
	  }
        if (hint.flags & PBaseSize)
	  {
	     base_w = hint.base_width;
	     base_h = hint.base_height;
	     if (base_w > max_w) max_w = base_w;
	     if (base_h > max_h) max_h = base_h;
	  }
	else
	  {
	     base_w = min_w;
	     base_h = min_h;
	  }
        if (hint.flags & PAspect)
	  {
	     if (hint.min_aspect.y > 0)
	       aspect_min = ((double)hint.min_aspect.x) / ((double)hint.min_aspect.y);
	     if (hint.max_aspect.y > 0)
	       aspect_max = ((double)hint.max_aspect.x) / ((double)hint.max_aspect.y);
	  }
     }
   b->client.min.w = min_w;
   b->client.min.h = min_h;
   b->client.max.w = max_w;
   b->client.max.h = max_h;
   b->client.base.w = base_w;
   b->client.base.h = base_h;
   b->client.step.w = step_w;
   b->client.step.h = step_h;
   b->client.min.aspect = aspect_min;
   b->client.max.aspect = aspect_max;
   b->changed = 1;

   D_RETURN;
}

void
e_icccm_get_mwm_hints(Window win, E_Border *b)
{
   static Atom  a_motif_wm_hints = 0;
   MWMHints    *mwmhints;
   int          size;
   
   D_ENTER;

   ECORE_ATOM(a_motif_wm_hints, "_MOTIF_WM_HINTS");
   
   mwmhints = ecore_window_property_get(win, a_motif_wm_hints, a_motif_wm_hints, &size);
   if (mwmhints)
     {
	int num;
	
	num = size / sizeof(int);
	if (num < PROP_MWM_HINTS_ELEMENTS) 
	  {
	     FREE(mwmhints);
	     D_RETURN;
	  }
	if (mwmhints->flags & MWM_HINTS_DECORATIONS)
	  {
	     b->client.border = 0;
	     b->client.handles = 0;
	     b->client.titlebar = 0;
	     if (mwmhints->decorations & MWM_DECOR_ALL)
	       {
		  b->client.border = 1;
		  b->client.handles = 1;
		  b->client.titlebar = 1;
	       }
	     if (mwmhints->decorations & MWM_DECOR_BORDER) b->client.border = 1;
	     if (mwmhints->decorations & MWM_DECOR_RESIZEH)  b->client.handles = 1;
	     if (mwmhints->decorations & MWM_DECOR_TITLE) b->client.titlebar = 1;
	  }
	FREE(mwmhints);
     }

   D_RETURN;
}

void
e_icccm_get_layer(Window win, E_Border *b)
{
   static Atom  a_win_layer = 0;
   int         *props;
   int          size;

   D_ENTER;

   ECORE_ATOM(a_win_layer, "_WIN_LAYER");
   
   props = ecore_window_property_get(win, a_win_layer, XA_CARDINAL, &size);
   if (props)
     {
	int num;
	
	num = size / sizeof(int);
	if (num > 0) b->client.layer = props[0];
	FREE(props);
     }

   D_RETURN;
}

void
e_icccm_get_title(Window win, E_Border *b)
{
   char *title;
   
   D_ENTER;

   title = ecore_window_get_title(win);

   if (b->client.title) 
     {
	if ((title) && (!strcmp(title, b->client.title))) 
	  {
	     FREE(title);
	     D_RETURN;
	  }
	b->changed = 1;
	FREE(b->client.title);
     }
   b->client.title = NULL;
   if (title) b->client.title = title;
   else e_strdup(b->client.title, "No Title");

   D_RETURN;
}


void
e_icccm_get_class(Window win, E_Border *b)
{
   D_ENTER;

   IF_FREE(b->client.name);
   IF_FREE(b->client.class);
   b->client.name = NULL;
   b->client.class = NULL;
   ecore_window_get_name_class(win, &(b->client.name), &(b->client.class));
   if (!b->client.name) e_strdup(b->client.name, "Unknown");
   if (!b->client.class) e_strdup(b->client.class, "Unknown");

   D_RETURN;
}

void
e_icccm_get_hints(Window win, E_Border *b)
{
   D_ENTER;

   ecore_window_get_hints(win, 
			  &(b->client.takes_focus),
			  &(b->client.initial_state), 
			  NULL, NULL, NULL,
			  &(b->client.group));

   D_RETURN;
}

void
e_icccm_get_machine(Window win, E_Border *b)
{
   D_ENTER;

   IF_FREE(b->client.machine);
   b->client.machine = NULL;
   b->client.machine = ecore_window_get_machine(win);

   D_RETURN;
}

void
e_icccm_get_command(Window win, E_Border *b)
{
   D_ENTER;

   IF_FREE(b->client.command);
   b->client.command = NULL;
   b->client.command = ecore_window_get_command(win);

   D_RETURN;
}

void
e_icccm_get_icon_name(Window win, E_Border *b)
{
   D_ENTER;

   IF_FREE(b->client.icon_name);
   b->client.icon_name = NULL;
   b->client.icon_name = ecore_window_get_icon_name(win);

   D_RETURN;
}

void
e_icccm_get_state(Window win, E_Border *b)
{
   D_ENTER;


   D_RETURN;
   UN(win);
   UN(b);
}

void
e_icccm_set_frame_size(Window win, int l, int r, int t, int b)
{
   static Atom  a_e_frame_size = 0;
   int props[4];

   D_ENTER;

   ECORE_ATOM(a_e_frame_size, "_E_FRAME_SIZE");
   props[0] = l;
   props[1] = r;
   props[2] = t;
   props[3] = b;
   ecore_window_property_set(win, a_e_frame_size, XA_CARDINAL, 32, props, 4);

   D_RETURN;
}

void
e_icccm_set_desk_area(Window win, int ax, int ay)
{
   static Atom  a_win_area = 0;
   int props[2];

   D_ENTER;

   ECORE_ATOM(a_win_area, "_WIN_AREA");
   props[0] = ax;
   props[1] = ay;
   ecore_window_property_set(win, a_win_area, XA_CARDINAL, 32, props, 2);

   D_RETURN;
}

void
e_icccm_set_desk_area_size(Window win, int ax, int ay)
{
   static Atom  a_win_area_count = 0;
   int props[2];

   D_ENTER;

   ECORE_ATOM(a_win_area_count, "_WIN_AREA_COUNT");
   props[0] = ax;
   props[1] = ay;
   ecore_window_property_set(win, a_win_area_count, XA_CARDINAL, 32, props, 2);

   D_RETURN;
}

void
e_icccm_set_desk(Window win, int d)
{
   static Atom  a_win_workspace = 0;
   int props[2];

   D_ENTER;

   ECORE_ATOM(a_win_workspace, "_WIN_WORKSPACE");
   props[0] = d;
   ecore_window_property_set(win, a_win_workspace, XA_CARDINAL, 32, props, 1);

   D_RETURN;
}

int
e_icccm_is_shaped(Window win)
{
   int w, h, num;
   int shaped = 1;
   XRectangle *rect;

   D_ENTER;

   ecore_window_get_geometry(win, NULL, NULL, &w, &h);
   rect = ecore_window_get_shape_rectangles(win, &num);

   if (!rect)
     D_RETURN_(1);

   if ((num == 1) && 
       (rect[0].x == 0) && (rect[0].y == 0) &&
       (rect[0].width == w) && (rect[0].height == h))
     shaped = 0;
   XFree(rect);

   D_RETURN_(shaped);
}

void
e_icccm_get_e_hack_launch_id(Window win, E_Border *b)
{
   static Atom  a_e_hack_launch_id = 0;
   int         *props;
   int          size;

   D_ENTER;

   ECORE_ATOM(a_e_hack_launch_id, "_E_HACK_LAUNCH_ID");
   
   props = ecore_window_property_get(win, a_e_hack_launch_id, XA_STRING, &size);
   if (props)
     {
	char *str;
	
	str = NEW(char, size + 1);
	ZERO(str, char, size + 1);
	memcpy(str, props, size);
	b->client.e.launch_id = atoi(str);
	FREE(str);
	FREE(props);
     }
   else
     b->client.e.launch_id = 0;

   D_RETURN;
}

void
e_icccm_handle_property_change(Atom a, E_Border *b)
{
   static Atom  a_wm_normal_hints = 0;
   static Atom  a_motif_wm_hints = 0;
   static Atom  a_wm_name = 0;
   static Atom  a_wm_class = 0;
   static Atom  a_wm_hints = 0;
   static Atom  a_wm_client_machine = 0;
   static Atom  a_wm_command = 0;
   static Atom  a_wm_icon_name = 0;
   static Atom  a_wm_state = 0;
   static Atom  a_e_hack_launch_id = 0;
   
   D_ENTER;

   ECORE_ATOM(a_wm_normal_hints, "WM_NORMAL_HINTS");
   ECORE_ATOM(a_motif_wm_hints, "_MOTIF_WM_HINTS");
   ECORE_ATOM(a_wm_name, "WM_NAME");
   ECORE_ATOM(a_wm_class, "WM_CLASS");
   ECORE_ATOM(a_wm_hints, "WM_HINTS"); 
   ECORE_ATOM(a_wm_client_machine, "WM_CLIENT_MACHINE"); 
   ECORE_ATOM(a_wm_command, "WM_COMMAND"); 
   ECORE_ATOM(a_wm_icon_name, "WM_ICON_NAME"); 
   ECORE_ATOM(a_wm_state, "WM_STATE"); 
   ECORE_ATOM(a_e_hack_launch_id, "_E_HACK_LAUNCH_ID");
   
   if (a == a_wm_normal_hints) e_icccm_get_size_info(b->win.client, b);
   else if (a == a_motif_wm_hints) e_icccm_get_mwm_hints(b->win.client, b);
   else if (a == a_wm_name) e_icccm_get_title(b->win.client, b);
   else if (a == a_wm_class) e_icccm_get_class(b->win.client, b);
   else if (a == a_wm_hints) e_icccm_get_hints(b->win.client, b);
   else if (a == a_wm_client_machine) e_icccm_get_machine(b->win.client, b);
   else if (a == a_wm_command) e_icccm_get_command(b->win.client, b);
   else if (a == a_wm_icon_name) e_icccm_get_icon_name(b->win.client, b);
   else if (a == a_wm_state) e_icccm_get_state(b->win.client, b);
   else if (a == a_e_hack_launch_id) e_icccm_get_e_hack_launch_id(b->win.client, b);
   
   D_RETURN;
}

void
e_icccm_handle_client_message(Ecore_Event_Message *e)
{
   D_ENTER;

   D_RETURN;
   UN(e);
}

void
e_icccm_advertise_e_compat(void)
{
   D_ENTER;


   D_RETURN;
}

void
e_icccm_advertise_mwm_compat(void)
{
   static Atom  a_motif_wm_info = 0;
   int props[2];
   
   D_ENTER;

   ECORE_ATOM(a_motif_wm_info, "_MOTIF_WM_INFO");
   props[0] = 2;
   props[0] = ecore_window_root();
   ecore_window_property_set(0, a_motif_wm_info, a_motif_wm_info, 32, props, 2);   

   D_RETURN;
}

void
e_icccm_advertise_gnome_compat(void)
{
   static Atom  a_win_supporting_wm_check = 0;
   static Atom  a_win_protocols = 0;
   static Atom  a_win_wm_name = 0;
   static Atom  a_win_wm_version = 0;
   static Atom  a_win_layer = 0;
   int props[32];
   Window win;

   D_ENTER;

   ECORE_ATOM(a_win_protocols, "_WIN_PROTOCOLS");
   ECORE_ATOM(a_win_layer, "_WIN_LAYER");
   props[0] = a_win_protocols;
   ecore_window_property_set(0, a_win_protocols, XA_ATOM, 32, props, 1);

   ECORE_ATOM(a_win_wm_name, "_WIN_WM_NAME");
   ecore_window_property_set(0, a_win_wm_name, XA_STRING, 8, "Enlightenment", strlen("Enlightenment"));
   ECORE_ATOM(a_win_wm_version, "_WIN_WM_VERSION");
   ecore_window_property_set(0, a_win_wm_version, XA_STRING, 8, "0.17.0", strlen("0.17.0"));
   
   ECORE_ATOM(a_win_supporting_wm_check, "_WIN_SUPPORTING_WM_CHECK");
   win = ecore_window_override_new(0, 0, 0, 7, 7);
   props[0] = win;
   ecore_window_property_set(win, a_win_supporting_wm_check, XA_CARDINAL, 32, props, 1); 
   ecore_window_property_set(0, a_win_supporting_wm_check, XA_CARDINAL, 32, props, 1); 

   D_RETURN;
}

void
e_icccm_advertise_kde_compat(void)
{
   D_ENTER;


   D_RETURN;
}

void
e_icccm_advertise_net_compat(void)
{
   D_ENTER;


   D_RETURN;
}
