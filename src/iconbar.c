#include "debug.h"
#include "iconbar.h"
#include "util.h"
#include "desktops.h"
#include "border.h"
#include "file.h"
#include "icons.h"

static E_Config_Base_Type *cf_iconbar = NULL;
static E_Config_Base_Type *cf_iconbar_icon = NULL;

static Evas_List iconbars = NULL;

/* internal func (iconbar use only) prototypes */

static void e_ib_bit_down_cb (void *data, Ebits_Object o, char *class, int bt,
			      int x, int y, int ox, int oy, int ow, int oh);
static void e_ib_bit_up_cb (void *data, Ebits_Object o, char *class, int bt,
			    int x, int y, int ox, int oy, int ow, int oh);

static void ib_scroll_timeout (int val, void *data);
static void ib_timeout (int val, void *data);
static void ib_cancel_launch_timeout (int val, void *data);

static void ib_bits_show (void *data);
static void ib_bits_hide (void *data);
static void ib_bits_move (void *data, double x, double y);
static void ib_bits_resize (void *data, double w, double h);
static void ib_bits_raise (void *data);
static void ib_bits_lower (void *data);
static void ib_bits_set_layer (void *data, int l);
static void ib_bits_set_clip (void *data, Evas_Object clip);
static void ib_bits_set_color_class (void *data, char *cc, int r, int g,
				     int b, int a);
static void ib_bits_get_min_size (void *data, double *w, double *h);
static void ib_bits_get_max_size (void *data, double *w, double *h);

static void ib_mouse_in (void *data, Evas _e, Evas_Object _o, int _b, int _x,
			 int _y);
static void ib_mouse_out (void *data, Evas _e, Evas_Object _o, int _b, int _x,
			  int _y);
static void ib_mouse_down (void *data, Evas _e, Evas_Object _o, int _b,
			   int _x, int _y);
static void ib_mouse_up (void *data, Evas _e, Evas_Object _o, int _b, int _x,
			 int _y);
static void ib_mouse_move (void *data, Evas _e, Evas_Object _o, int _b,
			   int _x, int _y);

static void e_iconbar_icon_cleanup (E_Iconbar_Icon * ic);

static void ib_child_handle (Ecore_Event * ev);

/* NB: comments here for illustration & helping people understand E's code */
/* This is a start of the comments. if you feel they are not quite good */
/* as you figure things out and if you think they could be more helpful */
/* please feel free to add to them to make them easier to read and be more */
/* helpful. */

/* static internal - called when iconbar bit has a mouse button pressed */
/* on it */
static void
e_ib_bit_down_cb (void *data, Ebits_Object o, char *class, int bt, int x,
		  int y, int ox, int oy, int ow, int oh)
{
  E_Iconbar *ib;

  D_ENTER;

  ib = (E_Iconbar *) data;
  if (!class)
    D_RETURN;
  if (!strcmp (class, "Scrollbar_Arrow1"))
    ib_scroll_timeout (8, ib);
  else if (!strcmp (class, "Scrollbar_Arrow2"))
    ib_scroll_timeout (-8, ib);
  else if (!strcmp (class, "Scrollbar_Trough"))
    {
    }

  D_RETURN;
  UN (o);
  UN (bt);
  UN (x);
  UN (y);
  UN (ox);
  UN (oy);
  UN (ow);
  UN (oh);

}

/* static internal - called when iconbar bit has a mouse button released */
/* on it */
static void
e_ib_bit_up_cb (void *data, Ebits_Object o, char *class, int bt, int x, int y,
		int ox, int oy, int ow, int oh)
{
  E_Iconbar *ib;

  D_ENTER;

  ib = (E_Iconbar *) data;
  if (!class)
    D_RETURN;
  if (!strcmp (class, "Scrollbar_Arrow1"))
    ib_scroll_timeout (0, ib);
  else if (!strcmp (class, "Scrollbar_Arrow2"))
    ib_scroll_timeout (0, ib);
  else if (!strcmp (class, "Scrollbar_Trough"))
    {
    }

  D_RETURN;
  UN (o);
  UN (bt);
  UN (x);
  UN (y);
  UN (ox);
  UN (oy);
  UN (ow);
  UN (oh);
}

/**
 * e_iconbar_cleanup - Iconbar destructor.
 * @ib: The iconbar to be cleaned up.
 *
 * How do we free these pesky little urchins...
 */
static void
e_iconbar_cleanup (E_Iconbar * ib)
{
  char buf[PATH_MAX];

  D_ENTER;

  /* remove from our list */
  iconbars = evas_list_remove (iconbars, ib);

  /* save scroll position */
  /* tell the view we attached to that somehting in it changed. this way */
  /* the view will now it needs to redraw */
  ib->view->changed = 1;
  /* free up our ebits */
  if (ib->bit)
    ebits_free (ib->bit);
  /* if we have any icons... */
  if (ib->icons)
    {
      Evas_List l;

      /* go thru the list of icon and unref each one.. ie - free it */
      for (l = ib->icons; l; l = l->next)
	{
	  E_Iconbar_Icon *ic;

	  ic = l->data;
	  e_object_unref (E_OBJECT (ic));
	}
      /* free the list itself */
      evas_list_free (ib->icons);
    }
  /* cleaup the clip object */
  if ((ib->view) && (ib->view->evas) && (ib->clip))
    evas_del_object (ib->view->evas, ib->clip);
  /* delete any timers intended to work on  this iconbar */
  snprintf (buf, PATH_MAX, "iconbar_reload:%s", ib->view->name);
  ecore_del_event_timer (buf);
  snprintf (buf, PATH_MAX, "iconbar_scroll:%s", ib->view->name);
  ecore_del_event_timer (buf);

  /* call the destructor of the base class */
  e_object_cleanup (E_OBJECT (ib));


  D_RETURN;
}


/**
 * e_iconbar_init - Init function
 *
 * Initialises the iconbar system
 */
void
e_iconbar_init ()
{
  D_ENTER;

  /* we set up config structure and types so the config system can just */
  /* read a db and dump it right into memory - including lists of stuff */

  /* a new config type - an iconbar icon */
  cf_iconbar_icon = e_config_type_new ();
  /* this is a member of the iconbar icon struct we want the config system */
  /* to get from the db for us. the key is "exec". the type is a string */
  /* the struct memebr is exec. the default value is "". see the config.h */
  /* header for more info */
  E_CONFIG_NODE (cf_iconbar_icon, "exec", E_CFG_TYPE_STR, NULL,
		 E_Iconbar_Icon, exec, 0, 0, "");
  E_CONFIG_NODE (cf_iconbar_icon, "wait", E_CFG_TYPE_INT, NULL,
		 E_Iconbar_Icon, wait, 0, 0, "");
  E_CONFIG_NODE (cf_iconbar_icon, "wait_timeout", E_CFG_TYPE_FLOAT, NULL,
		 E_Iconbar_Icon, wait_timeout, 0, 0, "");
  /* this memebr will be replaced by the relative key path in the db as a */
  /* string */
  E_CONFIG_NODE (cf_iconbar_icon, "image", E_CFG_TYPE_KEY, NULL,
		 E_Iconbar_Icon, image_path, 0, 0, "");

  /* a new config type - in this case the iconbar istelf. the only thing we */
  /* want the config system to do it fill it with iconbar icon members in */
  /* the list */
  cf_iconbar = e_config_type_new ();
  E_CONFIG_NODE (cf_iconbar, "icons", E_CFG_TYPE_LIST, cf_iconbar_icon,
		 E_Iconbar, icons, 0, 0, NULL);
  E_CONFIG_NODE (cf_iconbar, "scroll", E_CFG_TYPE_FLOAT, NULL, E_Iconbar,
		 scroll, 0, 0, NULL);

  ecore_event_filter_handler_add (ECORE_EVENT_CHILD, ib_child_handle);

  D_RETURN;
}

/**
 * e_iconbar_new - Iconbar constructor
 * @v:  The view for which an iconbar is to be constructed
 */
E_Iconbar *
e_iconbar_new (E_View * v)
{
  Evas_List l;
  char buf[PATH_MAX];
  E_Iconbar *ib;

  D_ENTER;

  /* first we want to load the iconbar data itself - ie the config info */
  /* for what icons we have and what they execute */
  snprintf (buf, PATH_MAX, "%s/.e_iconbar.db", v->model->dir);
  /* use the config system to simply load up the db and start making */
  /* structs and lists and stuff for us... we told it how to in init */
  ib = e_config_load (buf, "", cf_iconbar);
  /* flush image cache */
  {
    if (v->evas)
      {
	int size;

	size = evas_get_image_cache (v->evas);
	evas_set_image_cache (v->evas, 0);
	evas_set_image_cache (v->evas, size);
      }
  }
  /* flush edb cached handled */
  e_db_flush ();
  /* no iconbar config loaded ? return NULL */
  if (!ib)
    D_RETURN_ (NULL);

  /* now that the config system has doe the loading. we need to init the */
  /* object and set up ref counts and free method */
  e_object_init (E_OBJECT (ib), (E_Cleanup_Func) e_iconbar_cleanup);

  /* the iconbar needs to know what view it's in */
  ib->view = v;
  /* clip object = NULL */
  ib->clip = NULL;
  /* reset has been scrolled flag */
  ib->has_been_scrolled = 0;

  /* now go thru all the icons that were loaded */
  for (l = ib->icons; l; l = l->next)
    {
      E_Iconbar_Icon *ic;

      ic = l->data;
      /* and init the iocnbar icon object */
      e_object_init (E_OBJECT (ic), (E_Cleanup_Func) e_iconbar_icon_cleanup);

      /* and have the iconbar icon know what iconbar it belongs to */
      ic->iconbar = ib;
    }

  /* now we need to load up a bits file that tells us where in the view the */
  /* iconbar is meant to go. same place. just a slightly different name */
  snprintf (buf, PATH_MAX, "%s/.e_iconbar.bits.db", v->model->dir);
  ib->bit = ebits_load (buf);
  /* we didn't find one? */
  if (!ib->bit)
    {
      /* unref the iconbar (and thus it will get freed and all icons in it */
      e_object_unref (E_OBJECT (ib));
      /* return NULL - no iconbar worth doing here if we don't know where */
      /* to put it */
      D_RETURN_ (NULL);
    }
  ebits_set_classed_bit_callback (ib->bit, "Scrollbar_Arrow1",
				  CALLBACK_MOUSE_DOWN, e_ib_bit_down_cb, ib);
  ebits_set_classed_bit_callback (ib->bit, "Scrollbar_Arrow1",
				  CALLBACK_MOUSE_UP, e_ib_bit_up_cb, ib);
  ebits_set_classed_bit_callback (ib->bit, "Scrollbar_Arrow2",
				  CALLBACK_MOUSE_DOWN, e_ib_bit_down_cb, ib);
  ebits_set_classed_bit_callback (ib->bit, "Scrollbar_Arrow2",
				  CALLBACK_MOUSE_UP, e_ib_bit_up_cb, ib);

  /* add to our list of iconbars */
  iconbars = evas_list_append (iconbars, ib);

  /* aaah. our nicely constructed iconbar data struct with all the goodies */
  /* we need. return it. she's ready for use. */
  D_RETURN_ (ib);
}


/**
 * e_iconbar_icon_cleanup -- Iconbar icon destructor
 * @ic: The icon that is to be freed
 */
static void
e_iconbar_icon_cleanup (E_Iconbar_Icon * ic)
{
  D_ENTER;
  D ("iconbar icon cleanup\n");
  /* if we have an imageobject. nuke it */
  if (ic->image)
    evas_del_object (ic->iconbar->view->evas, ic->image);
  /* cleanup the imlib_image */
  if (ic->imlib_image)
  {
     imlib_context_set_image(ic->imlib_image);
     imlib_free_image();
  }
     
  /* free strings ... if they exist */
  IF_FREE (ic->image_path);
  IF_FREE (ic->exec);
  /* stop the timer for this icon */
  if (ic->hi.timer)
    {
      ecore_del_event_timer (ic->hi.timer);
      FREE (ic->hi.timer);
    }
  if (ic->hi.image)
    evas_del_object (ic->iconbar->view->evas, ic->hi.image);

  if (ic->launch_id_cb)
    {
      e_exec_broadcast_cb_del (ic->launch_id_cb);
      ic->launch_id_cb = NULL;
    }
  if (ic->launch_id)
    {
      char buf[PATH_MAX];

      snprintf (buf, PATH_MAX, "iconbar_launch_wait:%i", ic->launch_id);
      ecore_del_event_timer (buf);
      ic->launch_id = 0;
    }
  /* Call the destructor of the base class */
  e_object_cleanup (E_OBJECT (ic));

  D_RETURN;
}

/**
 * e_iconbar_realize - Iconbar initialization.
 * @ib: The iconbar to initalize
 *
 * Turns an iconbar into more than a
 * structure of config data -- actually create evas objcts
 * we can do something visual with
 */
void
e_iconbar_realize (E_Iconbar * ib)
{
  Evas_List l;

  D_ENTER;

  /* create clip object */
  ib->clip = evas_add_rectangle (ib->view->evas);
  evas_set_color (ib->view->evas, ib->clip, 255, 255, 255, 255);
  /* go thru every icon in the iconbar */
  for (l = ib->icons; l; l = l->next)
    {
      E_Iconbar_Icon *ic;
      char buf[PATH_MAX];

      ic = l->data;
      /* set the path of the image to load to be the iconbar db plus */
      /* the path of the key to the image memebr - that is actually */
      /* a lump of image data inlined in the iconbar db - so the icons */
      /* themselves follow the iconbar wherever it goes */
      snprintf (buf, PATH_MAX, "%s/.e_iconbar.db:%s", ib->view->model->dir,
		ic->image_path);
      /* add the icon image object */
      ic->image = evas_add_image_from_file (ib->view->evas, buf);
      /* add an imlib image so we can save it later */
      ic->imlib_image = imlib_load_image (buf);
      /* clip the icon */
      evas_set_clip (ib->view->evas, ic->image, ib->clip);
      /* set it to be semi-transparent */
      evas_set_color (ib->view->evas, ic->image, 255, 255, 255, 128);
      /* set up callbacks on events - so the ib_* functions will be */
      /* called when the corresponding event happens to the icon */
      evas_callback_add (ib->view->evas, ic->image, CALLBACK_MOUSE_IN,
			 ib_mouse_in, ic);
      evas_callback_add (ib->view->evas, ic->image, CALLBACK_MOUSE_OUT,
			 ib_mouse_out, ic);
      evas_callback_add (ib->view->evas, ic->image, CALLBACK_MOUSE_DOWN,
			 ib_mouse_down, ic);
      evas_callback_add (ib->view->evas, ic->image, CALLBACK_MOUSE_UP,
			 ib_mouse_up, ic);
      evas_callback_add (ib->view->evas, ic->image, CALLBACK_MOUSE_MOVE,
			 ib_mouse_move, ic);
    }
  /* add the ebit we loaded to the evas the iconbar exists in - now the */
  /* ebit is more than just structures as well. */
  ebits_add_to_evas (ib->bit, ib->view->evas);
  /* aaaaaaaaah. the magic of being able to replace a named bit in an ebit */
  /* (in this case we expect a bit called "Icons" to exist - the user will */
  /* have added a bit called this into the ebit to indicate where he/she */
  /* wants icons to go. we basically replace this bit with a virtual set */
  /* of callbacks that ebits will call if this bit is to be moved, resized */
  /* shown, hidden, raised, lowered etc. we provide the callbacks. */
  ebits_set_named_bit_replace (ib->bit, "Icons",
			       ib_bits_show,
			       ib_bits_hide,
			       ib_bits_move,
			       ib_bits_resize,
			       ib_bits_raise,
			       ib_bits_lower,
			       ib_bits_set_layer,
			       ib_bits_set_clip,
			       ib_bits_set_color_class,
			       ib_bits_get_min_size,
			       ib_bits_get_max_size, ib);
  /* now move this ebit to a really high layer.. so its ontop of a lot */
  ebits_set_layer (ib->bit, 10000);
  /* and now call "fix" - i called it fix cause it does a few things... */
  /* but fixes the iconbar so its the size of the view, in the right */
  /* place and arranges the icons in their right spots */
  e_iconbar_fix (ib);

  D_RETURN;
}

/**
 * e_iconbar_get_length - get lenght of the icons in the iconbar
 * @ib: The iconbar for which to fix the geometry
 * 
 * This functionc alculates the length of the iconbar (either horizontal)
 * or vertical - and returns that.
 * 
 */
double
e_iconbar_get_length (E_Iconbar * ib)
{
  double ix, iy, aw, ah;
  double len;
  Evas_List l;

  D_ENTER;

  /* init len */
  len = 0;
  /* find icon area geometry */
  ix = ib->icon_area.x;
  iy = ib->icon_area.y;
  aw = ib->icon_area.w;
  ah = ib->icon_area.h;

  /* loop throught icons */
  for (l = ib->icons; l; l = l->next)
    {
      E_Iconbar_Icon *ic;
      int iw, ih;

      ic = l->data;
      /* find out the original image size (of the image file) */
      evas_get_image_size (ic->iconbar->view->evas, ic->image, &iw, &ih);
      if (aw > ah)		/* horizontal */
	{
	  len += iw;
	}
      else			/* vertical */
	{
	  len += ih;
	}
    }
  /* return length */
  D_RETURN_ (len);
}

/**
 * e_iconbar_fix - iconbar geometry update
 * @ib: The iconbar for which to fix the geometry
 * 
 * This function corrects the geometry and visibility
 * of the iconbar gfx and icons
 */
void
e_iconbar_fix (E_Iconbar * ib)
{
  Evas_List l;
  double ix, iy, aw, ah;

  D_ENTER;

  /* move the ebit to 0,0 */
  ebits_move (ib->bit, 0, 0);
  /* resize it to fill the whole view. the internal geometry of the */
  /* ebit itself will determine where things woudl go. we just tell */
  /* the ebit where in the canvas its allowed to exist */
  ebits_resize (ib->bit, ib->view->size.w, ib->view->size.h);
  /* show it. harmless to do this all the time */
  ebits_show (ib->bit);
  /* tell the view we belong to something may have changed so it can draw */
  ib->view->changed = 1;

  /* the callbacks set up in th ebtis replace will set up what area in */
  /* the canvas icons can exist in. lets extract them here */
  ix = ib->icon_area.x;
  iy = ib->icon_area.y;
  aw = ib->icon_area.w;
  ah = ib->icon_area.h;

  /* if we have icons- show the clipper that will clip them */
  if (ib->icons)
    evas_show (ib->view->evas, ib->clip);
  /* no icons - hide the clipper as it will be a real object */
  else
    evas_hide (ib->view->evas, ib->clip);
  /* move the clip object to fill the icon area */
  evas_move (ib->view->evas, ib->clip, ix, iy);
  evas_resize (ib->view->evas, ib->clip, aw, ah);

  if (aw > ah)			/* horizontal */
    {
      double len;

      len = e_iconbar_get_length (ib);
      if (aw > len)
	{
	  if ((ib->scroll + len) > aw)
	    ib->scroll = aw - len;
	  else if (ib->scroll < 0)
	    ib->scroll = 0;
	}
      else
	{
	  if ((ib->scroll + len) > aw)
	    ib->scroll = aw - len;
	  else if (ib->scroll > 0)
	    ib->scroll = 0;
	}
      ix += ib->scroll;
    }
  else				/* vertical */
    {
      double len;

      len = e_iconbar_get_length (ib);
      if (ah > len)
	{
	  if ((ib->scroll + len) > ah)
	    ib->scroll = ah - len;
	  else if (ib->scroll < 0)
	    ib->scroll = 0;
	}
      else
	{
	  if ((ib->scroll + len) < ah)
	    ib->scroll = ah - len;
	  else if (ib->scroll > 0)
	    ib->scroll = 0;
	}
      iy += ib->scroll;
    }

  /* now go thru all the icons... */
  for (l = ib->icons; l; l = l->next)
    {
      E_Iconbar_Icon *ic;
      int iw, ih;
      double w, h;
      double ox, oy;

      ic = l->data;
      /* find out the original image size (of the image file) */
      evas_get_image_size (ic->iconbar->view->evas, ic->image, &iw, &ih);
      w = iw;
      h = ih;
      ox = 0;
      oy = 0;
      /* if the area to put icons is wider that it is tall... horizonatal */
      /* layout of the icons seems smart */
      if (aw > ah)		/* horizontal */
	{
	  /* if the icon height is bigger than the icon space */
	  if (h > ah)
	    {
	      /* scale the icon down in both directions soit fits */
	      w = (ah * w) / h;
	      h = ah;
	    }
	  /* center the icon vertically if its smaller */
	  ox = 0;
	  oy = (ah - h) / 2;

	  /* set the icons geometry */
	  ic->current.x = ix + ox;
	  ic->current.y = iy + oy;
	  ic->current.w = w;
	  ic->current.h = h;

	  /* advance our position counter to the next spot */
	  ix += w;
	}
      /* taller than it is wide. might be good to be vertical */
      else			/* vertical */
	{
	  /* if theicon width is bigger than the icon space */
	  if (w > aw)
	    {
	      /* scale it down to fit */
	      h = (aw * h) / w;
	      w = aw;
	    }
	  /* center it horizontally */
	  ox = (aw - w) / 2;
	  oy = 0;

	  /* set the icons geometry */
	  ic->current.x = ix + ox;
	  ic->current.y = iy + oy;
	  ic->current.w = w;
	  ic->current.h = h;

	  /* advance out counter to the next spot */
	  iy += h;
	}

      /* now move the icona nd resize it */
      evas_move (ic->iconbar->view->evas, ic->image, ic->current.x,
		 ic->current.y);
      evas_resize (ic->iconbar->view->evas, ic->image, ic->current.w,
		   ic->current.h);
      evas_set_image_fill (ic->iconbar->view->evas, ic->image, 0, 0,
			   ic->current.w, ic->current.h);
    }

  D_RETURN;
}

/**
 * e_iconbar_file_delete - Function to remove a file from an iconbox.
 * @v:    The view in which a file is removed
 * @file: Name of the removed file
 *
 * This function is called whenever a file is deleted from a view.
 */
void
e_iconbar_file_delete (E_View * v, char *file)
{
  D_ENTER;

  /* is the file of interest */
  if ((!strcmp (".e_iconbar.db", file)) ||
      (!strcmp (".e_iconbar.bits.db", file)))
    {
      /* if we have an iconbar.. delete it - because its files have been */
      /* nuked. no need to keep it around. */
      if (v->iconbar)
	{
	  e_object_unref (E_OBJECT (v->iconbar));
	  v->iconbar = NULL;
	}
    }

  D_RETURN;
}

/**
 * e_iconbar_save_out_final - save out final state of iconbar back to disk
 * @ib:   The iconbar
 *
 * This function saves the state of the iconbar to the db it comes from
 */
void
e_iconbar_save_out_final (E_Iconbar * ib)
{
  char buf[PATH_MAX];

  D_ENTER;

  if (ib->view)
    {
      E_DB_File *edb;
      Evas_List l;
      int i;

      snprintf (buf, PATH_MAX, "%s/.e_iconbar.db", ib->view->model->dir);
      D ("%s\n", buf);

      if (ib->changed)
	{
	  D ("ib changed\n") edb = e_db_open (buf);
	  if (edb)
	    {
	      D ("got edb\n");
	      for (l = ib->icons, i = 0; l; l = l->next, i++)
		{
		  E_Iconbar_Icon *ic = l->data;
		  char buf2[PATH_MAX];

		  if (ic)
		    {
		      /* save out exec */
		      snprintf (buf2, PATH_MAX, "/icons/%i/exec", i);
		      D ("set exec: %i\n", i);
		      e_db_str_set (edb, buf2, ic->exec);

		      /* save out image */
		      if (ic->imlib_image)
			{
			  imlib_context_set_image (ic->imlib_image);
			  imlib_image_attach_data_value ("compression", NULL,
							 9, NULL);
			  imlib_image_set_format ("db");

			  snprintf (buf2, PATH_MAX,
				    "%s/.e_iconbar.db:/icons/%i/image",
				    ib->view->model->dir, i);
			  D ("save image\n");
			  imlib_save_image (buf2);
			}
		    }
		}
	      D ("set count\n");
	      e_db_int_set (edb, "/icons/count", i);
	      D ("set scroll\n");
	      e_db_float_set (edb, "/scroll", ib->scroll);
	      D ("close db\n");
	      e_db_close (edb);

	    }
	}

      else
	{
	  E_DB_FLOAT_SET (buf, "/scroll", ib->scroll);
	}
    /*D ("set just_saved\n");
    ib->just_saved = 1;*/
    ib->changed = 0;

    }
  D_RETURN;
}

void
e_iconbar_handle_launch_id (Window win, void *data)
{
  E_Iconbar_Icon *ic;
  E_Border *b;

  ic = (E_Iconbar_Icon *) data;
  b = e_border_find_by_window (win);
  if (!b)
    return;
  if ((ic->launch_id) && (b->client.e.launch_id))
    {
      if (b->client.e.launch_id == ic->launch_id)
	{
	  if (ic->launch_id)
	    {
	      char buf[PATH_MAX];

	      snprintf (buf, PATH_MAX, "iconbar_launch_wait:%i",
			ic->launch_id);
	      ecore_del_event_timer (buf);
	    }
	  ic->launch_id = 0;
	  if (ic->launch_id_cb)
	    {
	      e_exec_broadcast_cb_del (ic->launch_id_cb);
	      ic->launch_id_cb = NULL;
	    }
	  evas_set_color (ic->iconbar->view->evas, ic->image, 255, 255, 255,
			  128);
	  ic->iconbar->view->changed = 1;
	}
    }
}

/* static (internal to iconbar use only) callbacks */

/* scroll timeout. called to continuously scroll when arrow button down */
static void
ib_scroll_timeout (int val, void *data)
{
  E_Iconbar *ib;
  char buf[PATH_MAX];

  D_ENTER;

  /* get our iconbar pointer */
  ib = (E_Iconbar *) data;

  snprintf (buf, PATH_MAX, "iconbar_scroll:%s", ib->view->name);
  if (val == 0)
    ecore_del_event_timer (buf);
  else
    {
      ib->has_been_scrolled = 1;
      ib->scroll += val;
      e_iconbar_fix (ib);
      ecore_add_event_timer (buf, 0.02, ib_scroll_timeout, val, ib);
    }
  D_RETURN;
}

static void
ib_cancel_launch_timeout (int val, void *data)
{
  E_Iconbar_Icon *ic;

  D_ENTER;

  ic = (E_Iconbar_Icon *) data;

  if (ic->launch_id)
    {
      ic->launch_id = 0;
      if (ic->launch_id_cb)
	{
	  e_exec_broadcast_cb_del (ic->launch_id_cb);
	  ic->launch_id_cb = NULL;
	}
      evas_set_color (ic->iconbar->view->evas, ic->image, 255, 255, 255, 128);
      ic->iconbar->view->changed = 1;
    }
  D_RETURN;
  UN (val);
}

/* this timeout is responsible for doing the mouse over animation */
static void
ib_timeout (int val, void *data)
{
  E_Iconbar_Icon *ic;
  double t;

  D_ENTER;

  /* get the iconbar icon we are dealign with */
  ic = (E_Iconbar_Icon *) data;
  /* val <= 0 AND we're hilited ? first call as a timeout handler. */
  if ((val <= 0) && (ic->hilited))
    {
      /* note the "start" time */
      ic->hi.start = ecore_get_time ();
      /* no hilite (animation) image */
      if (!ic->hi.image)
	{
	  char buf[PATH_MAX];

	  /* figure out its path */
	  snprintf (buf, PATH_MAX, "%s/.e_iconbar.db:%s",
		    ic->iconbar->view->model->dir, ic->image_path);
	  /* add it */
	  ic->hi.image = evas_add_image_from_file (ic->iconbar->view->evas,
						   buf);
	  /* put it high up */
	  evas_set_layer (ic->iconbar->view->evas, ic->hi.image, 20000);
	  /* dont allow it to capture any events (enter, leave etc. */
	  evas_set_pass_events (ic->iconbar->view->evas, ic->hi.image, 1);
	  /* show it */
	  evas_show (ic->iconbar->view->evas, ic->hi.image);
	}
      /* start at 0 */
      val = 0;
    }
  /* what time is it ? */
  t = ecore_get_time ();
  if (ic->launch_id)
    {
      evas_set_color (ic->iconbar->view->evas, ic->image, 255, 255, 255, 50);
      if (ic->hi.image)
	evas_set_color (ic->iconbar->view->evas, ic->hi.image, 255, 255, 255,
			0);
    }
  /* if the icon is hilited */
  else if (ic->hilited)
    {
      double x, y, w, h;
      double nw, nh, tt;
      int a;
      double speed;

      /* find out where the original icon image is */
      evas_get_geometry (ic->iconbar->view->evas, ic->image, &x, &y, &w, &h);
      /* tt is the time since we started */
      tt = t - ic->hi.start;
      /* the speed to run at - the less, the faster (ie a loop is 0.5 sec) */
      speed = 0.5;
      /* if we are beyond the time loop.. reset the start time to now */
      if (tt > speed)
	ic->hi.start = t;
      /* limit time to max loop time */
      if (tt > speed)
	tt = speed;
      /* calculate alpha to be invers of time sizne loop start */
      a = (int) (255.0 * (speed - tt));
      /* size is icon size + how far in loop we are */
      nw = w * ((tt / speed) + 1.0);
      nh = h * ((tt / speed) + 1.0);
      /* move the hilite icon to a good spot */
      evas_move (ic->iconbar->view->evas, ic->hi.image,
		 x + ((w - nw) / 2), y + ((h - nh) / 2));
      /* resize it */
      evas_resize (ic->iconbar->view->evas, ic->hi.image, nw, nh);
      /* reset its fill so ti fills its space */
      evas_set_image_fill (ic->iconbar->view->evas, ic->hi.image, 0, 0, nw,
			   nh);
      /* set its fade */
      evas_set_color (ic->iconbar->view->evas, ic->hi.image, 255, 255, 255,
		      a);
      /* incirment our count */
      val++;
    }
  /* if it snot hilited */
  else
    {
      double tt;
      int a;
      double speed;

      /* delete the animation object */
      if (ic->hi.image)
	evas_del_object (ic->iconbar->view->evas, ic->hi.image);
      ic->hi.image = NULL;

      /* if we were pulsating.. reset start timer */
      if (val > 0)
	{
	  ic->hi.start = t;
	  /* val back to 0 */
	  val = 0;
	}
      /* speed of the ramp */
      speed = 1.0;
      /* position on the fade out */
      tt = (t - ic->hi.start) / speed;
      if (tt > 1.0)
	tt = 1.0;
      /* alpha value caluclated on ramp position */
      a = (int) ((double) ((1.0 - tt) * 127.0) + 128.0);
      /* set alpha value */
      evas_set_color (ic->iconbar->view->evas, ic->image, 255, 255, 255, a);
      /* time is at end of ramp.. kill timer */
      if (tt == 1.0)
	{
	  /* free the timer name string */
	  IF_FREE (ic->hi.timer);
	  ic->hi.timer = NULL;
	}
      /* decrement count */
      val--;
    }
  /* if we have a timer name.. rerun the timer in 0.05 */
  if (ic->hi.timer)
    ecore_add_event_timer (ic->hi.timer, 0.05, ib_timeout, val, data);
  /* flag the view that we changed */
  ic->iconbar->view->changed = 1;

  D_RETURN;
}

/* called when an ebits object bit needs to be shown */
static void
ib_bits_show (void *data)
{
  E_Iconbar *ib;
  Evas_List l;

  D_ENTER;

  ib = (E_Iconbar *) data;
  /* show all the icons */
  for (l = ib->icons; l; l = l->next)
    {
      E_Iconbar_Icon *ic;

      ic = l->data;
      evas_show (ic->iconbar->view->evas, ic->image);
    }

  D_RETURN;
}

/* called when an ebit object bit needs to hide */
static void
ib_bits_hide (void *data)
{
  E_Iconbar *ib;
  Evas_List l;

  D_ENTER;

  ib = (E_Iconbar *) data;
  /* hide all the icons */
  for (l = ib->icons; l; l = l->next)
    {
      E_Iconbar_Icon *ic;

      ic = l->data;
      evas_hide (ic->iconbar->view->evas, ic->image);
    }

  D_RETURN;
}

/* called when an ebit objetc bit needs to move */
static void
ib_bits_move (void *data, double x, double y)
{
  E_Iconbar *ib;
  Evas_List l;

  D_ENTER;

  ib = (E_Iconbar *) data;
  /* dont do anything.. just record the geometry. we'll deal with it later */
  ib->icon_area.x = x;
  ib->icon_area.y = y;

  D_RETURN;
  UN (l);
}

/* called when an ebit object bit needs to resize */
static void
ib_bits_resize (void *data, double w, double h)
{
  E_Iconbar *ib;
  Evas_List l;

  D_ENTER;

  ib = (E_Iconbar *) data;
  /* dont do anything.. just record the geometry. we'll deal with it later */
  ib->icon_area.w = w;
  ib->icon_area.h = h;

  D_RETURN;
  UN (l);
}

/* called when the ebits object bit needs to be raised */
static void
ib_bits_raise (void *data)
{
  E_Iconbar *ib;
  Evas_List l;

  D_ENTER;

  ib = (E_Iconbar *) data;
  /* raise all the icons */
  for (l = ib->icons; l; l = l->next)
    {
      E_Iconbar_Icon *ic;

      ic = l->data;
      evas_raise (ic->iconbar->view->evas, ic->image);
    }

  D_RETURN;
}

/* called when the ebits object bit needs to be lowered */
static void
ib_bits_lower (void *data)
{
  E_Iconbar *ib;
  Evas_List l;

  D_ENTER;

  ib = (E_Iconbar *) data;
  /* lower all the icons */
  for (l = ib->icons; l; l = l->next)
    {
      E_Iconbar_Icon *ic;

      ic = l->data;
      evas_lower (ic->iconbar->view->evas, ic->image);
    }

  D_RETURN;
}

/* called when the ebits object bit needs to change layers */
static void
ib_bits_set_layer (void *data, int lay)
{
  E_Iconbar *ib;
  Evas_List l;

  D_ENTER;

  ib = (E_Iconbar *) data;
  /* set the layer for all the icons */
  for (l = ib->icons; l; l = l->next)
    {
      E_Iconbar_Icon *ic;

      ic = l->data;
      evas_set_layer (ic->iconbar->view->evas, ic->image, lay);
    }

  D_RETURN;
}

/* not used... err.. ebits clips for us to the maximum allowed space of */
/* the ebit object bit - dont know why i have this here */
static void
ib_bits_set_clip (void *data, Evas_Object clip)
{
  D_ENTER;


  D_RETURN;
  UN (data);
  UN (clip);
}

/* we arent going to recolor our icons here according to color class */
static void
ib_bits_set_color_class (void *data, char *cc, int r, int g, int b, int a)
{
  D_ENTER;


  D_RETURN;
  UN (data);
  UN (cc);
  UN (r);
  UN (g);
  UN (b);
  UN (a);
}

/* our minimum size for icon space is 0x0 */
static void
ib_bits_get_min_size (void *data, double *w, double *h)
{
  D_ENTER;

  *w = 0;
  *h = 0;

  D_RETURN;
  UN (data);
}

/* our maximum is huge */
static void
ib_bits_get_max_size (void *data, double *w, double *h)
{
  D_ENTER;

  *w = 999999;
  *h = 999999;

  D_RETURN;
  UN (data);
}





/* called on events on icons */

/* called when a mouse goes in on an icon object */
static void
ib_mouse_in (void *data, Evas _e, Evas_Object _o, int _b, int _x, int _y)
{
  E_Iconbar_Icon *ic;

  D_ENTER;

  /* get he iconbaricon pointer from the data member */
  ic = (E_Iconbar_Icon *) data;
  /* set hilited flag */
  ic->hilited = 1;
  /* make it more opaque */
  evas_set_color (ic->iconbar->view->evas, ic->image, 255, 255, 255, 255);
  /* if we havent started an animation timer - start one */
  if (!ic->hi.timer)
    {
      char buf[PATH_MAX];

      /* come up with a unique name for it */
      snprintf (buf, PATH_MAX, "iconbar:%s/%s", ic->iconbar->view->name,
		ic->image_path);
      e_strdup (ic->hi.timer, buf);
      /* call the timeout */
      ib_timeout (0, ic);
    }
  /* tell the view the iconbar is in.. something changed that might mean */
  /* a redraw is needed */
  ic->iconbar->view->changed = 1;


  D_RETURN;
  UN (_e);
  UN (_o);
  UN (_b);
  UN (_x);
  UN (_y);
}

/* called when a mouse goes out of an icon object */
static void
ib_mouse_out (void *data, Evas _e, Evas_Object _o, int _b, int _x, int _y)
{
  E_Iconbar_Icon *ic;

  D_ENTER;

  /* get he iconbaricon pointer from the data member */
  ic = (E_Iconbar_Icon *) data;
  /* unset hilited flag */
  ic->hilited = 0;
  /* tell the view the iconbar is in.. something changed that might mean */
  /* a redraw is needed */
  ic->iconbar->view->changed = 1;

  D_RETURN;
  UN (_e);
  UN (_o);
  UN (_b);
  UN (_x);
  UN (_y);
}

/* called when the mouse goes up on an icon object */
static void
ib_mouse_up (void *data, Evas _e, Evas_Object _o, int _b, int _x, int _y)
{
  E_Iconbar_Icon *ic;

  D_ENTER;

  /* get he iconbaricon pointer from the data member */
  ic = (E_Iconbar_Icon *) data;

  ic->mouse_down = 0;
  /* if we are moving the icon */
  if (ic->moving)
    {
      ic->moving = 0;

      e_iconbar_icon_move (ic, _x, _y);
    }

  /* Otherwise, not moving so execute, etc */
  else
    {
      /* if we're busy launching something.. dont run anything */
      if (ic->launch_id)
	D_RETURN;
      /* run something! */
      if (ic->exec)
	{
	  if (!ic->wait)
	    {
	      if (e_exec_run (ic->exec) < 0)
		{
		  /* FIXME: display error */
		}
	    }
	  else
	    {
	      int id_ret = 0;

	      ic->launch_pid =
		e_exec_in_dir_with_env (ic->exec, e_util_get_user_home (),
					&id_ret, NULL, NULL);
	      if (ic->launch_pid >= 0)
		{
		  ic->launch_id = id_ret;
		  if (id_ret > 0)
		    {
		      char buf[PATH_MAX];

		      ic->launch_id_cb =
			e_exec_broadcast_cb_add (e_iconbar_handle_launch_id,
						 ic);
		      snprintf (buf, PATH_MAX, "iconbar_launch_wait:%i",
				ic->launch_id);
		      if (ic->wait_timeout > 0.0)
			ecore_add_event_timer (buf, ic->wait_timeout,
					       ib_cancel_launch_timeout,
					       ic->launch_id, ic);
		      else
			ecore_add_event_timer (buf, 15.0,
					       ib_cancel_launch_timeout,
					       ic->launch_id, ic);
		      evas_set_color (ic->iconbar->view->evas, ic->image,
				      255, 255, 255, 50);
		      if (ic->hi.image)
			evas_set_color (ic->iconbar->view->evas, ic->hi.image,
					255, 255, 255, 0);
		    }
		}
	    }
	}
    }

  D_RETURN;
  UN (_e);
  UN (_o);
  UN (_b);
  UN (_x);
  UN (_y);
}

/* called when the mouse goes down on an icon object */
static void
ib_mouse_down (void *data, Evas _e, Evas_Object _o, int _b, int _x, int _y)
{
  E_Iconbar_Icon *ic;

  D_ENTER;

  ic = (E_Iconbar_Icon *) data;

  ic->down.x = _x;
  ic->down.y = _y;

  ic->mouse_down = _b;

  D_RETURN;
  UN (data);
  UN (_e);
  UN (_o);
  UN (_b);
  UN (_x);
  UN (_y);
}

/* called when a mouse goes out of an icon object */
static void
ib_mouse_move (void *data, Evas _e, Evas_Object _o, int _b, int _x, int _y)
{
  E_Iconbar_Icon *ic;

  D_ENTER;

  /* get he iconbaricon pointer from the data member */
  ic = (E_Iconbar_Icon *) data;

  if (ic->mouse_down)
    {
      int dx, dy;


      ic->mouse.x = _x;
      ic->mouse.y = _y;

      dx = ic->down.x - ic->mouse.x;
      dy = ic->down.y - ic->mouse.y;


      if (dx > 3 || dx < -3 || dy > 3 || dy < -3)
	{
	  ic->moving = 1;

	  evas_move (ic->iconbar->view->evas, ic->image,
		     ic->mouse.x - (ic->down.x - ic->current.x),
		     ic->mouse.y - (ic->down.y - ic->current.y));
	}

    }

  D_RETURN;
  UN (data);
  UN (_e);
  UN (_o);
  UN (_b);
  UN (_x);
  UN (_y);
}


void
e_iconbar_icon_move (E_Iconbar_Icon *ic, int x, int y)
{
   D_ENTER;

   D("in icon move\n");
  /* if dragged outside remove from list */
  if (x > ic->iconbar->icon_area.x + ic->iconbar->icon_area.w ||
      y > ic->iconbar->icon_area.y + ic->iconbar->icon_area.h)
    {
      evas_list_remove (ic->iconbar->icons, ic);

      /* make the changes */
      e_iconbar_fix (ic->iconbar);

      /* set flag and save */
      ic->iconbar->changed = 1;
      e_iconbar_save_out_final(ic->iconbar); 

      e_object_unref (E_OBJECT (ic));
    }

  /* otherwise move to the correct place in list */
  else
    {
      E_Iconbar_Icon *lic;
      Evas_List l;

      double aw = ic->iconbar->icon_area.w;
      double ah = ic->iconbar->icon_area.h;

      /* before first icon? move to start */
      lic = (E_Iconbar_Icon *) ic->iconbar->icons->data;
      /* horizontal */
      if (aw > ah && x < lic->current.x)
	{
	  ic->iconbar->icons = evas_list_remove (ic->iconbar->icons, ic);
	  ic->iconbar->icons =
	    evas_list_prepend_relative (ic->iconbar->icons, ic, lic);
	}
      /* vertical */
      else if (aw < ah && y < lic->current.y)
	{
	  ic->iconbar->icons = evas_list_remove (ic->iconbar->icons, ic);
	  ic->iconbar->icons =
	    evas_list_prepend_relative (ic->iconbar->icons, ic, lic);
	}

      /* not before first icon, check place among other icons */
      else
	{
	  for (l = ic->iconbar->icons; l; l = l->next)
	    {
	      lic = (E_Iconbar_Icon *) l->data;

	      /* if in same position, skip */
	      if (ic == lic)
		{
		  l = l->next;
		  if (l)
		    lic = (E_Iconbar_Icon *) l->data;
		  else
		    break;
		}
	      /* horizontal */
	      if (aw > ah)
		{
		  /* place before icon */
		  if (x > lic->current.x &&
		      x < lic->current.x + (lic->current.w / 2))
		    {
		      ic->iconbar->icons =
			evas_list_remove (ic->iconbar->icons, ic);
		      ic->iconbar->icons =
			evas_list_prepend_relative (ic->iconbar->icons,
						    ic, lic);
		    }
		  /* place after icon */
		  else if (x < lic->current.x + lic->current.w
			   && x >
			   lic->current.x + (lic->current.w / 2))
		    {
		      ic->iconbar->icons =
			evas_list_remove (ic->iconbar->icons, ic);
		      ic->iconbar->icons =
			evas_list_append_relative (ic->iconbar->icons, ic,
						   lic);
		    }
		  /* after last icon */
		  else if (x > lic->current.x + lic->current.w
			   && l->next == NULL)
		    {
		      ic->iconbar->icons =
			evas_list_remove (ic->iconbar->icons, ic);
		      ic->iconbar->icons =
			evas_list_append_relative (ic->iconbar->icons, ic,
						   lic);
		    }


		}
	      /* vertical */
	      else
		{
		  /* place before icon */
		  if (y > lic->current.y &&
		      y < lic->current.y + (lic->current.h / 2))
		    {
		      ic->iconbar->icons =
			evas_list_remove (ic->iconbar->icons, ic);
		      ic->iconbar->icons =
			evas_list_prepend_relative (ic->iconbar->icons,
						    ic, lic);
		    }
		  /* place after icon */
		  else if (y < lic->current.y + lic->current.h
			   && y >
			   lic->current.y + (lic->current.h / 2))
		    {
		      ic->iconbar->icons =
			evas_list_remove (ic->iconbar->icons, ic);
		      ic->iconbar->icons =
			evas_list_append_relative (ic->iconbar->icons, ic,
						   lic);
		    }
		  /* after last icon */
		  else if (y > lic->current.y + lic->current.h
			   && l->next == NULL)
		    {
		      ic->iconbar->icons =
			evas_list_remove (ic->iconbar->icons, ic);
		      ic->iconbar->icons =
			evas_list_append_relative (ic->iconbar->icons, ic,
						   lic);
		    }
		}
	    }
	}
      /* make the changes */
      e_iconbar_fix (ic->iconbar);

      /* set flag and save */
      ic->iconbar->changed = 1;
      e_iconbar_save_out_final(ic->iconbar); 
/*      ic->iconbar->just_saved = 0;*/
      e_view_ib_reload(ic->iconbar->view);

    }
  D_RETURN;
}


/* called when a dnd drop occurs on an iconbar */
void
e_iconbar_dnd_add_files (E_View *v, E_View *source, int num_files,
			 char **dnd_files)
{
  Evas_List execs = NULL;
  Evas_List l;

  int i;

  D_ENTER;

#if 0
  D("add files: %s\n", source->model->dir);
  for (i = 0; i < num_files; i++)
    {
       char *file = e_file_get_file(strdup(dnd_files[i]));
       E_Icon *ic = e_icon_find_by_file(source, file);
       
       if (ic)
       {
	  D("icon mime.base: %s\n", ic->info.mime.base);
	 if (!strcmp(ic->info.mime.base, "db"))
	 {
	    /* if its an icon db, set the icon */
	   D("db!\n");
	   for (l = v->iconbar->icons; l; l = l->next)
	   {
	      E_Iconbar_Icon *ibic;
	      char buf[PATH_MAX];

	      if (l->data)
		 ibic = (E_Iconbar_Icon *)(l->data);

	      if (ibic)
	      {
  	        if (v->iconbar->dnd.x > ibic->current.x &&
		    v->iconbar->dnd.x < ibic->current.x + ibic->current.w &&
		    v->iconbar->dnd.y > ibic->current.y &&
		    v->iconbar->dnd.y < ibic->current.y + ibic->current.h )
	        {
		  D("over icon: %s\n", ibic->exec);
                  snprintf(buf, PATH_MAX, "%s/%s:/icon/normal", ic->view->model->dir, ic->file);
		  D("set icon: %s\n", buf);

                  ibic->imlib_image = imlib_load_image(buf);
	
		  /* FIXME: this should be cleaner */
		  ibic->iconbar->changed = 1;
	          e_iconbar_save_out_final (ibic->iconbar);
		}
  	      }
	   }
	   break;
	 }
	 else if (e_file_can_exec (&ic->stat))
	 {
	   execs = evas_list_append (execs, ic);
	 }
       }
    }
  for (l = execs; l; l = l->next)
    {
      /* add exec icons */
      E_Icon *ic;
      E_Iconbar_Icon *ibic;
      char buf[PATH_MAX];

      D("now add the icon\n");

      if (l->data)
        ic = l->data;
      else
	D_RETURN;

      ibic = NEW(E_Iconbar_Icon, 1);
      ZERO(ibic, E_Iconbar_Icon, 1);

      e_object_init (E_OBJECT (ibic),
		     (E_Cleanup_Func) e_iconbar_icon_cleanup);
      if (v->iconbar)
        ibic->iconbar = v->iconbar;
      else
	 D("EEEEEEEEEEEEK: how the hell did this happen?");

      D("x: %f, v-dir: %s, ib-dir: %s\n", ibic->iconbar->icon_area.x, v->model->dir,  ibic->iconbar->view->model->dir);
       
      if (!ic->info.icon) D_RETURN;
      snprintf(buf, PATH_MAX, "%s:/icon/normal", ic->info.icon);
      ibic->image = evas_add_image_from_file (v->evas, buf);
      ibic->imlib_image = imlib_load_image (buf);
      ibic->image_path = strdup (ic->info.icon);
      snprintf(buf, PATH_MAX, "%s/%s", ic->view->model->dir, ic->file);
      ibic->exec = strdup(buf);

      evas_set_clip (v->evas, ibic->image, v->iconbar->clip);
      evas_set_color (v->evas, ibic->image, 255, 255, 255, 128);
      evas_set_layer (v->evas, ibic->image, 11000);
      evas_show(v->evas, ibic->image);
      evas_callback_add (v->evas, ibic->image, CALLBACK_MOUSE_IN,
			 ib_mouse_in, ibic);
      evas_callback_add (v->evas, ibic->image, CALLBACK_MOUSE_OUT,
			 ib_mouse_out, ibic);
      evas_callback_add (v->evas, ibic->image, CALLBACK_MOUSE_DOWN,
			 ib_mouse_down, ibic);
      evas_callback_add (v->evas, ibic->image, CALLBACK_MOUSE_UP,
			 ib_mouse_up, ibic);
      evas_callback_add (v->evas, ibic->image, CALLBACK_MOUSE_MOVE,
			 ib_mouse_move, ibic);

      ibic->iconbar->icons = evas_list_append(ibic->iconbar->icons, ibic);

      /* this adds the icon to the correct place in the list and saves */
      e_iconbar_icon_move(ibic, v->iconbar->dnd.x, v->iconbar->dnd.y);
    }

#endif
}


/* called when child processes exit */
static void
ib_child_handle (Ecore_Event * ev)
{
  Ecore_Event_Child *e;
  Evas_List l;

  D_ENTER;

  e = ev->event;
  for (l = iconbars; l; l = l->next)
    {
      E_Iconbar *ib;
      Evas_List ll;

      ib = l->data;
      for (ll = ib->icons; ll; ll = ll->next)
	{
	  E_Iconbar_Icon *ic;

	  ic = ll->data;
	  if (ic->launch_pid == e->pid)
	    {
	      ic->launch_pid = 0;
	      if (ic->launch_id)
		{
		  char buf[PATH_MAX];

		  snprintf (buf, PATH_MAX, "iconbar_launch_wait:%i",
			    ic->launch_id);
		  ecore_del_event_timer (buf);
		}
	      ic->launch_id = 0;
	      if (ic->launch_id_cb)
		{
		  e_exec_broadcast_cb_del (ic->launch_id_cb);
		  ic->launch_id_cb = NULL;
		}
	      evas_set_color (ic->iconbar->view->evas, ic->image, 255,
			      255, 255, 128);
	      ic->iconbar->view->changed = 1;
	      D_RETURN;
	    }
	}
    }

  D_RETURN;
}

E_Rect *
e_iconbar_get_resist_rect (E_Iconbar * ib)
{
  double x, y, w, h;
  int resist = 32;
  E_Rect *r;

  D_ENTER;

  ebits_get_named_bit_geometry (ib->bit, "Resist", &x, &y, &w, &h);

  r = NEW (E_Rect, 1);
  r->x = x;
  r->y = y;
  r->w = w;
  r->h = h;
  r->v1 = resist;

  D_RETURN_ (r);
}

void
e_iconbar_set_view_window_spacing (E_Iconbar * ib)
{
  double x, y, w, h;

  D_ENTER;

  ebits_get_named_bit_geometry (ib->bit, "Resist", &x, &y, &w, &h);

/* FIXME Why do the v->spacing.window.?'s need to be / 2? */
  if (h > w)			/* vertical */
    {
      if (x < ib->view->size.w / 2)	/* left */
	ib->view->spacing.window.l = (x + w) / 2 + 3;
      else			/* right */
	ib->view->spacing.window.r = (ib->view->size.w - x) / 2 + 15;
    }
  else				/* horizontal */
    {
      if (y < ib->view->size.h / 2)	/* top */
	ib->view->spacing.window.t = (y + h) / 2 + 3;
      else
	ib->view->spacing.window.b = (ib->view->size.h - y) / 2 + 15;
    }


  D_RETURN;
}
