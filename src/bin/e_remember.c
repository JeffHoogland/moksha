/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

#define REMEMBER_HIERARCHY 1
#define REMEMBER_SIMPLE 0

/* local subsystem functions */
static void _e_remember_free(E_Remember *rem);
static int _e_remember_sort_list(const void * d1, const void * d2);
static E_Remember  *_e_remember_find(E_Border *bd, int check_usable);
static void _e_remember_cb_hook_pre_post_fetch(void *data, E_Border *bd);
static void _e_remember_cb_hook_eval_post_new_border(void *data, E_Border *bd);
static Eina_List *hooks = NULL;

/* FIXME: match netwm window type */

/* local subsystem globals */

/* externally accessible functions */
EAPI int
e_remember_init(E_Startup_Mode mode)
{
   Eina_List *l = NULL;
   E_Remember *rem;
   int not_updated = 0;
   E_Border_Hook *h;

   if (mode == E_STARTUP_START)
     {
	EINA_LIST_FOREACH(e_config->remembers, l, rem)
	  {
	     if ((rem->apply & E_REMEMBER_APPLY_RUN) && (rem->prop.command))
	       e_util_head_exec(rem->prop.head, rem->prop.command);
	  }
     }

   h = e_border_hook_add(E_BORDER_HOOK_EVAL_PRE_POST_FETCH,
			 _e_remember_cb_hook_pre_post_fetch, NULL);
   if (h) hooks = eina_list_append(hooks, h);
   h = e_border_hook_add(E_BORDER_HOOK_EVAL_POST_NEW_BORDER,
			 _e_remember_cb_hook_eval_post_new_border, NULL);
   if (h) hooks = eina_list_append(hooks, h);

#if 1
   EINA_LIST_FOREACH(e_config->remembers, l, rem)
     {
	/* Code to bring up old configs to scratch. Can be removed
	 * after X amount of time, where X is a figure that will be
	 * decided by whoever remembers to take this out. */
	if (!rem->max_score)
	  {
	     int max_count = 0;
	     if (rem->match & E_REMEMBER_MATCH_NAME) max_count += 2;
	     if (rem->match & E_REMEMBER_MATCH_CLASS) max_count += 2;
	     if (rem->match & E_REMEMBER_MATCH_TITLE) max_count += 2;
	     if (rem->match & E_REMEMBER_MATCH_ROLE) max_count += 2;
	     if (rem->match & E_REMEMBER_MATCH_TYPE) max_count += 2;
	     if (rem->match & E_REMEMBER_MATCH_TRANSIENT) max_count +=2;
	     if (rem->apply_first_only) max_count++;
	     rem->max_score = max_count;
	     not_updated = 1;
	  }
     }

   if (not_updated)
     e_config->remembers = eina_list_sort(e_config->remembers, -1,
                                          _e_remember_sort_list);
#endif
   return 1;
}

EAPI int
e_remember_shutdown(void)
{
   E_Border_Hook *h;

   EINA_LIST_FREE(hooks, h)
     e_border_hook_del(h);

   return 1;
}

EAPI E_Remember *
e_remember_new(void)
{
   E_Remember *rem;

   rem = E_NEW(E_Remember, 1);
   if (!rem) return NULL;
   e_config->remembers = eina_list_prepend(e_config->remembers, rem);
   return rem;
}

EAPI int
e_remember_usable_get(E_Remember *rem)
{
   if ((rem->apply_first_only) && (rem->used_count > 0)) return 0;
   return 1;
}

EAPI void
e_remember_use(E_Remember *rem)
{
   rem->used_count++;
}

EAPI void
e_remember_unuse(E_Remember *rem)
{
   if (rem->used_count > 0)
     rem->used_count--;
   if ((rem->used_count == 0) && (rem->delete_me))
     _e_remember_free(rem);
}

EAPI void
e_remember_del(E_Remember *rem)
{
   if (rem->delete_me) return;
   if (rem->used_count != 0)
     {
	Eina_List *l = NULL;
	E_Border *bd;

	rem->delete_me = 1;
	EINA_LIST_FOREACH(e_border_client_list(), l, bd)
	  {
	     if (bd->remember == rem)
	       {
		  bd->remember = NULL;
		  e_remember_unuse(rem);
	       }
	  }
	return;
     }
   else
     _e_remember_free(rem); 
}

EAPI E_Remember *
e_remember_find_usable(E_Border *bd)
{
   E_Remember *rem;

   rem = _e_remember_find(bd, 1);
   return rem;
}

EAPI E_Remember *
e_remember_find(E_Border *bd)
{
   E_Remember *rem;

   rem = _e_remember_find(bd, 0);
   return rem;
}

EAPI void
e_remember_match_update(E_Remember *rem)
{
   int max_count = 0;

   if (rem->match & E_REMEMBER_MATCH_NAME) max_count += 2;
   if (rem->match & E_REMEMBER_MATCH_CLASS) max_count += 2;
   if (rem->match & E_REMEMBER_MATCH_TITLE) max_count += 2;
   if (rem->match & E_REMEMBER_MATCH_ROLE) max_count += 2;
   if (rem->match & E_REMEMBER_MATCH_TYPE) max_count += 2;
   if (rem->match & E_REMEMBER_MATCH_TRANSIENT) max_count +=2;
   if (rem->apply_first_only) max_count++;

   if (max_count != rem->max_score)
     {
	/* The number of matches for this remember has changed so we
	 * need to remove from list and insert back into the appropriate
	 * loction. */
	Eina_List *l = NULL;
	E_Remember *r;

	rem->max_score = max_count;
	e_config->remembers = eina_list_remove(e_config->remembers, rem);

	EINA_LIST_FOREACH(e_config->remembers, l, r)
	  {
	     if (r->max_score <= rem->max_score) break;
	  }

	if (l)
	   e_config->remembers = eina_list_prepend_relative_list(e_config->remembers, rem, l);
	else
	   e_config->remembers = eina_list_append(e_config->remembers, rem);
     }
}

EAPI int
e_remember_default_match_set(E_Remember *rem, E_Border *bd)
{
   const char *title, *clasz, *name, *role;
   
   if (rem->name) eina_stringshare_del(rem->name);
   if (rem->class) eina_stringshare_del(rem->class);
   if (rem->title) eina_stringshare_del(rem->title);
   if (rem->role) eina_stringshare_del(rem->role);
   rem->name = NULL;
   rem->class = NULL;
   rem->title = NULL;
   rem->role = NULL;

   name = bd->client.icccm.name;
   if (!name || name[0] == 0) name = NULL;
   clasz = bd->client.icccm.class;
   if (!clasz || clasz[0] == 0) clasz = NULL;
   role = bd->client.icccm.window_role;
   if (!role || role[0] == 0) role = NULL;
   
   int match = E_REMEMBER_MATCH_TRANSIENT;
   if (name && clasz)
     {
	match |= E_REMEMBER_MATCH_NAME | E_REMEMBER_MATCH_CLASS;
	rem->name = eina_stringshare_add(name);
	rem->class = eina_stringshare_add(clasz);
     }
   else if ((title = e_border_name_get(bd)) && title[0])
     {
	match |= E_REMEMBER_MATCH_TITLE;
	rem->title = eina_stringshare_add(title);
     }   
   if (role)
     {
	match |= E_REMEMBER_MATCH_ROLE;
	rem->role = eina_stringshare_add(role);
     }

   if (bd->client.icccm.transient_for != 0)
     rem->transient = 1;
   else
     rem->transient = 0;

   rem->match = match;
   
   return match;
}

EAPI void
e_remember_update(E_Border *bd)
{
   E_Remember *rem;
   
   if (bd->new_client) return;
   if (!bd->remember) return;
   if (bd->remember->keep_settings) return;
   
   rem = bd->remember;

   if (!(rem->name  || rem->class ||
	 rem->title || rem->role))
     {
	e_remember_del(rem); 
	return;
     }
   
   e_remember_match_update(rem);

   rem->type = bd->client.netwm.type;

   if (bd->fullscreen)
     {
	rem->prop.fullscreen = bd->fullscreen;
	rem->prop.pos_x = bd->saved.x;
	rem->prop.pos_y = bd->saved.y;
	rem->prop.pos_w = bd->saved.w;
	rem->prop.pos_h = bd->saved.h;
	rem->prop.layer = bd->saved.layer;
     }
   else
     {
	rem->prop.pos_x = bd->x - bd->zone->x;
	rem->prop.pos_y = bd->y - bd->zone->y;
	rem->prop.res_x = bd->zone->w;
	rem->prop.res_y = bd->zone->h;
	rem->prop.pos_w = bd->client.w;
	rem->prop.pos_h = bd->client.h;
	rem->prop.w = bd->client.w;
	rem->prop.h = bd->client.h;
	rem->prop.layer = bd->layer;
	rem->prop.fullscreen = bd->fullscreen;
     }

   rem->prop.lock_user_location = bd->lock_user_location;
   rem->prop.lock_client_location = bd->lock_client_location;
   rem->prop.lock_user_size = bd->lock_user_size;
   rem->prop.lock_client_size = bd->lock_client_size;
   rem->prop.lock_user_stacking = bd->lock_user_stacking;
   rem->prop.lock_client_stacking = bd->lock_client_stacking;
   rem->prop.lock_user_iconify = bd->lock_user_iconify;
   rem->prop.lock_client_iconify = bd->lock_client_iconify;
   rem->prop.lock_user_desk = bd->lock_user_desk;
   rem->prop.lock_client_desk = bd->lock_client_desk;
   rem->prop.lock_user_sticky = bd->lock_user_sticky;
   rem->prop.lock_client_sticky = bd->lock_client_sticky;
   rem->prop.lock_user_shade = bd->lock_user_shade;
   rem->prop.lock_client_shade = bd->lock_client_shade;
   rem->prop.lock_user_maximize = bd->lock_user_maximize;
   rem->prop.lock_client_maximize = bd->lock_client_maximize;
   rem->prop.lock_user_fullscreen = bd->lock_user_fullscreen;
   rem->prop.lock_client_fullscreen = bd->lock_client_fullscreen;
   rem->prop.lock_border = bd->lock_border;
   rem->prop.lock_close = bd->lock_close;
   rem->prop.lock_focus_in = bd->lock_focus_in;
   rem->prop.lock_focus_out = bd->lock_focus_out;
   rem->prop.lock_life = bd->lock_life;

   rem->prop.sticky = bd->sticky;

   if (bd->shaded)
     rem->prop.shaded = (100 + bd->shade.dir);
   else
     rem->prop.shaded = (50 + bd->shade.dir);

   rem->prop.skip_winlist = bd->user_skip_winlist;
   rem->prop.skip_pager   = bd->client.netwm.state.skip_pager;
   rem->prop.skip_taskbar = bd->client.netwm.state.skip_taskbar;
   rem->prop.icon_preference = bd->icon_preference;

   e_desk_xy_get(bd->desk, &rem->prop.desk_x, &rem->prop.desk_y);

   rem->prop.zone = bd->zone->num;
   rem->prop.head = bd->zone->container->manager->num;

   e_config_save_queue();
}

/* local subsystem functions */
static E_Remember *
_e_remember_find(E_Border *bd, int check_usable)
{
   Eina_List *l = NULL;
   E_Remember *rem;

#if REMEMBER_SIMPLE
   EINA_LIST_FOREACH(e_config->remembers, l, rem)
     {
	int required_matches;
	int matches;
	const char *title = "";

	matches = 0;
	required_matches = 0;
	if (rem->match & E_REMEMBER_MATCH_NAME) required_matches++;
	if (rem->match & E_REMEMBER_MATCH_CLASS) required_matches++;
	if (rem->match & E_REMEMBER_MATCH_TITLE) required_matches++;
	if (rem->match & E_REMEMBER_MATCH_ROLE) required_matches++;
	if (rem->match & E_REMEMBER_MATCH_TYPE) required_matches++;
	if (rem->match & E_REMEMBER_MATCH_TRANSIENT) required_matches++;

	if (bd->client.netwm.name) title = bd->client.netwm.name;
	else title = bd->client.icccm.title;

	if ((rem->match & E_REMEMBER_MATCH_NAME) &&
	    ((!e_util_strcmp(rem->name, bd->client.icccm.name)) ||
	     (e_util_both_str_empty(rem->name, bd->client.icccm.name))))
	  matches++;
	if ((rem->match & E_REMEMBER_MATCH_CLASS) &&
	    ((!e_util_strcmp(rem->class, bd->client.icccm.class)) ||
	     (e_util_both_str_empty(rem->class, bd->client.icccm.class))))
	  matches++;
	if ((rem->match & E_REMEMBER_MATCH_TITLE) &&
	    ((!e_util_strcmp(rem->title, title)) ||
	     (e_util_both_str_empty(rem->title, title))))
	  matches++;
	if ((rem->match & E_REMEMBER_MATCH_ROLE) &&
	    ((!e_util_strcmp(rem->role, bd->client.icccm.window_role)) ||
	     (e_util_both_str_empty(rem->role, bd->client.icccm.window_role))))
	  matches++;
	if ((rem->match & E_REMEMBER_MATCH_TYPE) &&
	    (rem->type == bd->client.netwm.type))
	  matches++;
	if ((rem->match & E_REMEMBER_MATCH_TRANSIENT) &&
	    (((rem->transient) && (bd->client.icccm.transient_for != 0)) ||
	     ((!rem->transient) && (bd->client.icccm.transient_for == 0))))
	  matches++;
	 if ((matches >= required_matches) && (!rem->delete_me))
	  return rem;
     }
   return NULL;
#endif
#if REMEMBER_HIERARCHY
   /* This search method finds the best possible match available and is
    * based on the fact that the list is sorted, with those remembers
    * with the most possible matches at the start of the list. This
    * means, as soon as a valid match is found, it is a match
    * within the set of best possible matches. */
   EINA_LIST_FOREACH(e_config->remembers, l, rem)
     {
        const char *title = "";

        if ((check_usable) && (!e_remember_usable_get(rem)))
          continue;

        if (bd->client.netwm.name) title = bd->client.netwm.name;
        else title = bd->client.icccm.title;

        /* For each type of match, check whether the match is
         * required, and if it is, check whether there's a match. If
         * it fails, then go to the next remember */
        if (rem->match & E_REMEMBER_MATCH_NAME &&
	    !e_util_glob_match(bd->client.icccm.name, rem->name))
          continue;
        if (rem->match & E_REMEMBER_MATCH_CLASS &&
	    !e_util_glob_match(bd->client.icccm.class, rem->class))
          continue;
        if (rem->match & E_REMEMBER_MATCH_TITLE &&
	    !e_util_glob_match(title, rem->title))
          continue;
        if (rem->match & E_REMEMBER_MATCH_ROLE &&
            e_util_strcmp(rem->role, bd->client.icccm.window_role) &&
            !e_util_both_str_empty(rem->role, bd->client.icccm.window_role))
          continue;
        if (rem->match & E_REMEMBER_MATCH_TYPE &&
            rem->type != bd->client.netwm.type)
          continue;
        if (rem->match & E_REMEMBER_MATCH_TRANSIENT &&
            !(rem->transient && bd->client.icccm.transient_for != 0) &&
            !(!rem->transient) && (bd->client.icccm.transient_for == 0))
          continue;

        return rem;
     }

   return NULL;
#endif
}

static void
_e_remember_free(E_Remember *rem)
{
   e_config->remembers = eina_list_remove(e_config->remembers, rem);
   if (rem->name) eina_stringshare_del(rem->name);
   if (rem->class) eina_stringshare_del(rem->class);
   if (rem->title) eina_stringshare_del(rem->title);
   if (rem->role) eina_stringshare_del(rem->role);
   if (rem->prop.border) eina_stringshare_del(rem->prop.border);
   if (rem->prop.command) eina_stringshare_del(rem->prop.command);
   free(rem);
}

static int
_e_remember_sort_list(const void * d1, const void * d2)
{
   const E_Remember *r1, *r2;

   r1 = d1;
   r2 = d2;
   if (r1->max_score >= r2->max_score)
     return -1;
   else
     return 1;
}

static void
_e_remember_cb_hook_pre_post_fetch(void *data, E_Border *bd)
{
   E_Remember *rem = NULL;

   if (!bd->new_client) return;

   if (!bd->remember)
     {
	rem = e_remember_find_usable(bd);
	if (rem)
	  {
	     bd->remember = rem;
	     e_remember_use(rem);
	  }
     }
   if (bd->remember)
     {
	rem = bd->remember;

	if (rem->apply & E_REMEMBER_APPLY_ZONE)
	  {
	     E_Zone *zone;

	     zone = e_container_zone_number_get(bd->zone->container, rem->prop.zone);
	     if (zone)
	       e_border_zone_set(bd, zone);
	  }
	if (rem->apply & E_REMEMBER_APPLY_DESKTOP)
	  {
	     E_Desk *desk;

	     desk = e_desk_at_xy_get(bd->zone, rem->prop.desk_x, rem->prop.desk_y);
	     if (desk)
	       {
		  e_border_desk_set(bd, desk);
		  if (e_config->desk_auto_switch)
		    e_desk_show(desk);
	       }
	  }
	if (rem->apply & E_REMEMBER_APPLY_SIZE)
	  {
	     bd->client.w = rem->prop.w;
	     bd->client.h = rem->prop.h;
	     /* we can trust internal windows */
	     if (bd->internal)
	       {
		  if (bd->zone->w != rem->prop.res_x)
		    {
		       if (bd->client.w > (bd->zone->w - 64))
			 bd->client.w = bd->zone->w - 64;
		    }
		  if (bd->zone->h != rem->prop.res_y)
		    {
		       if (bd->client.h > (bd->zone->h - 64))
			 bd->client.h = bd->zone->h - 64;
		    }
		  if (bd->client.icccm.min_w > bd->client.w)
		    bd->client.w = bd->client.icccm.min_w;
		  if (bd->client.icccm.max_w < bd->client.w)
		    bd->client.w = bd->client.icccm.max_w;
		  if (bd->client.icccm.min_h > bd->client.h)
		    bd->client.h = bd->client.icccm.min_h;
		  if (bd->client.icccm.max_h < bd->client.h)
		    bd->client.h = bd->client.icccm.max_h;
	       }
	     bd->w = bd->client.w + bd->client_inset.l + bd->client_inset.r;
	     bd->h = bd->client.h + bd->client_inset.t + bd->client_inset.b;
	     bd->changes.size = 1;
	     bd->changes.shape = 1;
	  }
	if ((rem->apply & E_REMEMBER_APPLY_POS) && (!bd->re_manage))
	  {
	     bd->x = rem->prop.pos_x;
	     bd->y = rem->prop.pos_y;
	     if (bd->zone->w != rem->prop.res_x)
	       {
		  int px;

		  px = bd->x + (bd->w / 2);
		  if (px < ((rem->prop.res_x * 1) / 3))
		    {
		       if (bd->zone->w >= (rem->prop.res_x / 3))
			 bd->x = rem->prop.pos_x;
		       else
			 bd->x = ((rem->prop.pos_x - 0) * bd->zone->w) /
			   (rem->prop.res_x / 3);
		    }
		  else if (px < ((rem->prop.res_x * 2) / 3))
		    {
		       if (bd->zone->w >= (rem->prop.res_x / 3))
			 bd->x = (bd->zone->w / 2) +
			   (px - (rem->prop.res_x / 2)) -
			   (bd->w / 2);
		       else
			 bd->x = (bd->zone->w / 2) +
			   (((px - (rem->prop.res_x / 2)) * bd->zone->w) /
			    (rem->prop.res_x / 3)) -
			   (bd->w / 2);
		    }
		  else
		    {
		       if (bd->zone->w >= (rem->prop.res_x / 3))
			 bd->x = bd->zone->w +
			   rem->prop.pos_x - rem->prop.res_x +
			   (rem->prop.w - bd->client.w);
		       else
			 bd->x = bd->zone->w +
			   (((rem->prop.pos_x - rem->prop.res_x) * bd->zone->w) /
			    (rem->prop.res_x / 3)) +
			   (rem->prop.w - bd->client.w);
		    }
		  if ((rem->prop.pos_x >= 0) && (bd->x < 0))
		    bd->x = 0;
		  else if (((rem->prop.pos_x + rem->prop.w) < rem->prop.res_x) &&
			   ((bd->x + bd->w) > bd->zone->w))
		    bd->x = bd->zone->w - bd->w;
	       }
	     if (bd->zone->h != rem->prop.res_y)
	       {
		  int py;

		  py = bd->y + (bd->h / 2);
		  if (py < ((rem->prop.res_y * 1) / 3))
		    {
		       if (bd->zone->h >= (rem->prop.res_y / 3))
			 bd->y = rem->prop.pos_y;
		       else
			 bd->y = ((rem->prop.pos_y - 0) * bd->zone->h) /
			   (rem->prop.res_y / 3);
		    }
		  else if (py < ((rem->prop.res_y * 2) / 3))
		    {
		       if (bd->zone->h >= (rem->prop.res_y / 3))
			 bd->y = (bd->zone->h / 2) +
			   (py - (rem->prop.res_y / 2)) -
			   (bd->h / 2);
		       else
			 bd->y = (bd->zone->h / 2) +
			   (((py - (rem->prop.res_y / 2)) * bd->zone->h) /
			    (rem->prop.res_y / 3)) -
			   (bd->h / 2);
		    }
		  else
		    {
		       if (bd->zone->h >= (rem->prop.res_y / 3))
			 bd->y = bd->zone->h +
			   rem->prop.pos_y - rem->prop.res_y +
			   (rem->prop.h - bd->client.h);
		       else
			 bd->y = bd->zone->h +
			   (((rem->prop.pos_y - rem->prop.res_y) * bd->zone->h) /
			    (rem->prop.res_y / 3)) +
			   (rem->prop.h - bd->client.h);
		    }
		  if ((rem->prop.pos_y >= 0) && (bd->y < 0))
		    bd->y = 0;
		  else if (((rem->prop.pos_y + rem->prop.h) < rem->prop.res_y) &&
			   ((bd->y + bd->h) > bd->zone->h))
		    bd->y = bd->zone->h - bd->h;
	       }
	     //		  if (bd->zone->w != rem->prop.res_x)
	     //		    bd->x = (rem->prop.pos_x * bd->zone->w) / rem->prop.res_x;
	     //		  if (bd->zone->h != rem->prop.res_y)
	     //		    bd->y = (rem->prop.pos_y * bd->zone->h) / rem->prop.res_y;
	     bd->x += bd->zone->x;
	     bd->y += bd->zone->y;
	     bd->placed = 1;
	     bd->changes.pos = 1;
	  }
	if (rem->apply & E_REMEMBER_APPLY_LAYER)
	  {
	     bd->layer = rem->prop.layer;
	     if (bd->layer == 100)
	       e_hints_window_stacking_set(bd, E_STACKING_NONE);
	     else if (bd->layer == 150)
	       e_hints_window_stacking_set(bd, E_STACKING_ABOVE);
	     e_container_border_raise(bd);
	  }
	if (rem->apply & E_REMEMBER_APPLY_BORDER)
	  {
	     if (rem->prop.border)
	       {
		  if (bd->bordername) eina_stringshare_del(bd->bordername);
		  if (rem->prop.border) bd->bordername = eina_stringshare_add(rem->prop.border);
		  else bd->bordername = NULL;
		  bd->client.border.changed = 1;
	       }
	  }
	if (rem->apply & E_REMEMBER_APPLY_FULLSCREEN)
	  {
	     if (rem->prop.fullscreen)
	       e_border_fullscreen(bd, e_config->fullscreen_policy);
	  }
	if (rem->apply & E_REMEMBER_APPLY_STICKY)
	  {
	     if (rem->prop.sticky) e_border_stick(bd);
	  }
	if (rem->apply & E_REMEMBER_APPLY_SHADE)
	  {
	     if (rem->prop.shaded >= 100)
	       e_border_shade(bd, rem->prop.shaded - 100);
	     else if (rem->prop.shaded >= 50)
	       e_border_unshade(bd, rem->prop.shaded - 50);
	  }
	if (rem->apply & E_REMEMBER_APPLY_LOCKS)
	  {
	     bd->lock_user_location = rem->prop.lock_user_location;
	     bd->lock_client_location = rem->prop.lock_client_location;
	     bd->lock_user_size = rem->prop.lock_user_size;
	     bd->lock_client_size = rem->prop.lock_client_size;
	     bd->lock_user_stacking = rem->prop.lock_user_stacking;
	     bd->lock_client_stacking = rem->prop.lock_client_stacking;
	     bd->lock_user_iconify = rem->prop.lock_user_iconify;
	     bd->lock_client_iconify = rem->prop.lock_client_iconify;
	     bd->lock_user_desk = rem->prop.lock_user_desk;
	     bd->lock_client_desk = rem->prop.lock_client_desk;
	     bd->lock_user_sticky = rem->prop.lock_user_sticky;
	     bd->lock_client_sticky = rem->prop.lock_client_sticky;
	     bd->lock_user_shade = rem->prop.lock_user_shade;
	     bd->lock_client_shade = rem->prop.lock_client_shade;
	     bd->lock_user_maximize = rem->prop.lock_user_maximize;
	     bd->lock_client_maximize = rem->prop.lock_client_maximize;
	     bd->lock_user_fullscreen = rem->prop.lock_user_fullscreen;
	     bd->lock_client_fullscreen = rem->prop.lock_client_fullscreen;
	     bd->lock_border = rem->prop.lock_border;
	     bd->lock_close = rem->prop.lock_close;
	     bd->lock_focus_in = rem->prop.lock_focus_in;
	     bd->lock_focus_out = rem->prop.lock_focus_out;
	     bd->lock_life = rem->prop.lock_life;
	  }
	if (rem->apply & E_REMEMBER_APPLY_SKIP_WINLIST)
	  bd->user_skip_winlist = rem->prop.skip_winlist;
	if (rem->apply & E_REMEMBER_APPLY_SKIP_PAGER)
	  bd->client.netwm.state.skip_pager = rem->prop.skip_pager;
	if (rem->apply & E_REMEMBER_APPLY_SKIP_TASKBAR)
	  bd->client.netwm.state.skip_taskbar = rem->prop.skip_taskbar;
	if (rem->apply & E_REMEMBER_APPLY_ICON_PREF)
	  bd->icon_preference = rem->prop.icon_preference;
	if (rem->apply & E_REMEMBER_SET_FOCUS_ON_START)
	  bd->want_focus = 1;
     }
}


static void
_e_remember_cb_hook_eval_post_new_border(void *data, E_Border *bd)
{
   if (!bd->new_client) return;

   if ((bd->internal) && (!bd->remember) &&
       (e_config->remember_internal_windows) &&
       (!bd->internal_no_remember))
     {
	E_Remember *rem;

	rem = e_remember_new();
	if (rem)
	  {
	     bd->remember = rem;
	     rem->match = 0;
	     
	     if (bd->client.icccm.name)
	       {
		  rem->name = eina_stringshare_add(bd->client.icccm.name);
		  rem->match |= E_REMEMBER_MATCH_NAME;
	       }
	     if (bd->client.icccm.class)
	       {
		  rem->class = eina_stringshare_add(bd->client.icccm.class);
		  rem->match |= E_REMEMBER_MATCH_CLASS;
	       }
	     if (bd->client.icccm.window_role)
	       {
		  rem->role = eina_stringshare_add(bd->client.icccm.window_role);
		  rem->match |= E_REMEMBER_MATCH_ROLE;
	       }
	     if (bd->client.netwm.type != ECORE_X_WINDOW_TYPE_UNKNOWN)
	       {
		  rem->match |= E_REMEMBER_MATCH_TYPE;
	       }

	     rem->match |= E_REMEMBER_MATCH_TRANSIENT;
	     rem->apply = E_REMEMBER_APPLY_POS | E_REMEMBER_APPLY_SIZE | E_REMEMBER_APPLY_BORDER;
	     e_remember_use(rem);
	     e_remember_update(bd);
	  }
     }
}
