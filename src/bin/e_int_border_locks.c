/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* PROTOTYPES - same all the time */
static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static Evas_Object *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

/* Actual config data we will be playing with whil the dialog is active */
struct _E_Config_Dialog_Data
{
   E_Border *border;
   /*- BASIC -*/
   int do_what_i_say;
   int protect_from_me;
   int keep_my_border;
   int important_window;
   int remember_locks;
   /*- ADVANCED -*/
   struct {
      struct {
	 int location;
	 int size;
	 int stacking;
	 int iconify;
	 int desk; /* unused atm */
	 int sticky;
	 int shade;
	 int maximize;
	 int fullscreen;
      } user, client;
      int border;
      int close;
      int focus_in; /* unused atm */
      int focus_out; /* unused atm */
      int life;
      
      int remember;
   } lock;
};

/* a nice easy setup function that does the dirty work */
EAPI void
e_int_border_locks(E_Border *bd)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   v = E_NEW(E_Config_Dialog_View, 1);
   if (v)
      {
         /* methods */
         v->create_cfdata           = _create_data;
         v->free_cfdata             = _free_data;
         v->basic.apply_cfdata      = _basic_apply_data;
         v->basic.create_widgets    = _basic_create_widgets;
         v->advanced.apply_cfdata   = _advanced_apply_data;
         v->advanced.create_widgets = _advanced_create_widgets;
	 v->override_auto_apply = 1;
         /* create config diaolg for bd object/data */
         cfd = e_config_dialog_new(bd->zone->container,
				   _("Window Locks"),
				   "E", "_border_locks_dialog",
				   NULL, 0, v, bd);
         bd->border_locks_dialog = cfd;
      }
}

/**--CREATE--**/
static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   cfdata->lock.user.location = (int)cfdata->border->lock_user_location & 0x1;
   cfdata->lock.user.size = (int)cfdata->border->lock_user_size & 0x1;
   cfdata->lock.user.stacking = (int)cfdata->border->lock_user_stacking & 0x1;
   cfdata->lock.user.iconify = (int)cfdata->border->lock_user_iconify & 0x1;
   cfdata->lock.user.desk = (int)cfdata->border->lock_user_desk & 0x1;
   cfdata->lock.user.sticky = (int)cfdata->border->lock_user_sticky & 0x1;
   cfdata->lock.user.shade = (int)cfdata->border->lock_user_shade & 0x1;
   cfdata->lock.user.maximize = (int)cfdata->border->lock_user_maximize & 0x1;
   cfdata->lock.user.fullscreen = (int)cfdata->border->lock_user_fullscreen & 0x1;
   cfdata->lock.client.location = (int)cfdata->border->lock_client_location & 0x1;
   cfdata->lock.client.size = (int)cfdata->border->lock_client_size & 0x1;
   cfdata->lock.client.stacking = (int)cfdata->border->lock_client_stacking & 0x1;
   cfdata->lock.client.iconify = (int)cfdata->border->lock_client_iconify & 0x1;
   cfdata->lock.client.desk = (int)cfdata->border->lock_client_desk & 0x1;
   cfdata->lock.client.sticky = (int)cfdata->border->lock_client_sticky & 0x1;
   cfdata->lock.client.shade = (int)cfdata->border->lock_client_shade & 0x1;
   cfdata->lock.client.maximize = (int)cfdata->border->lock_client_maximize & 0x1;
   cfdata->lock.client.fullscreen = (int)cfdata->border->lock_client_fullscreen & 0x1;
   cfdata->lock.border = (int)cfdata->border->lock_border & 0x1;
   cfdata->lock.close = (int)cfdata->border->lock_close & 0x1;
   cfdata->lock.focus_in = (int)cfdata->border->lock_focus_in & 0x1;
   cfdata->lock.focus_out = (int)cfdata->border->lock_focus_out & 0x1;
   cfdata->lock.life = (int)cfdata->border->lock_life & 0x1;
   if ((cfdata->border->remember) &&
       (cfdata->border->remember->apply & E_REMEMBER_APPLY_LOCKS))
     cfdata->lock.remember = 1;
     
   /* figure out basic config from the current locks */
   if (cfdata->lock.user.location || cfdata->lock.user.size ||
       cfdata->lock.user.stacking || cfdata->lock.user.iconify ||
       cfdata->lock.user.desk || cfdata->lock.user.sticky ||
       cfdata->lock.user.shade || cfdata->lock.user.maximize ||
       cfdata->lock.user.fullscreen)
     cfdata->protect_from_me = 1;
   if (cfdata->lock.client.location || cfdata->lock.client.size ||
       cfdata->lock.client.stacking || cfdata->lock.client.iconify ||
       cfdata->lock.client.desk || cfdata->lock.client.sticky ||
       cfdata->lock.client.shade || cfdata->lock.client.maximize ||
       cfdata->lock.client.fullscreen)
     cfdata->do_what_i_say = 1;
   if (cfdata->lock.border)
     cfdata->keep_my_border = 1;
   if (cfdata->lock.close || cfdata->lock.life)
     cfdata->important_window = 1;
   cfdata->remember_locks = cfdata->lock.remember;
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   /* Create cfdata - cfdata is a temporary block of config data that this
    * dialog will be dealing with while configuring. it will be applied to
    * the running systems/config in the apply methods
    */
   E_Config_Dialog_Data *cfdata;
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->border = cfd->data;
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   /* Free the cfdata */
   cfdata->border->border_locks_dialog = NULL;
   free(cfdata);
}

/**--APPLY--**/
static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   int flag;
   /* Actually take our cfdata settings and apply them in real life */
   
   flag = cfdata->do_what_i_say;
   cfdata->border->lock_client_location = flag;
   cfdata->border->lock_client_size = flag;
   cfdata->border->lock_client_stacking = flag;
   cfdata->border->lock_client_iconify = flag;
   cfdata->border->lock_client_desk = 0;
   cfdata->border->lock_client_sticky = flag;
   cfdata->border->lock_client_shade = flag;
   cfdata->border->lock_client_maximize = flag;
   cfdata->border->lock_client_fullscreen = flag;

   flag = cfdata->protect_from_me;
   cfdata->border->lock_user_location = flag;
   cfdata->border->lock_user_size = flag;
   cfdata->border->lock_user_stacking = flag;
   cfdata->border->lock_user_iconify = flag;
   cfdata->border->lock_user_desk = 0;
   cfdata->border->lock_user_sticky = flag;
   cfdata->border->lock_user_shade = flag;
   cfdata->border->lock_user_maximize = flag;
   cfdata->border->lock_user_fullscreen = flag;

   flag = cfdata->important_window;
   cfdata->border->lock_close = flag;
   cfdata->border->lock_life = flag;
   
   flag = cfdata->keep_my_border;
   cfdata->border->lock_border = flag;
   
   if (cfdata->remember_locks)
     {
	if (!cfdata->border->remember)
	  {
	     cfdata->border->remember = e_remember_new();
	     if (cfdata->border->remember)
		  e_remember_use(cfdata->border->remember);
	  }
	if (cfdata->border->remember)
	  {
	     cfdata->border->remember->apply |= E_REMEMBER_APPLY_LOCKS;
             cfdata->border->remember->match = e_remember_default_match(cfdata->border);
	     e_remember_update(cfdata->border->remember, cfdata->border);
	  }
     }
   else
     {
	if (cfdata->border->remember)
	  {
	     cfdata->border->remember->apply &= ~E_REMEMBER_APPLY_LOCKS;
	     if (cfdata->border->remember->apply == 0)
	       {
		  e_remember_unuse(cfdata->border->remember);
		  e_remember_del(cfdata->border->remember);
		  cfdata->border->remember = NULL;
	       }
	  }
     }
   e_config_save_queue();
   return 1; /* Apply was OK */
}

static int
_advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   /* Actually take our cfdata settings and apply them in real life */
   cfdata->border->lock_user_location = cfdata->lock.user.location;
   cfdata->border->lock_user_size = cfdata->lock.user.size;
   cfdata->border->lock_user_stacking = cfdata->lock.user.stacking;
   cfdata->border->lock_user_iconify = cfdata->lock.user.iconify;
   cfdata->border->lock_user_desk = cfdata->lock.user.desk;
   cfdata->border->lock_user_sticky = cfdata->lock.user.sticky;
   cfdata->border->lock_user_shade = cfdata->lock.user.shade;
   cfdata->border->lock_user_maximize = cfdata->lock.user.maximize;
   cfdata->border->lock_user_fullscreen = cfdata->lock.user.fullscreen;
   cfdata->border->lock_client_location = cfdata->lock.client.location;
   cfdata->border->lock_client_size = cfdata->lock.client.size;
   cfdata->border->lock_client_stacking = cfdata->lock.client.stacking;
   cfdata->border->lock_client_iconify = cfdata->lock.client.iconify;
   cfdata->border->lock_client_desk = cfdata->lock.client.desk;
   cfdata->border->lock_client_sticky = cfdata->lock.client.sticky;
   cfdata->border->lock_client_shade = cfdata->lock.client.shade;
   cfdata->border->lock_client_maximize = cfdata->lock.client.maximize;
   cfdata->border->lock_client_fullscreen = cfdata->lock.client.fullscreen;
   cfdata->border->lock_border = cfdata->lock.border;
   cfdata->border->lock_close = cfdata->lock.close;
   cfdata->border->lock_focus_in = cfdata->lock.focus_in;
   cfdata->border->lock_focus_out = cfdata->lock.focus_out;
   cfdata->border->lock_life = cfdata->lock.life;

   if (cfdata->lock.remember)
     {
	if (!cfdata->border->remember)
	  {
	     cfdata->border->remember = e_remember_new();
	     if (cfdata->border->remember)
		  e_remember_use(cfdata->border->remember);
	  }
	if (cfdata->border->remember)
	  {
	     cfdata->border->remember->apply |= E_REMEMBER_APPLY_LOCKS;
             cfdata->border->remember->match = e_remember_default_match(cfdata->border);
	     e_remember_update(cfdata->border->remember, cfdata->border);
	  }
     }
   else
     {
	if (cfdata->border->remember)
	  {
	     cfdata->border->remember->apply &= ~E_REMEMBER_APPLY_LOCKS;
	     if (cfdata->border->remember->apply == 0)
	       {
		  e_remember_unuse(cfdata->border->remember);
		  e_remember_del(cfdata->border->remember);
		  cfdata->border->remember = NULL;
	       }
	  }
     }
   e_config_save_queue();
   return 1; /* Apply was OK */
}

/**--GUI--**/
static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   /* generate the core widget layout for a basic dialog */
   Evas_Object *o, *ob, *of;
   
   o = e_widget_list_add(evas, 0, 0);
   
   of = e_widget_framelist_add(evas, _("Generic Locks"), 0);
   ob = e_widget_check_add(evas, _("Lock the Window so it does only what I tell it to"), &(cfdata->do_what_i_say));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Protect this window from me accidentally changing it"), &(cfdata->protect_from_me));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Protect this window from being accidentally closed because it is important"), &(cfdata->important_window));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Do not allow the border to change on this window"), &(cfdata->keep_my_border));
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   ob = e_widget_check_add(evas, _("Remember these Locks for this window next time it appears"), &(cfdata->remember_locks));
   e_widget_list_object_append(o, ob, 0, 0, 0.0);
   return o;
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   /* generate the core widget layout for an advanced dialog */
   Evas_Object *o, *ob, *of;
   
   o = e_widget_list_add(evas, 0, 0);
   
   of = e_widget_frametable_add(evas, _("Lock program changing:"), 0);
   ob = e_widget_check_add(evas, _("Position"), &(cfdata->lock.client.location));
   e_widget_frametable_object_append(of, ob, 0, 0, 1, 1, 1, 1, 1, 1);
   ob = e_widget_check_add(evas, _("Size"), &(cfdata->lock.client.size));
   e_widget_frametable_object_append(of, ob, 0, 1, 1, 1, 1, 1, 1, 1);
   ob = e_widget_check_add(evas, _("Stacking"), &(cfdata->lock.client.stacking));
   e_widget_frametable_object_append(of, ob, 0, 2, 1, 1, 1, 1, 1, 1);
   ob = e_widget_check_add(evas, _("Iconified state"), &(cfdata->lock.client.iconify));
   e_widget_frametable_object_append(of, ob, 0, 3, 1, 1, 1, 1, 1, 1);
   ob = e_widget_check_add(evas, _("Stickiness"), &(cfdata->lock.client.sticky));
   e_widget_frametable_object_append(of, ob, 1, 0, 1, 1, 1, 1, 1, 1);
   ob = e_widget_check_add(evas, _("Shaded state"), &(cfdata->lock.client.shade));
   e_widget_frametable_object_append(of, ob, 1, 1, 1, 1, 1, 1, 1, 1);
   ob = e_widget_check_add(evas, _("Maximized state"), &(cfdata->lock.client.maximize));
   e_widget_frametable_object_append(of, ob, 1, 2, 1, 1, 1, 1, 1, 1);
   ob = e_widget_check_add(evas, _("Fullscreen state"), &(cfdata->lock.client.fullscreen));
   e_widget_frametable_object_append(of, ob, 1, 3, 1, 1, 1, 1, 1, 1);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   of = e_widget_frametable_add(evas, _("Lock me from changing:"), 0);
   ob = e_widget_check_add(evas, _("Position"), &(cfdata->lock.user.location));
   e_widget_frametable_object_append(of, ob, 0, 0, 1, 1, 1, 1, 1, 1);
   ob = e_widget_check_add(evas, _("Size"), &(cfdata->lock.user.size));
   e_widget_frametable_object_append(of, ob, 0, 1, 1, 1, 1, 1, 1, 1);
   ob = e_widget_check_add(evas, _("Stacking"), &(cfdata->lock.user.stacking));
   e_widget_frametable_object_append(of, ob, 0, 2, 1, 1, 1, 1, 1, 1);
   ob = e_widget_check_add(evas, _("Iconified state"), &(cfdata->lock.user.iconify));
   e_widget_frametable_object_append(of, ob, 0, 3, 1, 1, 1, 1, 1, 1);
   ob = e_widget_check_add(evas, _("Stickiness"), &(cfdata->lock.user.sticky));
   e_widget_frametable_object_append(of, ob, 0, 4, 1, 1, 1, 1, 1, 1);
   ob = e_widget_check_add(evas, _("Shaded state"), &(cfdata->lock.user.shade));
   e_widget_frametable_object_append(of, ob, 1, 0, 1, 1, 1, 1, 1, 1);
   ob = e_widget_check_add(evas, _("Maximized state"), &(cfdata->lock.user.maximize));
   e_widget_frametable_object_append(of, ob, 1, 1, 1, 1, 1, 1, 1, 1);
   ob = e_widget_check_add(evas, _("Fullscreen state"), &(cfdata->lock.user.fullscreen));
   e_widget_frametable_object_append(of, ob, 1, 2, 1, 1, 1, 1, 1, 1);
   ob = e_widget_check_add(evas, _("Border style"), &(cfdata->lock.border));
   e_widget_frametable_object_append(of, ob, 1, 3, 1, 1, 1, 1, 1, 1);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   of = e_widget_framelist_add(evas, _("Stop me from:"), 0);
   ob = e_widget_check_add(evas, _("Closing the window"), &(cfdata->lock.close));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Exiting my login with this window open"), &(cfdata->lock.life));
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   ob = e_widget_check_add(evas, _("Remember these Locks"), &(cfdata->lock.remember));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   
   return o;
}
