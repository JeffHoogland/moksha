/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

static void _e_xinerama_clean(void);
static void _e_xinerama_update(void);

static Evas_List *all_screens = NULL;
static Evas_List *chosen_screens = NULL;

int
e_xinerama_init(void)
{
   _e_xinerama_update();
   return 1;
}

int
e_xinerama_shutdown(void)
{
   _e_xinerama_clean();
   return 1;
}

static void
_e_xinerama_clean(void)
{
   while (all_screens)
     {
	free(all_screens->data);
	all_screens = evas_list_remove_list(all_screens, all_screens);
     }
   while (chosen_screens)
     {
	free(chosen_screens->data);
	chosen_screens = evas_list_remove_list(chosen_screens, chosen_screens);
     }
}

static void
_e_xinerama_update(void)
{
   int i, n;
   Ecore_X_Window root, *roots;
   
   _e_xinerama_clean();
   roots = ecore_x_window_root_list(&n);
   if (roots)
     {
	int rw, rh;
	
	/* more than 1 root window - xinerama wont be active */
	if (n > 1)
	  {
	     free(roots);
	     return;
	  }
	/* first (and only) root window */
	root = roots[0];
	free(roots);
	/* get root size */
	ecore_x_window_size_get(root, &rw, &rh);
	/* get number of xinerama screens */
	n = ecore_x_xinerama_screen_count_get();
	if (n < 1)
	  {
	     E_Screen *scr;
	     
	     scr = calloc(1, sizeof(E_Screen));
	     scr->screen = 0;
	     scr->x = 0;
	     scr->y = 0;
	     scr->w = rw;
	     scr->h = rh;
	     all_screens = evas_list_append(all_screens, scr);
	  }
	else
	  {
	     for (i = 0; i < n; i++)
	       {
		  int x, y, w, h;
		  
		  /* get each xinerama screen geometry */
		  if (ecore_x_xinerama_screen_geometry_get(i, &x, &y, &w, &h))
		    {
		       E_Screen *scr;
		       
		       printf("E17 INIT: XINERAMA SCREEN: [%i], %ix%i+%i+%i\n",
			      i, w, h, x, y);
		       /* add it to our list */
		       scr = calloc(1, sizeof(E_Screen));
		       scr->screen = i;
		       scr->x = x;
		       scr->y = y;
		       scr->w = w;
		       scr->h = h;
		       all_screens = evas_list_append(all_screens, scr);
		    }
	       }
	  }
     }
   /* now go through all_screens... and build a list of chosen screens */
}
