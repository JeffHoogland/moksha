#include "e.h"
#include "e_mod_main.h"

#if defined (__FreeBSD__) || defined(__DragonFly__)
# include <sys/types.h>
# include <sys/sysctl.h>
# include <errno.h>
#endif

#ifdef __OpenBSD__
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/sensors.h>
#include <errno.h>
#include <err.h>
#endif

typedef struct
{
#if defined (__FreeBSD__) || defined(__DragonFly__) || defined (__OpenBSD__)
   int mib[CTL_MAXNAME];
#endif
#if defined (__FreeBSD__) || defined(__DragonFly__)
   unsigned int miblen;
#endif
   int dummy;
} Extn;

#if defined (__FreeBSD__) || defined(__DragonFly__)
static const char *sources[] =
  {
     "hw.acpi.thermal.tz0.temperature",
     "dev.cpu.0.temperature",
     "dev.aibs.0.temp.0",
     "dev.lm75.0.temperature",
     NULL
  };
#endif

Eina_List *
temperature_get_bus_files(const char *bus)
{
   Eina_List *result;
   Eina_List *therms;
   char path[PATH_MAX + PATH_MAX + 2];
   char busdir[PATH_MAX];
   char *name;

   result = NULL;

   snprintf(busdir, sizeof(busdir), "/sys/bus/%s/devices", bus);
   /* Look through all the devices for the given bus. */
   therms = ecore_file_ls(busdir);

   EINA_LIST_FREE(therms, name)
     {
        Eina_List *files;
        char *file;

        /* Search each device for temp*_input, these should be
         * temperature devices. */
        snprintf(path, sizeof(path), "%s/%s", busdir, name);
        files = ecore_file_ls(path);
        EINA_LIST_FREE(files, file)
          {
             if ((!strncmp("temp", file, 4)) &&
                 (!strcmp("_input", &file[strlen(file) - 6])))
               {
                  char *f;

                  snprintf(path, sizeof(path),
                           "%s/%s/%s", busdir, name, file);
                  f = strdup(path);
                  if (f) result = eina_list_append(result, f);
               }
             free(file);
          }
        free(name);
     }
   return result;
}

#ifdef __OpenBSD__
static struct sensor snsr;
static size_t slen = sizeof(snsr);
#endif

static void
init(Tempthread *tth)
{
   Eina_List *therms;
   char path[512];
#ifdef __OpenBSD__
   int dev, numt;
   struct sensordev snsrdev;
   size_t sdlen = sizeof(snsrdev);
#endif
#if defined (__FreeBSD__) || defined(__DragonFly__)
   unsigned i;
   size_t len;
   int rc;
#endif
   Extn *extn;

   if (tth->initted) return;
   tth->initted = EINA_TRUE;

   extn = calloc(1, sizeof(Extn));
   tth->extn = extn;

   if ((!tth->sensor_type) ||
       ((!tth->sensor_name) ||
        (tth->sensor_name[0] == 0)))
     {
        eina_stringshare_del(tth->sensor_name);
        tth->sensor_name = NULL;
        eina_stringshare_del(tth->sensor_path);
        tth->sensor_path = NULL;
#if defined (__FreeBSD__) || defined(__DragonFly__)
        for (i = 0; sources[i]; i++)
          {
             rc = sysctlbyname(sources[i], NULL, NULL, NULL, 0);
             if (rc == 0)
               {
                  tth->sensor_type = SENSOR_TYPE_FREEBSD;
                  tth->sensor_name = eina_stringshare_add(sources[i]);
                  break;
               }
          }
#elif defined(__OpenBSD__)
        extn->mib[0] = CTL_HW;
        extn->mib[1] = HW_SENSORS;

        for (dev = 0;; dev++)
          {
             extn->mib[2] = dev;
             if (sysctl(extn->mib, 3, &snsrdev, &sdlen, NULL, 0) == -1)
               {
                  if (errno == ENOENT) /* no further sensors */
                    break;
                  else
                    continue;
               }
             if (strcmp(snsrdev.xname, "cpu0") == 0)
               {
                  tth->sensor_type = SENSOR_TYPE_OPENBSD;
                  tth->sensor_name = strdup("cpu0");
                  break;
               }
             else if (strcmp(snsrdev.xname, "km0") == 0)
               {
                  tth->sensor_type = SENSOR_TYPE_OPENBSD;
                  tth->sensor_name = strdup("km0");
                  break;
               }
          }
#else
        therms = ecore_file_ls("/proc/acpi/thermal_zone");
        if (therms)
          {
             char *name;

             name = eina_list_data_get(therms);
             tth->sensor_type = SENSOR_TYPE_LINUX_ACPI;
             tth->sensor_name = eina_stringshare_add(name);
             eina_list_free(therms);
          }
        else
          {
             eina_list_free(therms);
             therms = ecore_file_ls("/sys/class/thermal");
             if (therms)
               {
                  char *name;
                  Eina_List *l;

                  EINA_LIST_FOREACH(therms, l, name)
                    {
                       if (!strncmp(name, "thermal", 7))
                         {
                            tth->sensor_type = SENSOR_TYPE_LINUX_SYS;
                            tth->sensor_name = eina_stringshare_add(name);
                            eina_list_free(therms);
                            therms = NULL;
                            break;
                         }
                    }
                  if (therms) eina_list_free(therms);
               }
             if (therms)
               {
                  if (ecore_file_exists("/proc/omnibook/temperature"))
                    {
                       tth->sensor_type = SENSOR_TYPE_OMNIBOOK;
                       tth->sensor_name = eina_stringshare_add("dummy");
                    }
                  else if (ecore_file_exists("/sys/devices/temperatures/sensor1_temperature"))
                    {
                       tth->sensor_type = SENSOR_TYPE_LINUX_PBOOK;
                       tth->sensor_name = eina_stringshare_add("dummy");
                    }
                  else if (ecore_file_exists("/sys/devices/temperatures/cpu_temperature"))
                    {
                       tth->sensor_type = SENSOR_TYPE_LINUX_MACMINI;
                       tth->sensor_name = eina_stringshare_add("dummy");
                    }
                  else if (ecore_file_exists("/sys/devices/platform/coretemp.0/temp1_input"))
                    {
                       tth->sensor_type = SENSOR_TYPE_LINUX_INTELCORETEMP;
                       tth->sensor_name = eina_stringshare_add("dummy");
                    }
                  else if (ecore_file_exists("/sys/devices/platform/thinkpad_hwmon/temp1_input"))
                    {
                       tth->sensor_type = SENSOR_TYPE_LINUX_THINKPAD;
                       tth->sensor_name = eina_stringshare_add("dummy");
                    }
                  else
                    {
                       // try the i2c bus
                       therms = temperature_get_bus_files("i2c");
                       if (therms)
                         {
                            char *name;

                            if ((name = eina_list_data_get(therms)))
                              {
                                 if (ecore_file_exists(name))
                                   {
                                      int len;

                                      snprintf(path, sizeof(path),
                                               "%s", ecore_file_file_get(name));
                                      len = strlen(path);
                                      if (len > 6) path[len - 6] = '\0';
                                      tth->sensor_type = SENSOR_TYPE_LINUX_I2C;
                                      tth->sensor_path = eina_stringshare_add(name);
                                      tth->sensor_name = eina_stringshare_add(path);
                                   }
                              }
                            eina_list_free(therms);
                         }
                       if (!tth->sensor_path)
                         {
                            // try the pci bus
                            therms = temperature_get_bus_files("pci");
                            if (therms)
                              {
                                 char *name;

                                 if ((name = eina_list_data_get(therms)))
                                   {
                                      if (ecore_file_exists(name))
                                        {
                                           int len;

                                           snprintf(path, sizeof(path),
                                                    "%s", ecore_file_file_get(name));
                                           len = strlen(path);
                                           if (len > 6) path[len - 6] = '\0';
                                           tth->sensor_type = SENSOR_TYPE_LINUX_PCI;
                                           tth->sensor_path = eina_stringshare_add(name);
                                           eina_stringshare_del(tth->sensor_name);
                                           tth->sensor_name = eina_stringshare_add(path);
                                        }
                                   }
                                 eina_list_free(therms);
                              }
                         }
                    }
               }
          }
#endif
     }
   if ((tth->sensor_type) && (tth->sensor_name) && (!tth->sensor_path))
     {
        char *name;

        switch (tth->sensor_type)
          {
           case SENSOR_TYPE_NONE:
             break;

           case SENSOR_TYPE_FREEBSD:
#if defined (__FreeBSD__) || defined(__DragonFly__)
             len = sizeof(extn->mib) / sizeof(extn->mib[0]);
             rc = sysctlnametomib(tth->sensor_name, extn->mib, &len);
             if (rc == 0)
               {
                  extn->miblen = len;
                  tth->sensor_path = eina_stringshare_add(tth->sensor_name);
               }
#endif
             break;

           case SENSOR_TYPE_OPENBSD:
#ifdef __OpenBSD__
             for (numt = 0; numt < snsrdev.maxnumt[SENSOR_TEMP]; numt++)
               {
                  extn->mib[4] = numt;
                  slen = sizeof(snsr);
                  if (sysctl(extn->mib, 5, &snsr, &slen, NULL, 0) == -1)
                    continue;
                  if (slen > 0 && (snsr.flags & SENSOR_FINVALID) == 0)
                    {
                       break;
                    }
               }
#endif
             break;

           case SENSOR_TYPE_OMNIBOOK:
             tth->sensor_path = eina_stringshare_add("/proc/omnibook/temperature");
             break;

           case SENSOR_TYPE_LINUX_MACMINI:
             tth->sensor_path = eina_stringshare_add("/sys/devices/temperatures/cpu_temperature");
             break;

           case SENSOR_TYPE_LINUX_PBOOK:
             tth->sensor_path = eina_stringshare_add("/sys/devices/temperatures/sensor1_temperature");
             break;

           case SENSOR_TYPE_LINUX_INTELCORETEMP:
             tth->sensor_path = eina_stringshare_add("/sys/devices/platform/coretemp.0/temp1_input");
             break;

           case SENSOR_TYPE_LINUX_THINKPAD:
             tth->sensor_path = eina_stringshare_add("/sys/devices/platform/thinkpad_hwmon/temp1_input");
             break;

           case SENSOR_TYPE_LINUX_I2C:
             therms = ecore_file_ls("/sys/bus/i2c/devices");

             EINA_LIST_FREE(therms, name)
               {
                  snprintf(path, sizeof(path),
                           "/sys/bus/i2c/devices/%s/%s_input",
                           name, tth->sensor_name);
                  if (ecore_file_exists(path))
                    {
                       tth->sensor_path = eina_stringshare_add(path);
                       /* We really only care about the first
                        * one for the default. */
                       break;
                    }
                  free(name);
               }
             break;

           case SENSOR_TYPE_LINUX_PCI:
             therms = ecore_file_ls("/sys/bus/pci/devices");

             EINA_LIST_FREE(therms, name)
               {
                  snprintf(path, sizeof(path),
                           "/sys/bus/pci/devices/%s/%s_input",
                           name, tth->sensor_name);
                  if (ecore_file_exists(path))
                    {
                       tth->sensor_path = eina_stringshare_add(path);
                       /* We really only care about the first
                        * one for the default. */
                       break;
                    }
                  free(name);
               }
             break;

           case SENSOR_TYPE_LINUX_ACPI:
             snprintf(path, sizeof(path),
                      "/proc/acpi/thermal_zone/%s/temperature",
                      tth->sensor_name);
             tth->sensor_path = eina_stringshare_add(path);
             break;

           case SENSOR_TYPE_LINUX_SYS:
             snprintf(path, sizeof(path),
                      "/sys/class/thermal/%s/temp", tth->sensor_name);
             tth->sensor_path = eina_stringshare_add(path);
             break;

           default:
             break;
          }
     }
}

static int
check(Tempthread *tth)
{
   FILE *f = NULL;
   int ret = 0;
   int temp = 0;
   char buf[512];
#if defined (__FreeBSD__) || defined(__DragonFly__)
   size_t len;
   size_t ftemp = 0;
#endif
#if defined (__FreeBSD__) || defined(__DragonFly__) || defined (__OpenBSD__)
   Extn *extn = tth->extn;
#endif

   /* TODO: Make standard parser. Seems to be two types of temperature string:
    * - Somename: <temp> C
    * - <temp>
    */
   switch (tth->sensor_type)
     {
      case SENSOR_TYPE_NONE:
        /* TODO: Slow down poller? */
        break;

      case SENSOR_TYPE_FREEBSD:
#if defined (__FreeBSD__) || defined(__DragonFly__)
        len = sizeof(ftemp);
        if (sysctl(extn->mib, extn->miblen, &ftemp, &len, NULL, 0) == 0)
          {
             temp = (ftemp - 2732) / 10;
             ret = 1;
          }
        else
          goto error;
#endif
        break;

      case SENSOR_TYPE_OPENBSD:
#ifdef __OpenBSD__
        if (sysctl(extn->mib, 5, &snsr, &slen, NULL, 0) != -1)
          {
             temp = (snsr.value - 273150000) / 1000000.0;
             ret = 1;
          }
        else
          goto error;
#endif
        break;

      case SENSOR_TYPE_OMNIBOOK:
        f = fopen(tth->sensor_path, "r");
        if (f)
          {
             char dummy[4096];

             if (fgets(buf, sizeof(buf), f) == NULL) goto error;
             fclose(f);
             f = NULL;
             if (sscanf(buf, "%s %s %i", dummy, dummy, &temp) == 3)
               ret = 1;
             else
               goto error;
          }
        else
          goto error;
        break;

      case SENSOR_TYPE_LINUX_MACMINI:
      case SENSOR_TYPE_LINUX_PBOOK:
        f = fopen(tth->sensor_path, "rb");
        if (f)
          {
             if (fgets(buf, sizeof(buf), f) == NULL) goto error;
             fclose(f);
             f = NULL;
             if (sscanf(buf, "%i", &temp) == 1)
               ret = 1;
             else
               goto error;
          }
        else
          goto error;
        break;

      case SENSOR_TYPE_LINUX_INTELCORETEMP:
      case SENSOR_TYPE_LINUX_I2C:
      case SENSOR_TYPE_LINUX_THINKPAD:
        f = fopen(tth->sensor_path, "r");
        if (f)
          {
             if (fgets(buf, sizeof(buf), f) == NULL) goto error;
             fclose(f);
             f = NULL;
             /* actually read the temp */
             if (sscanf(buf, "%i", &temp) == 1)
               ret = 1;
             else
               goto error;
             /* Hack for temp */
             temp = temp / 1000;
          }
        else
          goto error;
        break;

      case SENSOR_TYPE_LINUX_PCI:
        f = fopen(tth->sensor_path, "r");
        if (f)
          {
             if (fgets(buf, sizeof(buf), f) == NULL) goto error;
             fclose(f);
             f = NULL;
             /* actually read the temp */
             if (sscanf(buf, "%i", &temp) == 1)
               ret = 1;
             else
               goto error;
             /* Hack for temp */
             temp = temp / 1000;
          }
        else
          goto error;
        break;

      case SENSOR_TYPE_LINUX_ACPI:
        f = fopen(tth->sensor_path, "r");
        if (f)
          {
             char *p, *q;

             if (fgets(buf, sizeof(buf), f) == NULL) goto error;
             fclose(f);
             f = NULL;
             p = strchr(buf, ':');
             if (p)
               {
                  p++;
                  while (*p == ' ')
                    p++;
                  q = strchr(p, ' ');
                  if (q) *q = 0;
                  temp = atoi(p);
                  ret = 1;
               }
             else
               goto error;
          }
        else
          goto error;
        break;

      case SENSOR_TYPE_LINUX_SYS:
        f = fopen(tth->sensor_path, "r");
        if (f)
          {
             if (fgets(buf, sizeof(buf), f) == NULL) goto error;
             fclose(f);
             f = NULL;
             temp = atoi(buf);
             temp /= 1000;
             ret = 1;
          }
        else
          goto error;
        break;

      default:
        break;
     }

   if (ret) return temp;

   return -999;
error:
   if (f) fclose(f);
   tth->sensor_type = SENSOR_TYPE_NONE;
   eina_stringshare_del(tth->sensor_name);
   tth->sensor_name = NULL;
   eina_stringshare_del(tth->sensor_path);
   tth->sensor_path = NULL;
   return -999;
}

int
temperature_tempget_get(Tempthread *tth)
{
   int temp;

   init(tth);
   temp = check(tth);
   return temp;
}
