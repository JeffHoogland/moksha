#include "e.h"

// FIXME: backlight should be tied per zone but this implementation is just
// a signleton right now as thats 99% of use cases. but api supports
// doing more. for now make it work in the singleton

#define MODE_NONE  -1
#define MODE_RANDR 0
#define MODE_SYS   1

static double bl_val = 1.0;
static double bl_animval = 1.0;
static E_Backlight_Mode bl_mode = E_BACKLIGHT_MODE_NORMAL;
static int sysmode = MODE_NONE;
static Ecore_Animator *bl_anim = NULL;
static const char *bl_sysvalmax = NULL;
static const char *bl_sysval = NULL;
static Ecore_Event_Handler *bl_sys_exit_handler = NULL;
static Ecore_Exe *bl_sys_set_exe = NULL;
static Eina_Bool bl_sys_pending_set = EINA_FALSE;
static Eina_Bool bl_sys_set_exe_ready = EINA_TRUE;

static void _e_backlight_update(E_Zone *zone);
static void _e_backlight_set(E_Zone *zone, double val);
static Eina_Bool _bl_anim(void *data, double pos);
static char *_bl_read_file(const char *file);
static int _bl_sys_num_get(const char *file);
static void _bl_sys_find(void);
static void _bl_sys_level_get(void);
static Eina_Bool _e_bl_cb_exit(void *data __UNUSED__, int type __UNUSED__, void *event);
static void _bl_sys_level_set(double val);

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
   if (bl_anim) ecore_animator_del(bl_anim);
   bl_anim = NULL;
   if (bl_sysval) eina_stringshare_del(bl_sysvalmax);
   bl_sysvalmax = NULL;
   if (bl_sysval) eina_stringshare_del(bl_sysval);
   bl_sysval = NULL;
   if (bl_sys_exit_handler) ecore_event_handler_del(bl_sys_exit_handler);
   bl_sys_exit_handler = NULL;
   bl_sys_set_exe = NULL;
   bl_sys_pending_set = EINA_FALSE;
   return 1;
}

EAPI Eina_Bool
e_backlight_exists(void)
{
   if (sysmode == MODE_NONE) return EINA_FALSE;
   return EINA_TRUE;
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
   if (val < 0.0) val = 0.0;
   else if (val > 1.0) val = 1.0;
   if (val == bl_val) return;
   if (!zone) zone = e_util_zone_current_get(e_manager_current_get());
   bl_now = bl_val;
   bl_val = val;
   if (bl_mode != E_BACKLIGHT_MODE_NORMAL) return;
   if (tim < 0.0) tim = e_config->backlight.transition;
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
e_backlight_level_get(E_Zone *zone __UNUSED__)
{
   // zone == NULL == everything
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
        _bl_sys_find();
        if (bl_sysval)
          {
             sysmode = MODE_SYS;
             _bl_sys_level_get();
          }
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
   else if (sysmode == MODE_SYS)
     {
        if (bl_sysval)
          {
             _bl_sys_level_set(val);
          }
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

static char *
_bl_read_file(const char *file)
{
   FILE *f = fopen(file, "r");
   size_t len;
   char buf[4096], *p;
   if (!f) return NULL;
   len = fread(buf, 1, sizeof(buf) - 1, f);
   if (len == 0)
     {
        fclose(f);
        return NULL;
     }
   buf[len] = 0;
   for (p = buf; *p; p++)
     {
        if (p[0] == '\n') p[0] = 0;
     }
   fclose(f);
   return strdup(buf);
}

static int
_bl_sys_num_get(const char *file)
{
   char *max;
   int maxval = -1;
   
   max = _bl_read_file(file);
   if (max)
     {
        maxval = atoi(max);
        free(max);
     }
   return maxval;
}

typedef struct _Bl_Entry
{
   char type;
   const char *base;
   const char *max;
   const char *set;
} Bl_Entry;

static const Bl_Entry search[] =
{
   { 'F', "/sys/devices/virtual/backlight/acpi_video0", "max_brightness", "brightness" },
   { 'D', "/sys/devices/virtual/backlight", "max_brightness", "brightness" },
   { 'F', "/sys/class/leds/lcd-backlight", "max_brightness", "brightness" },
   { 'F', "/sys/class/backlight/acpi_video0", "max_brightness", "brightness" },
   { 'D', "/sys/class/backlight", "max_brightness", "brightness" }
};

static void
_bl_sys_find(void)
{
   int i, curlevel = 0;
   char *valstr;
   char file[4096] = "";

   for (i = 0; i < (int)(sizeof(search) / sizeof(Bl_Entry)); i++)
     {
        char buf[4096];
        const Bl_Entry *b = &(search[i]);
        
        if (b->type == 'F')
          {
             snprintf(buf, sizeof(buf), "%s/%s", b->base, b->set);
             valstr = _bl_read_file(buf);
             if (valstr)
               {
                  curlevel = atoi(valstr);
                  if (curlevel < 0)
                    {
                       free(valstr);
                       valstr = NULL;
                    }
                  else
                    {
                       bl_sysval = eina_stringshare_add(buf);
                       snprintf(file, sizeof(file), "%s/%s", b->base, b->max);
                       bl_sysvalmax = eina_stringshare_add(file);
                       free(valstr);
                       valstr = NULL;
                    }
               }
          }
        else if (b->type == 'D')
          {
             DIR *dirp = opendir(b->base);
             struct dirent *dp;
             
             if (dirp)
               {
                  while ((dp = readdir(dirp)))
                    {
                       if ((strcmp(dp->d_name, ".")) &&
                           (strcmp(dp->d_name, "..")))
                         {
                            snprintf(buf, sizeof(buf), "%s/%s/%s",
                                     b->base, dp->d_name, b->set);
                            valstr = _bl_read_file(buf);
                            if (valstr)
                              {
                                 curlevel = atoi(valstr);
                                 if (curlevel < 0)
                                   {
                                      free(valstr);
                                      valstr = NULL;
                                   }
                                 else
                                   {
                                      bl_sysval = eina_stringshare_add(buf);
                                      snprintf(file, sizeof(file), "%s/%s/%s",
                                               b->base, dp->d_name, b->max);
                                      bl_sysvalmax = eina_stringshare_add(file);
                                      free(valstr);
                                      valstr = NULL;
                                      break;
                                   }
                              }
                         }
                    }
                  closedir(dirp);
               }
          }
        if (file[0]) break;
     }
}

static void
_bl_sys_level_get(void)
{
   int maxval, val;
   
   if (!bl_sysval) return;
   maxval = _bl_sys_num_get(bl_sysvalmax);
   if (maxval <= 0) maxval = 255;
   val = _bl_sys_num_get(bl_sysval);
   if ((val >= 0) && (val <= maxval))
      bl_val = (double)val / (double)maxval;
   printf("%s, %s\n", bl_sysvalmax, bl_sysval);
   printf("GET: %i/%i (%1.3f)\n", val, maxval, bl_val);
}

static Eina_Bool
_e_bl_cb_ext_delay(void *data __UNUSED__)
{
   bl_sys_set_exe_ready = EINA_TRUE;
   if (bl_sys_pending_set)
     {
        bl_sys_pending_set = EINA_FALSE;
        _bl_sys_level_set(bl_val);
     }
   return EINA_FALSE;
}

static Eina_Bool
_e_bl_cb_exit(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Exe_Event_Del *ev;
   
   ev = event;
   if (ev->exe == bl_sys_set_exe)
     {
        bl_sys_set_exe_ready = EINA_FALSE;
        bl_sys_set_exe = NULL;
        ecore_timer_add(0.1, _e_bl_cb_ext_delay, NULL);
     }
   return ECORE_CALLBACK_RENEW;
}

static void
_bl_sys_level_set(double val)
{
   char buf[PATH_MAX];
   
   if (!bl_sys_exit_handler)
      bl_sys_exit_handler = ecore_event_handler_add(ECORE_EXE_EVENT_DEL,
                                                    _e_bl_cb_exit, NULL);
   if ((bl_sys_set_exe) || (!bl_sys_set_exe_ready))
     {
        bl_sys_pending_set = EINA_TRUE;
        return;
     }
//   printf("SET: %1.3f\n", val);
   snprintf(buf, sizeof(buf), 
            "%s/enlightenment/utils/enlightenment_backlight %i", 
            e_prefix_lib_get(), (int)(val * 1000.0));
   bl_sys_set_exe = ecore_exe_run(buf, NULL);
}
