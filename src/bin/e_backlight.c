#include "e.h"
#include <E_DBus.h>
#ifdef HAVE_HAL
#include <E_Hal.h>
#endif

// FIXME: backlight should be tied per zone but this implementation is just
// a signleton right now as thats 99% of use cases. but api supports
// doing more. for now make it work in the singleton

// FIXME: backlight should have config values for:
//        1. normal mode (eg on login/ start of e)
//        2. dim level (eg 0.5)
//        3. anim slide time (eg 0.5)

// FIXME: tried using hal backlight stuff... doesn't work
//#define HAL_BL 1

#define MODE_RANDR 0
#define MODE_HAL 1

static double bl_val = 1.0;
static double bl_animval = 1.0;
static E_Backlight_Mode bl_mode = E_BACKLIGHT_MODE_NORMAL;
static int sysmode = MODE_RANDR;
static Ecore_Animator *bl_anim = NULL;

#ifdef HAL_BL        
static E_DBus_Connection *_hal_conn = NULL;
static const char *_hal_bl_dev = NULL;
static const char *_hal_bl_iface = NULL;
static int _hal_nlevels = 1;
#endif

static void _e_backlight_update(E_Zone *zone);
static void _e_backlight_set(E_Zone *zone, double val);
static Eina_Bool _bl_anim(void *data, double pos);

EINTERN int
e_backlight_init(void)
{
   e_backlight_update();
   e_backlight_level_set(NULL, 0.0, 0.0);
   e_backlight_level_set(NULL, e_config->backlight.normal, 1.0);
   return 1;
}

EINTERN int
e_backlight_shutdown(void)
{
#ifdef HAL_BL        
   if (_hal_conn)
     {
        e_dbus_connection_close(_hal_conn);
        _hal_conn = NULL;
#ifdef HAVE_HAL
        e_hal_shutdown();
#endif
        e_dbus_shutdown();
        if (_hal_bl_dev)
          {
             eina_stringshare_del(_hal_bl_dev);
             _hal_bl_dev = NULL;
          }
        if (_hal_bl_iface)
          {
             eina_stringshare_del(_hal_bl_iface);
             _hal_bl_iface = NULL;
          }
     }
#endif   
   if (bl_anim) ecore_animator_del(bl_anim);
   bl_anim = NULL;
   return 1;
}

EAPI void
e_backlight_update(void)
{
   Eina_List *m, *c, *z;
   E_Manager *man;
   E_Container *con;
   E_Zone *zone;
   
   EINA_LIST_FOREACH(e_manager_list(), m, man)
     {
        EINA_LIST_FOREACH(man->containers, c, con)
          {
             EINA_LIST_FOREACH(con->zones, z, zone)
               {
                  _e_backlight_update(zone);
               }
          }
     }
}

EAPI void
e_backlight_level_set(E_Zone *zone, double val, double tim)
{
   double bl_now;
   // zone == NULL == everything
   // set backlight associated with zone to val over period of tim
   // if tim == 0.0 - then do it instantnly, if time == -1 use some default
   // transition time
   if (!zone) e_backlight_update();
   else _e_backlight_update(zone);
   if (val == bl_val) return;
   if (!zone) zone = e_util_zone_current_get(e_manager_current_get());
   bl_now = bl_val;
   bl_val = val;
   if (bl_mode != E_BACKLIGHT_MODE_NORMAL) return;
   if (tim < 0.0) tim = e_config->backlight.transition;
   // FIXME: save bl level for normal
   if (tim == 0.0)
     {
        if (bl_anim)
          {
             ecore_animator_del(bl_anim);
             bl_anim = NULL;
          }
        _e_backlight_set(zone, val);
       return;
     }
   if (bl_anim) ecore_animator_del(bl_anim);
   bl_anim = ecore_animator_timeline_add(tim, _bl_anim, zone);
   bl_animval = bl_now;
}

EAPI double
e_backlight_level_get(E_Zone *zone)
{
   // zone == NULL == everything
   if (!zone) e_backlight_update();
   else _e_backlight_update(zone);
   return bl_val;
}

EAPI void
e_backlight_mode_set(E_Zone *zone, E_Backlight_Mode mode)
{
   // zone == NULL == everything
   if (bl_mode == mode) return;
   bl_mode = mode;
   if      (bl_mode == E_BACKLIGHT_MODE_NORMAL)
      e_backlight_level_set(zone, bl_val, -1.0);
   else if (bl_mode == E_BACKLIGHT_MODE_OFF)
      e_backlight_level_set(zone, 0.0, -1.0);
   else if (bl_mode == E_BACKLIGHT_MODE_DIM)
      e_backlight_level_set(zone, 0.0, -1.0);
   else if (bl_mode == E_BACKLIGHT_MODE_DIM)
      e_backlight_level_set(zone, e_config->backlight.dim, -1.0);
   else if (bl_mode == E_BACKLIGHT_MODE_MAX)
      e_backlight_level_set(zone, 1.0, -1.0);
}

EAPI E_Backlight_Mode
e_backlight_mode_get(E_Zone *zone __UNUSED__)
{
   // zone == NULL == everything
   return bl_mode;
}

/* local subsystem functions */
#ifdef HAL_BL        
#ifdef HAVE_HAL
void
_e_backlight_hal_val_reply(void *data __UNUSED__, 
                           DBusMessage *reply, 
                           DBusError *error)
{
   dbus_uint32_t val;
   
   if (dbus_error_is_set(error))
     {
        printf("Error: %s - %s\n", error->name, error->message);
        return;
     }
   dbus_message_get_args(reply, error, DBUS_TYPE_UINT32, 
                         &val, DBUS_TYPE_INVALID);
   printf("Received: %i\n", val);
}
   
static void
_e_backlight_hal_val_get(void)
{
   DBusMessage *msg;
   
   if (!_hal_bl_dev) return;
   msg = dbus_message_new_method_call
   ("org.freedesktop.Hal",
       _hal_bl_dev,
       _hal_bl_iface,
       "GetBrightness"
   );
   e_dbus_message_send(_hal_conn, msg, _e_backlight_hal_val_reply, -1, NULL);
   dbus_message_unref(msg);
}

static void
_e_backlight_hal_val_set(double val)
{
   DBusMessage *msg;
   dbus_uint32_t ival = 0;
   
   if (!_hal_bl_dev) return;
   msg = dbus_message_new_method_call
   ("org.freedesktop.Hal",
       _hal_bl_dev,
       _hal_bl_iface,
       "SetBrightness"
   );
   ival = val * (_hal_nlevels - 1);
   printf("hal set %i\n", ival);
   dbus_message_append_args(msg, DBUS_TYPE_UINT32, &ival, DBUS_TYPE_INVALID);
   dbus_message_set_no_reply(msg, EINA_TRUE);
   e_dbus_message_send(_hal_conn, msg, NULL, -1, NULL);
   dbus_message_unref(msg);
}

static void
_e_backlight_prop(void      *data __UNUSED__,
                  void      *reply_data,
                  DBusError *error)
{
   E_Hal_Properties *ret = reply_data;
   int err;
   int nlevels;
   const Eina_List *sl;
   
   if (!ret) goto error;
   
   if (dbus_error_is_set(error))
     {
        dbus_error_free(error);
        goto error;
     }
   nlevels = e_hal_property_bool_get(ret, "laptop_panel.num_levels", &err);
   if (err) goto error;
   _hal_nlevels = nlevels;
   printf("nlevels: %i\n", nlevels);

   sl = e_hal_property_strlist_get(ret, "info.interfaces", &err);
   if (err) goto error;
   if (sl)
     {
        if (_hal_bl_iface) eina_stringshare_del(_hal_bl_iface);
        _hal_bl_iface = eina_stringshare_add(sl->data);
        printf("%s\n", _hal_bl_iface);
     }
   _e_backlight_hal_val_get();
   return;
error:
   if (_hal_bl_dev)
     {
        eina_stringshare_del(_hal_bl_dev);
        _hal_bl_dev = NULL;
     }
   if (_hal_bl_iface)
     {
        eina_stringshare_del(_hal_bl_iface);
        _hal_bl_iface = NULL;
     }
}

static void
_e_backlight_panel_found(void *user_data __UNUSED__,
                            void           *reply_data,
                            DBusError      *error)
{
   E_Hal_Manager_Find_Device_By_Capability_Return *ret = reply_data;
   Eina_List *l;
   char *device;
   
   if (!ret || !ret->strings) return;
   
   if (dbus_error_is_set(error))
     {
        dbus_error_free(error);
        return;
     }
   if (!_hal_bl_dev)
     {
        EINA_LIST_FOREACH(ret->strings, l, device)
          {
             printf("BL+: %s\n", device);
             if (!_hal_bl_dev) _hal_bl_dev = eina_stringshare_add(device);
          }
     }
   if (_hal_bl_dev)
     {
        e_hal_device_get_all_properties(_hal_conn, _hal_bl_dev,
                                        _e_backlight_prop, NULL);
     }
}
#endif
#endif

static void
_e_backlight_update(E_Zone *zone)
{
   double x_bl = -1.0;
   Ecore_X_Window root;
   Ecore_X_Randr_Output *out;
   int num = 0;

   root = zone->container->manager->root;
   // try randr
   out = ecore_x_randr_window_outputs_get(root, &num);
   if ((out) && (num > 0))
      x_bl = ecore_x_randr_output_backlight_level_get(root, out[0]);
   if (out) free(out);
   if (x_bl >= 0.0)
     {
        bl_val = x_bl;
        sysmode = MODE_RANDR;
     }
   else
     {
#ifdef HAL_BL        
#ifdef HAVE_HAL
        sysmode = MODE_HAL;
        if (!_hal_conn)
           {
              e_dbus_init();
              e_hal_init();
              _hal_conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
           }
        if (_hal_conn)
          {
             if (!_hal_bl_dev)
                e_hal_manager_find_device_by_capability
                (_hal_conn, "laptop_panel",
                    _e_backlight_panel_found, NULL);
          }
#endif
#endif        
     }
}

static void
_e_backlight_set(E_Zone *zone, double val)
{
   if (sysmode == MODE_RANDR)
     {
        Ecore_X_Window root;
        Ecore_X_Randr_Output *out;
        int num = 0;
        
        root = zone->container->manager->root;
        out = ecore_x_randr_window_outputs_get(root, &num);
        if ((out) && (num > 0))
          {
             ecore_x_randr_output_backlight_level_set(root, out[0], val);
          }
        if (out) free(out);
     }
   else if (sysmode == MODE_HAL)
     {
#ifdef HAL_BL        
#ifdef HAVE_HAL
        _e_backlight_hal_val_set(val);
#endif
#endif        
     }
}

static Eina_Bool
_bl_anim(void *data, double pos)
{
   E_Zone *zone = data;
   double v;
   
   // FIXME: if zone is deleted while anim going... bad things.
   pos = ecore_animator_pos_map(pos, ECORE_POS_MAP_DECELERATE, 0.0, 0.0);
   v = (bl_animval * (1.0 - pos)) + (bl_val *pos);
   _e_backlight_set(zone, v);
   if (pos >= 1.0)
     {
        bl_anim = NULL;
        return EINA_FALSE;
     }
   return EINA_TRUE;
}
