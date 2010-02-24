#ifndef E_ILLUME_H
# define E_ILLUME_H

/* include standard E header */
# include "e.h"

/**
 * @mainpage Illume
 * 
 * @image html e.png
 * 
 * @author Christopher Michael
 * @date 2010
 * 
 * @section illume_toc_sec Table of contents
 * 
 * <ul>
 *   <li> @ref illume_intro_sec
 * </ul>
 * 
 * @section illume_intro_sec Introduction to Illume
 * 
 * Illume is a module for Enlightenment that modifies the user interface of 
 * enlightenment to work cleanly and nicely on a mobile device - such as an 
 * Openmoko phone. It is resolution independent meaning that it can 
 * accommodate a very wide range of devices, from cell phones and PDAs to 
 * tablets and desktops. Illume has been designed from the ground up to 
 * support more than one screen in more than one way (multihead and xinerama).
 * 
 * @warning This is a work in progress and as such is subject to change.
 */

/**
 * @file e_illume.h
 * 
 * This header provides the various defines, structures and functions that 
 * make writing illume policies easier.
 * 
 * For details on the available functions, see @ref E_Illume_Main_Group.
 * 
 * For details on the configuration structure, see @ref E_Illume_Config_Group.
 * 
 * For details on the virtual keyboard, see @ref E_Illume_Keyboard_Group.
 * 
 * For details on the Policy API, see @ref E_Illume_Policy_Group.
 * 
 * For details on quickpanels, see @ref E_Illume_Quickpanel_Group.
 */

/**
 * @defgroup E_Illume_Keyboard_Group Illume Keyboard Information
 * 
 * The following group defines information needed to interact with the 
 * Virtual Keyboard.
 * 
 */

/**
 * @enum E_Illume_Keyboard_Layout
 * 
 * enumeration for available keyboard layout modes
 * 
 * @ingroup E_Illume_Keyboard_Group
 */
typedef enum _E_Illume_Keyboard_Layout 
{
   E_ILLUME_KEYBOARD_LAYOUT_NONE, /**< no keyboard layout specified. */
   E_ILLUME_KEYBOARD_LAYOUT_DEFAULT, /**< default keyboard layout. */
   E_ILLUME_KEYBOARD_LAYOUT_ALPHA, /**< alpha keyboard layout. */
   E_ILLUME_KEYBOARD_LAYOUT_NUMERIC, /**< numeric keyboard layout */
   E_ILLUME_KEYBOARD_LAYOUT_PIN, /**< pin keyboard layout */
   E_ILLUME_KEYBOARD_LAYOUT_PHONE_NUMBER, /**< phone number keyboard layout */
   E_ILLUME_KEYBOARD_LAYOUT_HEX, /**< hex keyboard layout */
   E_ILLUME_KEYBOARD_LAYOUT_TERMINAL, /**< terminal keyboard layout */
   E_ILLUME_KEYBOARD_LAYOUT_PASSWORD, /**< password keyboard layout */
   E_ILLUME_KEYBOARD_LAYOUT_IP, /**< IP keyboard layout */
   E_ILLUME_KEYBOARD_LAYOUT_HOST, /**< host keyboard layout */
   E_ILLUME_KEYBOARD_LAYOUT_FILE, /**< file keyboard layout */
   E_ILLUME_KEYBOARD_LAYOUT_URL, /**< url keyboard layout */
   E_ILLUME_KEYBOARD_LAYOUT_KEYPAD, /**< keypad keyboard layout */
   E_ILLUME_KEYBOARD_LAYOUT_J2ME /**< J2ME keyboard layout */
} E_Illume_Keyboard_Layout;

/**
 * @brief structure for keyboard.
 * 
 * @ingroup E_Illume_Keyboard_Group
 */
typedef struct _E_Illume_Keyboard 
{
   E_Object e_obj_inherit;

   E_Border *border;
   Ecore_Timer *timer;
   Ecore_Animator *animator;

   E_Illume_Keyboard_Layout layout;
   /**< current @ref E_Illume_Keyboard_Layout mode of the keyboard */

   Eina_List *waiting_borders;

   double start, len;
   int adjust, adjust_start, adjust_end;

   unsigned char visible : 1;
   /**< flag to indicate if the keyboard is currently visible */
   unsigned char disabled : 1;
   /**< flag to indicate if the keyboard is currently disabled */
   unsigned char fullscreen : 1;
   /**< flag to indicate if the keyboard is currently fullscreen */
} E_Illume_Keyboard;

/**
 * @defgroup E_Illume_Policy_Group Illume Policy Information
 * 
 * The following group defines information needed to implement an Illume 
 * Policy.
 * 
 * @warning There are some requirements that every policy must implement and 
 * some things are optional. Please reference @ref E_Illume_Policy structure 
 * for the requirements.
 */

/**
 * @def E_ILLUME_POLICY_API_VERSION
 * @brief Current version of the Policy API that is supported by the Illume module.
 * 
 * @warning Policies not written to match this version will fail to load.
 * 
 * @ingroup E_Illume_Policy_Group
 */
# define E_ILLUME_POLICY_API_VERSION 2

/**
 * @brief structure for policy API.
 * 
 * @details When Illume tries to load a policy, it will check for the 
 * existince of this structure. If it is not found, the policy will fail 
 * to load.
 * 
 * @warning This structure is required for Illume to load a policy.
 * 
 * @ingroup E_Illume_Policy_Group
 */
typedef struct _E_Illume_Policy_Api 
{
   int version;
   /**< The version of this policy. */

   const char *name;
   /**< The name of this policy. */
   const char *label;
   /**< The label of this policy. */
} E_Illume_Policy_Api;


typedef struct _E_Illume_Policy E_Illume_Policy;

/**
 * @brief structure for policy
 * 
 * This structure actually holds the policy functions that Illume will call 
 * at the appropriate times.
 * 
 * @ingroup E_Illume_Policy_Group
 */
struct _E_Illume_Policy 
{
   E_Object e_obj_inherit;

   E_Illume_Policy_Api *api;
   /**< pointer to the @ref E_Illume_Policy_Api structure.
    * @warning Policies are required to implement this or they will fail to 
    * load. */

   void *handle;

   struct 
     {
        void *(*init) (E_Illume_Policy *p);
        /**< pointer to the function that Illume will call to initialize this 
         * policy. @warning Policies are required to implement this function. */

        int (*shutdown) (E_Illume_Policy *p);
        /**< pointer to the function that Illume will call to shutdown this 
         * policy. @warning Policies are required to implement this function. */

        void (*border_add) (E_Border *bd);
        /**< pointer to the function that Illume will call when a new border 
         * gets added. @note This function is optional. */

        void (*border_del) (E_Border *bd);
        /**< pointer to the function that Illume will call when a border gets 
         * deleted. @note This function is optional. */

        void (*border_focus_in) (E_Border *bd);
        /**< pointer to the function that Illume will call when a border gets 
         * focus. @note This function is optional. */

        void (*border_focus_out) (E_Border *bd);
        /**< pointer to the function that Illume will call when a border loses 
         * focus. @note This function is optional. */

        void (*border_activate) (E_Border *bd);
        /**< pointer to the function that Illume will call when a border gets 
         * an activate message. @note This function is optional. */

        void (*border_post_fetch) (E_Border *bd);
        /**< pointer to the function that Illume will call when E signals a 
         * border post fetch. @note This function is optional. */

        void (*border_post_assign) (E_Border *bd);
        /**< pointer to the function that Illume will call when E signals a 
         * border post assign. @note This function is optional. */

        void (*border_show) (E_Border *bd);
        /**< pointer to the function that Illume will call when a border gets 
         * shown. @note This function is optional. */

        void (*zone_layout) (E_Zone *zone);
        /**< pointer to the function that Illume will call when a Zone needs 
         * to update it's layout. @note This function is optional. */

        void (*zone_move_resize) (E_Zone *zone);
        /**< pointer to the function that Illume will call when a Zone gets 
         * moved or resized. @note This function is optional. */

        void (*zone_mode_change) (E_Zone *zone, Ecore_X_Atom mode);
        /**< pointer to the function that Illume will call when the layout 
         * mode of a Zone changes. @note This function is optional. */

        void (*zone_close) (E_Zone *zone);
        /**< pointer to the function that Illume will call when the user has 
         * requested a border get closed. This is usually signaled from the 
         * Softkey window. @note This function is optional. */

        void (*drag_start) (E_Border *bd);
        /**< pointer to the function that Illume will call when the user has 
         * started to drag the Indicator/Softkey windows.
         * @note This function is optional. */

        void (*drag_end) (E_Border *bd);
        /**< pointer to the function that Illume will call when the user has 
         * stopped draging the Indicator/Softkey windows.
         * @note This function is optional. */

        void (*focus_back) (E_Zone *zone);
        /**< pointer to the function that Illume will call when the user has 
         * requested to cycle the focused border backwards. This is typically 
         * signalled from the Softkey window.
         * @note This function is optional. */

        void (*focus_forward) (E_Zone *zone);
        /**< pointer to the function that Illume will call when the user has 
         * requested to cycle the focused border forward. This is typically 
         * signalled from the Softkey window.
         * @note This function is optional. */

        void (*focus_home) (E_Zone *zone);
        /**< pointer to the function that Illume will call when the user has 
         * requested that Home window be focused.
         * @note This function is optional. */

        void (*property_change) (Ecore_X_Event_Window_Property *event);
        /**< pointer to the function that Illume will call when properties 
         * change on a window. @note This function is optional. */
     } funcs;
};

/**
 * @defgroup E_Illume_Config_Group Illume Configuration Information
 * 
 * The following group defines information pertaining to Illume Configuration.
 */

/**
 * @brief structure for Illume configuration.
 * 
 * @ingroup E_Illume_Config_Group
 */
typedef struct _E_Illume_Config 
{
   int version;

   struct 
     {
        struct 
          {
             int duration;
          } vkbd, quickpanel;
     } animation;

   struct 
     {
        const char *name;
        struct 
          {
             const char *class, *name, *title;
             int type;
             struct 
               {
                  int class, name, title, type;
               } match;
          } vkbd, indicator, softkey, home;
        Eina_List *zones;
     } policy;
} E_Illume_Config;

/**
 * @brief structure for Illume zone configuration.
 * 
 * @ingroup E_Illume_Config_Group
 */
typedef struct _E_Illume_Config_Zone 
{
   int id;
   struct 
     {
        int dual, side;
     } mode;

   /* NB: These are not configurable by user...just placeholders */
   struct 
     {
        int size;
     } vkbd, indicator, softkey;
} E_Illume_Config_Zone;

/**
 * @defgroup E_Illume_Quickpanel_Group Illume Quickpanel Information
 * 
 * The following group defines information pertaining to Illume Quickpanels.
 */

/**
 * @brief structure for Illume Quickpanels.
 * 
 * @ingroup E_Illume_Quickpanel_Group
 */
typedef struct _E_Illume_Quickpanel 
{
   E_Object e_obj_inherit;

   E_Zone *zone;
   Eina_List *borders;
   Ecore_Timer *timer;
   Ecore_Animator *animator;
   double start, len;
   int h, ih, adjust, adjust_start, adjust_end;
   unsigned char visible : 1;
   /**< flag to indicate if the quickpanel is currently visible */
} E_Illume_Quickpanel;

/* define function prototypes that policies can use */
EAPI E_Illume_Config_Zone *e_illume_zone_config_get(int id);

/* general functions */
EAPI Eina_Bool e_illume_border_is_indicator(E_Border *bd);
EAPI Eina_Bool e_illume_border_is_softkey(E_Border *bd);
EAPI Eina_Bool e_illume_border_is_keyboard(E_Border *bd);
EAPI Eina_Bool e_illume_border_is_home(E_Border *bd);
EAPI Eina_Bool e_illume_border_is_splash(E_Border *bd);
EAPI Eina_Bool e_illume_border_is_dialog(E_Border *bd);
EAPI Eina_Bool e_illume_border_is_qt_frame(E_Border *bd);
EAPI Eina_Bool e_illume_border_is_fullscreen(E_Border *bd);
EAPI Eina_Bool e_illume_border_is_conformant(E_Border *bd);
EAPI Eina_Bool e_illume_border_is_quickpanel(E_Border *bd);

EAPI void e_illume_border_min_get(E_Border *bd, int *w, int *h);
EAPI E_Border *e_illume_border_at_xy_get(E_Zone *zone, int x, int y);
EAPI E_Border *e_illume_border_parent_get(E_Border *bd);
EAPI void e_illume_border_show(E_Border *bd);
EAPI void e_illume_border_hide(E_Border *bd);

/* indicator functions */
EAPI E_Border *e_illume_border_indicator_get(E_Zone *zone);
EAPI void e_illume_border_indicator_pos_get(E_Zone *zone, int *x, int *y);

/* softkey functions */
EAPI E_Border *e_illume_border_softkey_get(E_Zone *zone);
EAPI void e_illume_border_softkey_pos_get(E_Zone *zone, int *x, int *y);

/* keyboard functions */
EAPI E_Illume_Keyboard *e_illume_keyboard_get(void);
EAPI void e_illume_keyboard_safe_app_region_get(E_Zone *zone, int *x, int *y, int *w, int *h);

/* home functions */
EAPI E_Border *e_illume_border_home_get(E_Zone *zone);
EAPI Eina_List *e_illume_border_home_borders_get(E_Zone *zone);

/* quickpanel functions */
EAPI E_Illume_Quickpanel *e_illume_quickpanel_by_zone_get(E_Zone *zone);

#endif
