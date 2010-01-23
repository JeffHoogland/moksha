/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

static void _e_xinerama_clean(void);
static void _e_xinerama_update(void);
static int _e_xinerama_cb_screen_sort(const void *data1, const void *data2);

static Eina_List *all_screens = NULL;
static Eina_List *chosen_screens = NULL;
static Eina_List *fake_screens = NULL;

EAPI int
e_xinerama_init(void)
{
   _e_xinerama_update();
   return 1;
}

EAPI int
e_xinerama_shutdown(void)
{
   _e_xinerama_clean();
   return 1;
}

EAPI void
e_xinerama_update(void)
{
   _e_xinerama_clean();
   _e_xinerama_update();
}

EAPI const Eina_List *
e_xinerama_screens_get(void)
{
   if (fake_screens) return fake_screens;
   return chosen_screens;
}

EAPI const Eina_List *
e_xinerama_screens_all_get(void)
{
   if (fake_screens) return fake_screens;
   return all_screens;
}

EAPI void
e_xinerama_fake_screen_add(int x, int y, int w, int h)
{
   E_Screen *scr;

   scr = E_NEW(E_Screen, 1);
   scr->screen = eina_list_count(fake_screens);
   scr->escreen = scr->screen;
   scr->x = x;
   scr->y = y;
   scr->w = w;
   scr->h = h;
   fake_screens = eina_list_append(fake_screens, scr);
}

/* local subsystem functions */
static void
_e_xinerama_clean(void)
{
   E_FREE_LIST(all_screens, E_FREE);
   chosen_screens = eina_list_free(chosen_screens);
   E_FREE_LIST(fake_screens, E_FREE);
}

static void
_e_xinerama_update(void)
{
   int n;
   Ecore_X_Window *roots;
   Eina_List *l;
   E_Screen *scr;

   roots = ecore_x_window_root_list(&n);
   if (roots)
     {
	int i;
	int rw, rh;
	Ecore_X_Window root;

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

	     scr = E_NEW(E_Screen, 1);
	     scr->screen = 0;
	     scr->x = 0;
	     scr->y = 0;
	     scr->w = rw;
	     scr->h = rh;
	     all_screens = eina_list_append(all_screens, scr);
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
		       scr = E_NEW(E_Screen, 1);
		       scr->screen = i;
		       scr->x = x;
		       scr->y = y;
		       scr->w = w;
		       scr->h = h;
		       all_screens = eina_list_append(all_screens, scr);
		    }
	       }
	  }
     }
   /* now go through all_screens... and build a list of chosen screens */
   EINA_LIST_FOREACH(all_screens, l, scr)
     {
	Eina_List *ll;
	E_Screen *scr2;
	int add = 1;
	Eina_List *removes;

	removes = NULL;
	/* does this screen intersect with any we have chosen? */
	EINA_LIST_FOREACH(chosen_screens, ll, scr2)
	  {
	     /* if they intersect */
	     if (E_INTERSECTS(scr->x, scr->y, scr->w, scr->h,
			      scr2->x, scr2->y, scr2->w, scr2->h))
	       {
		  int sz, sz2;

		  /* calculate pixel area */
		  sz = scr->w * scr->h;
		  sz2 = scr2->w * scr2->h;
		  /* if the one we already have is bigger, DONT add the new */
		  if (sz > sz2)
		    removes = eina_list_append(removes, scr2);
		  /* add the old to a list to remove */
		  else
		    add = 0;
	       }
	  }
	/* if there are screens to remove - remove them */
	EINA_LIST_FREE(removes, scr2)
	  {
	     chosen_screens = eina_list_remove(chosen_screens, scr2);
	  }
	/* if this screen is to be added, add it */
	if (add)
	  chosen_screens = eina_list_append(chosen_screens, scr);
     }
   chosen_screens = eina_list_sort(chosen_screens,
				   eina_list_count(chosen_screens),
				   _e_xinerama_cb_screen_sort);
   n = 0;
   EINA_LIST_FOREACH(chosen_screens, l, scr)
     {
	printf("E17 INIT: XINERAMA CHOSEN: [%i], %ix%i+%i+%i\n",
	       scr->screen, scr->w, scr->h, scr->x, scr->y);
	scr->escreen = n;
	n++;
     }
}

static int
_e_xinerama_cb_screen_sort(const void *data1, const void *data2)
{
   const E_Screen *scr, *scr2;
   int dif;

   scr = data1;
   scr2 = data2;
   if (scr->x != scr2->x)
     return scr->x - scr2->x;
   else if (scr->y != scr2->y)
     return scr->y - scr2->y;
   else
     {
       dif = (scr2->w * scr2->h) - (scr->w * scr->h);
       if (dif == 0) return scr->screen - scr2->screen;
     }
   return dif;
}
