#include "match.h"

void
e_match_set_props(E_Border *b)
{
#if 0   
   /* if we have a match that says to ignore prog coords: */
   b->client.pos.requested = 0;
   /* if we have a match that applies a specifi border: */
   IF_FREE(b->border_style);
   e_strdup(b->border_style, match_style);
   /* if we have a match that specifies a location */
   b->client.pos.requested = 1;
   b->client.pos.gravity = NorthWestGravity;
   b->client.pos.x = match_x;
   b->client.pos.y = match_y;
   b->client.no_place = 1;
   /* if we have a match specifying desk area (only valid with loc match */
   b->client.pos.x += (match_area_x - b->desk->desk.area.x) * b->desk->real.w;
   b->client.pos.y += (match_area_y - b->desk->desk.area.y) * b->desk->real.h;
   b->client.area.x = match_area_x;
   b->client.area.y = match_area_y;
   /* if we have a match specifying a size */
   b->current.requested.w = match_w;
   b->current.requested.h = match_h;
   e_window_resize(b->win.client, match_w, match_h);
   /* if we have a match specifying a desktop */
   b->client.desk = match_desk;   
   e_border_raise(b);
   if (b->client.desk != b->desk->desk.desk) b->current.requested.visible = 0;
   b->client.no_place = 1;
   /* if we have a match specifying stickyness */
   b->client.sticky = match_sticky;
   /* if we have a match specifying layer */
   b->client.layer = match_layer;
#endif   
}
