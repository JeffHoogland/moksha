#include "e.h"

/* local subsystem functions */
static void _e_config_dialog_free(E_Config_Dialog *cfd);
static void _e_config_dialog_go(E_Config_Dialog *cfd, E_Config_Dialog_CFData_Type type);
static Eina_Bool _e_config_dialog_cb_auto_apply_timer(void *data);
static void _e_config_dialog_cb_dialog_del(void *obj);
static void _e_config_dialog_cb_ok(void *data, E_Dialog *dia);
static void _e_config_dialog_cb_apply(void *data, E_Dialog *dia);
static void _e_config_dialog_cb_advanced(void *data, void *data2);
static void _e_config_dialog_cb_basic(void *data, void *data2);
static int  _e_config_dialog_check_changed(E_Config_Dialog *cfd, unsigned char def);
static void _e_config_dialog_cb_changed(void *data, Evas_Object *obj);
static void _e_config_dialog_cb_close(void *data, E_Dialog *dia);

/* local subsystem globals */
static Eina_List *_e_config_dialog_list = NULL;

/* externally accessible functions */

EAPI E_Config_Dialog *
e_config_dialog_new(E_Container *con, const char *title, const char *name, const char *class, const char *icon, int icon_size, E_Config_Dialog_View *view, void *data)
{
   E_Config_Dialog *cfd;

   cfd = E_OBJECT_ALLOC(E_Config_Dialog, E_CONFIG_DIALOG_TYPE, 
                        _e_config_dialog_free);
   cfd->view = view;
   cfd->con = con;
   cfd->title = eina_stringshare_add(title);
   cfd->name = eina_stringshare_add(name);
   cfd->class = eina_stringshare_add(class);
   if (icon)
     {
	cfd->icon = eina_stringshare_add(icon);
	cfd->icon_size = icon_size;
     }
   cfd->data = data;
   cfd->hide_buttons = 1;
   cfd->cfg_changed = 0;
   cfd->cfg_changed_auto = 1;

   if (cfd->view->override_auto_apply)
     {
	/* Dialog Requested To Not Auto-Apply */
	if ((cfd->view->basic.apply_cfdata) || 
            (cfd->view->advanced.apply_cfdata))
	  cfd->hide_buttons = 0;
     }
   else
     {
	/* Ok To Override, Or Not Specified. Use Config Value */
	if (e_config->cfgdlg_auto_apply)
	  cfd->hide_buttons = 1;
	else
	  {
	     if ((cfd->view->basic.apply_cfdata) || 
                 (cfd->view->advanced.apply_cfdata))
	       cfd->hide_buttons = 0;
	  }
     }

   switch (e_config->cfgdlg_default_mode)
     {
      case E_CONFIG_DIALOG_CFDATA_TYPE_BASIC:
	if (cfd->view->basic.create_widgets)
	  _e_config_dialog_go(cfd, E_CONFIG_DIALOG_CFDATA_TYPE_BASIC);
	break;
      case E_CONFIG_DIALOG_CFDATA_TYPE_ADVANCED:
	if (cfd->view->advanced.create_widgets)
	  _e_config_dialog_go(cfd, E_CONFIG_DIALOG_CFDATA_TYPE_ADVANCED);
	else if (cfd->view->basic.create_widgets)
	  _e_config_dialog_go(cfd, E_CONFIG_DIALOG_CFDATA_TYPE_BASIC);	
	break;
     }
   _e_config_dialog_list = eina_list_append(_e_config_dialog_list, cfd);

   return cfd;
}

EAPI int
e_config_dialog_find(const char *name, const char *class)
{
   Eina_List *l;
   E_Config_Dialog *cfd;

   EINA_LIST_FOREACH(_e_config_dialog_list, l, cfd)
     {
	if ((!e_util_strcmp(name, cfd->name)) &&
	    (!e_util_strcmp(class, cfd->class)))
	  {
	     E_Zone *z;

	     z = e_util_zone_current_get(e_manager_current_get());
	     e_border_uniconify(cfd->dia->win->border);
	     e_win_raise(cfd->dia->win);	     
	     if (z->container == cfd->dia->win->border->zone->container)
	       e_border_desk_set(cfd->dia->win->border, e_desk_current_get(z));
	     else
	       {
		  if (!cfd->dia->win->border->sticky)
		    e_desk_show(cfd->dia->win->border->desk);
		  ecore_x_pointer_warp(cfd->dia->win->border->zone->container->win,
				       cfd->dia->win->border->zone->x + (cfd->dia->win->border->zone->w / 2),
				       cfd->dia->win->border->zone->y + (cfd->dia->win->border->zone->h / 2));
	       }
	     e_border_unshade(cfd->dia->win->border, E_DIRECTION_DOWN);
	     if ((e_config->focus_setting == E_FOCUS_NEW_DIALOG) ||
		 (e_config->focus_setting == E_FOCUS_NEW_WINDOW))
	       e_border_focus_set(cfd->dia->win->border, 1, 1);
	     return 1;
	  }
     }
   return 0;
}

EAPI E_Config_Dialog *
e_config_dialog_get(const char *name, const char *class) 
{
   Eina_List *l;
   E_Config_Dialog *cfd;

   EINA_LIST_FOREACH(_e_config_dialog_list, l, cfd)
     {
	if (!cfd) continue;
	if ((!e_util_strcmp(name, cfd->name)) &&
	    (!e_util_strcmp(class, cfd->class)))
	  {
	     return cfd;
	  }
     }
   return NULL;
}

/* local subsystem functions */
static void
_e_config_dialog_free(E_Config_Dialog *cfd)
{
   _e_config_dialog_list = eina_list_remove(_e_config_dialog_list, cfd);
   if (cfd->auto_apply_timer) _e_config_dialog_cb_auto_apply_timer(cfd);
   if (cfd->title) eina_stringshare_del(cfd->title);
   if (cfd->name) eina_stringshare_del(cfd->name);
   if (cfd->class) eina_stringshare_del(cfd->class);
   if (cfd->icon) eina_stringshare_del(cfd->icon);
   if (cfd->cfdata)
     {
	cfd->view->free_cfdata(cfd, cfd->cfdata);
	cfd->cfdata = NULL;
     }
   if (cfd->dia)
     {
	e_object_del_attach_func_set(E_OBJECT(cfd->dia), NULL);
	e_object_del(E_OBJECT(cfd->dia));
	cfd->dia = NULL;
     }
   E_FREE(cfd->view);
   E_FREE(cfd);
}

static void
_e_config_dialog_go(E_Config_Dialog *cfd, E_Config_Dialog_CFData_Type type)
{
   Evas *evas;
   E_Dialog *pdia;
   Evas_Object *o, *ob, *sf;
   Evas_Coord mw = 0, mh = 0;
   char buf[256];

   pdia = cfd->dia;
   /* FIXME: get name/class form new call and use here */
   /* if (type == E_CONFIG_DIALOG_CFDATA_TYPE_BASIC)
    *   snprintf(buf, sizeof(buf), "%s...%s", cfd->class, "BASIC");
    * else
    *   snprintf(buf, sizeof(buf), "%s...%s", cfd->class, "ADVANCED"); */
   snprintf(buf, sizeof(buf), "_config::%s", cfd->class);
   
   if (!pdia)  /* creating window for the first time */
     {
       if ((cfd->view->normal_win) || (e_config->cfgdlg_normal_wins))
         cfd->dia = e_dialog_normal_win_new(cfd->con, cfd->name, buf);
       else
         cfd->dia = e_dialog_new(cfd->con, cfd->name, buf);
       e_object_del_attach_func_set(E_OBJECT(cfd->dia), 
                                    _e_config_dialog_cb_dialog_del);
     }         /* window was created before - deleting content only */
   else if (cfd->dia->content_object)
     evas_object_del(cfd->dia->content_object);

   cfd->view_type = type;
   cfd->dia->data = cfd;
   e_dialog_title_set(cfd->dia, cfd->title);
   if (!cfd->cfdata) cfd->cfdata = cfd->view->create_cfdata(cfd);

   evas = e_win_evas_get(cfd->dia->win);
   if (type == E_CONFIG_DIALOG_CFDATA_TYPE_BASIC)
     {
	if (cfd->view->advanced.create_widgets)
	  {
	     o = e_widget_list_add(e_win_evas_get(cfd->dia->win), 0, 0);
	     ob = cfd->view->basic.create_widgets(cfd, evas, cfd->cfdata);
	     if (cfd->view->scroll)
	       {
		  e_widget_size_min_resize(ob);
		  sf = e_widget_scrollframe_simple_add(evas, ob);
		  e_widget_list_object_append(o, sf, 1, 1, 0.0);
	       }
	     else
	       e_widget_list_object_append(o, ob, 1, 1, 0.0);
	     ob = e_widget_button_add(evas,
				      _("Advanced"), "go-next",
				      _e_config_dialog_cb_advanced, cfd, NULL);
	     e_widget_list_object_append(o, ob, 0, 0, 1.0);
	  }
	else
	  {
	     o = cfd->view->basic.create_widgets(cfd, evas, cfd->cfdata);
	     if (cfd->view->scroll)
	       {
		  e_widget_size_min_resize(o);
		  o = e_widget_scrollframe_simple_add(evas, o);
	       }
	  }
     }
   else
     {
	if (cfd->view->basic.create_widgets)
	  {
	     o = e_widget_list_add(evas, 0, 0);
	     ob = cfd->view->advanced.create_widgets(cfd, evas, cfd->cfdata);
	     if (cfd->view->scroll)
	       {
		  e_widget_size_min_resize(ob);
		  sf = e_widget_scrollframe_simple_add(evas, ob);
		  e_widget_list_object_append(o, sf, 1, 1, 0.0);
	       }
	     else
	       e_widget_list_object_append(o, ob, 1, 1, 0.0);
	     ob = e_widget_button_add(evas, 
				      _("Basic"), "go-next",
				      _e_config_dialog_cb_basic, cfd, NULL);
	     e_widget_list_object_append(o, ob, 0, 0, 1.0);
	  }
	else
	  {
	     o = cfd->view->advanced.create_widgets(cfd, evas, cfd->cfdata);
	     if (cfd->view->scroll)
	       {
		  e_widget_size_min_resize(o);
		  o = e_widget_scrollframe_simple_add(evas, o);
	       }
	  }
     }

   e_widget_size_min_get(o, &mw, &mh);
   e_widget_on_change_hook_set(o, _e_config_dialog_cb_changed, cfd);
   e_dialog_content_set(cfd->dia, o, mw, mh);

   if (!pdia) /* dialog window was created in this function call - need to create buttons once */
     {
       if (!cfd->hide_buttons)
         {
	    e_dialog_button_add(cfd->dia, _("OK"), NULL,
                                _e_config_dialog_cb_ok, cfd);
	    e_dialog_button_add(cfd->dia, _("Apply"), NULL,
                                _e_config_dialog_cb_apply, cfd);
	    if (!cfd->cfg_changed)
	      {
	         e_dialog_button_disable_num_set(cfd->dia, 0, 1);
	         e_dialog_button_disable_num_set(cfd->dia, 1, 1);
	      }
         }
       e_dialog_button_add(cfd->dia, _("Close"), NULL,
                           _e_config_dialog_cb_close, cfd);
    }

   if (cfd->cfg_changed_auto)
     {
	int changed;

	changed = _e_config_dialog_check_changed(cfd, 0);
	e_config_dialog_changed_set(cfd, changed);
     }

   e_dialog_show(cfd->dia);
   if (cfd->icon) e_dialog_border_icon_set(cfd->dia, cfd->icon);
}

static Eina_Bool
_e_config_dialog_cb_auto_apply_timer(void *data)
{
   E_Config_Dialog *cfd;

   cfd = data;

   if (cfd->auto_apply_timer) ecore_timer_del(cfd->auto_apply_timer);
   cfd->auto_apply_timer = NULL;

   if (cfd->view_type == E_CONFIG_DIALOG_CFDATA_TYPE_BASIC)
     {
	if (cfd->view->basic.apply_cfdata)
	  cfd->view->basic.apply_cfdata(cfd, cfd->cfdata);
     }
   else
     {
	if (cfd->view->advanced.apply_cfdata)
	  cfd->view->advanced.apply_cfdata(cfd, cfd->cfdata);
     }
   return ECORE_CALLBACK_CANCEL;
}

static void
_e_config_dialog_cb_dialog_del(void *obj)
{
   E_Dialog *dia;
   E_Config_Dialog *cfd;

   dia = obj;
   cfd = dia->data;
   if (cfd->auto_apply_timer) _e_config_dialog_cb_auto_apply_timer(cfd);
   cfd->dia = NULL;
   e_object_del(E_OBJECT(cfd));
}

static void
_e_config_dialog_cb_ok(void *data __UNUSED__, E_Dialog *dia)
{
   E_Config_Dialog *cfd;
   int ok = 0;

   cfd = dia->data;
   if (cfd->view_type == E_CONFIG_DIALOG_CFDATA_TYPE_BASIC)
     {
        if (cfd->view->basic.apply_cfdata)
          ok = cfd->view->basic.apply_cfdata(cfd, cfd->cfdata);
     }
   else
     {
        if (cfd->view->advanced.apply_cfdata)
          ok = cfd->view->advanced.apply_cfdata(cfd, cfd->cfdata);
     }
   if (ok) 
     e_util_defer_object_del(E_OBJECT(cfd));
}

static void
_e_config_dialog_cb_apply(void *data __UNUSED__, E_Dialog *dia)
{
   E_Config_Dialog *cfd;
   int ok = 0;

   cfd = dia->data;
   if (cfd->view_type == E_CONFIG_DIALOG_CFDATA_TYPE_BASIC)
     {
        if (cfd->view->basic.apply_cfdata)
          ok = cfd->view->basic.apply_cfdata(cfd, cfd->cfdata);
     }
   else
     {
        if (cfd->view->advanced.apply_cfdata)
          ok = cfd->view->advanced.apply_cfdata(cfd, cfd->cfdata);
     }
   if ((ok) && (!cfd->hide_buttons))
     {
	cfd->cfg_changed = 0;
	e_dialog_button_disable_num_set(cfd->dia, 0, 1);
	e_dialog_button_disable_num_set(cfd->dia, 1, 1);
     }
}

static void
_e_config_dialog_cb_advanced(void *data, void *data2 __UNUSED__)
{
   E_Config_Dialog *cfd;

   cfd = data;
   if (cfd->auto_apply_timer) _e_config_dialog_cb_auto_apply_timer(cfd);
   _e_config_dialog_go(cfd, E_CONFIG_DIALOG_CFDATA_TYPE_ADVANCED);
}

static void
_e_config_dialog_cb_basic(void *data, void *data2 __UNUSED__)
{
   E_Config_Dialog *cfd;

   cfd = data;
   if (cfd->auto_apply_timer) _e_config_dialog_cb_auto_apply_timer(cfd);
   _e_config_dialog_go(cfd, E_CONFIG_DIALOG_CFDATA_TYPE_BASIC);
}

static void
_e_config_dialog_changed(E_Config_Dialog *cfd)
{
   if (!cfd->hide_buttons)
     {
	cfd->cfg_changed = 1;
	e_dialog_button_disable_num_set(cfd->dia, 0, 0);
	e_dialog_button_disable_num_set(cfd->dia, 1, 0);
     }
   else
     {
	if (cfd->auto_apply_timer) ecore_timer_del(cfd->auto_apply_timer);
        cfd->auto_apply_timer = NULL;
	cfd->auto_apply_timer = 
          ecore_timer_add(0.5, _e_config_dialog_cb_auto_apply_timer, cfd);
     }
}

static void
_e_config_dialog_unchanged(E_Config_Dialog *cfd)
{
   if (!cfd->hide_buttons)
     {
	e_dialog_button_disable_num_set(cfd->dia, 0, 1);
	e_dialog_button_disable_num_set(cfd->dia, 1, 1);
     }
   else
     {
	if (cfd->auto_apply_timer)
	  {
	     ecore_timer_del(cfd->auto_apply_timer);
	     cfd->auto_apply_timer = NULL;
	  }
     }
}

static int
_e_config_dialog_check_changed(E_Config_Dialog *cfd, unsigned char def)
{
   int changed = 0;

   if (cfd->view_type == E_CONFIG_DIALOG_CFDATA_TYPE_BASIC)
     {
        if (cfd->view->basic.check_changed)
          changed = cfd->view->basic.check_changed(cfd, cfd->cfdata);
	else
          changed = def;
     }
   else if (cfd->view_type == E_CONFIG_DIALOG_CFDATA_TYPE_ADVANCED)
      {
	 if (cfd->view->advanced.check_changed)
           changed = cfd->view->advanced.check_changed(cfd, cfd->cfdata);
	 else
	   changed = def;
      }

   return changed;
}

static void
_e_config_dialog_cb_changed(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog *cfd;
   int changed;

   cfd = data;
   if (!cfd->cfg_changed_auto) return;
   changed = _e_config_dialog_check_changed(cfd, 1);

   e_config_dialog_changed_set(cfd, changed);
}

static void
_e_config_dialog_cb_close(void *data __UNUSED__, E_Dialog *dia)
{
   E_Config_Dialog *cfd;
   int ok = 1;

   cfd = dia->data;
   if (cfd->auto_apply_timer) _e_config_dialog_cb_auto_apply_timer(cfd);
   if (cfd->view->close_cfdata)
     ok = cfd->view->close_cfdata(cfd, cfd->cfdata);
   if (ok) e_util_defer_object_del(E_OBJECT(cfd));
}

EAPI void
e_config_dialog_changed_auto_set(E_Config_Dialog *cfd, unsigned char value)
{
   if (!cfd) return;
   cfd->cfg_changed_auto = !!value;
}

EAPI void
e_config_dialog_changed_set(E_Config_Dialog *cfd, unsigned char value)
{
   if (!cfd) return;
   cfd->cfg_changed = !!value;
   if (cfd->cfg_changed)
     _e_config_dialog_changed(cfd);
   else
     _e_config_dialog_unchanged(cfd);
}
