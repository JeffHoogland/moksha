/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e_alert.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <X11/Xlib.h>
#include <X11/X.h>
#include <Ecore.h>

/* local subsystem functions */

/* local subsystem globals */
static Display     *dd = NULL;
static char        *title = NULL, *str1 = NULL, *str2 = NULL;
static Font         font = 0;
static XFontStruct *fs = NULL;
static GC           gc = 0;
static Window       win = 0, b1 = 0, b2 = 0;
static int          ww = 320, hh = 240;

/* externally accessible functions */
EAPI int
e_alert_init(const char *disp)
{
   XGCValues            gcv;
   int                  wid, hih, mask;
   XSetWindowAttributes att;
   
   dd = XOpenDisplay(disp);
   if (!dd) return 0;
   font = XLoadFont(dd, "fixed");
   fs = XQueryFont(dd, font);
   
   /* dont i18n this - i dont want gettext doing anything as this is called 
      from a segv */
   title = "Enlightenment Error";
   str1 = "(F1) Recover";
   str2 = "(F2) Exit";
   
   wid = DisplayWidth(dd, DefaultScreen(dd));
   hih = DisplayHeight(dd, DefaultScreen(dd));
   att.background_pixel  = WhitePixel(dd, DefaultScreen(dd));
   att.border_pixel      = BlackPixel(dd, DefaultScreen(dd));
   att.override_redirect = True;
   mask = CWBackPixel | CWBorderPixel | CWOverrideRedirect;
   
   win = XCreateWindow(dd, DefaultRootWindow(dd), 
		       (wid - ww) / 2, (hih - hh) / 2, ww, hh, 0,
		       CopyFromParent, InputOutput,
		       CopyFromParent, mask, &att);

   b1 = XCreateWindow(dd, win, -100, -100, 1, 1, 0, CopyFromParent,
		      InputOutput, CopyFromParent, mask, &att);
   b2 = XCreateWindow(dd, win, -100, -100, 1, 1, 0, CopyFromParent,
		      InputOutput, CopyFromParent, mask, &att);
   XMapWindow(dd, b1);
   XMapWindow(dd, b2);

   gc = XCreateGC(dd, win, 0, &gcv);
   XSetForeground(dd, gc, att.border_pixel);
   XSelectInput(dd, win, KeyPressMask | KeyReleaseMask | ExposureMask);

   return 1;
}

EAPI int
e_alert_shutdown(void)
{
   XDestroyWindow(dd, win);
   XFreeGC(dd, gc);
   XFreeFont(dd, fs);
   XCloseDisplay(dd);
   title = NULL;
   str1 = NULL;
   str2 = NULL;
   dd = NULL;
   font = 0;
   fs = NULL;
   gc = 0;
   return 1;
}

EAPI void
e_alert_show(const char *text)
{
   int                  w, i, j, k;
   char                 line[1024];
   XEvent               ev;
   int                  fw, fh, mh;
   KeyCode              key;
   int                  button;

   if (!text) return;

   if ((!dd) || (!fs))
     {
	fputs(text, stderr);
	fflush(stderr);
	exit(-1);
     }
   
   fh = fs->ascent + fs->descent;
   mh = ((ww - 20) / 2) - 20;

   /* fixed size... */
   w = 20;
   XMoveResizeWindow(dd, b1, w, hh - 15 - fh, mh + 10, fh + 10);
   XSelectInput(dd, b1, ButtonPressMask | ButtonReleaseMask | ExposureMask);
   w = ww - 20 - mh;
   XMoveResizeWindow(dd, b2, w, hh - 15 - fh, mh + 10, fh + 10);
   XSelectInput(dd, b2, ButtonPressMask | ButtonReleaseMask | ExposureMask);

   XMapWindow(dd, win);
   XGrabPointer(dd, win, True, ButtonPressMask | ButtonReleaseMask,
		GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
   XGrabKeyboard(dd, win, False, GrabModeAsync, GrabModeAsync, CurrentTime);
   XSetInputFocus(dd, win, RevertToPointerRoot, CurrentTime);
   XSync(dd, False);
   
   button = 0;
   for (; button == 0;)
     {
	XNextEvent(dd, &ev);
	switch (ev.type)
	  {
	   case KeyPress:
	     key = XKeysymToKeycode(dd, XStringToKeysym("F1"));
	     if (key == ev.xkey.keycode)
	       {
		  button = 1;
		  break;
	       }
	     key = XKeysymToKeycode(dd, XStringToKeysym("F2"));
	     if (key == ev.xkey.keycode)
	       {
		  button = 2;
		  break;
	       }
	     break;
	   case ButtonPress:
	     if (ev.xbutton.window == b1)
	       button = 1;
	     else if (ev.xbutton.window == b2)
	       button = 2;
	     break;
	   case Expose:
	     while (XCheckTypedWindowEvent(dd, ev.xexpose.window, Expose, &ev));
	     
	     /* outline */
	     XDrawRectangle(dd, win, gc, 0, 0, ww - 1, hh - 1);
	     
	     XDrawRectangle(dd, win, gc, 2, 2, ww - 4 - 1, fh + 4 - 1);
	     
	     fw = XTextWidth(fs, title, strlen(title));
	     XDrawString(dd, win, gc, 2 + 2 + ((ww - 4 - 4 - fw) / 2) , 2 + 2 + fs->ascent, title, strlen(title));
	     
	     i = 0;
	     j = 0;
	     k = 2 + fh + 4 + 2;
	     while (text[i])
	       {
		  line[j++] = text[i++];
		  if (line[j - 1] == '\n')
		    {
		       line[j - 1] = 0;
		       j = 0;
		       XDrawString(dd, win, gc, 4, k + fs->ascent, line, strlen(line));
		       k += fh + 2;
		    }
	       }
	     fw = XTextWidth(fs, str1, strlen(str1));
	     XDrawRectangle(dd, b1, gc, 0, 0, mh - 1, fh + 10 - 1);
	     XDrawString(dd, b1, gc, 5 + ((mh - fw) / 2), 5 + fs->ascent, str1, strlen(str1));

	     fw = XTextWidth(fs, str2, strlen(str2));
	     XDrawRectangle(dd, b2, gc, 0, 0, mh - 1, fh + 10 - 1);
	     XDrawString(dd, b2, gc, 5 + ((mh - fw) / 2), 5 + fs->ascent, str2, strlen(str2));
	     
	     XSync(dd, False);
	     break;

	  default:
	     break;
	  }
     }
   XDestroyWindow(dd, win);
   XSync(dd, False);
   
   switch (button)
     {
      case 1:
	ecore_app_restart();
	break;
      case 2:
	exit(-11);
	break;
      default:
	break;
     }
}

/* local subsystem functions */
