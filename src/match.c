#include "debug.h"
#include "match.h"
#include "config.h"

void
e_match_set_props(E_Border * b)
{
   char                buf[PATH_MAX];
   E_DB_File          *db;
   int                 ok;

   D_ENTER;

   if ((!b->client.name) || (!b->client.class))
      D_RETURN;
   db = e_db_open(e_config_get("match"));
   snprintf(buf, PATH_MAX, "match/%s/%s/match", b->client.name,
	    b->client.class);
   ok = e_db_int_get(db, buf, &(b->client.matched.matched));
   if (!ok)
     {
	e_db_close(db);
	D_RETURN;
     }
   snprintf(buf, PATH_MAX, "match/%s/%s/prog_location/ignore", b->client.name,
	    b->client.class);
   b->client.matched.prog_location.matched =
      e_db_int_get(db, buf, &(b->client.matched.prog_location.ignore));
   snprintf(buf, PATH_MAX, "match/%s/%s/border/border", b->client.name,
	    b->client.class);
   b->client.matched.border.style = e_db_str_get(db, buf);
   b->client.matched.border.matched = (int)b->client.matched.border.style;
   snprintf(buf, PATH_MAX, "match/%s/%s/location/x", b->client.name,
	    b->client.class);
   b->client.matched.location.matched =
      e_db_int_get(db, buf, &(b->client.matched.location.x));
   snprintf(buf, PATH_MAX, "match/%s/%s/location/y", b->client.name,
	    b->client.class);
   b->client.matched.location.matched =
      e_db_int_get(db, buf, &(b->client.matched.location.y));
   snprintf(buf, PATH_MAX, "match/%s/%s/desk_area/x", b->client.name,
	    b->client.class);
   b->client.matched.desk_area.matched =
      e_db_int_get(db, buf, &(b->client.matched.desk_area.x));
   snprintf(buf, PATH_MAX, "match/%s/%s/desk_area/y", b->client.name,
	    b->client.class);
   b->client.matched.desk_area.matched =
      e_db_int_get(db, buf, &(b->client.matched.desk_area.y));
   snprintf(buf, PATH_MAX, "match/%s/%s/size/w", b->client.name,
	    b->client.class);
   b->client.matched.size.matched =
      e_db_int_get(db, buf, &(b->client.matched.size.w));
   snprintf(buf, PATH_MAX, "match/%s/%s/size/h", b->client.name,
	    b->client.class);
   b->client.matched.size.matched =
      e_db_int_get(db, buf, &(b->client.matched.size.h));
   snprintf(buf, PATH_MAX, "match/%s/%s/desktop/desk", b->client.name,
	    b->client.class);
   b->client.matched.desktop.matched =
      e_db_int_get(db, buf, &(b->client.matched.desktop.desk));
   snprintf(buf, PATH_MAX, "match/%s/%s/sticky/sticky", b->client.name,
	    b->client.class);
   b->client.matched.sticky.matched =
      e_db_int_get(db, buf, &(b->client.matched.sticky.sticky));
   snprintf(buf, PATH_MAX, "match/%s/%s/layer/layer", b->client.name,
	    b->client.class);
   b->client.matched.layer.matched =
      e_db_int_get(db, buf, &(b->client.matched.layer.layer));

   if (b->client.matched.prog_location.matched)
     {
	b->client.pos.requested = 0;
     }
   if (b->client.matched.border.matched)
     {
	IF_FREE(b->border_style);
	b->border_style = b->client.matched.border.style;
     }
   if (b->client.matched.location.matched)
     {
	b->client.pos.requested = 1;
	b->client.pos.gravity = NorthWestGravity;
	b->client.pos.x = b->client.matched.location.x;
	b->client.pos.y = b->client.matched.location.y;
	b->client.no_place = 1;
     }
   if (b->client.matched.desk_area.matched)
     {
	b->client.pos.x +=
	   (b->client.matched.desk_area.x -
	    b->desk->desk.area.x) * b->desk->real.w;
	b->client.pos.y +=
	   (b->client.matched.desk_area.y -
	    b->desk->desk.area.y) * b->desk->real.h;
	b->client.area.x = b->client.matched.desk_area.x;
	b->client.area.y = b->client.matched.desk_area.y;
     }
   if (b->client.matched.size.matched)
     {
	b->current.requested.w = b->client.matched.size.w;
	b->current.requested.h = b->client.matched.size.h;
	ecore_window_resize(b->win.client, b->client.matched.size.w,
			    b->client.matched.size.h);
     }
   if (b->client.matched.desktop.matched)
     {
	b->client.desk = b->client.matched.desktop.desk;
	e_border_raise(b);
	if (b->client.desk != b->desk->desk.desk)
	   b->current.requested.visible = 0;
	b->client.no_place = 1;
     }
   if (b->client.matched.sticky.matched)
     {
	b->client.sticky = b->client.matched.sticky.sticky;
     }
   if (b->client.matched.layer.matched)
     {
	b->client.layer = b->client.matched.layer.layer;
     }

   e_db_close(db);

   D_RETURN;
}

void
e_match_save_props(E_Border * b)
{
   char                buf[PATH_MAX];
   E_DB_File          *db;

   D_ENTER;

   if ((!b->client.name) || (!b->client.class))
      D_RETURN;

   db = e_db_open(e_config_get("match"));
   if (!db)
      D_RETURN;

   snprintf(buf, PATH_MAX, "match/%s/%s/match", b->client.name,
	    b->client.class);
   e_db_int_set(db, buf, b->client.matched.matched);

   if (b->client.matched.location.matched)
     {
	b->client.matched.location.x = b->current.x;
	b->client.matched.location.y = b->current.y;
	snprintf(buf, PATH_MAX, "match/%s/%s/location/x", b->client.name,
		 b->client.class);
	e_db_int_set(db, buf, b->client.matched.location.x);
	snprintf(buf, PATH_MAX, "match/%s/%s/location/y", b->client.name,
		 b->client.class);
	e_db_int_set(db, buf, b->client.matched.location.y);
     }
   else
     {
	snprintf(buf, PATH_MAX, "match/%s/%s/location/x", b->client.name,
		 b->client.class);
	e_db_data_del(db, buf);
	snprintf(buf, PATH_MAX, "match/%s/%s/location/y", b->client.name,
		 b->client.class);
	e_db_data_del(db, buf);
     }

   if (b->client.matched.size.matched)
     {
	b->client.matched.size.w = b->client.w;
	b->client.matched.size.h = b->client.h;
	snprintf(buf, PATH_MAX, "match/%s/%s/size/w", b->client.name,
		 b->client.class);
	e_db_int_set(db, buf, b->client.matched.size.w);
	snprintf(buf, PATH_MAX, "match/%s/%s/size/h", b->client.name,
		 b->client.class);
	e_db_int_set(db, buf, b->client.matched.size.h);
     }
   else
     {
	snprintf(buf, PATH_MAX, "match/%s/%s/size/w", b->client.name,
		 b->client.class);
	e_db_data_del(db, buf);
	snprintf(buf, PATH_MAX, "match/%s/%s/size/h", b->client.name,
		 b->client.class);
	e_db_data_del(db, buf);
     }

   if (b->client.matched.desktop.matched)
     {
	b->client.matched.desktop.desk = b->client.desk;
	snprintf(buf, PATH_MAX, "match/%s/%s/desktop/desk", b->client.name,
		 b->client.class);
	e_db_int_set(db, buf, b->client.matched.desktop.desk);
     }
   else
     {
	snprintf(buf, PATH_MAX, "match/%s/%s/desktop/desk", b->client.name,
		 b->client.class);
	e_db_data_del(db, buf);
     }

   if (b->client.matched.sticky.matched)
     {
	b->client.matched.sticky.sticky = b->client.sticky;
	snprintf(buf, PATH_MAX, "match/%s/%s/sticky/sticky", b->client.name,
		 b->client.class);
	e_db_int_set(db, buf, b->client.matched.sticky.sticky);
     }
   else
     {
	snprintf(buf, PATH_MAX, "match/%s/%s/sticky/sticky", b->client.name,
		 b->client.class);
	e_db_data_del(db, buf);
     }

   if (b->client.matched.prog_location.matched)
     {
	snprintf(buf, PATH_MAX, "match/%s/%s/prog_location/ignore",
		 b->client.name, b->client.class);
	e_db_int_set(db, buf, b->client.matched.prog_location.ignore);
     }
   else
     {
	snprintf(buf, PATH_MAX, "match/%s/%s/prog_location/ignore",
		 b->client.name, b->client.class);
	e_db_data_del(db, buf);
     }

   e_db_close(db);
   e_db_runtime_flush();
   D_RETURN;
}
