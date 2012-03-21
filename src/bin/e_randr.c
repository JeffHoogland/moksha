/*
 * vim:ts=8:sw=3:sts=8:expandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_randr_private.h"

#define ECORE_X_RANDR_1_1   ((1 << 16) | 1)
#define ECORE_X_RANDR_1_2   ((1 << 16) | 2)
#define ECORE_X_RANDR_1_3   ((1 << 16) | 3)

#define Ecore_X_Randr_None   0
#define Ecore_X_Randr_Unset -1

/*
 * Save mechanism:
 * Single monitor:
 * - Save monitor using the resolution
 *
 * Multiple monitors:
 * - Use the EDID information to make sure we restore the right monitor.
 * - Depend on the sequence in which the XIDs are returned provided by the X
 *   server. This means that relative indexes are used for setup store/restore.
 *
 */

static Eina_Bool _init(void);
static void      _shutdown(void);
static Eina_Bool _screen_info_refresh(void);
static Eina_Bool _e_event_config_loaded_cb(void *data, int type, void *e);
static void      _event_listeners_add(void);
static void      _event_listeners_remove(void);

EAPI E_Randr_Screen_Info e_randr_screen_info;
static Eina_List *_event_handlers = NULL;

EINTERN Eina_Bool
e_randr_init(void)
{
   return _init();
}

EINTERN int
e_randr_shutdown(void)
{
   _shutdown();
   return 1;
}

EAPI Eina_Bool
e_randr_screen_info_refresh(void)
{
   return _screen_info_refresh();
}

static Eina_Bool
_init(void)
{
   if (!e_randr_screen_info_refresh())
     return EINA_FALSE;
   _event_listeners_add();

   if (e_config->randr_serialized_setup)
     {
        e_randr_try_restore_configuration();

        if (e_randr_screen_info.randr_version >= ECORE_X_RANDR_1_2)
          {
             if (e_config->randr_serialized_setup->outputs_policies)
               _12_policies_restore();
          }
     }

   return EINA_TRUE;
}

static void
_shutdown(void)
{
   if (e_randr_screen_info.randr_version == ECORE_X_RANDR_1_1)
     {
        _11_screen_info_free(e_randr_screen_info.rrvd_info.randr_info_11);
        e_randr_screen_info.rrvd_info.randr_info_11 = NULL;
     }
   else if (e_randr_screen_info.randr_version >= ECORE_X_RANDR_1_2)
     {
        _12_screen_info_free(e_randr_screen_info.rrvd_info.randr_info_12);
        e_randr_screen_info.rrvd_info.randr_info_12 = NULL;
     }
   _event_listeners_remove();
}


/**
 * @return EINA_TRUE if info could be refreshed, else EINA_FALSE
 */
static Eina_Bool
_screen_info_refresh(void)
{
   Ecore_X_Window *roots;
   Ecore_X_Window root;
   int n;

   EINA_SAFETY_ON_FALSE_RETURN_VAL(ecore_x_randr_query(), EINA_FALSE);

   if (!(roots = ecore_x_window_root_list(&n))) return EINA_FALSE;
   /* first (and only) root window */
   root = roots[0];
   free(roots);

   e_randr_screen_info.randr_version = ecore_x_randr_version_get();
   e_randr_screen_info.root = root;
   e_randr_screen_info.rrvd_info.randr_info_11 = NULL;

   // Value set/retrieval helper functions
   if (e_randr_screen_info.randr_version == ECORE_X_RANDR_1_1)
     {
        return _11_screen_info_refresh();
     }
   else if (e_randr_screen_info.randr_version >= ECORE_X_RANDR_1_2)
     {
        return _12_screen_info_refresh();
     }

   return EINA_FALSE;
}

static Eina_Bool
_e_event_config_loaded_cb(void *data __UNUSED__, int type, void *ev __UNUSED__)
{
   if (type != E_EVENT_CONFIG_LOADED) return EINA_TRUE;

   _try_restore_configuration();

   return EINA_FALSE;
}

static void
_event_listeners_add(void)
{
   _event_handlers = eina_list_append(_event_handlers, ecore_event_handler_add(E_EVENT_CONFIG_LOADED, _e_event_config_loaded_cb, NULL));

   if (e_randr_screen_info.randr_version >= ECORE_X_RANDR_1_2)
     {
        _12_event_listeners_add();
     }
}

// "Free" helper functions
static void
_event_listeners_remove(void)
{
   Ecore_Event_Handler *_event_handler = NULL;
   EINA_LIST_FREE (_event_handlers, _event_handler)
     ecore_event_handler_del(_event_handler);

   if (e_randr_screen_info.randr_version >= ECORE_X_RANDR_1_2)
     {
        _12_event_listeners_remove();
     }
}
