#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>

#ifdef __FreeBSD__
# include <sys/ioctl.h>
# include <sys/sysctl.h>
# ifdef __i386__
#  include <machine/apm_bios.h>
# endif
#endif
#ifdef HAVE_CFBASE_H
# include <CFBase.h>
# include <CFNumber.h>
# include <CFArray.h>
# include <CFDictionary.h>
# include <CFRunLoop.h>
# include <ps/IOPSKeys.h>
# include <ps/IOPowerSources.h>
#endif

#include <Ecore.h>
#include <Ecore_File.h>
#include <Ecore_Con.h>

/* define MAX for all OS, instead of plenty #include */
#ifndef MAX
# define MAX(a, b) ((a) < (b) ? (b) : (a))
#endif

/* supported batery system schemes - irrespective of OS */
#define CHECK_NONE                   0
#define CHECK_ACPI                   1
#define CHECK_APM                    2
#define CHECK_PMU                    3
#define CHECK_SYS_CLASS_POWER_SUPPLY 4

#define SYS_PWR

static void      init(void);
static Eina_Bool poll_cb(void *data);

static int poll_interval = 512;
static Ecore_Poller *poller = NULL;

static int mode = CHECK_NONE;

static int time_left = -2;
static int battery_full = -2;
static int have_battery = -2;
static int have_power = -2;

static const char *sys_power_dir = "/sys/class/power_supply";

/* UTILS */
static int
int_file_get(const char *file)
{
   int val = -1;
   FILE *f = fopen(file, "r");
   if (f)
     {
        char buf[256];
        char *str = fgets(buf, sizeof(buf), f);
        if (str) val = atoi(str);
        fclose(f);
     }
   return val;
}

static char *
str_file_get(const char *file)
{
   char *val = NULL;
   FILE *f = fopen(file, "r");
   if (f)
     {
        char buf[PATH_MAX];
        char *str = fgets(buf, sizeof(buf), f);
        if (str)
          {
             size_t len = strlen(str);
             if ((len > 0) && (str[len - 1] == '\n'))
               {
                  len--;
                  str[len] = 0;
               }
             val = malloc(len + 1);
             if (val) memcpy(val, str, len + 1);
          }
        fclose(f);
     }
   return val;
}

static int
int_get(const char *buf)
{
   const char *p = strchr(buf, ':');
   if (!p) return 0;
   p++;
   while (*p == ' ') p++;
   return atoi(p);
}

static char *
str_get(const char *buf)
{
   const char *p = strchr(buf, ':');
   const char *q;
   char *ret;

   if (!p) return NULL;
   p++;
   while (*p == ' ') p++;

   q = p + strlen(p) - 1;
   while ((q > p) && ((*q == ' ') || (*q == '\n'))) q--;

   if (q < p) return NULL;
   q++;
   ret = malloc(q - p + 1);
   if (!ret) return NULL;
   memcpy(ret, p, q - p);
   ret[q - p] = '\0';
   return ret;
}

static char *
file_str_entry_get(FILE       *f,
                   const char *entry)
{
   char buf[4096];
   char *tmp;

   tmp = fgets(buf, sizeof(buf), f);
   if (!tmp)
     {
        EINA_LOG_ERR("unexpected end of file, expected: '%s'", entry);
        return NULL;
     }
   if (strcmp(tmp, entry) != 0)
     {
        EINA_LOG_ERR("unexpected file entry, expected: '%s'", entry);
        return NULL;
     }
   tmp = str_get(tmp);
   if (!tmp)
     {
        EINA_LOG_ERR("unexpected file entry, missing value for '%s'", entry);
        return NULL;
     }
   return tmp;
}

#ifdef __FreeBSD__

#define BATTERY_STATE_NONE        0
#define BATTERY_STATE_DISCHARGING 1
#define BATTERY_STATE_CHARGING    2
#define BATTERY_STATE_REMOVED     7

/***---***/
static void
bsd_acpi_init(void)
{
   /* nothing to do */
}

static void
bsd_acpi_check(void)
{
   int bat_val = 0;
   int mib_state[4];
   int mib_life[4];
   int mib_time[4];
   int mib_units[4];
   size_t len;
   int state = 0;
   int level = 0;
   int time_min = 0;
   int life = 0;
   int batteries = 0;

   time_left = -1;
   battery_full = -1;
   have_battery = 0;
   have_power = 0;

   /* Read some information on first run. */
   len = 4;
   sysctlnametomib("hw.acpi.battery.state", mib_state, &len);
   len = sizeof(state);
   if (sysctl(mib_state, 4, &state, &len, NULL, 0) == -1)
     /* ERROR */
     state = -1;

   len = 4;
   sysctlnametomib("hw.acpi.battery.life", mib_life, &len);
   len = sizeof(life);
   if (sysctl(mib_life, 4, &life, &len, NULL, 0) == -1)
     /* ERROR */
     level = -1;
   bat_val = life;

   len = 4;
   sysctlnametomib("hw.acpi.battery.time", mib_time, &len);
   len = sizeof(time);
   if (sysctl(mib_time, 4, &time_min, &len, NULL, 0) == -1)
     /* ERROR */
     time_min = -1;

   len = 4;
   sysctlnametomib("hw.acpi.battery.units", mib_units, &len);
   len = sizeof(batteries);
   if (sysctl(mib_time, 4, &batteries, &len, NULL, 0) == -1)
     /* ERROR */
     batteries = 1;

   if (time_min >= 0) time_left = time_min * 60;

   if (batteries == 1) /* hw.acpi.battery.units = 1 means NO BATTS */
     time_left = -1;
   else if ((state == BATTERY_STATE_CHARGING) ||
            (state == BATTERY_STATE_DISCHARGING))
     {
        have_battery = 1;
        if (state == BATTERY_STATE_CHARGING) have_power = 1;
        else if (state == BATTERY_STATE_DISCHARGING)
          have_power = 0;
        if (level == -1) time_left = -1;
        else if (time_min == -1)
          {
             time_left = -1;
             battery_full = bat_val;
          }
        else battery_full = bat_val;
     }
   else
     {
        have_battery = 1;
        battery_full = 100;
        time_left = -1;
        have_power = 1;
     }
}

/***---***/
# ifdef __i386__
static void
bsd_apm_init(void)
{
   /* nothing to do */
}

static void
bsd_apm_check(void)
{
   int ac_stat, bat_stat, bat_val, time_val;
   char buf[4096];
   int hours, minutes;
   int apm_fd = -1;
   struct apm_info info;

   time_left = -1;
   battery_full = -1;
   have_battery = 0;
   have_power = 0;

   apm_fd = open("/dev/apm", O_RDONLY);
   if ((apm_fd != -1) && (ioctl(apm_fd, APMIO_GETINFO, &info) != -1))
     {
        /* set values */
         ac_stat = info.ai_acline;
         bat_stat = info.ai_batt_stat;
         bat_val = info.ai_batt_life;
         time_val = info.ai_batt_time;
     }
   else return;

   if (info.ai_batteries == 1) /* ai_batteries == 1 means NO battery,
                               * ai_batteries == 2 means 1 battery */
     {
        have_power = 1;
        return;
     }

   if (ac_stat) /* Wallpowered */
     {
        have_power = 1;
        have_battery = 1;
        switch (bat_stat) /* On FreeBSD the time_val is -1 when AC ist plugged
                           * in. This means we don't know how long the battery
                           * will recharge */
          {
           case 0:
             battery_full = 100;
             break;

           case 1:
             battery_full = 50;
             break;

           case 2:
             battery_full = 25;
             break;

           case 3:
             battery_full = 100;
             break;
          }
     }
   else /* Running on battery */
     {
        have_battery = 1;
        battery_full = bat_val;
        time_left = time_val;
     }
}

# endif

#elif defined(HAVE_CFBASE_H) /* OS X */
/***---***/
static void darwin_init(void);
static void darwin_check(void);

static void
darwin_init(void)
{
   /* nothing to do */
}

static void
darwin_check(void)
{
   const void *values;
   int device_num, device_count;
   int currentval = 0, maxval = 0;
   char buf[4096];
   CFTypeRef blob;
   CFArrayRef sources;
   CFDictionaryRef device_dict;

   time_left = -1;
   battery_full = -1;
   have_battery = 0;
   have_power = 0;

   /* Retrieve the power source data and the array of sources. */
   blob = IOPSCopyPowerSourcesInfo();
   sources = IOPSCopyPowerSourcesList(blob);
   device_count = CFArrayGetCount(sources);
   for (device_num = 0; device_num < device_count; device_num++)
     {
        CFTypeRef ps;

        /* Retrieve a dictionary of values for this device and the count of keys in the dictionary. */
        ps = CFArrayGetValueAtIndex(sources, device_num);
        device_dict = IOPSGetPowerSourceDescription(blob, ps);
        /* Retrieve the charging key and save the present charging value if one exists. */
        if (CFDictionaryGetValueIfPresent(device_dict,
                                          CFSTR(kIOPSIsChargingKey), &values))
          {
             have_battery = 1;
             if (CFBooleanGetValue(values) > 0) have_power = 1;
             break;
          }
     }

   if (!have_battery)
     {
        CFRelease(sources);
        CFRelease(blob);
        have_power = 1;
        return;
     }

   /* Retrieve the current capacity key. */
   values = CFDictionaryGetValue(device_dict, CFSTR(kIOPSCurrentCapacityKey));
   CFNumberGetValue(values, kCFNumberSInt32Type, &currentval);
   /* Retrieve the max capacity key. */
   values = CFDictionaryGetValue(device_dict, CFSTR(kIOPSMaxCapacityKey));
   CFNumberGetValue(values, kCFNumberSInt32Type, &maxval);
   /* Calculate the percentage charged. */
   battery_full = (currentval * 100) / maxval;

   /* Retrieve the remaining battery power or time until charged in minutes. */
   if (!have_power)
     {
        values = CFDictionaryGetValue(device_dict, CFSTR(kIOPSTimeToEmptyKey));
        CFNumberGetValue(values, kCFNumberSInt32Type, &currentval);
        time_left = currentval * 60;
     }
   else
     {
        values = CFDictionaryGetValue(device_dict, CFSTR(kIOPSTimeToFullChargeKey));
        CFNumberGetValue(values, kCFNumberSInt32Type, &currentval);
        time_left = currentval * 60;
     }
   CFRelease(sources);
   CFRelease(blob);
}

#else

/***---***/
/* new linux power class api to get power info - brand new and this code
 * may have bugs, but it is a good attempt to get it right */
#if 0
static Eina_Bool linux_sys_class_power_supply_cb_event_fd_active(void             *data,
                                                                 Ecore_Fd_Handler *fd_handler);
static void      linux_sys_class_power_supply_check(void);
#endif
static void      linux_sys_class_power_supply_init(void);

typedef struct _Sys_Class_Power_Supply_Uevent Sys_Class_Power_Supply_Uevent;

#define BASIS_CHARGE  1
#define BASIS_ENERGY  2
#define BASIS_VOLTAGE 3

struct _Sys_Class_Power_Supply_Uevent
{
   char             *name;
   int               fd;
   Ecore_Fd_Handler *fd_handler;

   int               present;

   int               basis;
   int               basis_empty;
   int               basis_full;

   unsigned char     have_current_avg : 1;
   unsigned char     have_current_now : 1;
};

static Eina_List *events = NULL;

#if 0
static Ecore_Timer *sys_class_delay_check = NULL;

static Eina_Bool
linux_sys_class_power_supply_cb_delay_check(void *data)
{
   linux_sys_class_power_supply_init();
   poll_cb(NULL);
   sys_class_delay_check = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static Ecore_Timer *re_init_timer = NULL;

static Eina_Bool
linux_sys_class_power_supply_cb_re_init(void *data)
{
   Sys_Class_Power_Supply_Uevent *sysev;

   if (events)
     {
        EINA_LIST_FREE(events, sysev)
          {
//	     if (sysev->fd_handler)
//	       ecore_main_fd_handler_del(sysev->fd_handler);
//	     if (sysev->fd >= 0) close(sysev->fd);
                   free(sysev->name);
                   free(sysev);
          }
     }
   linux_sys_class_power_supply_init();
   re_init_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
linux_sys_class_power_supply_cb_event_fd_active(void             *data,
                                                Ecore_Fd_Handler *fd_handler)
{
   Sys_Class_Power_Supply_Uevent *sysev;

   sysev = data;
   if (ecore_main_fd_handler_active_get(fd_handler, ECORE_FD_READ))
     {
        int lost = 0;
        for (;; )
          {
             char buf[1024];
             int num;

             if ((num = read(sysev->fd, buf, sizeof(buf))) < 1)
               {
                  lost = ((errno == EIO) ||
                          (errno == EBADF) ||
                          (errno == EPIPE) ||
                          (errno == EINVAL) ||
                          (errno == ENOSPC) ||
                          (errno == ENODEV));
                  if (num <= 0) break;
               }
          }
        if (lost)
          {
             events = eina_list_remove(events, sysev);

//	     if (sysev->fd_handler)
//	       ecore_main_fd_handler_del(sysev->fd_handler);
//	     if (sysev->fd >= 0) close(sysev->fd);
             free(sysev->name);
             free(sysev);

             if (re_init_timer) ecore_timer_del(re_init_timer);
             re_init_timer = ecore_timer_add(1.0, linux_sys_class_power_supply_cb_re_init, NULL);
          }
        else
          {
             if (sys_class_delay_check) ecore_timer_del(sys_class_delay_check);
             sys_class_delay_check = ecore_timer_add(0.2, linux_sys_class_power_supply_cb_delay_check, NULL);
          }
     }
   return ECORE_CALLBACK_CANCEL;
}

#endif
static void
linux_sys_class_power_supply_sysev_init(Sys_Class_Power_Supply_Uevent *sysev)
{
   char buf[4096];
   const char *dir = sys_power_dir;

   sysev->basis = 0;
   sysev->have_current_avg = 0;
   sysev->have_current_now = 0;

   snprintf(buf, sizeof(buf), "%s/%s/present", dir, sysev->name);
   sysev->present = int_file_get(buf);
   if (!sysev->present) return;

   snprintf(buf, sizeof(buf), "%s/%s/current_avg", dir, sysev->name);
   if (ecore_file_exists(buf)) sysev->have_current_avg = 1;
   snprintf(buf, sizeof(buf), "%s/%s/current_now", dir, sysev->name);
   if (ecore_file_exists(buf)) sysev->have_current_now = 1;

   snprintf(buf, sizeof(buf), "%s/%s/voltage_max", dir, sysev->name);
   if (ecore_file_exists(buf)) sysev->basis = BASIS_VOLTAGE;
   snprintf(buf, sizeof(buf), "%s/%s/voltage_max_design", dir, sysev->name);
   if (ecore_file_exists(buf)) sysev->basis = BASIS_VOLTAGE;

   snprintf(buf, sizeof(buf), "%s/%s/energy_full", dir, sysev->name);
   if (ecore_file_exists(buf)) sysev->basis = BASIS_ENERGY;
   snprintf(buf, sizeof(buf), "%s/%s/energy_full_design", dir, sysev->name);
   if (ecore_file_exists(buf)) sysev->basis = BASIS_ENERGY;

   snprintf(buf, sizeof(buf), "%s/%s/charge_full", dir, sysev->name);
   if (ecore_file_exists(buf)) sysev->basis = BASIS_CHARGE;
   snprintf(buf, sizeof(buf), "%s/%s/charge_full_design", dir, sysev->name);
   if (ecore_file_exists(buf)) sysev->basis = BASIS_CHARGE;

   if (sysev->basis == BASIS_CHARGE)
     {
        snprintf(buf, sizeof(buf), "%s/%s/charge_full", dir, sysev->name);
        sysev->basis_full = int_file_get(buf);
        snprintf(buf, sizeof(buf), "%s/%s/charge_empty", dir, sysev->name);
        sysev->basis_empty = int_file_get(buf);
        if (sysev->basis_full < 0)
          {
             snprintf(buf, sizeof(buf), "%s/%s/charge_full_design", dir, sysev->name);
             sysev->basis_full = int_file_get(buf);
          }
        if (sysev->basis_empty < 0)
          {
             snprintf(buf, sizeof(buf), "%s/%s/charge_empty_design", dir, sysev->name);
             sysev->basis_empty = int_file_get(buf);
          }
     }
   else if (sysev->basis == BASIS_ENERGY)
     {
        snprintf(buf, sizeof(buf), "%s/%s/energy_full", dir, sysev->name);
        sysev->basis_full = int_file_get(buf);
        snprintf(buf, sizeof(buf), "%s/%s/energy_empty", dir, sysev->name);
        sysev->basis_empty = int_file_get(buf);
        if (sysev->basis_full < 0)
          {
             snprintf(buf, sizeof(buf), "%s/%s/energy_full_design", dir, sysev->name);
             sysev->basis_full = int_file_get(buf);
          }
        if (sysev->basis_empty < 0)
          {
             snprintf(buf, sizeof(buf), "%s/%s/energy_empty_design", dir, sysev->name);
             sysev->basis_empty = int_file_get(buf);
          }
     }
   else if (sysev->basis == BASIS_VOLTAGE)
     {
        snprintf(buf, sizeof(buf), "%s/%s/voltage_max", dir, sysev->name);
        sysev->basis_full = int_file_get(buf);
        snprintf(buf, sizeof(buf), "%s/%s/voltage_min", dir, sysev->name);
        sysev->basis_empty = int_file_get(buf);
        if (sysev->basis_full < 0)
          {
             snprintf(buf, sizeof(buf), "%s/%s/voltage_max_design", dir, sysev->name);
             sysev->basis_full = int_file_get(buf);
          }
        if (sysev->basis_empty < 0)
          {
             snprintf(buf, sizeof(buf), "%s/%s/voltage_min_design", dir, sysev->name);
             sysev->basis_empty = int_file_get(buf);
          }
     }
}

static int
linux_sys_class_power_supply_is_battery(char *name)
{
   int fd;
   int ret = 0;
   char buf[256];
   const char *dir = sys_power_dir;

   snprintf(buf, sizeof(buf), "%s/%s/type", dir, name);
   fd = open(buf, O_RDONLY);
   if (fd < 0)
     {
        ret = 0;
        goto NO_OPEN;
     }
   else if (read(fd, buf, sizeof(buf)) < 1)
     ret = 0;
   else if (!strncmp(buf, "Battery", 7))
     ret = 1;

   close(fd);

NO_OPEN:
   return ret;
}

static void
linux_sys_class_power_supply_init(void)
{
   Eina_List *l;

   if (events)
     {
        Sys_Class_Power_Supply_Uevent *sysev;

        EINA_LIST_FOREACH(events, l, sysev)
          linux_sys_class_power_supply_sysev_init(sysev);
     }
   else
     {
        Eina_List *bats;
        char *name;
//	char buf[4096];

        bats = ecore_file_ls("/sys/class/power_supply/");
//        bats = ecore_file_ls("./TST");
        if (bats)
          {
             events = NULL;

             EINA_LIST_FREE(bats, name)
               {
                  Sys_Class_Power_Supply_Uevent *sysev;

                  if (!(linux_sys_class_power_supply_is_battery(name)))
                    {
                       free(name);
                       continue;
                    }

                  sysev = (Sys_Class_Power_Supply_Uevent *)calloc(1, sizeof(Sys_Class_Power_Supply_Uevent));
                  sysev->name = name;
//		  snprintf(buf, sizeof(buf), "/sys/class/power_supply/%s/uevent", name);
//		  sysev->fd = open(buf, O_RDONLY);
//		  if (sysev->fd >= 0)
//		    sysev->fd_handler = ecore_main_fd_handler_add(sysev->fd,
//								  ECORE_FD_READ,
//								  linux_sys_class_power_supply_cb_event_fd_active,
//								  sysev,
//								  NULL, NULL);
                  events = eina_list_append(events, sysev);
                  linux_sys_class_power_supply_sysev_init(sysev);
               }
          }
     }
}

static void
linux_sys_class_power_supply_check(void)
{
   Eina_List *l;
   char *name;
   char buf[4096];
   const char *dir = sys_power_dir;

   battery_full = -1;
   time_left = -1;
   have_battery = 0;
   have_power = 0;

   if (events)
     {
        Sys_Class_Power_Supply_Uevent *sysev;
        int total_pwr_now;
        int total_pwr_max;
        int nofull = 0;

        total_pwr_now = 0;
        total_pwr_max = 0;
        time_left = 0;
        EINA_LIST_FOREACH(events, l, sysev)
          {
             char *tmp;
             int present = 0;
             int charging = -1;
             int capacity = -1;
             int current = -1;
             int time_to_full = -1;
             int time_to_empty = -1;
             int full = -1;
             int pwr_now = -1;
             int pwr_empty = -1;
             int pwr_full = -1;
             int pwr = 0;

             name = sysev->name;

             /* fetch more generic info */
             // init
             present = sysev->present;
             if (!present) continue;

             snprintf(buf, sizeof(buf), "%s/%s/capacity", dir, name);
             capacity = int_file_get(buf);
             if (sysev->have_current_avg)
               {
                  snprintf(buf, sizeof(buf), "%s/%s/current_avg", dir, name);
                  current = int_file_get(buf);
               }
             else if (sysev->have_current_now)
               {
                  snprintf(buf, sizeof(buf), "%s/%s/current_now", dir, name);
                  current = int_file_get(buf);
               }

             /* FIXME: do we get a uevent on going from charging to full?
              * if so, move this to init */
             snprintf(buf, sizeof(buf), "%s/%s/status", dir, name);
             tmp = str_file_get(buf);
             if (tmp)
               {
                  full = 0;
                  if (!strncasecmp("discharging", tmp, 11)) charging = 0;
                  else if (!strncasecmp("unknown", tmp, 7))
                    charging = 0;
                  else if (!strncasecmp("not charging", tmp, 12))
                    charging = 0;
                  else if (!strncasecmp("charging", tmp, 8))
                    charging = 1;
                  else if (!strncasecmp("full", tmp, 4))
                    {
                       full = 1;
                       charging = 0;
                    }
                  free(tmp);
               }
             /* some batteries can/will/want to predict how long they will
              * last. if so - take what the battery says. too bad if it's
              * wrong. that's a buggy battery or driver */
             if (!full)
               {
                  nofull++;
                  if (charging)
                    {
                       snprintf(buf, sizeof(buf), "%s/%s/time_to_full_now", dir, name);
                       time_to_full = int_file_get(buf);
                    }
                  else
                    {
                       snprintf(buf, sizeof(buf), "%s/%s/time_to_empty_now", dir, name);
                       time_to_empty = int_file_get(buf);
                    }
               }

             /* now get charge, energy and voltage. take the one that provides
              * the best info (charge first, then energy, then voltage */
             if (sysev->basis == BASIS_CHARGE)
               snprintf(buf, sizeof(buf), "%s/%s/charge_now", dir, name);
             else if (sysev->basis == BASIS_ENERGY)
               snprintf(buf, sizeof(buf), "%s/%s/energy_now", dir, name);
             else if (sysev->basis == BASIS_VOLTAGE)
                snprintf(buf, sizeof(buf), "%s/%s/voltage_now", dir, name);
             pwr_now = int_file_get(buf);
             pwr_empty = sysev->basis_empty;
             pwr_full = sysev->basis_full;
             if ((sysev->basis == BASIS_VOLTAGE) &&
                 (capacity >= 0))
               {
                  /* if we use voltage as basis.. we're not very accurate
                   * so we should prefer capacity readings */
                  pwr_empty = -1;
                  pwr_full = -1;
                  pwr_now = -1;
               }

             if (pwr_empty < 0) pwr_empty = 0;

             if ((pwr_full > 0) && (pwr_full > pwr_empty))
               {
                  if (full) pwr_now = pwr_full;
                  else
                    {
                       if (pwr_now < 0)
                          pwr_now = (((long long)capacity * ((long long)pwr_full - (long long)pwr_empty)) / 100) + pwr_empty;
                    }
                  
                  if (sysev->present) have_battery = 1;
                  if (charging)
                    {
                       pwr_now = pwr_now;
                       have_power = 1;
                       if (time_to_full >= 0)
                         {
                            if (time_to_full > time_left)
                               time_left = time_to_full;
                         }
                       else
                         {
                            if (current == 0) time_left = 0;
                            else if (current < 0)
                               time_left = -1;
                            else
                              {
                                 pwr = (((long long)pwr_full - (long long)pwr_now) * 3600) / -current;
                                 if (pwr > time_left) time_left = pwr;
                              }
                         }
                    }
                  else
                    {
                       have_power = 0;
                       if (time_to_empty >= 0) time_left += time_to_empty;
                       else
                         {
                            if (time_to_empty < 0)
                              {
                                 if (current > 0)
                                   {
                                      pwr = (((long long)pwr_now - (long long)pwr_empty) * 3600) / current;
                                      time_left += pwr;
                                   }
                              }
                         }
                    }
                  total_pwr_now += pwr_now - pwr_empty;
                  total_pwr_max += pwr_full - pwr_empty;
               }
             /* simple current battery fallback */
             else
               {
                  if (sysev->present) have_battery = 1;
                  if (charging) have_power = 1;
                  total_pwr_max = 100;
                  total_pwr_now = capacity;
                  if (total_pwr_now < 100) nofull = 1;
               }
          }
        if (total_pwr_max > 0)
          battery_full = ((long long)total_pwr_now * 100) / total_pwr_max;
        if (nofull == 0)
          time_left = -1;
     }
}

/***---***/
/* "here and now" ACPI based power checking. is there for linux and most
 * modern laptops. as of linux 2.6.24 it is replaced with
 * linux_sys_class_power_supply_init/check() though as this is the new
 * power class api to poll for power stuff
 */
static Eina_Bool linux_acpi_cb_acpid_add(void *data,
                                         int   type,
                                         void *event);
static Eina_Bool linux_acpi_cb_acpid_del(void *data,
                                         int   type,
                                         void *event);
static Eina_Bool linux_acpi_cb_acpid_data(void *data,
                                          int   type,
                                          void *event);
static void linux_acpi_init(void);
static void linux_acpi_check(void);

static int acpi_max_full = -1;
static int acpi_max_design = -1;
static Ecore_Con_Server *acpid = NULL;
static Ecore_Event_Handler *acpid_handler_add = NULL;
static Ecore_Event_Handler *acpid_handler_del = NULL;
static Ecore_Event_Handler *acpid_handler_data = NULL;
static Ecore_Timer *delay_check = NULL;
static int event_fd = -1;
static Ecore_Fd_Handler *event_fd_handler = NULL;

static Eina_Bool
linux_acpi_cb_delay_check(void *data __UNUSED__)
{
   linux_acpi_init();
   poll_cb(NULL);
   delay_check = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
linux_acpi_cb_acpid_add(void *data  __UNUSED__,
                        int type    __UNUSED__,
                        void *event __UNUSED__)
{
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
linux_acpi_cb_acpid_del(void *data  __UNUSED__,
                        int type    __UNUSED__,
                        void *event __UNUSED__)
{
   ecore_con_server_del(acpid);
   acpid = NULL;
   if (acpid_handler_add) ecore_event_handler_del(acpid_handler_add);
   acpid_handler_add = NULL;
   if (acpid_handler_del) ecore_event_handler_del(acpid_handler_del);
   acpid_handler_del = NULL;
   if (acpid_handler_data) ecore_event_handler_del(acpid_handler_data);
   acpid_handler_data = NULL;
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
linux_acpi_cb_acpid_data(void *data  __UNUSED__,
                         int type    __UNUSED__,
                         void *event __UNUSED__)
{
   if (delay_check) ecore_timer_del(delay_check);
   delay_check = ecore_timer_add(0.2, linux_acpi_cb_delay_check, NULL);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
linux_acpi_cb_event_fd_active(void *data        __UNUSED__,
                              Ecore_Fd_Handler *fd_handler)
{
   if (ecore_main_fd_handler_active_get(fd_handler, ECORE_FD_READ))
     {
        int lost = 0;
        for (;; )
          {
             char buf[1024];
             int num;

             if ((num = read(event_fd, buf, sizeof(buf))) < 1)
               {
                  lost = ((errno == EIO) ||
                          (errno == EBADF) ||
                          (errno == EPIPE) ||
                          (errno == EINVAL) ||
                          (errno == ENOSPC));
                  if (num == 0) break;
               }
          }
        if (lost)
          {
             ecore_main_fd_handler_del(event_fd_handler);
             event_fd_handler = NULL;
             close(event_fd);
             event_fd = -1;
          }
        else
          {
             if (delay_check) ecore_timer_del(delay_check);
             delay_check = ecore_timer_add(0.2, linux_acpi_cb_delay_check, NULL);
          }
     }
   return ECORE_CALLBACK_RENEW;
}

static void
linux_acpi_init(void)
{
   Eina_List *powers;
   Eina_List *bats;

   bats = ecore_file_ls("/proc/acpi/battery");
   if (bats)
     {
        char *name;

        have_power = 0;
        powers = ecore_file_ls("/proc/acpi/ac_adapter");
        if (powers)
          {
             char *name;

             EINA_LIST_FREE(powers, name)
               {
                  char buf[4096];
                  FILE *f;

                  snprintf(buf, sizeof(buf), "/proc/acpi/ac_adapter/%s/state", name);
                  f = fopen(buf, "r");
                  if (f)
                    {
                       char *tmp;

     /* state */
                       tmp = fgets(buf, sizeof(buf), f);
                       if (tmp) tmp = str_get(tmp);
                       if (tmp)
                         {
                            if (!strcmp(tmp, "on-line")) have_power = 1;
                            free(tmp);
                         }
                    }

                  free(name);
               }
          }

        have_battery = 0;
        acpi_max_full = 0;
        acpi_max_design = 0;
        EINA_LIST_FREE(bats, name)
          {
             char buf[4096];
             FILE *f;

             snprintf(buf, sizeof(buf), "/proc/acpi/battery/%s/info", name);
             f = fopen(buf, "r");
             if (f)
               {
                  char *tmp;

     /* present */
                  tmp = fgets(buf, sizeof(buf), f);
                  if (tmp) tmp = str_get(tmp);
                  if (tmp)
                    {
                       if (!strcmp(tmp, "yes")) have_battery = 1;
                       free(tmp);
                    }
     /* design cap */
                  tmp = fgets(buf, sizeof(buf), f);
                  if (tmp) tmp = str_get(tmp);
                  if (tmp)
                    {
                       if (strcmp(tmp, "unknown")) acpi_max_design += atoi(tmp);
                       free(tmp);
                    }
     /* last full cap */
                  tmp = fgets(buf, sizeof(buf), f);
                  if (tmp) tmp = str_get(tmp);
                  if (tmp)
                    {
                       if (strcmp(tmp, "unknown")) acpi_max_full += atoi(tmp);
                       free(tmp);
                    }
                  fclose(f);
               }

             free(name);
          }
     }
   if (!acpid)
     {
        acpid = ecore_con_server_connect(ECORE_CON_LOCAL_SYSTEM,
                                         "/var/run/acpid.socket", -1, NULL);
        if (acpid)
          {
             acpid_handler_add = ecore_event_handler_add(ECORE_CON_EVENT_SERVER_ADD,
                                                         linux_acpi_cb_acpid_add, NULL);
             acpid_handler_del = ecore_event_handler_add(ECORE_CON_EVENT_SERVER_DEL,
                                                         linux_acpi_cb_acpid_del, NULL);
             acpid_handler_data = ecore_event_handler_add(ECORE_CON_EVENT_SERVER_DATA,
                                                          linux_acpi_cb_acpid_data, NULL);
          }
        else
          {
             if (event_fd < 0)
               {
                  event_fd = open("/proc/acpi/event", O_RDONLY);
                  if (event_fd >= 0)
                    event_fd_handler = ecore_main_fd_handler_add(event_fd,
                                                                 ECORE_FD_READ,
                                                                 linux_acpi_cb_event_fd_active,
                                                                 NULL,
                                                                 NULL, NULL);
               }
          }
     }
}

static void
linux_acpi_check(void)
{
   Eina_List *bats;

   battery_full = -1;
   time_left = -1;
   have_battery = 0;
   have_power = 0;

   bats = ecore_file_ls("/proc/acpi/battery");
   if (bats)
     {
        char *name;
        int rate = 0;
        int capacity = 0;

        EINA_LIST_FREE(bats, name)
          {
             char buf[4096];
             char *tmp;
             FILE *f;

             snprintf(buf, sizeof(buf), "/proc/acpi/battery/%s/state", name);
             free(name);
             f = fopen(buf, "r");
             if (!f) continue;

             tmp = file_str_entry_get(f, "present:");
             if (!tmp) goto fclose_and_continue;
             if (!strcasecmp(tmp, "yes")) have_battery = 1;
             free(tmp);

             tmp = file_str_entry_get(f, "capacity state:");
             if (!tmp) goto fclose_and_continue;
             free(tmp);

             tmp = file_str_entry_get(f, "charging state:");
             if (!tmp) goto fclose_and_continue;
             if ((have_power == 0) && (!strcasecmp(tmp, "charging")))
               have_power = 1;
             free(tmp);

             tmp = file_str_entry_get(f, "present rate:");
             if (!tmp) goto fclose_and_continue;
             if (strcasecmp(tmp, "unknown")) rate += atoi(tmp);
             free(tmp);

             tmp = file_str_entry_get(f, "remaining capacity:");
             if (!tmp) goto fclose_and_continue;
             if (strcasecmp(tmp, "unknown")) capacity += atoi(tmp);
             free(tmp);

fclose_and_continue:
             fclose(f);
          }

        if (acpi_max_full > 0)
          battery_full = 100 * (long long)capacity / acpi_max_full;
        else if (acpi_max_design > 0)
          battery_full = 100 * (long long)capacity / acpi_max_design;
        else
          battery_full = -1;
        if (rate <= 0) time_left = -1;
        else
          {
             if (have_power)
               time_left = (3600 * ((long long)acpi_max_full - (long long)capacity)) / rate;
             else
               time_left = (3600 * (long long)capacity) / rate;
          }
     }
}

/***---***/
/* old school apm support - very old laptops and some devices support this.
 * this is here for legacy support and i wouldn't suggest spending any
 * effort on it as it is complete below as best i know, but could have missed
 * one or 2 things, but not worth fixing */
static void linux_apm_init(void);
static void linux_apm_check(void);

static void
linux_apm_init(void)
{
   /* nothing to do */
}

static void
linux_apm_check(void)
{
   FILE *f;
   char s1[32], s2[32], s3[32], *endptr;
   int apm_flags, ac_stat, bat_stat, bat_flags, bat_val, time_val;

   battery_full = -1;
   time_left = -1;
   have_battery = 0;
   have_power = 0;

   f = fopen("/proc/apm", "r");
   if (!f) return;

   if (fscanf(f, "%*s %*s %x %x %x %x %31s %31s %31s",
              &apm_flags, &ac_stat, &bat_stat, &bat_flags, s1, s2, s3) != 7)
     {
        fclose(f);
        return;
     }
   fclose(f);

   bat_val = strtol(s1, &endptr, 10);
   if (*endptr != '%') return;

   if (!strcmp(s3, "sec")) time_val = atoi(s2);
   else if (!strcmp(s3, "min"))
     time_val = atoi(s2) * 60;
   else time_val = 0;

   if ((bat_flags != 0xff) && (bat_flags & 0x80))
     {
        have_battery = 0;
        have_power = 0;
        battery_full = 100;
        time_left = 0;
        return;
     }

   if (bat_val >= 0)
     {
        have_battery = 1;
        have_power = ac_stat;
        battery_full = bat_val;
        if (battery_full > 100) battery_full = 100;
        if (ac_stat == 1) time_left = -1;
        else time_left = time_val;
     }
   else
     {
        switch (bat_stat)
          {
           case 0: /* high */
             have_battery = 1;
             have_power = ac_stat;
             battery_full = 100;
             time_left = -1;
             break;

           case 1: /* medium */
             have_battery = 1;
             have_power = ac_stat;
             battery_full = 50;
             time_left = -1;
             break;

           case 2: /* low */
             have_battery = 1;
             have_power = ac_stat;
             battery_full = 25;
             time_left = -1;
             break;

           case 3: /* charging */
             have_battery = 1;
             have_power = ac_stat;
             battery_full = 100;
             time_left = -1;
             break;
          }
     }
}

/***---***/
/* for older mac powerbooks. legacy as well like linux_apm_init/check. leave
 * it alone unless you have to touch it */
static void linux_pmu_init(void);
static void linux_pmu_check(void);

static void
linux_pmu_init(void)
{
   /* nothing to do */
}

static void
linux_pmu_check(void)
{
   FILE *f;
   char buf[4096];
   Eina_List *bats;
   char *name;
   int ac = 0;
   int charge = 0;
   int max_charge = 0;
   int seconds = 0;
   int curcharge = 0;
   int curmax = 0;

   f = fopen("/proc/pmu/info", "r");
   if (f)
     {
        char *tmp;
        /* Skip driver */
        tmp = fgets(buf, sizeof(buf), f);
        if (!tmp)
          {
             EINA_LOG_ERR("no driver info in /proc/pmu/info");
             goto fclose_and_continue;
          }
        /* Skip firmware */
        tmp = fgets(buf, sizeof(buf), f);
        if (!tmp)
          {
             EINA_LOG_ERR("no firmware info in /proc/pmu/info");
             goto fclose_and_continue;
          }
        /* Read ac */
        tmp = fgets(buf, sizeof(buf), f);
        if (!tmp)
          {
             EINA_LOG_ERR("no AC info in /proc/pmu/info");
             goto fclose_and_continue;
          }
        ac = int_get(buf);
fclose_and_continue:
        fclose(f);
     }
   bats = ecore_file_ls("/proc/pmu");
   if (bats)
     {
        have_battery = 1;
        have_power = ac;
        EINA_LIST_FREE(bats, name)
          {
             if (strncmp(name, "battery", 7)) continue;
             snprintf(buf, sizeof(buf), "/proc/pmu/%s", name);
             f = fopen(buf, "r");
             if (f)
               {
                  int timeleft = 0;
                  int current = 0;

                  while (fgets(buf, sizeof (buf), f))
                    {
                       char *token;

                       if ((token = strtok(buf, ":")))
                         {
                            if (!strncmp("charge", token, 6))
                              charge = atoi(strtok(0, ": "));
                            else if (!strncmp("max_charge", token, 9))
                              max_charge = atoi(strtok(0, ": "));
                            else if (!strncmp("current", token, 7))
                              current = atoi(strtok(0, ": "));
                            else if (!strncmp("time rem", token, 8))
                              timeleft = atoi(strtok(0, ": "));
                            else
                              strtok(0, ": ");
                         }
                    }
                  curmax += max_charge;
                  curcharge += charge;
                  fclose(f);
                  if (!current)
                    {
     /* Neither charging nor discharging */
                    }
                  else if (!ac)
                    {
     /* When on dc, we are discharging */
                        seconds += timeleft;
                    }
                  else
                    {
     /* Charging - works in parallel */
                        seconds = MAX(timeleft, seconds);
                    }
               }

             free(name);
          }
        if (max_charge > 0) battery_full = ((long long)charge * 100) / max_charge;
        else battery_full = 0;
        time_left = seconds;
     }
   else
     {
        have_power = ac;
        have_battery = 0;
        battery_full = -1;
        time_left = -1;
     }
}

#endif

static int
dir_has_contents(const char *dir)
{
   Eina_List *bats;
   char *file;
   int count;

   bats = ecore_file_ls(dir);

   count = eina_list_count(bats);
   EINA_LIST_FREE(bats, file)
     free(file);
   if (count > 0) return 1;
   return 0;
}

static void
init(void)
{
#ifdef __FreeBSD__
   int acline;
   size_t len;

   len = sizeof(acline);
   if (!sysctlbyname("hw.acpi.acline", &acline, &len, NULL, 0))
     {
        int acline_mib[3] = {-1};

        len = 3;
        if (!sysctlnametomib("hw.acpi.acline", acline_mib, &len))
          {
             mode = CHECK_ACPI;
             bsd_acpi_init();
          }
     }
   else
     {
#ifdef __i386__
        if (ecore_file_exists("/dev/apm"))
          {
             mode = CHECK_APM;
             bsd_apm_init();
          }
#endif
     }
#elif defined(HAVE_CFBASE_H) /* OS X */
   darwin_init();
#else
   if ((ecore_file_is_dir(sys_power_dir)) && (dir_has_contents(sys_power_dir)))
     {
        mode = CHECK_SYS_CLASS_POWER_SUPPLY;
        linux_sys_class_power_supply_init();
     }
   else if (ecore_file_is_dir("/proc/acpi")) /* <= 2.6.24 */
     {
        mode = CHECK_ACPI;
        linux_acpi_init();
     }
   else if (ecore_file_exists("/proc/apm"))
     {
        mode = CHECK_APM;
        linux_apm_init();
     }
   else if (ecore_file_is_dir("/proc/pmu"))
     {
        mode = CHECK_PMU;
        linux_pmu_init();
     }
#endif
}

static Eina_Bool
poll_cb(void *data __UNUSED__)
{
   int ptime_left;
   int pbattery_full;
   int phave_battery;
   int phave_power;

   ptime_left = time_left;
   pbattery_full = battery_full;
   phave_battery = have_battery;
   phave_power = have_power;

#ifdef __FreeBSD__
   switch (mode)
     {
      case CHECK_ACPI:
        bsd_acpi_check();
        break;

#ifdef __i386__
      case CHECK_APM:
        bsd_apm_check();
        break;

#endif
      default:
        battery_full = -1;
        time_left = -1;
        have_battery = 0;
        have_power = 0;
        break;
     }
#elif defined(HAVE_CFBASE_H) /* OS X */
   darwin_check();
   return ECORE_CALLBACK_RENEW;
#else
   switch (mode)
     {
      case CHECK_ACPI:
        linux_acpi_check();
        break;

      case CHECK_APM:
        linux_apm_check();
        break;

      case CHECK_PMU:
        linux_pmu_check();
        break;

      case CHECK_SYS_CLASS_POWER_SUPPLY:
        linux_sys_class_power_supply_check();
        break;

      default:
        battery_full = -1;
        time_left = -1;
        have_battery = 0;
        have_power = 0;
        break;
     }
#endif
   if ((ptime_left != time_left) ||
       (pbattery_full != battery_full) ||
       (phave_battery != have_battery) ||
       (phave_power != have_power))
     {
        if ((time_left < 0) &&
            ((have_battery) && (battery_full < 0)))
          printf("ERROR\n");
        else
          printf("%i %i %i %i %i\n",
                 battery_full, time_left, time_left, have_battery, have_power);
        fflush(stdout);
     }
   return ECORE_CALLBACK_RENEW;
}

int
main(int   argc,
     char *argv[])
{
   if (argc != 2)
     {
        printf("ARGS INCORRECT!\n");
        return 0;
     }
   poll_interval = atoi(argv[1]);

   ecore_init();
   ecore_file_init();
   ecore_con_init();

   init();
   poller = ecore_poller_add(ECORE_POLLER_CORE, poll_interval, poll_cb, NULL);
   poll_cb(NULL);

   ecore_main_loop_begin();

   ecore_con_shutdown();
   ecore_file_shutdown();
   ecore_shutdown();

   return 0;
}

