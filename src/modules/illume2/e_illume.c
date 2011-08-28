#include "e_illume_private.h"
#include "e_mod_config.h"

/**
 * @defgroup E_Illume_Main_Group Illume API Information
 * 
 * The following group defines variables, structures, and functions available 
 * to a policy.
 */

/**
 * Returns the @ref E_Illume_Config_Zone structure for a specific zone.
 * 
 * @param id The id of the E_Zone.
 * @return The @ref E_Illume_Config_Zone structure for this zone.
 * 
 * @note This function will return a new @ref E_Illume_Config_Zone structure 
 * if none exists. This new @ref E_Illume_Config_Zone will be added to the 
 * existing list of @ref E_Illume_Config_Zone structures automatically.
 * 
 * @ingroup E_Illume_Config_Group
 */
EAPI E_Illume_Config_Zone *
e_illume_zone_config_get(int id) 
{
   Eina_List *l;
   E_Illume_Config_Zone *cz = NULL;

   /* loop existing zone configs and look for this id */
   EINA_LIST_FOREACH(_e_illume_cfg->policy.zones, l, cz) 
     {
        if (cz->id != id) continue;
        return cz;
     }

   /* we did not find an existing one for this zone, so create a new one */
   cz = E_NEW(E_Illume_Config_Zone, 1);
   cz->id = id;
   cz->mode.dual = 0;
   cz->mode.side = 0;

   /* add it to the list */
   _e_illume_cfg->policy.zones = 
     eina_list_append(_e_illume_cfg->policy.zones, cz);

   /* save it in config */
   e_mod_illume_config_save();

   /* return a fallback */
   return cz;
}

/**
 * Determine if a given border is an Indicator window.
 * 
 * @param bd The border to test.
 * @return EINA_TRUE if it is an Indicator window, EINA_FALSE otherwise.
 * 
 * @note If @p bd is NULL then this function will return EINA_FALSE.
 * 
 * @note It is assumed that Indicator windows are of type 
 * ECORE_X_WINDOW_TYPE_DOCK.
 * 
 * @ingroup E_Illume_Main_Group
 */
EAPI Eina_Bool 
e_illume_border_is_indicator(E_Border *bd) 
{
   /* make sure we have a border */
   if (!bd) return EINA_FALSE;

   /* indicator windows should be set to dock type, so check for that */
   if (bd->client.netwm.type != ECORE_X_WINDOW_TYPE_DOCK) return EINA_FALSE;

   /* we have a dock window, check against any matches in config */

   /* check if we are matching on name */
   if (_e_illume_cfg->policy.indicator.match.name) 
     {
        if ((bd->client.icccm.name) && 
            (!strcmp(bd->client.icccm.name, 
                     _e_illume_cfg->policy.indicator.name)))
          return EINA_TRUE;
     }

   /* check if we are matching on class */
   if (_e_illume_cfg->policy.indicator.match.class) 
     {
        if ((bd->client.icccm.class) && 
            (!strcmp(bd->client.icccm.class, 
                     _e_illume_cfg->policy.indicator.class)))
          return EINA_TRUE;
     }

   /* check if we are matching on title */
   if (_e_illume_cfg->policy.indicator.match.title) 
     {
        const char *title;

        if ((title = e_border_name_get(bd)))
          if (!strcmp(title, _e_illume_cfg->policy.indicator.title))
            return EINA_TRUE;
     }

   /* return a fallback */
   return EINA_FALSE;
}

/**
 * Determine if a given border is a Softkey window.
 * 
 * @param bd The border to test.
 * @return EINA_TRUE if it is a Softkey window, EINA_FALSE otherwise.
 * 
 * @note If @p bd is NULL then this function will return EINA_FALSE.
 * 
 * @note It is assumed that Softkey windows are of type 
 * ECORE_X_WINDOW_TYPE_DOCK.
 * 
 * @ingroup E_Illume_Main_Group
 */
EAPI Eina_Bool 
e_illume_border_is_softkey(E_Border *bd) 
{
   /* make sure we have a border */
   if (!bd) return EINA_FALSE;

   /* legacy code from illume 1 */
   if (bd->client.qtopia.soft_menu) return EINA_TRUE;

   /* softkey windows should be set to dock type, so check for that */
   if (bd->client.netwm.type != ECORE_X_WINDOW_TYPE_DOCK) return EINA_FALSE;

   /* we have a softkey window, check against any matches in config */

   /* check if we are matching on name */
   if (_e_illume_cfg->policy.softkey.match.name) 
     {
        if ((bd->client.icccm.name) && 
            (!strcmp(bd->client.icccm.name, 
                     _e_illume_cfg->policy.softkey.name)))
          return EINA_TRUE;
     }

   /* check if we are matching on class */
   if (_e_illume_cfg->policy.softkey.match.class) 
     {
        if ((bd->client.icccm.class) && 
            (!strcmp(bd->client.icccm.class, 
                     _e_illume_cfg->policy.softkey.class)))
          return EINA_TRUE;
     }

   /* check if we are matching on title */
   if (_e_illume_cfg->policy.softkey.match.title) 
     {
        const char *title;

        if ((title = e_border_name_get(bd)))
          if (!strcmp(title, _e_illume_cfg->policy.softkey.title))
            return EINA_TRUE;
     }

   /* return a fallback */
   return EINA_FALSE;
}

/**
 * Determine if a given border is a Keyboard window.
 * 
 * @param bd The border to test.
 * @return EINA_TRUE if it is a Keyboard window, EINA_FALSE otherwise.
 * 
 * @note If @p bd is NULL then this function will return EINA_FALSE.
 * 
 * @ingroup E_Illume_Main_Group
 */
EAPI Eina_Bool 
e_illume_border_is_keyboard(E_Border *bd) 
{
   /* make sure we have a border */
   if (!bd) return EINA_FALSE;

   /* check for specific flag first */
   if (bd->client.vkbd.vkbd) return EINA_TRUE;

   /* legacy code from illume 1 */
   if ((bd->client.icccm.name) && 
       ((!strcmp(bd->client.icccm.name, "multitap-pad"))) && 
       (bd->client.netwm.state.skip_taskbar) && 
       (bd->client.netwm.state.skip_pager))
     return EINA_TRUE;

   /* check if we are matching on name */
   if (_e_illume_cfg->policy.vkbd.match.name) 
     {
        if ((bd->client.icccm.name) && 
            (!strcmp(bd->client.icccm.name, 
                     _e_illume_cfg->policy.vkbd.name)))
          return EINA_TRUE;
     }

   /* check if we are matching on class */
   if (_e_illume_cfg->policy.vkbd.match.class) 
     {
        if ((bd->client.icccm.class) && 
            (!strcmp(bd->client.icccm.class, 
                     _e_illume_cfg->policy.vkbd.class)))
          return EINA_TRUE;
     }

   /* check if we are matching on title */
   if (_e_illume_cfg->policy.vkbd.match.title) 
     {
        const char *title;

        if ((title = e_border_name_get(bd)))
          if (!strcmp(title, _e_illume_cfg->policy.vkbd.title))
            return EINA_TRUE;
     }

   /* return a fallback */
   return EINA_FALSE;
}

/**
 * Determine if a given border is a Home window.
 * 
 * @param bd The border to test.
 * @return EINA_TRUE if it is a Home window, EINA_FALSE otherwise.
 * 
 * @note If @p bd is NULL then this function will return EINA_FALSE.
 * 
 * @ingroup E_Illume_Main_Group
 */
EAPI Eina_Bool 
e_illume_border_is_home(E_Border *bd) 
{
   /* make sure we have a border */
   if (!bd) return EINA_FALSE;

   /* skip windows which are not either 'normal' windows, or 'unknown' windows
    * NB: Let 'unknown' windows pass through as a safety */
   if ((bd->client.netwm.type != ECORE_X_WINDOW_TYPE_NORMAL) && 
       (bd->client.netwm.type != ECORE_X_WINDOW_TYPE_UNKNOWN))
     return EINA_FALSE;

   /* check if we are matching on name */
   if (_e_illume_cfg->policy.home.match.name) 
     {
        if ((bd->client.icccm.name) && 
            (!strcmp(bd->client.icccm.name, 
                     _e_illume_cfg->policy.home.name)))
          return EINA_TRUE;
     }

   /* check if we are matching on class */
   if (_e_illume_cfg->policy.home.match.class) 
     {
        if ((bd->client.icccm.class) && 
            (!strcmp(bd->client.icccm.class, 
                     _e_illume_cfg->policy.home.class)))
          return EINA_TRUE;
     }

   /* check if we are matching on title */
   if (_e_illume_cfg->policy.home.match.title) 
     {
        const char *title;

        if ((title = e_border_name_get(bd)))
          if (!strcmp(title, _e_illume_cfg->policy.home.title))
            return EINA_TRUE;
     }

   /* return a fallback */
   return EINA_FALSE;
}

/**
 * Determine if a given border is a splash screen.
 * 
 * @param bd The border to test.
 * @return EINA_TRUE if it is a splash screen, EINA_FALSE otherwise.
 * 
 * @note If @p bd is NULL then this function will return EINA_FALSE.
 * 
 * @ingroup E_Illume_Main_Group
 */
EAPI Eina_Bool 
e_illume_border_is_splash(E_Border *bd) 
{
   /* make sure we have a border */
   if (!bd) return EINA_FALSE;

   /* check actual type */
   if (bd->client.netwm.type == ECORE_X_WINDOW_TYPE_SPLASH) return EINA_TRUE;

   /* check for transient flag */
//   if (bd->client.icccm.transient_for != 0) return EINA_TRUE;

   /* NB: may or may not need to handle these. Needs Testing
   if (bd->client.netwm.extra_types) 
     printf("\t\tBorder has extra types: %s\n", bd->client.icccm.class);
   */

   /* return a fallback */
   return EINA_FALSE;
}

/**
 * Determine if a given border is a dialog.
 * 
 * @param bd The border to test.
 * @return EINA_TRUE if it is a dialog, EINA_FALSE otherwise.
 * 
 * @note If @p bd is NULL then this function will return EINA_FALSE.
 * 
 * @ingroup E_Illume_Main_Group
 */
EAPI Eina_Bool 
e_illume_border_is_dialog(E_Border *bd) 
{
   /* make sure we have a border */
   if (!bd) return EINA_FALSE;

   /* check actual type */
   if (bd->client.netwm.type == ECORE_X_WINDOW_TYPE_DIALOG) return EINA_TRUE;

   /* check for transient flag */
   if (bd->client.icccm.transient_for != 0) return EINA_TRUE;

   /* check for client leader */
   /* NB: disabled currently as some GTK windows set this even tho they are 
    * not a dialog. */
//   if (bd->client.icccm.client_leader) return EINA_TRUE;

   /* NB: may or may not need to handle these. Needs Testing
   if (bd->client.netwm.extra_types) 
     printf("\t\tBorder has extra types: %s\n", bd->client.icccm.class);
   */

   /* return a fallback */
   return EINA_FALSE;
}

/**
 * Determine if a given border is a QT VCLSalFrame.
 * 
 * @param bd The border to test.
 * @return EINA_TRUE if it is a VCLSalFrame, EINA_FALSE otherwise.
 * 
 * @note If @p bd is NULL then this function will return EINA_FALSE.
 * 
 * @ingroup E_Illume_Main_Group
 */
EAPI Eina_Bool 
e_illume_border_is_qt_frame(E_Border *bd) 
{
   /* make sure we have a border */
   if (!bd) return EINA_FALSE;

   /* make sure we have the icccm name and compare it */
   if ((bd->client.icccm.name) && 
       (!strncmp(bd->client.icccm.name, "VCLSalFrame", 11)))
     return EINA_TRUE;

   /* return a fallback */
   return EINA_FALSE;
}

/**
 * Determine if a given border is a fullscreen window.
 * 
 * @param bd The border to test.
 * @return EINA_TRUE if it is fullscreen, EINA_FALSE otherwise.
 * 
 * @note If @p bd is NULL then this function will return EINA_FALSE.
 * 
 * @ingroup E_Illume_Main_Group
 */
EAPI Eina_Bool 
e_illume_border_is_fullscreen(E_Border *bd) 
{
   /* make sure we have a border */
   if (!bd) return EINA_FALSE;

   /* check for fullscreen */
   if ((bd->fullscreen) || (bd->need_fullscreen)) return EINA_TRUE;

   /* return a fallback */
   return EINA_FALSE;
}

/**
 * Determine if a given border is an illume conformant window.
 * 
 * @param bd The border to test.
 * @return EINA_TRUE if it is conformant, EINA_FALSE otherwise.
 * 
 * @note If @p bd is NULL then this function will return EINA_FALSE.
 * 
 * @ingroup E_Illume_Main_Group
 */
EAPI Eina_Bool 
e_illume_border_is_conformant(E_Border *bd) 
{
   /* make sure we have a border */
   if (!bd) return EINA_FALSE;

   /* return if it is conformant or not */
   return bd->client.illume.conformant.conformant;
}

/**
 * Determine if a given border is a quickpanel window.
 * 
 * @param bd The border to test.
 * @return EINA_TRUE if it is a quickpanel, EINA_FALSE otherwise.
 * 
 * @note If @p bd is NULL then this function will return EINA_FALSE.
 * 
 * @ingroup E_Illume_Main_Group
 */
EAPI Eina_Bool 
e_illume_border_is_quickpanel(E_Border *bd) 
{
   /* make sure we have a border */
   if (!bd) return EINA_FALSE;

   /* return if it is a quickpanel or not */
   return bd->client.illume.quickpanel.quickpanel;
}

/**
 * Determine if the border request a fixed size.
 * 
 * @param bd The border to get the minium space for.
 * @return EINA_TRUE if border requested fixed size, EINA_FALSE otherwise.
 *
 * @note If @p bd is NULL then this function will return EINA_FALSE.
 *
 * @ingroup E_Illume_Main_Group
 */

EAPI Eina_Bool e_illume_border_is_fixed_size(E_Border *bd)
{
   /* make sure we have a border */
   if (!bd) return EINA_FALSE;

   if ((bd->client.icccm.min_w == bd->client.icccm.max_w) &&
       (bd->client.icccm.min_h == bd->client.icccm.max_h))
     return EINA_TRUE;
   
   if ((bd->client.mwm.exists) && 
       !((bd->client.mwm.func & ECORE_X_MWM_HINT_FUNC_ALL) ||
	 (bd->client.mwm.func & ECORE_X_MWM_HINT_FUNC_MAXIMIZE) ||
	 (bd->client.mwm.func & ECORE_X_MWM_HINT_FUNC_RESIZE)))
     return EINA_TRUE;

   return EINA_FALSE;
}

/**
 * Retrieves the minimum space required to display this border.
 * 
 * @param bd The border to get the minium space for.
 * @param w Pointer to an integer into which the width is to be stored.
 * @param h Pointer to an integer into which the height is to be stored.
 * 
 * @note if @p bd is NULL then @p w and @p h will return @c 0.
 * 
 * @ingroup E_Illume_Main_Group
 */
EAPI void 
e_illume_border_min_get(E_Border *bd, int *w, int *h) 
{
   if (w) *w = 0;
   if (h) *h = 0;
   if (!bd) return;

   if (w) 
     {
        if (bd->client.icccm.base_w > bd->client.icccm.min_w) 
          *w = bd->client.icccm.base_w;
        else
          *w = bd->client.icccm.min_w;
     }
   if (h) 
     {
        if (bd->client.icccm.base_h > bd->client.icccm.min_h)
          *h = bd->client.icccm.base_h;
        else
          *h = bd->client.icccm.min_h;
     }
}

/**
 * Retrieves a border, given an x and y coordinate, from a zone.
 * 
 * @param zone The zone.
 * @param x The X coordinate to check for border at.
 * @param y The Y coordinate to check for border at.
 * 
 * @note if @p zone is NULL then this function will return NULL.
 * 
 * @warning Both X and Y coordinates are required to reliably detect a border.
 * 
 * @ingroup E_Illume_Main_Group
 */
EAPI E_Border *
e_illume_border_at_xy_get(E_Zone *zone, int x, int y) 
{
   Eina_List *l;
   E_Border *bd;

   /* make sure we have a zone */
   if (!zone) return NULL;

   /* loop the border client list */
   /* NB: We use e_border_client_list here, rather than 
    * e_container_border_list, because e_border_client_list is faster.
    * This is done in reverse order so we get the most recent border first */
   EINA_LIST_REVERSE_FOREACH(e_border_client_list(), l, bd) 
     {
        /* check zone and skip borders not on this zone */
        if (bd->zone != zone) continue;

        /* skip invisibles */
        if (!bd->visible) continue;

        /* check position against given coordinates */
        if ((bd->x != x) || (bd->y != y)) continue;

        /* filter out borders we don't want */
        if (e_illume_border_is_indicator(bd)) continue;
        if (e_illume_border_is_softkey(bd)) continue;
        if (e_illume_border_is_keyboard(bd)) continue;
        if (e_illume_border_is_quickpanel(bd)) continue;
        if (e_illume_border_is_home(bd)) continue;

        /* found one, return it */
        return bd;
     }

   /* return a fallback */
   return NULL;
}

/**
 * Retrieve the parent of a given dialog.
 * 
 * @param bd The border to get the parent of.
 * @return The border's parent, or NULL if no parent exists.
 * 
 * @note If @p bd is NULL then this function will return NULL.
 * 
 * @ingroup E_Illume_Main_Group
 */
EAPI E_Border *
e_illume_border_parent_get(E_Border *bd) 
{
   /* make sure we have a border */
   if (!bd) return NULL;

   /* check for border's parent */
   if (bd->parent) return bd->parent;

   /* NB: TEST CODE - may need to check bd->leader here */
   if (bd->leader) 
     printf("\tDialog Has Leader: %s\n", bd->client.icccm.name);

   /* check for transient */
   if (bd->client.icccm.transient_for) 
     {
        /* try to find this borders parent */
        return e_border_find_by_client_window(bd->client.icccm.transient_for);
     }
   else if (bd->client.icccm.client_leader) 
     {
        /* NB: using client_leader as parent. THIS NEEDS THOROUGH TESTING !! */
        return e_border_find_by_client_window(bd->client.icccm.client_leader);
     }

   /* return a fallback */
   return NULL;
}

/**
 * Show a given border.
 * 
 * @param bd The border to show.
 * 
 * @note If @p bd is NULL then this function will return.
 * 
 * @ingroup E_Illume_Main_Group
 */
EAPI void 
e_illume_border_show(E_Border *bd) 
{
   /* make sure we have a border */
   if (!bd) return;

   e_border_uniconify(bd);
   e_border_raise(bd);
   e_border_show(bd);
   return;

#if 0   
   unsigned int visible = 1;

   /* NB: We handle shows this way so we don't get extra layout events from 
    * the e_border calls */
   e_container_border_lower(bd);
   e_container_shape_show(bd->shape);
   if (!bd->need_reparent) ecore_x_window_show(bd->client.win);
   e_hints_window_visible_set(bd);
   bd->visible = 1;
   bd->changes.visible = 1;
   ecore_x_window_prop_card32_set(bd->client.win, E_ATOM_MAPPED, &visible, 1);
   ecore_x_window_prop_card32_set(bd->client.win, E_ATOM_MANAGED, &visible, 1);
#endif   
}

/**
 * Hide a given border.
 * 
 * @param bd The border to hide.
 * 
 * @note If @p bd is NULL then this function will return.
 * 
 * @ingroup E_Illume_Main_Group
 */
EAPI void 
e_illume_border_hide(E_Border *bd) 
{
   /* make sure we have a border */
   if (!bd) return;

   e_border_iconify(bd);
   return;

#if 0   
   unsigned int visible = 0;

   /* NB: We handle hides this way so we don't get extra layout events from 
    * the e_border calls */
   e_container_shape_hide(bd->shape);
   if (!bd->iconic) e_hints_window_hidden_set(bd);
   bd->visible = 0;
   bd->changes.visible = 1;
   ecore_x_window_prop_card32_set(bd->client.win, E_ATOM_MAPPED, &visible, 1);
#endif   
}

/**
 * Retrieve the Indicator window on a given zone.
 * 
 * @param zone The zone.
 * @return The Indicator border, or NULL if no Indicator exists.
 * 
 * @note If @p zone is NULL then this function will return NULL.
 * 
 * @ingroup E_Illume_Main_Group
 */
EAPI E_Border *
e_illume_border_indicator_get(E_Zone *zone) 
{
   Eina_List *l;
   E_Border *bd;

   /* make sure we have a zone */
   if (!zone) return NULL;

   /* loop the border client list */
   /* NB: We use e_border_client_list here, rather than 
    * e_container_border_list, because e_border_client_list is faster */
   EINA_LIST_FOREACH(e_border_client_list(), l, bd) 
     {
        /* check zone and skip borders not on this zone */
        if (bd->zone != zone) continue;

        /* skip borders that are not indicators */
        if (!e_illume_border_is_indicator(bd)) continue;

        /* found one, return it */
        return bd;
     }

   /* return a fallback */
   return NULL;
}

/**
 * Retrieves the current position of the Indicator window.
 * 
 * @param zone The zone on which to retrieve the Indicator position.
 * @param x Pointer to an integer into which the left is to be stored.
 * @param y Pointer to an integer into which the top is to be stored.
 * 
 * @note if @p zone is NULL then @p x, @p y, @p w, and @p h will return @c 0.
 * 
 * @ingroup E_Illume_Main_Group
 */
EAPI void 
e_illume_border_indicator_pos_get(E_Zone *zone, int *x, int *y) 
{
   E_Border *ind;

   if (x) *x = 0;
   if (y) *y = 0;

   /* make sure we have a zone */
   if (!zone) return;

   /* set default values */
   if (x) *x = zone->x;
   if (y) *y = zone->y;

   /* try and get the Indicator on this zone */
   if (!(ind = e_illume_border_indicator_get(zone))) return;

   /* return Indicator position(s) */
   if (x) *x = ind->x;
   if (y) *y = ind->y;
}

/**
 * Retrieve the Softkey window on a given zone.
 * 
 * @param zone The zone.
 * @return The Softkey border, or NULL if no Softkey exists.
 * 
 * @note If @p zone is NULL then this function will return NULL.
 * 
 * @ingroup E_Illume_Main_Group
 */
EAPI E_Border *
e_illume_border_softkey_get(E_Zone *zone) 
{
   Eina_List *l;
   E_Border *bd;

   /* make sure we have a zone */
   if (!zone) return NULL;

   /* loop the border client list */
   /* NB: We use e_border_client_list here, rather than 
    * e_container_border_list, because e_border_client_list is faster */
   EINA_LIST_FOREACH(e_border_client_list(), l, bd) 
     {
        /* check zone and skip borders not on this zone */
        if (bd->zone != zone) continue;

        /* skip borders that are not indicators */
        if (!e_illume_border_is_softkey(bd)) continue;

        /* found one, return it */
        return bd;
     }

   /* return a fallback */
   return NULL;
}

/**
 * Retrieves the current position of the Softkey window.
 * 
 * @param zone The zone on which to retrieve the Softkey position.
 * @param x Pointer to an integer into which the left is to be stored.
 * @param y Pointer to an integer into which the top is to be stored.
 * 
 * @note if @p zone is NULL then @p x, @p y, @p w, and @p h will return @c 0.
 * 
 * @ingroup E_Illume_Main_Group
 */
EAPI void 
e_illume_border_softkey_pos_get(E_Zone *zone, int *x, int *y) 
{
   E_Border *sft;

   if (x) *x = 0;
   if (y) *y = 0;

   /* make sure we have a zone */
   if (!zone) return;

  /* set default values */
   if (x) *x = zone->x;
   if (y) *y = zone->y;

   /* try and get the Softkey on this zone */
   if (!(sft = e_illume_border_softkey_get(zone))) return;

   /* return Indicator position(s) */
   if (x) *x = sft->x;
   if (y) *y = sft->y;
}

/**
 * Retrieve the Keyboard.
 * 
 * @return The @ref E_Illume_Keyboard, or NULL if no keyboard exists.
 * 
 * @ingroup E_Illume_Keyboard_Group
 */
EAPI E_Illume_Keyboard *
e_illume_keyboard_get(void) 
{
   /* make sure we have a keyboard */
   if (!_e_illume_kbd) return NULL;

   /* return the keyboard */
   return _e_illume_kbd;
}

/**
 * Retrieves the available screen space not occupied by the virtual keyboard.
 * 
 * @param zone The zone on which to retrieve the available space.
 * @param x Pointer to an integer into which the left is to be stored.
 * @param y Pointer to an integer into which the top is to be stored.
 * @param w Pointer to an integer into which the width is to be stored.
 * @param h Pointer to an integer into which the height is to be stored.
 * 
 * @note if @p zone is NULL then @p x, @p y, @p w, and @p h will return @c 0.
 * 
 * @warning This function does not account for space or position of Indicator 
 * or Softkey windows.
 * 
 * @ingroup E_Illume_Keyboard_Group
 */
EAPI void 
e_illume_keyboard_safe_app_region_get(E_Zone *zone, int *x, int *y, int *w, int *h) 
{
   if (x) *x = 0;
   if (y) *y = 0;
   if (w) *w = 0;
   if (h) *h = 0;

   /* make sure we have a zone */
   if (!zone) return;

   /* set default values */
   if (x) *x = zone->x;
   if (y) *y = zone->y;
   if (w) *w = zone->w;
   if (h) *h = zone->h;

   /* if the keyboard is disabled, get out */
   if ((!_e_illume_kbd->visible) || (_e_illume_kbd->disabled)) return;

   /* if we don't have a border, get out */
   /* NB: This basically means that we have the vkbd structure, but no 
    * app or module present to act as the vkbd */
   if (!_e_illume_kbd->border) return;

   /* if the keyboard border is not on this zone, get out */
   if (_e_illume_kbd->border->zone != zone) return;

   if (!_e_illume_kbd->animator) 
     {
        if (h) 
          {
             *h -= _e_illume_kbd->border->h;
             if (*h < 0) *h = 0;
          }
     }
}

/**
 * Retrieve the Home window on a given zone.
 * 
 * @param zone The zone.
 * @return The Home window, or NULL if no Home window exists.
 * 
 * @note If @p zone is NULL then this function will return NULL.
 * 
 * @ingroup E_Illume_Main_Group
 */
EAPI E_Border *
e_illume_border_home_get(E_Zone *zone) 
{
   E_Border *bd;
   Eina_List *l;

   /* make sure we have a zone */
   if (!zone) return NULL;

   /* loop the border client list */
   /* NB: We use e_border_client_list here, rather than 
    * e_container_border_list, because e_border_client_list is faster */
   EINA_LIST_FOREACH(e_border_client_list(), l, bd) 
     {
        /* check zone and skip borders not on this zone */
        if (bd->zone != zone) continue;

        /* skip borders that are not home windows */
        if (!e_illume_border_is_home(bd)) continue;

        /* found one, return it */
        return bd;
     }

   /* return a fallback */
   return NULL;
}

/**
 * Retrieve the list of Home windows on a given zone.
 * 
 * @param zone The zone.
 * @return A list of existing Home windows, or NULL if none exist.
 * 
 * @note If @p zone is NULL then this function will return NULL.
 * 
 * @ingroup E_Illume_Main_Group
 */
EAPI Eina_List *
e_illume_border_home_borders_get(E_Zone *zone) 
{
   Eina_List *ret = NULL, *l;
   E_Border *bd;

   /* make sure we have a zone */
   if (!zone) return NULL;

   /* loop the border client list */
   /* NB: We use e_border_client_list here, rather than 
    * e_container_border_list, because e_border_client_list is faster */
   EINA_LIST_FOREACH(e_border_client_list(), l, bd) 
     {
        /* check zone and skip borders not on this zone */
        if (bd->zone != zone) continue;

        /* skip borders that are not home windows */
        if (!e_illume_border_is_home(bd)) continue;

        /* found one, append it to the list */
        ret = eina_list_append(ret, bd);
     }

   /* return the list */
   return ret;
}

/**
 * Retrieve the Illume Quickpanel on a given zone.
 * 
 * @param zone The zone on which to retrieve the Quickpanel.
 * @return The @ref E_Illume_Quickpanel on this zone, or NULL if none exists.
 * 
 * @note If @p zone is NULL then this function will return NULL.
 * 
 * @ingroup E_Illume_Quickpanel_Group
 */
EAPI E_Illume_Quickpanel *
e_illume_quickpanel_by_zone_get(E_Zone *zone) 
{
   E_Illume_Quickpanel *qp;
   Eina_List *l;

   /* make sure we have a zone */
   if (!zone) return NULL;

   /* loop the list of quickpanels, looking for one on this zone */
   EINA_LIST_FOREACH(_e_illume_qps, l, qp)
     if (qp->zone == zone) return qp;

   /* return a fallback */
   return NULL;
}

/**
 * Show the Illume Quickpanel on a given zone.
 * 
 * @param zone The zone on which to show the Quickpanel.
 * 
 * @note If @p zone is NULL then this function will return.
 * 
 * @ingroup E_Illume_Quickpanel_Group
 */
EAPI void 
e_illume_quickpanel_show(E_Zone *zone) 
{
   if (!zone) return;
   ecore_x_e_illume_quickpanel_state_send(zone->black_win, 
                                          ECORE_X_ILLUME_QUICKPANEL_STATE_ON);
}

/**
 * Hide the Illume Quickpanel on a given zone.
 * 
 * @param zone The zone on which to hide the Quickpanel.
 * 
 * @note If @p zone is NULL then this function will return.
 * 
 * @ingroup E_Illume_Quickpanel_Group
 */
EAPI void 
e_illume_quickpanel_hide(E_Zone *zone) 
{
   if (!zone) return;
   ecore_x_e_illume_quickpanel_state_send(zone->black_win, 
                                          ECORE_X_ILLUME_QUICKPANEL_STATE_OFF);
}
