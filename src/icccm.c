#include "e.h"
#include "icccm.h"

void
e_icccm_move_resize(Window win, int x, int y, int w, int h)
{
   e_window_send_event_move_resize(win, x, y, w, h);
}

void
e_icccm_delete(Window win)
{
   static Atom a_wm_delete_window = 0;
   static Atom a_wm_protocols = 0;
   int *props;
   int size;
   int del_win = 0;
   
   E_ATOM(a_wm_delete_window, "WM_DELETE_WINDOW");
   E_ATOM(a_wm_protocols, "WM_PROTOCOLS");
   
   props = e_window_property_get(win, a_wm_protocols, XA_ATOM, &size);
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
	e_window_send_client_message(win, a_wm_protocols, 32, data);
     }
   else
     {
	e_window_kill_client(win);
     }
}

void
e_icccm_state_mapped(Window win)
{
   static Atom a_wm_state = 0;
   unsigned int data[2];
   
   E_ATOM(a_wm_state, "WM_STATE");
   data[0] = NormalState;
   data[1] = 0;
   e_window_property_set(win, a_wm_state, a_wm_state, 32, data, 2);
}

void
e_icccm_state_iconified(Window win)
{
   static Atom a_wm_state = 0;
   unsigned int data[2];
   
   E_ATOM(a_wm_state, "WM_STATE");
   data[0] = IconicState;
   data[1] = 0;
   e_window_property_set(win, a_wm_state, a_wm_state, 32, data, 2);
}

void
e_icccm_state_withdrawn(Window win)
{
   static Atom a_wm_state = 0;
   unsigned int data[2];
   
   E_ATOM(a_wm_state, "WM_STATE");
   data[0] = WithdrawnState;
   data[1] = 0;
   e_window_property_set(win, a_wm_state, a_wm_state, 32, data, 2);
}

void
e_icccm_adopt(Window win)
{
   e_window_add_to_save_set(win);
}

void
e_icccm_release(Window win)
{
   e_window_del_from_save_set(win);
}

void
e_icccm_get_size_info(Window win, E_Border *b)
{
   int x, y, w, h;
   int base_w, base_h, min_w, min_h, max_w, max_h, grav, step_w, step_h;
   double aspect_min, aspect_max;
   int mask;
   XSizeHints hint;
   
   x = 0; y = 0; w = 0; h = 0;
   e_window_get_geometry(win, &x, &y, &w, &h);
   
   printf("window at %i %i\n", x, y);
   
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
   if (e_window_get_wm_size_hints(win, &hint, &mask))
     {
	if (hint.flags & PWinGravity) grav = hint.win_gravity;
	if ((hint.flags & USPosition) || ((hint.flags & PPosition)))
	  {
	  }
	else
	  {
	     x = rand()%640;
	     y = rand()%480;	     
	  }
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
   else
     {
	/* get x,y location of client */
	x = rand()%640;
	y = rand()%480;
     }
     {
	int pl, pr, pt, pb;
	
	pl = pr = pt = pb = 0;
	if (b->bits.l) ebits_get_insets(b->bits.l, &pl, &pr, &pt, &pb);
	b->current.requested.x = x - pl;
	b->current.requested.y = y - pt;
	b->current.requested.w = w + pl + pr;
	b->current.requested.h = h + pt + pb;
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
     }
}

void
e_icccm_get_mwm_hints(Window win, E_Border *b)
{
   static Atom  a_motif_wm_hints = 0;
   MWMHints    *mwmhints;
   int          size;
   
   E_ATOM(a_motif_wm_hints, "_MOTIF_WM_HINTS");
   
   mwmhints = e_window_property_get(win, a_motif_wm_hints, a_motif_wm_hints, &size);
   if (mwmhints)
     {
	int i, num;
	
	num = size / sizeof(int);
	if (num < PROP_MWM_HINTS_ELEMENTS) 
	  {
	     FREE(mwmhints);
	     return;
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
}

void
e_icccm_get_layer(Window win, E_Border *b)
{
   static Atom  a_win_layer = 0;
   int         *props;
   int          size;

   E_ATOM(a_win_layer, "_WIN_LAYER");
   
   props = e_window_property_get(win, a_win_layer, XA_CARDINAL, &size);
   if (props)
     {
	int i, num;
	
	num = size / sizeof(int);
	if (num > 0) b->client.layer = props[0];
	FREE(props);
     }
}

void
e_icccm_set_frame_size(Window win, int l, int r, int t, int b)
{
   static Atom  a_e_frame_size = 0;
   int props[4];

   E_ATOM(a_e_frame_size, "_E_FRAME_SIZE");
   props[0] = l;
   props[1] = r;
   props[2] = t;
   props[3] = b;
   e_window_property_set(win, a_e_frame_size, XA_CARDINAL, 32, props, 4);
}

void
e_icccm_set_desk_area(Window win, int ax, int ay)
{
   static Atom  a_win_area = 0;
   int props[2];

   E_ATOM(a_win_area, "_WIN_AREA");
   props[0] = ax;
   props[1] = ay;
   e_window_property_set(win, a_win_area, XA_CARDINAL, 32, props, 2);
}

void
e_icccm_set_desk_area_size(Window win, int ax, int ay)
{
   static Atom  a_win_area_count = 0;
   int props[2];

   E_ATOM(a_win_area_count, "_WIN_AREA_COUNT");
   props[0] = ax;
   props[1] = ay;
   e_window_property_set(win, a_win_area_count, XA_CARDINAL, 32, props, 2);
}

void
e_icccm_set_desk(Window win, int d)
{
   static Atom  a_win_workspace = 0;
   int props[2];

   E_ATOM(a_win_workspace, "_WIN_WORKSPACE");
   props[0] = d;
   e_window_property_set(win, a_win_workspace, XA_CARDINAL, 32, props, 1);
}

void
e_icccm_advertise_e_compat(void)
{
}

void
e_icccm_advertise_mwm_compat(void)
{
   static Atom  a_motif_wm_info = 0;
   int props[2];
   
   E_ATOM(a_motif_wm_info, "_MOTIF_WM_INFO");
   props[0] = 2;
   props[0] = e_window_root();
   e_window_property_set(0, a_motif_wm_info, a_motif_wm_info, 32, props, 2);   
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

   E_ATOM(a_win_protocols, "_WIN_PROTOCOLS");
   E_ATOM(a_win_protocols, "_WIN_LAYER");
   props[0] = a_win_protocols;
   e_window_property_set(0, a_win_protocols, XA_ATOM, 32, props, 1);

   E_ATOM(a_win_wm_name, "_WIN_WM_NAME");
   e_window_property_set(win, a_win_wm_name, XA_STRING, 8, "Enlightenment", strlen("Enlightenment"));
   E_ATOM(a_win_wm_version, "_WIN_WM_VERSION");
   e_window_property_set(win, a_win_wm_version, XA_STRING, 8, "0.17.0", strlen("0.17.0"));
   
   E_ATOM(a_win_supporting_wm_check, "_WIN_SUPPORTING_WM_CHECK");
   win = e_window_override_new(0, 0, 0, 7, 7);
   props[0] = win;
   e_window_property_set(win, a_win_supporting_wm_check, XA_CARDINAL, 32, props, 1); 
   e_window_property_set(0, a_win_supporting_wm_check, XA_CARDINAL, 32, props, 1); 
}

void
e_icccm_advertise_kde_compat(void)
{
}

void
e_icccm_advertise_net_compat(void)
{
}
