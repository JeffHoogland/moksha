#include "iconbar.h"

static Evas_List iconbars;

E_Iconbar *
e_iconbar_new(E_View *v)
{
  /* vertical lines (left, center, right) and title */
  /*	Evas_Object ovl, ovc, ovr, ot; */
  
  E_Iconbar *ib;
  
  ib = NEW(E_Iconbar, 1);
  ZERO(ib, E_Iconbar, 1);
  OBJ_INIT(ib, e_iconbar_free);
  
  printf("in ib_new()\n");
  
  ib->v = v;
  ib->e = v->evas;
  /*	ib->name = strdup(v->dir); */
  
  /* e_iconbar_draw(ib); */
  iconbars = evas_list_append(iconbars, ib);
  
  if(e_iconbar_config(ib) == 0)
    return NULL;

  return ib;
}

void
e_iconbar_realize(E_Iconbar *ib)
{
  printf("in e_iconbar_realize()\n");	
  
  if(ib->geom.conf.left < 0)
    ib->geom.left = ib->geom.conf.left + ib->v->size.w;
  if(ib->geom.conf.top < 0)
    ib->geom.top = ib->geom.conf.top + ib->v->size.h;
  if(ib->geom.conf.w == 0)
    ib->geom.w = ib->v->size.w;
  if(ib->geom.conf.h == 0)
    ib->geom.h = ib->v->size.h;
  
  printf("w: %i, h: %i\nt: %i, l: %i\n", ib->geom.w, ib->geom.h, ib->geom.top, ib->geom.left);
  
  
  /* horizontal */
  if (ib->geom.horizontal)
    {
      ib->obj.scroll = evas_add_rectangle(ib->e);
      ib->obj.line_l = evas_add_image_from_file(ib->e, ib->image.hline);
      ib->obj.line_c = evas_add_image_from_file(ib->e, ib->image.hline);
      ib->obj.line_r = evas_add_image_from_file(ib->e, ib->image.hline);
      ib->obj.title = evas_add_image_from_file(ib->e, ib->image.title);
      ib->obj.clip = evas_add_rectangle(ib->e);
      
      evas_get_image_size(ib->e, ib->obj.title, &ib->geom.title_w, &ib->geom.title_h);
      evas_get_image_size(ib->e, ib->obj.line_l, &ib->geom.line_w, &ib->geom.line_h);
      
      evas_set_layer(ib->e, ib->obj.scroll, 400);
      
      evas_resize(ib->e, ib->obj.scroll, ib->geom.w, ib->geom.scroll_w);
      evas_resize(ib->e, ib->obj.line_l, ib->geom.w, ib->geom.line_h);
      evas_resize(ib->e, ib->obj.line_c, ib->geom.w, ib->geom.line_h);
      evas_resize(ib->e, ib->obj.line_r, ib->geom.w, ib->geom.line_h);
      evas_resize(ib->e, ib->obj.clip, ib->geom.w, ib->geom.h);      
      
      evas_set_image_fill(ib->e, ib->obj.line_l, 0, 0, ib->geom.line_w, ib->geom.line_h);
      evas_set_image_fill(ib->e, ib->obj.line_c, 0, 0, ib->geom.line_w, ib->geom.line_h);
      evas_set_image_fill(ib->e, ib->obj.line_r, 0, 0, ib->geom.line_w, ib->geom.line_h);      
      
      evas_move(ib->e, ib->obj.scroll, ib->geom.left, ib->geom.top + ib->geom.h - ib->geom.scroll_w);
      evas_move(ib->e, ib->obj.line_l, ib->geom.left, ib->geom.top);
      evas_move(ib->e, ib->obj.line_c, ib->geom.left, ib->geom.top + ib->geom.h - ib->geom.scroll_w);
      evas_move(ib->e, ib->obj.line_r, ib->geom.left, ib->geom.top + ib->geom.h);
      evas_move(ib->e, ib->obj.title, ib->geom.left - ib->geom.title_w - 5, ib->geom.top + (ib->geom.h - ib->geom.title_h) / 2);
      evas_move(ib->e, ib->obj.clip, ib->geom.left, ib->geom.top);
      
      evas_set_color(ib->e, ib->obj.scroll, 129, 129, 129, 0);
      evas_set_color(ib->e, ib->obj.clip, 255, 255, 255, 255);
    }
  
  /* vertical */
  if (!ib->geom.horizontal)
    {
      ib->obj.scroll = evas_add_rectangle(ib->e);
      ib->obj.line_l = evas_add_image_from_file(ib->e, ib->image.vline);
      ib->obj.line_c = evas_add_image_from_file(ib->e, ib->image.vline);
      ib->obj.line_r = evas_add_image_from_file(ib->e, ib->image.vline);
      ib->obj.title = evas_add_image_from_file(ib->e, ib->image.title);
      ib->obj.clip = evas_add_rectangle(ib->e);
      
      evas_get_image_size(ib->e, ib->obj.title, &ib->geom.title_w, &ib->geom.title_h);
      evas_get_image_size(ib->e, ib->obj.line_l, &ib->geom.line_w, &ib->geom.line_h);
      
      evas_set_layer(ib->e, ib->obj.scroll, 400);
      
      evas_resize(ib->e, ib->obj.scroll, ib->geom.scroll_w, ib->geom.h);
      evas_resize(ib->e, ib->obj.line_l, ib->geom.line_w, ib->geom.h);
      evas_resize(ib->e, ib->obj.line_c, ib->geom.line_w, ib->geom.h);
      evas_resize(ib->e, ib->obj.line_r, ib->geom.line_w, ib->geom.h);
      evas_resize(ib->e, ib->obj.clip, ib->geom.w, ib->geom.h);
      
      evas_set_image_fill(ib->e, ib->obj.line_l, 0, 0, ib->geom.line_w, ib->geom.line_h);
      evas_set_image_fill(ib->e, ib->obj.line_c, 0, 0, ib->geom.line_w, ib->geom.line_h);
      evas_set_image_fill(ib->e, ib->obj.line_r, 0, 0, ib->geom.line_w, ib->geom.line_h);
      
      evas_move(ib->e, ib->obj.scroll, ib->geom.left + ib->geom.w - ib->geom.scroll_w, ib->geom.top);
      evas_move(ib->e, ib->obj.line_l, ib->geom.left, ib->geom.top);
      evas_move(ib->e, ib->obj.line_c, ib->geom.left + ib->geom.w - ib->geom.scroll_w, ib->geom.top);
      evas_move(ib->e, ib->obj.line_r, ib->geom.left + ib->geom.w, ib->geom.top);
      evas_move(ib->e, ib->obj.title, ib->geom.left + ((ib->geom.w - ib->geom.title_w - ib->geom.scroll_w) / 2 ), ib->geom.top - ib->geom.title_h - 5);
      evas_move(ib->e, ib->obj.clip, ib->geom.left, ib->geom.top);
      
      evas_set_color(ib->e, ib->obj.scroll, 129, 129, 129, 0);
      evas_set_color(ib->e, ib->obj.clip, 255, 255, 255, 255);
    }
  
  
  /* Clip icons */
  {
    Evas_List l;
    for (l = ib->icons; l; l = l->next)
      {
	E_Iconbar_Icon *i;
	i = l->data;
	evas_set_clip(ib->e, i->image, ib->obj.clip);
      }
  }
  
  /* show the iconbar */
  evas_show(ib->e, ib->obj.scroll);
  evas_show(ib->e, ib->obj.line_l);
  evas_show(ib->e, ib->obj.line_c);
  evas_show(ib->e, ib->obj.line_r);
  evas_show(ib->e, ib->obj.title);
  evas_show(ib->e, ib->obj.clip);
  
  evas_callback_add(ib->e, ib->obj.scroll, CALLBACK_MOUSE_MOVE, s_mouse_move, ib);
  evas_callback_add(ib->e, ib->obj.scroll, CALLBACK_MOUSE_IN, s_mouse_in, ib);
  evas_callback_add(ib->e, ib->obj.scroll, CALLBACK_MOUSE_OUT, s_mouse_out, ib);
  
  e_iconbar_fix_icons(ib);

  printf("realized!\n");
}


E_Iconbar_Icon *
e_iconbar_new_icon(E_Iconbar *ib, char *image, char *exec)
{
  E_Iconbar_Icon *i; 
  
  i = NEW(E_Iconbar_Icon, 1);
  i->image = evas_add_image_from_file(ib->e, image);
  i->exec = strdup(exec);
  evas_get_image_size(ib->e, i->image, &(i->w), &(i->h));
  
  if (ib->geom.horizontal)
    {
      i->x = 0;
      i->y = (ib->geom.h - i->h - ib->geom.scroll_w) / 2 + ib->geom.top;
    }
  else
    {
      i->x = (ib->geom.w -  i->w - ib->geom.scroll_w) / 2 + ib->geom.left;
      i->y = 0;
    }
  
  printf("x: %f, y: %f\n", i->x, i->y);
  evas_callback_add(ib->e, i->image, CALLBACK_MOUSE_IN, i_mouse_in, NULL);
  evas_callback_add(ib->e, i->image, CALLBACK_MOUSE_OUT, i_mouse_out, NULL);
  evas_callback_add(ib->e, i->image, CALLBACK_MOUSE_DOWN, i_mouse_down, i->exec);
  
  evas_set_color(ib->e, i->image, 255, 255, 255, 128);
  evas_set_layer(ib->e, i->image, 400);
  /* evas_set_clip(ib->e, i->image, ib->obj.clip); */
  /* printf("before ib->icons set\n"); */
  ib->icons = evas_list_append(ib->icons, i);
  /* printf("after set\n"); */
  
  return i;
}	


int
e_iconbar_config(E_Iconbar *ib)
{
  E_DB_File *db;
  char buf[4096], *userdir;
  
  /*	userdir = e_config_user_dir(); */
  /*	sprintf(buf, "%sbehavior/iconbar.db", userdir); */
  sprintf(buf, "%s/.e_iconbar.db", ib->v->dir);
  ib->db = strdup(buf);
  db = e_db_open_read(ib->db);
  
  if (!db)
    {
      /*		ib->no_show = 1;
			
      db = e_db_open(ib->db);
      e_db_int_set(db, "/ib/num", 0);
      e_db_int_set(db, "/ib/geom/w", 75);
      e_db_int_set(db, "/ib/geom/h", 620);
      e_db_int_set(db, "/ib/geom/top", 165);
      e_db_int_set(db, "/ib/geom/left", -150);
      e_db_int_set(db, "/ib/geom/scroll_w", 16);
      e_db_int_set(db, "/ib/geom/horizontal", 0);
      e_db_int_set(db, "/ib/scroll_when_less", 0);
      e_db_str_set(db, "/ib/image/title", "/usr/local/share/enlightenment/data/ib_title.png");
      e_db_str_set(db, "/ib/image/vline", "/usr/local/share/enlightenment/data/vline.png");
      e_db_str_set(db, "/ib/image/hline", "/usr/local/share/enlightenment/data/hline.png");
      */
      return 0;
    }
  
  e_db_int_get(db, "/ib/geom/w", &(ib->geom.conf.w) );
  e_db_int_get(db, "/ib/geom/h", &(ib->geom.conf.h));
  e_db_int_get(db, "/ib/geom/top", &(ib->geom.conf.top));
  e_db_int_get(db, "/ib/geom/left", &(ib->geom.conf.left));
  e_db_int_get(db, "/ib/geom/scroll_w", &(ib->geom.scroll_w));
  e_db_int_get(db, "/ib/geom/horizontal", &(ib->geom.horizontal));
  e_db_int_get(db, "/ib/scroll_when_less", &(ib->scroll_when_less));
  ib->image.title = e_db_str_get(db, "/ib/image/title");
  ib->image.vline = e_db_str_get(db, "/ib/image/vline");
  ib->image.hline = e_db_str_get(db, "/ib/image/hline");
  
  ib->start = 0.0;
  ib->icons = NULL;
  
  ib->geom.left = ib->geom.conf.left;
  ib->geom.top = ib->geom.conf.top;
  ib->geom.w = ib->geom.conf.w;
  ib->geom.h = ib->geom.conf.h;
  
  /*
    {
    double w, h;
    evas_get_viewport(ib->e, NULL, NULL, &w, &h);
    
    if(ib->geom.conf.left < 0) 
    ib->geom.left = ib->geom.conf.left + w;
    else
    ib->geom.left = ib->geom.conf.left;
    if(ib->geom.conf.top < 0)
    ib->geom.top = ib->geom.conf.top + h;
    else
    ib->geom.top = ib->geom.conf.top;
    }

  */
  printf("w: %i, h: %i\nt: %i, l: %i\n", ib->geom.w, ib->geom.h, ib->geom.top, ib->geom.left);
  
  
  {
    int i, num;
    
    if (e_db_int_get(db, "/ib/num", &num))
      {
	printf("making %i icons...\n", num);
	
	for ( i = 0; i < num; i++)
	  {
	    char *icon, *exec, buf[4096];
	    
	    sprintf(buf, "/ib/%i/icon", i);
	    /* printf("reading #%i's icon...\n", i); */
	    icon = e_db_str_get(db, buf);
	    /* printf("icon: %s, reading #%i's exec...\n", icon, i); */
	    sprintf(buf, "/ib/%i/exec", i);
	    exec = e_db_str_get(db, buf);
	    printf("exec: %s... creating icon\n", exec);
	    
	    /* printf("creating icon, %s, %s\n", icon, exec); */
	    e_iconbar_new_icon(ib, icon, exec);
	    printf("created...\n");
	  }
      }
  }
  /* printf("created icon.\n"); */
  
  e_db_close(db);
  
  return 1;
}
	

void
e_iconbar_fix_icons(E_Iconbar *ib)
{
  Evas_List l;
  double cur, spacer;
  
  spacer = 20.0;
  
  if (!ib->geom.horizontal)
    {
      cur = ib->geom.top + ib->start + spacer;
      for (l = ib->icons; l; l = l->next)
	{
	  E_Iconbar_Icon *i;
	  
	  i = l->data;
	  i->y = cur;
	  i->x = (ib->geom.w -  i->w - ib->geom.scroll_w) / 2 + ib->geom.left;
	  cur = cur + i->h + spacer;
	  
	  evas_move(ib->e, i->image, i->x, i->y);
	  evas_show(ib->e, i->image);
	}
      
      ib->length = cur - ib->start - ib->geom.top;
    }  
  else 
    {
      cur = ib->geom.left + ib->start + spacer;
      for (l = ib->icons; l; l = l->next)
	{
	  E_Iconbar_Icon *i;
	  
	  i = l->data;
	  i->x = cur;
	  i->y = (ib->geom.h - i->h - ib->geom.scroll_w) / 2 + ib->geom.top;
	  cur = cur + i->w + spacer;
	  
	  evas_move(ib->e, i->image, i->x, i->y);
	  evas_show(ib->e, i->image);
	}
      
      ib->length = cur - ib->start - ib->geom.left;
    }
}
	
void
e_iconbar_scroll(int val, void *data)
{
  E_Iconbar *ib;
  double vis_length;
  
  /* printf("before scroll data to ib set\n"); */
  ib = data;
  /* printf("after data set\n");  */
  /* printf("start: %f, speed: %f\n", ib->start, ib->speed);  */
  ib->start = ib->start - ib->speed;
  if (ib->geom.horizontal) vis_length = ib->geom.w;
  else vis_length = ib->geom.h;
  
  if (ib->length > vis_length)
    {
      if (ib->start > 0)
	ib->start = 0;
      else if (ib->start < vis_length - ib->length)
	ib->start = vis_length - ib->length;
    }  
  else if (ib->scroll_when_less) 
    {
      /* icons scroll even if they don't fill the bar */
      if (ib->start < 0)
	ib->start = 0;
      else if (ib->start > vis_length - ib->length)
	ib->start = vis_length - ib->length;
      
    }  
  else
    {
      ib->start = 0;
    }
  /* printf("before fix\n");  */
  e_iconbar_fix_icons(ib);
  /* printf("after fix\n");  */
  
  if (ib->scrolling)
    {
      /* printf("before timer\n");  */
      ecore_add_event_timer("e_iconbar_scroll()", 0.01, e_iconbar_scroll, 1, ib);
      /* printf("after timer\n");  */
    }  
}

void
i_mouse_in(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y)
{
  evas_set_color(_e, _o, 255, 255, 255, 255);	
}

void
i_mouse_out(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y)
{
  evas_set_color(_e, _o, 255, 255, 255, 128);	
}

void
i_mouse_down(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y)
{
  char *file = _data;
  e_exec_run(file);
}


void
s_mouse_move(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y)
{	
  E_Iconbar *ib;
  
  double center;
  int top_speed;
  double r;
  
  /* printf("set ib in s_mouse_move\n"); */

  ib = _data;
  
  top_speed = 5;
  
  if (ib->geom.horizontal)
    {
      center = ib->geom.left + (.5 * ib->geom.w);
      r = (_x - center) / (.5 * ib->geom.w);
    }
  else
    {
      center = ib->geom.top + (.5 * ib->geom.h);
      r = (_y - center) / (.5 * ib->geom.h);
    }
  
  ib->speed = r * (double)top_speed;
  
  e_iconbar_scroll(1, ib);
}

void
s_mouse_in(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y)
{
  E_Iconbar *ib;
  ib = _data;
  /* printf("setting scrolling\n"); */
  ib->scrolling = 1;
  /* printf("set scrolling\n"); */
}

void
s_mouse_out(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y)
{
  E_Iconbar *ib;
  ib = _data;
  ib->scrolling = 0;
}

void
e_iconbar_free(E_Iconbar *ib)
{
  
  Evas_List l;
  
  printf("in e_iconbar_clean_up()\n");
  
  iconbars = evas_list_remove(iconbars, ib);
  
  for (l = ib->icons; l; l = l->next)
    {
      E_Iconbar_Icon *i;
      i = l->data;
      FREE(i);
    }
  FREE(ib->icons);
  FREE(ib);
  e_db_runtime_flush();
  
  printf("e_iconbar_clean_up() run successfully\n");
}


static void
e_iconbar_idle(void *data)
{
  Evas_List l;
  
  for (l = iconbars; l; l = l->next)
    {
      E_Iconbar *ib;
      
      ib = l->data;
      e_iconbar_update(ib);
      /* e_iconbar_redraw(ib);  */
    }
}

void
e_iconbar_update(E_Iconbar *ib)
{
  if (ib->v->changed)
    {
      e_iconbar_redraw(ib);
    }
}


void
e_iconbar_redraw(E_Iconbar *ib)
{
  if(ib->geom.conf.left < 0)
    ib->geom.left = ib->geom.conf.left + ib->v->size.w;
  if(ib->geom.conf.top < 0)
    ib->geom.top = ib->geom.conf.top + ib->v->size.h;
  if(ib->geom.conf.w == 0)
    ib->geom.w = ib->v->size.w;
  if(ib->geom.conf.h == 0)
    ib->geom.h = ib->v->size.h;
  
    
  /* horizontal */
  if (ib->geom.horizontal)
    {
      evas_move(ib->e, ib->obj.scroll, ib->geom.left, ib->geom.top + ib->geom.h - ib->geom.scroll_w);
      evas_move(ib->e, ib->obj.line_l, ib->geom.left, ib->geom.top);
      evas_move(ib->e, ib->obj.line_c, ib->geom.left, ib->geom.top + ib->geom.h - ib->geom.scroll_w);
      evas_move(ib->e, ib->obj.line_r, ib->geom.left, ib->geom.top + ib->geom.h);
      evas_move(ib->e, ib->obj.title, ib->geom.left - ib->geom.title_w - 5, ib->geom.top + (ib->geom.h - ib->geom.title_h) / 2);
      evas_move(ib->e, ib->obj.clip, ib->geom.left, ib->geom.top);
    }
  
  /* vertical */
  if (!ib->geom.horizontal)
    {
      evas_move(ib->e, ib->obj.scroll, ib->geom.left + ib->geom.w - ib->geom.scroll_w, ib->geom.top);
      evas_move(ib->e, ib->obj.line_l, ib->geom.left, ib->geom.top);
      evas_move(ib->e, ib->obj.line_c, ib->geom.left + ib->geom.w - ib->geom.scroll_w, ib->geom.top);
      evas_move(ib->e, ib->obj.line_r, ib->geom.left + ib->geom.w, ib->geom.top);
      evas_move(ib->e, ib->obj.title, ib->geom.left + ((ib->geom.w - ib->geom.title_w - ib->geom.scroll_w) / 2 ), ib->geom.top - ib->geom.title_h - 5);
      evas_move(ib->e, ib->obj.clip, ib->geom.left, ib->geom.top);
    }
  
  
  /* Clip icons */
  {
    Evas_List l;
    for (l = ib->icons; l; l = l->next)
      {
	E_Iconbar_Icon *i;
	i = l->data;
	evas_set_clip(ib->e, i->image, ib->obj.clip);
      }
  }
  
  /* show the iconbar */
  evas_show(ib->e, ib->obj.scroll);
  evas_show(ib->e, ib->obj.line_l);
  evas_show(ib->e, ib->obj.line_c);
  evas_show(ib->e, ib->obj.line_r);
  evas_show(ib->e, ib->obj.title);
  evas_show(ib->e, ib->obj.clip);
  
  evas_callback_add(ib->e, ib->obj.scroll, CALLBACK_MOUSE_MOVE, s_mouse_move, ib);
  evas_callback_add(ib->e, ib->obj.scroll, CALLBACK_MOUSE_IN, s_mouse_in, ib);
  evas_callback_add(ib->e, ib->obj.scroll, CALLBACK_MOUSE_OUT, s_mouse_out, ib);
  
  e_iconbar_fix_icons(ib);
}

void
e_iconbar_init()
{
  ecore_event_filter_idle_handler_add(e_iconbar_idle, NULL);
}
