#include "config.h"
#include "e_hack.h"

/* prototypes */
static void __e_hack_set_properties(Display *display, Window window);

/* dlopened xlib so we can find the symbols in the real xlib to call them */
static void *lib_xlib = NULL;

/* the function that actually sets the properties on toplevel window */
static void
__e_hack_set_properties(Display *display, Window window)
{
   static Atom a_launch_id = 0;
   static Atom a_user_id = 0;
   static Atom a_process_id = 0;
   static Atom a_p_process_id = 0;
   static Atom a_machine_name = 0;
   static Atom a_user_name = 0;
   static Atom a_desk = 0;
   static Atom a_zone = 0;
   static Atom a_container = 0;
   static Atom a_manager = 0;
   char *env = NULL;

   if (!a_launch_id)    a_launch_id    = XInternAtom(display, "_E_HACK_LAUNCH_ID", False);
   if (!a_user_id)      a_user_id      = XInternAtom(display, "_E_HACK_USER_ID", False);
   if (!a_process_id)   a_process_id   = XInternAtom(display, "_E_HACK_PROCESS_ID", False);
   if (!a_p_process_id) a_p_process_id = XInternAtom(display, "_E_HACK_PARENT_PROCESS_ID", False);
   if (!a_machine_name) a_machine_name = XInternAtom(display, "_E_HACK_MACHINE_NAME", False);
   if (!a_user_name)    a_user_name    = XInternAtom(display, "_E_HACK_USER_NAME", False);
   if (!a_desk)         a_desk         = XInternAtom(display, "_E_HACK_DESK", False);
   if (!a_zone)         a_zone         = XInternAtom(display, "_E_HACK_ZONE", False);
   if (!a_container)    a_container    = XInternAtom(display, "_E_HACK_CONTAINER", False);
   if (!a_manager)      a_manager      = XInternAtom(display, "_E_HACK_MANAGER", False);

   if ((env = getenv("E_LAUNCH_ID")))
      XChangeProperty(display, window, a_launch_id, XA_STRING, 8, PropModeReplace, (unsigned char *)env, strlen(env));
     {
	uid_t uid;
	pid_t pid, ppid;
	struct utsname ubuf;
	char buf[4096];
	
	uid = getuid();
	pid = getpid();
	ppid = getppid();

	snprintf(buf, sizeof(buf), "%i", uid);
	XChangeProperty(display, window, a_user_id, XA_STRING, 8, PropModeReplace, (unsigned char *)buf, strlen(buf));
	snprintf(buf, sizeof(buf), "%i", pid);
	XChangeProperty(display, window, a_process_id, XA_STRING, 8, PropModeReplace, (unsigned char *)buf, strlen(buf));
	snprintf(buf, sizeof(buf), "%i", ppid);
	XChangeProperty(display, window, a_p_process_id, XA_STRING, 8, PropModeReplace, (unsigned char *)buf, strlen(buf));
	if (!uname(&ubuf))
	  {
	     snprintf(buf, sizeof(buf), "%s", ubuf.nodename);
	     XChangeProperty(display, window, a_machine_name, XA_STRING, 8, PropModeReplace, (unsigned char *)buf, strlen(buf));
	  }
	else
	   XChangeProperty(display, window, a_machine_name, XA_STRING, 8, PropModeReplace, (unsigned char *)" ", 1);
     }
   if ((env = getenv("USER")))
      XChangeProperty(display, window, a_user_name, XA_STRING, 8, PropModeReplace, (unsigned char *)env, strlen(env));
   if ((env = getenv("E_DESK")))
      XChangeProperty(display, window, a_desk, XA_STRING, 8, PropModeReplace, (unsigned char *)env, strlen(env));
   if ((env = getenv("E_ZONE")))
      XChangeProperty(display, window, a_zone, XA_STRING, 8, PropModeReplace, (unsigned char *)env, strlen(env));
   if ((env = getenv("E_CONTAINER")))
      XChangeProperty(display, window, a_container, XA_STRING, 8, PropModeReplace, (unsigned char *)env, strlen(env));
   if ((env = getenv("E_MANAGER")))
      XChangeProperty(display, window, a_manager, XA_STRING, 8, PropModeReplace, (unsigned char *)env, strlen(env));
}

/* XCreateWindow intercept hack */
Window
XCreateWindow(
	      Display *display,
	      Window parent,
	      int x, int y,
	      unsigned int width, unsigned int height,
	      unsigned int border_width,
	      int depth,
	      unsigned int class,
	      Visual *visual,
	      unsigned long valuemask,
	      XSetWindowAttributes *attributes
	      )
{
   static Window (*func)
      (
       Display *display,
       Window parent,
       int x, int y,
       unsigned int width, unsigned int height,
       unsigned int border_width,
       int depth,
       unsigned int class,
       Visual *visual,
       unsigned long valuemask,
       XSetWindowAttributes *attributes
       ) = NULL;
   int i;

   /* find the real Xlib and the real X function */
   if (!lib_xlib) lib_xlib = dlopen("libX11.so", RTLD_GLOBAL | RTLD_LAZY);
   if (!func) func = dlsym (lib_xlib, "XCreateWindow");

   /* multihead screen handling loop */
   for (i = 0; i < ScreenCount(display); i++)
     {
	/* if the window is created as a toplevel window */
	if (parent == RootWindow(display, i))
	  {
	     Window window;
	     
	     /* create it */
	     window = (*func) (display, parent, x, y, width, height, 
				border_width, depth, class, visual, valuemask, 
				attributes);
	     /* set properties */
	     __e_hack_set_properties(display, window);
	     /* return it */
	     return window;
	  }
     }
   /* normal child window - create as usual */
   return (*func) (display, parent, x, y, width, height, border_width, depth,
		   class, visual, valuemask, attributes);
}

/* XCreateSimpleWindow intercept hack */
Window
XCreateSimpleWindow(
		    Display *display,
		    Window parent,
		    int x, int y,
		    unsigned int width, unsigned int height,
		    unsigned int border_width,
		    unsigned long border,
		    unsigned long background
		    )
{
   static Window (*func)
      (
       Display *display,
       Window parent,
       int x, int y,
       unsigned int width, unsigned int height,
       unsigned int border_width,
       unsigned long border,
       unsigned long background
       ) = NULL;
   int i;
   
   /* find the real Xlib and the real X function */
   if (!lib_xlib) lib_xlib = dlopen("libX11.so", RTLD_GLOBAL | RTLD_LAZY);
   if (!func) func = dlsym (lib_xlib, "XCreateSimpleWindow");
   
   /* multihead screen handling loop */
   for (i = 0; i < ScreenCount(display); i++)
     {
	/* if the window is created as a toplevel window */
	if (parent == RootWindow(display, i))
	  {
	     Window window;
	     
	     /* create it */
	     window = (*func) (display, parent, x, y, width, height, 
				border_width, border, background);
	     /* set properties */
	     __e_hack_set_properties(display, window);
	     /* return it */
	     return window;
	  }
     }
   /* normal child window - create as usual */
   return (*func) (display, parent, x, y, width, height, 
		   border_width, border, background);
}

/* XReparentWindow intercept hack */
int
XReparentWindow(
		Display *display,
		Window window,
		Window parent,
		int x, int y
		)
{
   static int (*func)
      (
       Display *display,
       Window window,
       Window parent,
       int x, int y
       ) = NULL;
   int i;
   
   /* find the real Xlib and the real X function */
   if (!lib_xlib) lib_xlib = dlopen("libX11.so", RTLD_GLOBAL | RTLD_LAZY);
   if (!func) func = dlsym (lib_xlib, "XReparentWindow");
   
   /* multihead screen handling loop */
   for (i = 0; i < ScreenCount(display); i++)
     {
	/* if the window is created as a toplevel window */
	if (parent == RootWindow(display, i))
	  {
	     /* set properties */
	     __e_hack_set_properties(display, window);
	     /* reparent it */
	     return (*func) (display, window, parent, x, y);
	  }
     }
   /* normal child window reparenting - reparent as usual */
   return (*func) (display, window, parent, x, y);
}
