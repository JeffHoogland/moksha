/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

static void _e_xinerama_clean(void);
static void _e_xinerama_update(void);
static int _e_xinerama_cb_screen_sort(void *data1, void *data2);

static Evas_List *all_screens = NULL;
static Evas_List *chosen_screens = NULL;
static Evas_List *fake_screens = NULL;

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

EAPI const Evas_List *
e_xinerama_screens_get(void)
{
   if (fake_screens) return fake_screens;
   return chosen_screens;
}

EAPI const Evas_List *
e_xinerama_screens_all_get(void)
{
   if (fake_screens) return fake_screens;
   return all_screens;
}

EAPI void
e_xinerama_fake_screen_add(int x, int y, int w, int h)
{
   E_Screen *scr;

   scr = calloc(1, sizeof(E_Screen));
   scr->screen = evas_list_count(fake_screens);
   scr->escreen = scr->screen;
   scr->x = x;
   scr->y = y;
   scr->w = w;
   scr->h = h;
   fake_screens = evas_list_append(fake_screens, scr);
}

/* local subsystem functions */
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
	chosen_screens = evas_list_remove_list(chosen_screens, chosen_screens);
     }
   while (fake_screens)
     {
	free(fake_screens->data);
	fake_screens = evas_list_remove_list(fake_screens, fake_screens);
     }
}

static void
_e_xinerama_update(void)
{
   int n;
   Ecore_X_Window *roots;
   Evas_List *l;

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
   for (l = all_screens; l; l = l->next)
     {
	Evas_List *ll;
	E_Screen *scr;
	int add = 0;
	Evas_List *removes;

	scr = l->data;
	add = 1;
	removes = NULL;
	/* does this screen intersect with any we have chosen? */
	for (ll = chosen_screens; ll; ll = ll->next)
	  {
	     E_Screen *scr2;

	     scr2 = ll->data;
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
		    removes = evas_list_append(removes, scr2);
		  /* add the old to a list to remove */
		  else
		    add = 0;
	       }
	  }
	/* if there are screens to remove - remove them */
	while (removes)
	  {
	     chosen_screens = evas_list_remove(chosen_screens, removes->data);
	     removes = evas_list_remove_list(removes, removes);
	  }
	/* if this screen is to be added, add it */
	if (add)
	  chosen_screens = evas_list_append(chosen_screens, scr);
     }
   chosen_screens = evas_list_sort(chosen_screens,
				   evas_list_count(chosen_screens),
				   _e_xinerama_cb_screen_sort);
   for (n = 0, l = chosen_screens; l; l = l->next, n++)
     {
        E_Screen *scr;

	scr = l->data;
	printf("E17 INIT: XINERAMA CHOSEN: [%i], %ix%i+%i+%i\n",
	       scr->screen, scr->w, scr->h, scr->x, scr->y);
	scr->escreen = n;
     }
}

static int
_e_xinerama_cb_screen_sort(void *data1, void *data2)
{
   E_Screen *scr, *scr2;
   int dif;

   scr = data1;
   scr2 = data2;
   dif = (scr2->w * scr2->h) - (scr->w * scr->h);
   if (dif == 0) return scr->screen - scr2->screen;
   return dif;
}
