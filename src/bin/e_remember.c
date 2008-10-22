/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

#define REMEMBER_HIERARCHY 1
#define REMEMBER_SIMPLE 0

/* local subsystem functions */
static void _e_remember_free(E_Remember *rem);
static int _e_remember_sort_list(void * d1, void * d2);
static E_Remember  *_e_remember_find(E_Border *bd, int check_usable);

/* FIXME: match netwm window type */

/* local subsystem globals */

/* externally accessible functions */
EAPI int
e_remember_init(E_Startup_Mode mode)
{
   Eina_List *l = NULL;
   int not_updated = 0;

   if (mode == E_STARTUP_START)
     {
	for (l = e_config->remembers; l; l = l->next)
	  {
	     E_Remember *rem;

	     rem = l->data;
	     if ((rem->apply & E_REMEMBER_APPLY_RUN) && (rem->prop.command))
	       e_util_head_exec(rem->prop.head, rem->prop.command);
	  }
     }

#if 1
   for (l = e_config->remembers; l; l = l->next)
     {
        E_Remember *rem;

        rem = l->data;
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

	rem->delete_me = 1;
	for (l = e_border_client_list(); l; l = l->next)
	  {
	     E_Border *bd;

	     bd = l->data;
	     if (bd->remember == rem)
	       {
		  bd->remember = NULL;
		  e_remember_unuse(rem);
	       }
	  }
	return;
     }
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

	for (l = e_config->remembers; l; l = l->next)
	  {
	     r = l->data;
	     if (r->max_score <= rem->max_score) break;
	  }

	if (l)
	   e_config->remembers = eina_list_prepend_relative_list(e_config->remembers, rem, l);
	else
	   e_config->remembers = eina_list_append(e_config->remembers, rem);
     }
}

EAPI int
e_remember_default_match(E_Border *bd)
{
   int match = E_REMEMBER_MATCH_TRANSIENT;
   if ((bd->client.icccm.name) &&
	 (bd->client.icccm.class) &&
	 (bd->client.icccm.name[0] != 0) &&
	 (bd->client.icccm.class[0] != 0))
     match |= E_REMEMBER_MATCH_NAME | E_REMEMBER_MATCH_CLASS;
   else
     if ((e_border_name_get(bd))[0] != 0)
       match |= E_REMEMBER_MATCH_TITLE;

   if ((bd->client.icccm.window_role) && 
	 (bd->client.icccm.window_role[0] != 0))
     match |= E_REMEMBER_MATCH_ROLE;

   if (bd->client.netwm.type != ECORE_X_WINDOW_TYPE_UNKNOWN)
     match |= E_REMEMBER_MATCH_TYPE;

   return match;
}

EAPI void
e_remember_update(E_Remember *rem, E_Border *bd)
{
   if (!rem) return;
   if (bd->new_client) return;
   if (rem->name) eina_stringshare_del(rem->name);
   if (rem->class) eina_stringshare_del(rem->class);
   if (rem->title) eina_stringshare_del(rem->title);
   if (rem->role) eina_stringshare_del(rem->role);
   if (rem->prop.border) eina_stringshare_del(rem->prop.border);
   if (rem->prop.command) eina_stringshare_del(rem->prop.command);
   rem->name = NULL;
   rem->class = NULL;
   rem->title = NULL;
   rem->role = NULL;
   rem->prop.border = NULL;
   rem->prop.command = NULL;

   if (bd->client.icccm.name)
     rem->name = eina_stringshare_add(bd->client.icccm.name);
   if (bd->client.icccm.class)
     rem->class = eina_stringshare_add(bd->client.icccm.class);
   if (bd->client.netwm.name)
     rem->title = eina_stringshare_add(bd->client.netwm.name);
   else if (bd->client.icccm.title)
     rem->title = eina_stringshare_add(bd->client.icccm.title);
   if (bd->client.icccm.window_role)
     rem->role = eina_stringshare_add(bd->client.icccm.window_role);

   e_remember_match_update(rem);

   rem->type = bd->client.netwm.type;

   if (bd->client.icccm.transient_for != 0)
     rem->transient = 1;
   else
     rem->transient = 0;

   rem->prop.pos_x = bd->x - bd->zone->x;
   rem->prop.pos_y = bd->y - bd->zone->y;
   rem->prop.res_x = bd->zone->w;
   rem->prop.res_y = bd->zone->h;
   rem->prop.pos_w = bd->client.w;
   rem->prop.pos_h = bd->client.h;

   rem->prop.w = bd->client.w;
   rem->prop.h = bd->client.h;

   rem->prop.layer = bd->layer;

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

   if (bd->bordername)
     rem->prop.border = eina_stringshare_add(bd->bordername);

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

   if ((bd->client.icccm.command.argc > 0) &&
       (bd->client.icccm.command.argv))
     {
	char buf[4096];
	int i, j, k;

	buf[0] = 0;
	k = 0;
	for (i = 0; i < bd->client.icccm.command.argc; i++)
	  {
	     if (i > 0)
	       {
		  buf[k] = ' ';
		  k++;
	       }
	     for (j = 0; bd->client.icccm.command.argv[i][j]; j++)
	       {
		  if (k >= (sizeof(buf) - 10))
		    {
		       buf[k] = 0;
		       goto done;
		    }
		  if ((bd->client.icccm.command.argv[i][j] == ' ') ||
		      (bd->client.icccm.command.argv[i][j] == '\t') ||
		      (bd->client.icccm.command.argv[i][j] == '\\') ||
		      (bd->client.icccm.command.argv[i][j] == '\"') ||
		      (bd->client.icccm.command.argv[i][j] == '\'') ||
		      (bd->client.icccm.command.argv[i][j] == '$') ||
		      (bd->client.icccm.command.argv[i][j] == '%'))
		    {
		       buf[k] = '\\';
		       k++;
		    }
		  buf[k] = bd->client.icccm.command.argv[i][j];
		  k++;
	       }
	  }
	buf[k] = 0;
	done:
	rem->prop.command = eina_stringshare_add(buf);
     }

   e_config_save_queue();
}

/* local subsystem functions */
static E_Remember *
_e_remember_find(E_Border *bd, int check_usable)
{
   Eina_List *l = NULL;

#if REMEMBER_SIMPLE
   for (l = e_config->remembers; l; l = l->next)
     {
	E_Remember *rem;
	int required_matches;
	int matches;
	const char *title = "";

	rem = l->data;
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
   for (l = e_config->remembers; l; l = l->next)
     {
        E_Remember *rem;
        const char *title = "";

        rem = l->data;
        if ((check_usable) && (!e_remember_usable_get(rem)))
          continue;

        if (bd->client.netwm.name) title = bd->client.netwm.name;
        else title = bd->client.icccm.title;

        /* For each type of match, check whether the match is
         * required, and if it is, check whether there's a match. If
         * it fails, then go to the next remember */
        if (rem->match & E_REMEMBER_MATCH_NAME &&
            e_util_strcmp(rem->name, bd->client.icccm.name) &&
            !e_util_both_str_empty(rem->name, bd->client.icccm.name))
          continue;
        if (rem->match & E_REMEMBER_MATCH_CLASS &&
            e_util_strcmp(rem->class, bd->client.icccm.class) &&
            !e_util_both_str_empty(rem->class, bd->client.icccm.class))
          continue;
        if (rem->match & E_REMEMBER_MATCH_TITLE &&
            e_util_strcmp(rem->title, title) &&
            !e_util_both_str_empty(rem->title, title))
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
_e_remember_sort_list(void * d1, void * d2)
{
   E_Remember *r1, *r2;

   r1 = d1;
   r2 = d2;
   if (r1->max_score >= r2->max_score)
     return -1;
   else
     return 1;
}
