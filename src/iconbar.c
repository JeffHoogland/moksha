#include "iconbar.h"
#include "util.h"

static E_Config_Base_Type *cf_iconbar      = NULL;
static E_Config_Base_Type *cf_iconbar_icon = NULL;

/* internal func (iconbar use only) prototypes */

static void ib_reload_timeout(int val, void *data);
static void ib_timeout(int val, void *data);

static void ib_bits_show(void *data);
static void ib_bits_hide(void *data);
static void ib_bits_move(void *data, double x, double y);
static void ib_bits_resize(void *data, double w, double h);
static void ib_bits_raise(void *data);
static void ib_bits_lower(void *data);
static void ib_bits_set_layer(void *data, int l);
static void ib_bits_set_clip(void *data, Evas_Object clip);
static void ib_bits_set_color_class(void *data, char *cc, int r, int g, int b, int a);
static void ib_bits_get_min_size(void *data, double *w, double *h);
static void ib_bits_get_max_size(void *data, double *w, double *h);

static void ib_mouse_in(void *data, Evas _e, Evas_Object _o, int _b, int _x, int _y);
static void ib_mouse_out(void *data, Evas _e, Evas_Object _o, int _b, int _x, int _y);
static void ib_mouse_down(void *data, Evas _e, Evas_Object _o, int _b, int _x, int _y);
static void ib_mouse_up(void *data, Evas _e, Evas_Object _o, int _b, int _x, int _y);

/* NB: comments here for illustration & helping people understand E's code */
/* This is a start of the comments. if you feel they are not quite good */
/* as you figure things out and if you think they could be more helpful */
/* please feel free to add to them to make them easier to read and be more */
/* helpful. */

/**
 * e_iconbar_init - Init function
 *
 * Initialises the iconbar system
 */
void
e_iconbar_init()
{
   /* we set up config structure and types so the config system can just */
   /* read a db and dump it right into memory - including lists of stuff */
   
   /* a new config type - an iconbar icon */   
   cf_iconbar_icon = e_config_type_new();
   /* this is a member of the iconbar icon struct we want the config system */
   /* to get from the db for us. the key is "exec". the type is a string */
   /* the struct memebr is exec. the default value is "". see the config.h */
   /* header for more info */
   E_CONFIG_NODE(cf_iconbar_icon, "exec",  E_CFG_TYPE_STR, NULL, E_Iconbar_Icon, exec, 0, 0, "");
   /* this memebr will be replaced by the relative key path in the db as a */
   /* string */
   E_CONFIG_NODE(cf_iconbar_icon, "image", E_CFG_TYPE_KEY, NULL, E_Iconbar_Icon, image_path, 0, 0, "");
   
   /* a new config type - in this case the iconbar istelf. the only thing we */
   /* want the config system to do it fill it with iconbar icon members in */
   /* the list */
   cf_iconbar = e_config_type_new();
   E_CONFIG_NODE(cf_iconbar, "icons", E_CFG_TYPE_LIST, cf_iconbar_icon, E_Iconbar, icons, 0, 0, NULL);
}

/**
 * e_iconbar_new - Iconbar constructor
 * @v:  The view for which an iconbar is to be constructed
 */
E_Iconbar *
e_iconbar_new(E_View *v)
{
   Evas_List l;
   char buf[PATH_MAX];
   E_Iconbar *ib;
   
   /* first we want to load the iconbar data itself - ie the config info */
   /* for what icons we have and what they execute */
   sprintf(buf, "%s/.e_iconbar.db", v->dir);   
   /* use the config system to simply load up the db and start making */
   /* structs and lists and stuff for us... we told it how to in init */
   ib = e_config_load(buf, "", cf_iconbar);
   /* flush image cache */
     {
	int size;
	
	size = imlib_get_cache_size();
	imlib_set_cache_size(0);
	imlib_set_cache_size(size);
     }
   /* flush edb cached handled */
   e_db_flush();
   /* no iconbar config loaded ? return NULL */
   if (!ib) return NULL;
   
   /* now that the config system has doe the loading. we need to init the */
   /* object and set up ref counts and free method */
   OBJ_INIT(ib, e_iconbar_free);
   
   /* the iconbar needs to know what view it's in */
   ib->view = v;
   
   /* now go thru all the icons that were loaded */
   for (l = ib->icons; l; l = l->next)
     {
	E_Iconbar_Icon *ic;
	
	ic = l->data;
	/* and init the iocnbar icon object */
	OBJ_INIT(ic, e_iconbar_icon_free);
	/* and have the iconbar icon knwo what iconbar it belongs to */
	ic->iconbar = ib;
     }
   
   /* now we need to load up a bits file that tells us where in the view the */
   /* iconbar is meant to go. same place. just a slightly different name */
   sprintf(buf, "%s/.e_iconbar.bits.db", v->dir);   
   ib->bit = ebits_load(buf);
   /* we didn't find one? */
   if (!ib->bit)
     {
	/* unref the iconbar (and thus it will get freed and all icons in it */
	OBJ_DO_FREE(ib);
	/* return NULL - no iconbar worth doing here if we don't know where */
	/* to put it */
	return NULL;
     }
   /* aaah. our nicely constructed iconbar data struct with all the goodies */
   /* we need. return it. she's ready for use. */
   return ib;
}

/**
 * e_iconbar_free - Iconbar destructor.
 * @ib: The iconbar to be freed
 *
 * How do we free these pesky little urchins...
 */
void
e_iconbar_free(E_Iconbar *ib)
{
   char buf[PATH_MAX];

   /* tell the view we attached to that somehting in it changed. this way */
   /* the view will now it needs to redraw */
   ib->view->changed = 1;
   /* free up our ebits */
   if (ib->bit) ebits_free(ib->bit);
   /* if we have any icons... */
   if (ib->icons)
     {
	Evas_List l;
  
	/* go thru the list of icon and unref each one.. ie - free it */
	for (l = ib->icons; l; l = l->next)
	  {
	     E_Iconbar_Icon *ic;
	     
	     ic = l->data;
	     OBJ_DO_FREE(ic);
	  }
	/* free the list itself */
	evas_list_free(ib->icons);
     }
   /* delete any timers intended to work on  this iconbar */
   sprintf(buf, "iconbar_reload:%s", ib->view->dir);
   ecore_del_event_timer(buf);
   /* free the iconbar struct */
   FREE(ib);
}

/**
 * e_iconbar_icon_free -- Iconbar icon destructor
 * @ic: The icon that is to be freed
 */
void
e_iconbar_icon_free(E_Iconbar_Icon *ic)
{
   /* if we have an imageobject. nuke it */
   if (ic->image) evas_del_object(ic->iconbar->view->evas, ic->image);
   /* free strings ... if they exist */
   IF_FREE(ic->image_path);
   IF_FREE(ic->exec);
   /* stop the timer for this icon */
   if (ic->hi.timer)
     {
	ecore_del_event_timer(ic->hi.timer);
	FREE(ic->hi.timer);
     }
   if (ic->hi.image) evas_del_object(ic->iconbar->view->evas, ic->hi.image);
   /* nuke that struct */
   FREE(ic);
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
e_iconbar_realize(E_Iconbar *ib)
{
   Evas_List l;

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
	sprintf(buf, "%s/.e_iconbar.db:%s", ib->view->dir, ic->image_path);
	/* add the icon image object */
	ic->image = evas_add_image_from_file(ib->view->evas, buf);
	/* set it to be semi-transparent */
	evas_set_color(ib->view->evas, ic->image, 255, 255, 255, 128);
	/* set up callbacks on events - so the ib_* functions will be */
	/* called when the corresponding event happens to the icon */
	evas_callback_add(ib->view->evas, ic->image, CALLBACK_MOUSE_IN, ib_mouse_in, ic);
	evas_callback_add(ib->view->evas, ic->image, CALLBACK_MOUSE_OUT, ib_mouse_out, ic);
	evas_callback_add(ib->view->evas, ic->image, CALLBACK_MOUSE_DOWN, ib_mouse_down, ic);
	evas_callback_add(ib->view->evas, ic->image, CALLBACK_MOUSE_UP, ib_mouse_up, ic);
     }
   /* add the ebit we loaded to the evas the iconbar exists in - now the */
   /* ebit is more than just structures as well. */
   ebits_add_to_evas(ib->bit, ib->view->evas);
   /* aaaaaaaaah. the magic of being able to replace a named bit in an ebit */
   /* (in this case we expect a bit called "Icons" to exist - the user will */
   /* have added a bit called this into the ebit to indicate where he/she */
   /* wants icons to go. we basically replace this bit with a virtual set */
   /* of callbacks that ebits will call if this bit is to be moved, resized */
   /* shown, hidden, raised, lowered etc. we provide the callbacks. */
   ebits_set_named_bit_replace(ib->bit, "Icons",
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
			       ib_bits_get_max_size,
			       ib);
   /* now move this ebit to a really high layer.. so its ontop of a lot */
   ebits_set_layer(ib->bit, 10000);
   /* and now call "fix" - i called it fix cause it does a few things... */
   /* but fixes the iconbar so its the size of the view, in the right */
   /* place and arranges the icons in their right spots */
   e_iconbar_fix(ib);
}

/**
 * e_iconbar_fix - iconbar geometry update
 * @ib: The iconbar for which to fix the geometry
 * 
 * This function corrects the geometry and visibility
 * of the iconbar gfx and icons
 */
void
e_iconbar_fix(E_Iconbar *ib)
{
   Evas_List l;
   double ix, iy, aw, ah;

   /* move the ebit to 0,0 */
   ebits_move(ib->bit, 0, 0);
   /* resize it to fill the whole view. the internal geometry of the */
   /* ebit itself will determine where things woudl go. we just tell */
   /* the ebit where in the canvas its allowed to exist */
   ebits_resize(ib->bit, ib->view->size.w, ib->view->size.h);
   /* show it. harmless to do this all the time */
   ebits_show(ib->bit);
   /* tell the view we belong to something may have changed so it can draw */
   ib->view->changed = 1;

   /* the callbacks set up in th ebtis replace will set up what area in */
   /* the canvas icons can exist in. lets extract them here */
   ix = ib->icon_area.x;
   iy = ib->icon_area.y;
   aw = ib->icon_area.w;
   ah = ib->icon_area.h;

   /* not used yet... */
   if (aw > ah) /* horizontal */
     {
     }
   else /* vertical */
     {
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
	evas_get_image_size(ic->iconbar->view->evas, ic->image, &iw, &ih);
	w = iw;
	h = ih;
	ox = 0;
	oy = 0;
	/* if the area to put icons is wider that it is tall... horizonatal */
	/* layout of the icons seems smart */
	if (aw > ah) /* horizontal */
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
	     /* now move and resize it */
	     evas_move(ic->iconbar->view->evas, ic->image, ix + ox, iy + oy);
	     evas_resize(ic->iconbar->view->evas, ic->image, w, h);
	     /* advance our position counter to the next spot */
	     ix += w;
	  }
	/* taller than it is wide. might be good to be vertical */
	else /* vertical */
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
	     /* now move the icona nd resize it */
	     evas_move(ic->iconbar->view->evas, ic->image, ix + ox, iy + oy);
	     evas_resize(ic->iconbar->view->evas, ic->image, w, h);
	     /* advance out counter to the next spot */
	     iy += h;
	  }
     }
}

/**
 * e_iconbar_file_add - Adds a file to a view
 * @v:    The view in which a file is added
 * @file: Name of the added file
 *
 * This function is called from the
 * view code whenever a file is added to a view. The iconbar code here
 * determines if the file add is of interest
 * and if it is, in 0.5 secs will do a "reload
 */
void
e_iconbar_file_add(E_View *v, char *file)
{
   /* is the file of interest ? */
   if ((!strcmp(".e_iconbar.db", file)) ||
       (!strcmp(".e_iconbar.bits.db", file)))
     {
	char buf[PATH_MAX];
	
	/* unique timer name */
	sprintf(buf, "iconbar_reload:%s", v->dir);
	/* in 0.5 secs call our timout handler */
	ecore_add_event_timer(buf, 0.5, ib_reload_timeout, 0, v);
     }
}

/**
 * e_iconbar_file_delete - Function to remove a file from an iconbox.
 * @v:    The view in which a file is removed
 * @file: Name of the removed file
 *
 * This function is called whenever a file is deleted from a view.
 */
void
e_iconbar_file_delete(E_View *v, char *file)
{
   /* is the file of interest */
   if ((!strcmp(".e_iconbar.db", file)) ||
       (!strcmp(".e_iconbar.bits.db", file)))
     {
	/* if we have an iconbar.. delete it - because its files have been */
	/* nuked. no need to keep it around. */
	if (v->iconbar) 
	  {
	     OBJ_DO_FREE(v->iconbar);
	     v->iconbar = NULL;
	  }
     }
}

/**
 * e_iconbar_file_change - File change update function
 * @v:    The view in which a file changes
 * @file: Name of the changed file
 *
 * This function gets called whenever a file changes in a view
 */
void
e_iconbar_file_change(E_View *v, char *file)
{
   /* is the file that changed of interest */
   if ((!strcmp(".e_iconbar.db", file)) ||
       (!strcmp(".e_iconbar.bits.db", file)))
     {
	char buf[PATH_MAX];
	
	/* unique timer name */
	sprintf(buf, "iconbar_reload:%s", v->dir);
	/* in 0.5 secsm call the realod timeout */
	ecore_add_event_timer(buf, 0.5, ib_reload_timeout, 0, v);
     }
}



/* static (internal to iconbar use only) callbacks */

/* reload timeout. called whenevr iconbar special files changed/added to */
/* a view */
static void
ib_reload_timeout(int val, void *data)
{
   E_View *v;

   /* get our view pointer */
   v = (E_View *)data;
   /* if we have an iconbar.. well nuke it */
   if (v->iconbar) OBJ_DO_FREE(v->iconbar);
   v->iconbar = NULL;
   /* try load a new iconbar */
   if (!v->iconbar) v->iconbar = e_iconbar_new(v);
   /* if the iconbar loaded and theres an evas - we're realized */
   /* so realize the iconbar */
   if ((v->iconbar) && (v->evas)) e_iconbar_realize(v->iconbar);   
}

/* this timeout is responsible for doing the mouse over animation */
static void
ib_timeout(int val, void *data)
{
   E_Iconbar_Icon *ic;
   double t;
   
   /* get the iconbar icon we are dealign with */
   ic = (E_Iconbar_Icon *)data;
   /* val <= 0 AND we're hilited ? first call as a timeout handler. */
   if ((val <= 0) && (ic->hilited))
     {
	char *n;
	
	/* note the "start" time */
	ic->hi.start = ecore_get_time();
	/* no hilite (animation) image */
	if (!ic->hi.image)
	  {
	     char buf[PATH_MAX];
	     
	     /* figure out its path */
	     sprintf(buf, "%s/.e_iconbar.db:%s", 
		     ic->iconbar->view->dir, ic->image_path);
	     /* add it */
	     ic->hi.image = evas_add_image_from_file(ic->iconbar->view->evas, 
						     buf);
	     /* put it high up */
	     evas_set_layer(ic->iconbar->view->evas, ic->hi.image, 20000);
	     /* dont allow it to capture any events (enter, leave etc. */
	     evas_set_pass_events(ic->iconbar->view->evas, ic->hi.image, 1);
	     /* show it */
	     evas_show(ic->iconbar->view->evas, ic->hi.image);
	  }
	/* start at 0 */
	val = 0;
     }
   /* what tame is it ? */
   t = ecore_get_time();
   /* if the icon is hilited */
   if (ic->hilited)
     {
	double x, y, w, h;
	double nw, nh, tt;
	int a;
	double speed;
	
	/* find out where the original icon image is */
	evas_get_geometry(ic->iconbar->view->evas, ic->image, &x, &y, &w, &h);
	/* tt is the time since we started */
	tt = t - ic->hi.start;
	/* the speed to run at - the less, the faster (ie a loop is 0.5 sec) */
	speed = 0.5;
	/* if we are beyond the time loop.. reset the start time to now */
	if (tt > speed) ic->hi.start = t;
	/* limit time to max loop time */
	if (tt > speed) tt = speed;
	/* calculate alpha to be invers of time sizne loop start */
	a = (int)(255.0 * (speed - tt));
	/* size is icon size + how far in loop we are */
	nw = w * ((tt / speed) + 1.0);
	nh = h * ((tt / speed) + 1.0);
	/* move the hilite icon to a good spot */
	evas_move(ic->iconbar->view->evas, ic->hi.image,
		  x + ((w - nw) / 2), y + ((h - nh) / 2));
	/* resize it */
	evas_resize(ic->iconbar->view->evas, ic->hi.image, nw, nh);
	/* reset its fill so ti fills its space */
	evas_set_image_fill(ic->iconbar->view->evas, ic->hi.image, 0, 0, nw, nh);
	/* set its fade */
	evas_set_color(ic->iconbar->view->evas, ic->hi.image, 255, 255, 255, a);	
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
	if (ic->hi.image) evas_del_object(ic->iconbar->view->evas, ic->hi.image);
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
	if (tt > 1.0) tt = 1.0;
	/* alpha value caluclated on ramp position */
	a = (int)((double)((1.0 - tt) * 127.0) + 128.0);
	/* set alpha value */
	evas_set_color(ic->iconbar->view->evas, ic->image, 255, 255, 255, a);
	/* time is at end of ramp.. kill timer */
	if (tt == 1.0)
	  {
	     /* free the timer name string */
	     IF_FREE(ic->hi.timer);
	     ic->hi.timer = NULL;
	  }
	/* decrement count */
	val--;
     }
   /* if we have a timer name.. rerun the timer in 0.05 */
   if (ic->hi.timer)
     ecore_add_event_timer(ic->hi.timer, 0.05, ib_timeout, val, data);
   /* flag the view that we changed */
   ic->iconbar->view->changed = 1;
}

/* called when an ebits object bit needs to be shown */
static void
ib_bits_show(void *data)
{
   E_Iconbar *ib;
   Evas_List l;
   
   ib = (E_Iconbar *)data;
   /* show all the icons */
   for (l = ib->icons; l; l = l->next)
     {
	E_Iconbar_Icon *ic;
	
	ic = l->data;
	evas_show(ic->iconbar->view->evas, ic->image);
     }
}

/* called when an ebit object bit needs to hide */
static void
ib_bits_hide(void *data)
{
   E_Iconbar *ib;
   Evas_List l;
   
   ib = (E_Iconbar *)data;
   /* hide all the icons */
   for (l = ib->icons; l; l = l->next)
     {
	E_Iconbar_Icon *ic;
	
	ic = l->data;
	evas_hide(ic->iconbar->view->evas, ic->image);
     }
}

/* called when an ebit objetc bit needs to move */
static void
ib_bits_move(void *data, double x, double y)
{
   E_Iconbar *ib;
   Evas_List l;
   
   ib = (E_Iconbar *)data;
   /* dont do anything.. just record the geometry. we'll deal with it later */
   ib->icon_area.x = x;
   ib->icon_area.y = y;
}

/* called when an ebit object bit needs to resize */
static void
ib_bits_resize(void *data, double w, double h)
{
   E_Iconbar *ib;
   Evas_List l;
   
   ib = (E_Iconbar *)data;
   /* dont do anything.. just record the geometry. we'll deal with it later */
   ib->icon_area.w = w;
   ib->icon_area.h = h;
}

/* called when the ebits object bit needs to be raised */
static void
ib_bits_raise(void *data)
{
   E_Iconbar *ib;
   Evas_List l;
   
   ib = (E_Iconbar *)data;
   /* raise all the icons */
   for (l = ib->icons; l; l = l->next)
     {
	E_Iconbar_Icon *ic;
	
	ic = l->data;
	evas_raise(ic->iconbar->view->evas, ic->image);
     }
}

/* called when the ebits object bit needs to be lowered */
static void
ib_bits_lower(void *data)
{
   E_Iconbar *ib;
   Evas_List l;
   
   ib = (E_Iconbar *)data;
   /* lower all the icons */
   for (l = ib->icons; l; l = l->next)
     {
	E_Iconbar_Icon *ic;
	
	ic = l->data;
	evas_lower(ic->iconbar->view->evas, ic->image);
     }
}

/* called when the ebits object bit needs to change layers */
static void
ib_bits_set_layer(void *data, int lay)
{
   E_Iconbar *ib;
   Evas_List l;
   
   ib = (E_Iconbar *)data;
   /* set the layer for all the icons */
   for (l = ib->icons; l; l = l->next)
     {
	E_Iconbar_Icon *ic;
	
	ic = l->data;
	evas_set_layer(ic->iconbar->view->evas, ic->image, lay);
     }
}

/* not used... err.. ebits clips for us to the maximum allowed space of */
/* the ebit object bit - dont know why i have this here */
static void
ib_bits_set_clip(void *data, Evas_Object clip)
{
}

/* we arent going to recolor our icons here according to color class */
static void
ib_bits_set_color_class(void *data, char *cc, int r, int g, int b, int a)
{
}

/* our minimum size for icon space is 0x0 */
static void
ib_bits_get_min_size(void *data, double *w, double *h)
{
   *w = 0;
   *h = 0;
}

/* our maximum is huge */
static void
ib_bits_get_max_size(void *data, double *w, double *h)
{
   *w = 999999;
   *h = 999999;
}





/* called on events on icons */

/* called when a mouse goes in on an icon object */
static void
ib_mouse_in(void *data, Evas _e, Evas_Object _o, int _b, int _x, int _y)
{
   E_Iconbar_Icon *ic;

   /* get he iconbaricon pointer from the data member */
   ic = (E_Iconbar_Icon *)data;
   /* set hilited flag */
   ic->hilited = 1;
   /* make it more opaque */
   evas_set_color(ic->iconbar->view->evas, ic->image, 255, 255, 255, 255);
   /* if we havent started an animation timer - start one */
   if (!ic->hi.timer)
     {
	char buf[PATH_MAX];
	
	/* come up with a unique name for it */
	sprintf(buf, "iconbar:%s/%s", ic->iconbar->view->dir, ic->image_path);
	e_strdup(ic->hi.timer, buf);
	/* call the timeout */
	ib_timeout(0, ic);
     }
   /* tell the view the iconbar is in.. something changed that might mean */
   /* a redraw is needed */
   ic->iconbar->view->changed = 1;
	   
}

/* called when a mouse goes out of an icon object */
static void
ib_mouse_out(void *data, Evas _e, Evas_Object _o, int _b, int _x, int _y)
{
   E_Iconbar_Icon *ic;
   
   /* get he iconbaricon pointer from the data member */
   ic = (E_Iconbar_Icon *)data;
   /* unset hilited flag */
   ic->hilited = 0;
   /* tell the view the iconbar is in.. something changed that might mean */
   /* a redraw is needed */
   ic->iconbar->view->changed = 1;
}

/* called when the mouse goes down on an icon object */
static void
ib_mouse_down(void *data, Evas _e, Evas_Object _o, int _b, int _x, int _y)
{
   E_Iconbar_Icon *ic;
   
   /* get he iconbaricon pointer from the data member */
   ic = (E_Iconbar_Icon *)data;
   /* run something! */
   if (ic->exec) e_exec_run(ic->exec);
}

/* called when the mouse goes up on an icon object */
static void
ib_mouse_up(void *data, Evas _e, Evas_Object _o, int _b, int _x, int _y)
{
}
