#include "view.h"
#include "icons.h"
#include "debug.h"
#include "globals.h"
#include "cursors.h"
#include "file.h" 
#include "util.h"

static void e_icon_down_cb(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y);
static void e_icon_up_cb(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y);
static void e_icon_in_cb(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y);
static void e_icon_out_cb(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y);
static void e_icon_move_cb(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y);


static void
e_icon_down_cb(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y)
{
   E_Icon *ic;
   Ecore_Event *ev;
   Ecore_Event_Mouse_Down *e;
   
   D_ENTER;
   
   ev = e_view_get_current_event();
   if (!ev) D_RETURN;
   e = ev->event;
   ic = _data;
   ic->view->select.down.x = _x;
   ic->view->select.down.y = _y;
   ic->state.clicked = 1;
   e_icon_update_state(ic);
   if (_b == 1)
     {
	if (e->double_click)
	  {
	     e_icon_exec(ic);
	     ic->state.just_executed = 1;
	  }
	else
	  {
	     if (!ic->state.selected)
	       {
		  if ((e->mods & multi_select_mod))
		    { 
		       e_icon_select(ic);
		    }
		  else
		    {
		       e_view_deselect_all_except(ic);
		       e_icon_select(ic);
		    }
		  ic->state.just_selected = 1;
	       }
	  }
     }
   else if (_b == 2)
     {
     }
   else if (_b == 3)
     {
     }

   D_RETURN;
   UN(_e);
   UN(_o);
}

static void
e_icon_up_cb(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y)
{
   E_Icon *ic;
   Ecore_Event *ev;
   Ecore_Event_Mouse_Up *e;
   
   D_ENTER;
   
   ev = e_view_get_current_event();
   if (!ev) D_RETURN;
   e = ev->event;
   ic = _data;
   if (ic->view->drag.started)
     {
	int x, y;
	
	ic->state.clicked = 0;
	ic->state.just_selected = 0;
	e_icon_update_state(ic);
	ecore_window_no_ignore(ic->view->drag.win);
	ecore_window_destroy(ic->view->drag.win);
	ic->view->drag.started = 0;
	if(e->mods & ECORE_EVENT_KEY_MODIFIER_SHIFT)
	  ecore_dnd_set_mode_copy();
	else
	  ecore_dnd_set_mode_move();
	ecore_dnd_set_data(ic->view->win.base);

	/* FIXME: if button use is right mouse then do an ask */

	/* Handle dnd motion(drop) - dragging==0 */
	ecore_pointer_xy_get(&x, &y);
	ecore_window_dnd_handle_motion(ic->view->win.base, x, y, 0);
	ecore_window_dnd_finished();
	D_RETURN;	
     }
   if (_b == 1)
     {
	if (ic->state.just_executed)
	  {
	     ic->state.just_executed = 0;
	  }
	else
	  {
	     if ((e->mods & multi_select_mod))
	       {
		  if ((ic->state.selected) && (!ic->state.just_selected))
		    e_icon_deselect(ic);
	       }
	     else
	       {
		  e_view_deselect_all_except(ic);
		  e_icon_select(ic);
	       }
	  }
	ic->state.just_selected = 0;
     }
   ic->state.clicked = 0;
   e_icon_update_state(ic);

   D_RETURN;
   UN(_e);
   UN(_o);
   UN(_x);
   UN(_y);
}

static void
e_icon_in_cb(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y)
{
   E_Icon *ic;
   
   D_ENTER;
   
   ic = _data;
   e_cursors_display_in_window(ic->view->win.main, "View_Icon");

   D_RETURN;
   UN(_e);
   UN(_o);
   UN(_b);
   UN(_x);
   UN(_y);
}

static void
e_icon_out_cb(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y)
{
   E_Icon *ic;
   
   ic = _data;
   e_cursors_display_in_window(ic->view->win.main, "View");

   D_RETURN;
   UN(_e);
   UN(_o);
   UN(_b);
   UN(_x);
   UN(_y);
}

static void
e_icon_move_cb(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y)
{
   E_Icon *ic;
   Ecore_Event *ev;
   Ecore_Event_Mouse_Move *e;
   
   D_ENTER;
   
   ev = e_view_get_current_event();

   if (!ev)
     D_RETURN;

   e = ev->event;
   ic = _data;

   if (!ic->state.clicked)
     D_RETURN;

   if (!ic->view->drag.started)
     {
	int dx, dy;
	
	dx = ic->view->select.down.x - _x;
	dy = ic->view->select.down.y - _y;
	if (dx < 0) dx = -dx;
	if (dy < 0) dy = -dy;
	if ((dx > 3) || (dy > 3))
	  {
	     Pixmap pmap, mask;
	     Evas_List l;
	     int x, y, xx, yy, rw, rh, downx, downy, wx, wy, ww, wh;
	     
	     /* find extents of icons to be dragged */
	     x = 999999999;
	     y = 999999999;
	     xx = -999999999;
	     yy = -999999999;
	     for (l = views; l; l = l->next)
	       {
		  E_View *v;
		  Evas_List ll;
		  
		  v = l->data;
		  D("sel count %i\n", v->select.count);
		  if (v->select.count > 0)
		    {
		       for (ll = v->icons; ll; ll = ll->next)
			 {
			    E_Icon *ic;
			    
			    ic = ll->data;
			    if (ic->state.selected)
			      {
				 int ix, iy, iw, ih;
				 
				 ix = ic->view->scroll.x + ic->geom.x + v->location.x;
				 iy = ic->view->scroll.y + ic->geom.y + v->location.y;
				 iw = ic->geom.w;
				 ih = ic->geom.h;
				 if (ix < x) x = ix;
				 if (iy < y) y = iy;
				 if ((ix + iw) > xx) xx = ix + iw;
				 if ((iy + ih) > yy) yy = iy + ih;
			      }
			 }
		    }
	       }
	     ecore_window_get_geometry(0, NULL, NULL, &rw, &rh);
	     downx = ic->view->select.down.x + ic->view->location.x;
	     downy = ic->view->select.down.y + ic->view->location.y;
	     
	     wx = x;
	     ww = xx - x;
	     if (wx < - (rw - downx)) 
	       {
		  wx = - (rw - downx);
		  ww -= (wx - x);
	       }
	     if ((wx + ww) > (rw + downx))
	       ww = (rw + downx) - wx;
	     wy = y;
	     wh = yy - y;
	     if (wy < - (rh - downy)) 
	       {
		  wy = - (rh - downy);
		  wh -= (wy - y);
	       }
	     if ((wy + wh) > (rh + downy))
	       wh = (rh + downy) - wy;
	     
	     
	     ic->view->drag.x = wx + ic->view->location.x;
	     ic->view->drag.y = wy + ic->view->location.y;
	     ic->view->drag.offset.x = downx - ic->view->drag.x;
	     ic->view->drag.offset.y = downy - ic->view->drag.y;
	     
	     if ((ww < 1) || (wh < 1)) D_RETURN;
	     ic->view->drag.win = ecore_window_override_new(0, wx, wy, ww, wh);
	     pmap = ecore_pixmap_new(ic->view->drag.win, ww, wh, 0);
	     mask = ecore_pixmap_new(ic->view->drag.win, ww, wh, 1);
	       {
		  Imlib_Image im;
		  
		  im = imlib_create_image(ww, wh);
		  imlib_context_set_image(im);
		  imlib_image_set_has_alpha(1);
		  imlib_context_set_blend(1);
		  imlib_image_clear();
                  imlib_context_set_color_modifier(NULL);
		  imlib_context_set_cliprect(0, 0, 0, 0);
		  imlib_context_set_angle(0);
		  
		  for (l = views; l; l = l->next)
		    {
		       E_View *v;
		       Evas_List ll;
		       
		       v = l->data;
		       if (v->select.count > 0)
			 {
			    for (ll = v->icons; ll; ll = ll->next)			      {
				 E_Icon *ic;
				 
				 ic = ll->data;
				 if (ic->state.selected)
				   {
				      double ix, iy;
				      int icx, icy;
				      Imlib_Image im2;
				      char icon[PATH_MAX];
				      
				      evas_get_geometry(ic->view->evas,
							ic->obj.icon,
							&ix, &iy, NULL, NULL);
				      icx = ix + v->location.x - wx;
				      icy = iy + v->location.y - wy;
				      if (!ic->info.icon)
					{
					   D("EEEEEEEEEEK %s has no icon\n", ic->file);
					   D_RETURN;
					}
				      if (ic->state.clicked)
					{
					   sprintf(icon, "%s:/icon/clicked", ic->info.icon);
					}
				      else if (ic->state.selected)
					{
					   sprintf(icon, "%s:/icon/selected", ic->info.icon);
					}
				      else
					{
					   sprintf(icon, "%s:/icon/normal", ic->info.icon);
					}
				      im2 = imlib_load_image(icon);
				      if (im2)
					{
					   int iw, ih;
					   
					   imlib_context_set_image(im2);
					   iw = imlib_image_get_width();
					   ih = imlib_image_get_height();
					   imlib_context_set_image(im);
					   imlib_blend_image_onto_image(im2, 1,
									0, 0, iw, ih,
									icx, icy, iw, ih);
					   imlib_context_set_image(im2);
					   imlib_free_image();
					   imlib_context_set_image(im);
					}
				      else
					{
					   D("eek cant load\n");
					}
				   }
			      }
			 }
		    }
		  imlib_context_set_image(im);
		  if (ww * wh < (200 * 200)) imlib_context_set_dither_mask(1);
		  else imlib_context_set_dither_mask(0);
		  imlib_context_set_dither(1);
		  imlib_context_set_drawable(pmap);
		  imlib_context_set_mask(mask);
		  imlib_context_set_blend(0);
		  imlib_context_set_color_modifier(NULL);
		  imlib_render_image_on_drawable(0, 0);
		  imlib_free_image();
	       }
	     ecore_window_set_background_pixmap(ic->view->drag.win, pmap);
	     ecore_window_set_shape_mask(ic->view->drag.win, mask);
	     ecore_window_ignore(ic->view->drag.win);
	     ecore_window_raise(ic->view->drag.win);
	     ecore_window_show(ic->view->drag.win);
	     ecore_pixmap_free(pmap);
	     ecore_pixmap_free(mask);

	     /* Initiate dnd */
	     ecore_dnd_set_mode_copy();
	     ecore_dnd_set_data(ic->view->win.base);
	     
	     ecore_dnd_own_selection(ic->view->win.base);

	     ic->view->drag.started = 1;
	  }
     }

   if (ic->view->drag.started)
     {
	int x, y;
	
	x = _x - ic->view->drag.offset.x;
	y = _y - ic->view->drag.offset.y;
	ic->view->drag.x = x;
	ic->view->drag.y = y;
	ic->view->drag.update = 1;
	ic->view->changed = 1;

	if(e->mods & ECORE_EVENT_KEY_MODIFIER_SHIFT)
	  {
	   ecore_dnd_set_mode_copy();
	   ic->view->drag.drop_mode = E_DND_COPY;
	  }
	else
	  {
	   ecore_dnd_set_mode_move();
	   ic->view->drag.drop_mode = E_DND_MOVE;
	  }
	ecore_dnd_set_data(ic->view->win.base);

	/* Handle dnd motion - dragging==1 */
	ecore_pointer_xy_get(&x, &y);
	ecore_window_dnd_handle_motion( ic->view->win.base, x, y, 1);
     }

   D_RETURN;
   UN(_e);
   UN(_o);
   UN(_b);
}


static void
e_icon_cleanup(E_Icon *ic)
{
   D_ENTER;
   
   /* FIXME: free stuff here! this leaks ... */
   /* (think I got them all) */

   if (ic->obj.event1)
     {
       evas_del_object(ic->view->evas, ic->obj.event1);
       evas_del_object(ic->view->evas, ic->obj.event2);
     }

   if (ic->obj.sel.under.icon) ebits_free(ic->obj.sel.under.icon);
   if (ic->obj.sel.under.text) ebits_free(ic->obj.sel.under.text);
   if (ic->obj.sel.over.icon) ebits_free(ic->obj.sel.over.icon);
   if (ic->obj.sel.over.text) ebits_free(ic->obj.sel.over.text);

   e_object_cleanup(E_OBJECT(ic));

   D_RETURN;
}

E_Icon *
e_icon_new(void)
{
   E_Icon *ic;
   
   D_ENTER;
   
   ic = NEW(E_Icon, 1);
   ZERO(ic, E_Icon, 1);
   
   e_object_init(E_OBJECT(ic), (E_Cleanup_Func) e_icon_cleanup);

   D_RETURN_(ic);
}



E_Icon *
e_icon_find_by_file(E_View *view, char *file)
{
   Evas_List l;
   
   D_ENTER;
   
   for (l = view->icons; l; l = l->next)
     {
	E_Icon *ic;
	
	ic = l->data;
	if ((ic) && (ic->file) && (file) && (!strcmp(ic->file, file)))
	  D_RETURN_(ic);
     }
   D_RETURN_(NULL);
}

void
e_icon_show(E_Icon *ic)
{
   D_ENTER;

   
   if (ic->state.visible) D_RETURN;
   ic->state.visible = 1;
   if (!ic->obj.event1)
     {
	ic->obj.event1 = evas_add_rectangle(ic->view->evas);
	ic->obj.event2 = evas_add_rectangle(ic->view->evas);
	evas_set_color(ic->view->evas, ic->obj.event1, 0, 0, 0, 0);
	evas_set_color(ic->view->evas, ic->obj.event2, 0, 0, 0, 0);
	evas_callback_add(ic->view->evas, ic->obj.event1, CALLBACK_MOUSE_DOWN, e_icon_down_cb, ic);
	evas_callback_add(ic->view->evas, ic->obj.event1, CALLBACK_MOUSE_UP, e_icon_up_cb, ic);
	evas_callback_add(ic->view->evas, ic->obj.event1, CALLBACK_MOUSE_IN, e_icon_in_cb, ic);
	evas_callback_add(ic->view->evas, ic->obj.event1, CALLBACK_MOUSE_OUT, e_icon_out_cb, ic);
	evas_callback_add(ic->view->evas, ic->obj.event1, CALLBACK_MOUSE_MOVE, e_icon_move_cb, ic);
	evas_callback_add(ic->view->evas, ic->obj.event2, CALLBACK_MOUSE_DOWN, e_icon_down_cb, ic);
	evas_callback_add(ic->view->evas, ic->obj.event2, CALLBACK_MOUSE_UP, e_icon_up_cb, ic);
	evas_callback_add(ic->view->evas, ic->obj.event2, CALLBACK_MOUSE_IN, e_icon_in_cb, ic);
	evas_callback_add(ic->view->evas, ic->obj.event2, CALLBACK_MOUSE_OUT, e_icon_out_cb, ic);
	evas_callback_add(ic->view->evas, ic->obj.event2, CALLBACK_MOUSE_MOVE, e_icon_move_cb, ic);
     }
   evas_set_layer(ic->view->evas, ic->obj.icon, 200);
   e_text_set_layer(ic->obj.text, 200);
   evas_set_layer(ic->view->evas, ic->obj.event1, 210);
   evas_set_layer(ic->view->evas, ic->obj.event2, 210);
   
   evas_show(ic->view->evas, ic->obj.icon);
   e_text_show(ic->obj.text);
   evas_show(ic->view->evas, ic->obj.event1);
   evas_show(ic->view->evas, ic->obj.event2);

   D_RETURN;
}

void
e_icon_hide(E_Icon *ic)
{
   D_ENTER;
   
   if (!ic->state.visible) D_RETURN;
   ic->state.visible = 0;
   evas_hide(ic->view->evas, ic->obj.icon);
   e_text_hide(ic->obj.text);
   evas_hide(ic->view->evas, ic->obj.event1);
   evas_hide(ic->view->evas, ic->obj.event2);

   /* Hide any selection in the view */
   if(ic->obj.sel.under.icon) ebits_hide(ic->obj.sel.under.icon);
   if(ic->obj.sel.under.text) ebits_hide(ic->obj.sel.under.text);
   if(ic->obj.sel.over.icon) ebits_hide(ic->obj.sel.over.icon);
   if(ic->obj.sel.over.text) ebits_hide(ic->obj.sel.over.text);

   D_RETURN;
}

void
e_icon_hide_delete_pending(E_Icon *ic)
{
   D_ENTER;
   
   if (!ic->state.visible) D_RETURN;
   if(ic->state.selected)
     {
       if( ic->view->drag.drop_mode == E_DND_MOVE)
	 {
	   evas_hide(ic->view->evas, ic->obj.icon);
	   ic->state.drag_delete = 1;
	 }
       else
	 /* copy... */
	 {
	   evas_show(ic->view->evas, ic->obj.icon);
	   ic->state.drag_delete = 0;
	 }
     }

   D_RETURN;
}

void
e_icon_show_delete_end(E_Icon *ic, E_dnd_enum dnd_pending_mode)
{
   D_ENTER;
   
   if (!ic->state.visible) D_RETURN;
   if(ic->state.drag_delete)
     {
       if(dnd_pending_mode==E_DND_DELETED || dnd_pending_mode==E_DND_COPIED)
       {
	 ic->state.drag_delete = 0;
	 if(dnd_pending_mode==E_DND_COPIED)
	   evas_show(ic->view->evas, ic->obj.icon);
       }
     }

   D_RETURN;
}

void
e_icon_apply_xy(E_Icon *ic)
{
   D_ENTER;
   
   /* threse calc icon extents for: */
   /*  [I]  */
   /*  Ig   */
   /* [txt] */
   
   if (ic->geom.text.w > ic->geom.icon.w) ic->geom.w = ic->geom.text.w;
   else ic->geom.w = ic->geom.icon.w;
   ic->geom.h = ic->geom.icon.h + ic->geom.text.h + ic->view->spacing.icon.g;
   
   evas_resize(ic->view->evas, ic->obj.event1, 
	       ic->geom.icon.w, ic->geom.icon.h);
   evas_resize(ic->view->evas, ic->obj.event2, 
	       ic->geom.text.w, ic->geom.text.h);
   evas_move(ic->view->evas, ic->obj.event1,
	     ic->view->scroll.x + ic->geom.x + ((ic->geom.w - ic->geom.icon.w) / 2),
	     ic->view->scroll.y + ic->geom.y);
   evas_move(ic->view->evas, ic->obj.event2,
	     ic->view->scroll.x + ic->geom.x + ((ic->geom.w - ic->geom.text.w) / 2),
	     ic->view->scroll.y + ic->geom.y + ic->geom.icon.h + ic->view->spacing.icon.g);	     
   evas_move(ic->view->evas, ic->obj.icon, 
	     ic->view->scroll.x + ic->geom.x + ((ic->geom.w - ic->geom.icon.w) / 2), 
	     ic->view->scroll.y + ic->geom.y);
   e_text_move(ic->obj.text,
	       ic->view->scroll.x + ic->geom.x + ((ic->geom.w - ic->geom.text.w) / 2), 
	       ic->view->scroll.y + ic->geom.y + ic->geom.icon.h + ic->view->spacing.icon.g);
   if (ic->obj.sel.under.icon)
     {
	int pl, pr, pt, pb;
	
	ebits_get_insets(ic->obj.sel.under.icon, &pl, &pr, &pt, &pb);
	ebits_move(ic->obj.sel.under.icon, 
		   ic->view->scroll.x + ic->geom.x + ((ic->geom.w - ic->geom.icon.w) / 2) - pl,
		   ic->view->scroll.y + ic->geom.y - pt);
	ebits_resize(ic->obj.sel.under.icon,
		     ic->geom.icon.w + pl + pr, ic->geom.icon.h + pt + pb);
	ebits_show(ic->obj.sel.under.icon);
     }
   if (ic->obj.sel.under.text)
     {
	int pl, pr, pt, pb;
	
	ebits_get_insets(ic->obj.sel.under.text, &pl, &pr, &pt, &pb);
	ebits_move(ic->obj.sel.under.text, 
		   ic->view->scroll.x + ic->geom.x + ((ic->geom.w - ic->geom.text.w) / 2) - pl, 
		   ic->view->scroll.y + ic->geom.y + ic->geom.icon.h + ic->view->spacing.icon.g - pt);
	ebits_resize(ic->obj.sel.under.text,
		     ic->geom.text.w + pl  + pr, ic->geom.text.h + pt + pb);
	ebits_show(ic->obj.sel.under.text);
     }
   if (ic->obj.sel.over.icon)
     {
	int pl, pr, pt, pb;
	
	ebits_get_insets(ic->obj.sel.over.icon, &pl, &pr, &pt, &pb);
	ebits_move(ic->obj.sel.over.icon, 
		   ic->view->scroll.x + ic->geom.x + ((ic->geom.w - ic->geom.icon.w) / 2) - pl,
		   ic->view->scroll.y + ic->geom.y - pt);
	ebits_resize(ic->obj.sel.over.icon,
		     ic->geom.icon.w + pl + pr, ic->geom.icon.h + pt + pb);
	ebits_show(ic->obj.sel.over.icon);
     }
   if (ic->obj.sel.over.text)
     {
	int pl, pr, pt, pb;
	
	ebits_get_insets(ic->obj.sel.over.text, &pl, &pr, &pt, &pb);
	ebits_move(ic->obj.sel.over.text, 
		   ic->view->scroll.x + ic->geom.x + ((ic->geom.w - ic->geom.text.w) / 2) - pl, 
		   ic->view->scroll.y + ic->geom.y + ic->geom.icon.h + ic->view->spacing.icon.g - pt);
	ebits_resize(ic->obj.sel.over.text,
		     ic->geom.text.w + pl  + pr, ic->geom.text.h + pt + pb);
	ebits_show(ic->obj.sel.over.text);
     }
   if ((ic->geom.x != ic->prev_geom.x) || (ic->geom.y != ic->prev_geom.y))
     {
	ic->q.write_xy = 1;
	e_view_queue_icon_xy_record(ic->view);
     }
   if (ic->geom.x != ic->prev_geom.x) ic->view->extents.valid = 0;
   else if (ic->geom.y != ic->prev_geom.y) ic->view->extents.valid = 0;
   else if (ic->geom.w != ic->prev_geom.w) ic->view->extents.valid = 0;
   else if (ic->geom.h != ic->prev_geom.h) ic->view->extents.valid = 0;
   
   ic->prev_geom = ic->geom;
   ic->prev_geom.x = ic->geom.x;
   ic->prev_geom.y = ic->geom.y;
   ic->prev_geom.w = ic->geom.w;
   ic->prev_geom.h = ic->geom.h;

   D_RETURN;
}


void      
e_icon_check_permissions(E_Icon *ic)
{
  D_ENTER;

  if (!ic || !ic->info.mime.base || ic->stat.st_ino == 0)
    D_RETURN;

  if (!strcmp(ic->info.mime.base, "dir"))
    {
      if (e_file_can_exec(&ic->stat))
	evas_set_color(ic->view->evas, ic->obj.icon, 255, 255, 255, 255);
      else
	evas_set_color(ic->view->evas, ic->obj.icon, 128, 128, 128, 128);
    } 

  D_RETURN;
}


void
e_icon_initial_show(E_Icon *ic)
{
   D_ENTER;
   
   /* check if we have enuf info and we havent been shown yet */
   if (!ic->info.icon) D_RETURN;
   if (ic->state.visible) D_RETURN;
   
   /* first. lets figure out the size of the icon */
   evas_get_image_size(ic->view->evas, ic->obj.icon, 
		       &(ic->geom.icon.w), &(ic->geom.icon.h));
     {
	double tw, th;
	
	e_text_get_geometry(ic->obj.text, NULL, NULL, &tw, &th);
	ic->geom.text.w = (int)tw;
	ic->geom.text.h = (int)th;
     }
   
   /* now lets allocate space for it if we need to */
   ic->geom.x = 999999;
   ic->geom.y = 999999;
   
   /* if needed queue a tiemout for a resort */
   e_view_queue_resort(ic->view);
   
   /* actually show the icon */
   e_icon_apply_xy(ic);
   e_icon_show(ic);

   D_RETURN;
}

void
e_icon_set_mime(E_Icon *ic, char *base, char *mime)
{
   int diff = 0;
   
   D_ENTER;
   
   if (!ic->info.mime.base) diff = 1;
   if (!ic->info.mime.type) diff = 1;
   if ((ic->info.mime.base) && (strcmp(ic->info.mime.base, base))) diff = 1;
   if ((ic->info.mime.type) && (strcmp(ic->info.mime.base, mime))) diff = 1;
   if (!diff) D_RETURN;
   if (ic->info.mime.base) free(ic->info.mime.base);
   if (ic->info.mime.type) free(ic->info.mime.type);
   ic->info.mime.base = NULL;
   ic->info.mime.type = NULL;
   ic->info.mime.base = strdup(base);
   ic->info.mime.type = strdup(mime);
   
   D("%40s: %s/%s\n", ic->file, base, mime);
   
   /* effect changes here */
   if (ic->info.custom_icon) 
     {
	if (ic->info.icon) free(ic->info.icon);
	ic->info.icon = NULL;
	ic->info.icon = strdup(ic->info.custom_icon);
	evas_set_image_file(ic->view->evas, ic->obj.icon, ic->info.custom_icon);
	e_view_queue_resort(ic->view);	
	D_RETURN;
     }
   /* find an icon */
     {
	char icon[PATH_MAX];
	char type[PATH_MAX];	
	char *p;
	int done = 0;
	
	strcpy(type, ic->info.mime.type);
	for (p = type; *p; p++) 
	  {
	     if (*p == '/') *p = '-';
	  }
	do 
	  {
	     sprintf(icon, "%s/data/icons/%s/%s.db", PACKAGE_DATA_DIR,
		     ic->info.mime.base, type);
	     if (e_file_exists(icon))
	       {
		  done = 1;
		  break;
	       }
	     p = strrchr(type, '-');
	     if (p) *p = 0;
	  }
	while (p);
	if (!done)
	  {
	     if (!e_file_exists(icon))
	       {
		  sprintf(icon, "%s/data/icons/%s/default.db", PACKAGE_DATA_DIR,
			  ic->info.mime.base);
		  if (!e_file_exists(icon))
		    sprintf(icon, "%s/data/icons/unknown/default.db", PACKAGE_DATA_DIR);
	       }
	  }
	ic->info.icon = strdup(icon);
     }

   e_icon_update_state(ic);

   D_RETURN;
}

void
e_icon_set_link(E_Icon *ic, char *link)
{
   D_ENTER;
   
   if ((!link) && (ic->info.link))
     {
	free(ic->info.link);
	ic->info.link = NULL;
	/* effect changes here */
     }
   else if (link)
     {
	if ((ic->info.link) && (!strcmp(ic->info.link, link)))
	  {
	     free(ic->info.link);
	     ic->info.link = strdup(link);
	     /* effect changes here */
	  }
     }

   D_RETURN;
}


void
e_icon_update_state(E_Icon *ic)
{
   char icon[PATH_MAX];
   int iw, ih;
   int gw, gh;
   
   D_ENTER;
   
   if (!ic->info.icon)
     {
	D("EEEEEEEEEEK %s has no icon\n", ic->file);
	D_RETURN;
     }
   if (ic->state.clicked)
     {
	sprintf(icon, "%s:/icon/clicked", ic->info.icon);
     }
   else if (ic->state.selected)
     {
	sprintf(icon, "%s:/icon/selected", ic->info.icon);
     }
   else
     {
	sprintf(icon, "%s:/icon/normal", ic->info.icon);
     }
   if ((ic->state.selected) && 
       (!ic->obj.sel.under.icon) && 
       (!ic->obj.sel.over.icon))
     {
	char file[PATH_MAX];

/*	
	sprintf(file, "%s/file.bits.db", e_config_get("selections"));
	ic->obj.sel.over.icon = ebits_load(file);
	sprintf(file, "%s/text.bits.db", e_config_get("selections"));
	ic->obj.sel.over.text = ebits_load(file);
 */
	sprintf(file, "%s/file.bits.db", e_config_get("selections"));
	ic->obj.sel.under.icon = ebits_load(file);
	sprintf(file, "%s/text.bits.db", e_config_get("selections"));
	ic->obj.sel.under.text = ebits_load(file);
	if (ic->obj.sel.under.icon) 
	  {
	     ebits_add_to_evas(ic->obj.sel.under.icon, ic->view->evas);
	     ebits_set_layer(ic->obj.sel.under.icon, 195);
	  }
	if (ic->obj.sel.under.text)
	  {
	     ebits_add_to_evas(ic->obj.sel.under.text, ic->view->evas);
	     ebits_set_layer(ic->obj.sel.under.text, 195);
	  }
	if (ic->obj.sel.over.icon) 
	  {
	     ebits_add_to_evas(ic->obj.sel.over.icon, ic->view->evas);
	     ebits_set_layer(ic->obj.sel.over.icon, 205);
	  }
	if (ic->obj.sel.over.text)
	  {
	     ebits_add_to_evas(ic->obj.sel.over.text, ic->view->evas);
	     ebits_set_layer(ic->obj.sel.over.text, 205);
	  }
     }
   else if ((!ic->state.selected) && 
	    ((ic->obj.sel.under.icon) ||
	     (ic->obj.sel.over.icon)))
     {
	if (ic->obj.sel.under.icon) ebits_free(ic->obj.sel.under.icon);
	if (ic->obj.sel.under.text) ebits_free(ic->obj.sel.under.text);
	if (ic->obj.sel.over.icon) ebits_free(ic->obj.sel.over.icon);
	if (ic->obj.sel.over.text) ebits_free(ic->obj.sel.over.text);
	ic->obj.sel.under.icon = NULL;
	ic->obj.sel.under.text = NULL;
	ic->obj.sel.over.icon = NULL;
	ic->obj.sel.over.text = NULL;
     }
   evas_set_image_file(ic->view->evas, ic->obj.icon, icon);
   evas_get_image_size(ic->view->evas, ic->obj.icon, &iw, &ih);
   gw = ic->geom.icon.w;
   gh = ic->geom.icon.h;
   e_icon_apply_xy(ic);
   if ((iw == gw) && (ih == gh)) D_RETURN;
   e_view_queue_resort(ic->view);

   D_RETURN;
}


void
e_icon_invert_selection(E_Icon *ic)
{
   D_ENTER;
   
   if (ic->state.selected) e_icon_deselect(ic);
   else e_icon_select(ic);

   D_RETURN;
}

void
e_icon_select(E_Icon *ic)
{
   D_ENTER;
   
   if (!ic->state.selected)
     {
	ic->state.selected = 1;
	ic->view->select.count++;
	e_icon_update_state(ic);
     }

   D_RETURN;
}

void
e_icon_deselect(E_Icon *ic)
{
   D_ENTER;
   
   if (ic->state.selected)
     {
	ic->state.selected = 0;
	ic->view->select.count--;
	e_icon_update_state(ic);
     }

   D_RETURN;
}

void
e_icon_exec(E_Icon *ic)
{
   D_ENTER;
   
   if (!strcmp(ic->info.mime.base, "dir") &&
       e_file_can_exec(&ic->stat))
     {
	E_View *v;
	char buf[PATH_MAX];
	
	v = e_view_new();
	v->size.w = 400;
	v->size.h = 300;
	v->options.back_pixmap = 0;
	sprintf(buf, "%s/%s", ic->view->dir, ic->file);
	D("new dir >%s<\n", buf);
	v->dir = strdup(buf);
	e_view_bg_load(v);
	e_view_realize(v);
	ecore_window_set_title(v->win.base, ic->file);
	ecore_window_set_name_class(v->win.base, "FileView", "E");
	ecore_window_set_min_size(v->win.base, 8, 8);
     }

   e_icon_deselect(ic);

   D_RETURN;
}
