/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */
static void _e_remember_free(E_Remember *rem);

/* FIXME: match netwm window type
 * FIXME: match transient (is it a transient for something or not)
 */

/* local subsystem globals */

/* externally accessible functions */

EAPI int
e_remember_init(E_Startup_Mode mode)
{
   Evas_List *l;
 
   if (mode == E_STARTUP_START)
     {
	for (l = e_config->remembers; l; l = l->next)
	  {
	     E_Remember *rem;
	     
	     rem = l->data;
	     if ((rem->apply & E_REMEMBER_APPLY_RUN) && 
		 (rem->prop.command))
	       {
		  printf("REMEMBER CMD: \"%s\"\n", rem->prop.command);
		  e_util_head_exec(rem->prop.head, rem->prop.command);
	       }
	  }
     }
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
   e_config->remembers = evas_list_prepend(e_config->remembers, rem);
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
	Evas_List *l;
	
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
e_remember_find(E_Border *bd)
{
   Evas_List *l;
   
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
}

EAPI void
e_remember_update(E_Remember *rem, E_Border *bd)
{
   if (rem->name) evas_stringshare_del(rem->name);
   if (rem->class) evas_stringshare_del(rem->class);
   if (rem->title) evas_stringshare_del(rem->title);
   if (rem->role) evas_stringshare_del(rem->role);
   if (rem->prop.border) evas_stringshare_del(rem->prop.border);
   if (rem->prop.command) evas_stringshare_del(rem->prop.command);
   rem->name = NULL;
   rem->class = NULL;
   rem->title = NULL;
   rem->role = NULL;
   rem->prop.border = NULL;
   rem->prop.command = NULL;
   
   if (bd->client.icccm.name)
     rem->name = evas_stringshare_add(bd->client.icccm.name);
   if (bd->client.icccm.class)
     rem->class = evas_stringshare_add(bd->client.icccm.class);
   if (bd->client.netwm.name)
     rem->title = evas_stringshare_add(bd->client.netwm.name);
   else if (bd->client.icccm.title)
     rem->title = evas_stringshare_add(bd->client.icccm.title);
   if (bd->client.icccm.window_role)
     rem->role = evas_stringshare_add(bd->client.icccm.window_role);

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

   if (bd->client.border.name)
     rem->prop.border = evas_stringshare_add(bd->client.border.name);
   
   rem->prop.sticky = bd->sticky;
   
   if (bd->shaded)
     rem->prop.shaded = 100 + bd->shade.dir;
   else
     rem->prop.shaded = 50 + bd->shade.dir;
   
   rem->prop.skip_winlist = bd->user_skip_winlist;
   
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
	rem->prop.command = evas_stringshare_add(buf);
     }
   
   
   e_config_save_queue();
}

/* local subsystem functions */
static void
_e_remember_free(E_Remember *rem)
{
   e_config->remembers = evas_list_remove(e_config->remembers, rem);
   if (rem->name) evas_stringshare_del(rem->name);
   if (rem->class) evas_stringshare_del(rem->class);
   if (rem->title) evas_stringshare_del(rem->title);
   if (rem->role) evas_stringshare_del(rem->role);
   if (rem->prop.border) evas_stringshare_del(rem->prop.border);
   if (rem->prop.command) evas_stringshare_del(rem->prop.command);
   free(rem);
}
