#include "config.h"
#include "e_hack.h"

/* FIXME:
 * * gnome-terminal does this funky thing where a new gnome-temrinal process
 * tries to message an  existing one asking it to create a new terminal. this
 * means none of these properties ever end up on a new term window - in fact
 * only the ones that are on the first term process. we need a way of knowing
 * this, OR making sure no new iwndows other than the first appear with these
 * properties. this also leads to handling splash windows - we might want then
 * the first 2 or 3 windows with it.
 * 
 * we need to discuss this... it's an interesting hack that solves a LOT of
 * things (and that we can maybe in future expand to hacking away at gtk and
 * qt directly)
 * 
 * anyway - for now this is fairly harmless and just adds a property to all
 * top-level app windows
 */

/* prototypes */
static void __e_hack_set_properties(Display *display, Window window);

/* dlopened xlib so we can find the symbols in the real xlib to call them */
static void *lib_xlib = NULL;

/* the function that actually sets the properties on toplevel window */
static void
__e_hack_set_properties(Display *display, Window window)
{
   static Atom a_hack = 0;
   char *env = NULL;
   char buf[4096];
   char buf2[4096];
   uid_t uid;
   pid_t pid, ppid;
   struct utsname ubuf;

   if (!a_hack) a_hack = XInternAtom(display, "__E_HACK", False);
   buf[0] = 0;
   buf[sizeof(buf) - 1] = 0;
   uid = getuid();
   snprintf(buf2, sizeof(buf2), "uid: %i\n", uid);
   strncat(buf, buf2, sizeof(buf) - strlen(buf) - 1);
   pid = getpid();
   snprintf(buf2, sizeof(buf2), "pid: %i\n", pid);
   strncat(buf, buf2, sizeof(buf) - strlen(buf) - 1);
   ppid = getppid();
   snprintf(buf2, sizeof(buf2), "ppid: %i\n", ppid);
   strncat(buf, buf2, sizeof(buf) - strlen(buf) - 1);
   if (!uname(&ubuf))
     {
	snprintf(buf2, sizeof(buf2), "machine_name: %s\n", ubuf.nodename);
	strncat(buf, buf2, sizeof(buf) - strlen(buf) - 1);
     }
   if ((env = getenv("E_LAUNCH_ID")))
     {
	snprintf(buf2, sizeof(buf2), "launch_id: %s\n", env);
	strncat(buf, buf2, sizeof(buf) - strlen(buf) - 1);
     }
   if ((env = getenv("USER")))
     {
	snprintf(buf2, sizeof(buf2), "username: %s\n", env);
	strncat(buf, buf2, sizeof(buf) - strlen(buf) - 1);
     }
   if ((env = getenv("E_DESK")))
     {
	snprintf(buf2, sizeof(buf2), "e_desk: %s\n", env);
	strncat(buf, buf2, sizeof(buf) - strlen(buf) - 1);
     }
   if ((env = getenv("E_ZONE")))
     {
	snprintf(buf2, sizeof(buf2), "e_zone: %s\n", env);
	strncat(buf, buf2, sizeof(buf) - strlen(buf) - 1);
     }
   if ((env = getenv("E_CONTAINER")))
     {
	snprintf(buf2, sizeof(buf2), "e_container: %s\n", env);
	strncat(buf, buf2, sizeof(buf) - strlen(buf) - 1);
     }
   if ((env = getenv("E_MANAGER")))
     {
	snprintf(buf2, sizeof(buf2), "e_manager: %s\n", env);
	strncat(buf, buf2, sizeof(buf) - strlen(buf) - 1);
     }
   XChangeProperty(display, window, a_hack, XA_STRING, 8, PropModeReplace, (unsigned char *)buf, strlen(buf));
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
