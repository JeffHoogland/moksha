#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

#include <Eeze.h>

/* local subsystem functions */
static int
_bl_write_file(const char *file, int val)
{
   char buf[256];
   int fd = open(file, O_WRONLY);
   if (fd < 0)
     {
        perror("open");
        return -1;
     }
   snprintf(buf, sizeof(buf), "%i", val);
   if (write(fd, buf, strlen(buf)) != (int)strlen(buf))
     {
        perror("write");
        close(fd);
        return -1;
     }
   close(fd);
   return 0;
}

/* local subsystem globals */
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

/* externally accessible functions */
int
main(int argc, char **argv)
{
   int i;
   int level;
   char *valstr;
   const char *f;
   int maxlevel = 0, curlevel = -1;
   Eina_List *devs;
   char buf[4096] = "";
   
   for (i = 1; i < argc; i++)
     {
        if ((!strcmp(argv[i], "-h")) ||
            (!strcmp(argv[i], "-help")) ||
            (!strcmp(argv[i], "--help")))
          {
             printf("This is an internal tool for Enlightenment.\n"
                    "do not use it.\n");
             exit(0);
          }
     }
   if (argc == 2)
     level = atoi(argv[1]);
   else
     exit(1);

   if (setuid(0) != 0)
     {
        printf("ERROR: UNABLE TO ASSUME ROOT PRIVILEGES\n");
        exit(5);
     }
   if (setgid(0) != 0)
     {
        printf("ERROR: UNABLE TO ASSUME ROOT GROUP PRIVILEGES\n");
        exit(7);
     }

   eeze_init();
   devs = eeze_udev_find_by_filter("backlight", NULL, NULL);
   if (!devs) return -1;
   EINA_LIST_FREE(devs, f)
     {
        const char *str;

        str = eeze_udev_syspath_get_sysattr(f, "max_brightness");
        if (str)
          {
             maxlevel = atoi(str);
             eina_stringshare_del(str);
             str = eeze_udev_syspath_get_sysattr(f, "brightness");
             if (str)
               {
                  curlevel = atoi(str);
                  eina_stringshare_del(str);
               }
          }

        if (maxlevel <= 0) maxlevel = 255;
        if (curlevel >= 0)
          {
             curlevel = ((maxlevel * level) + (500 / maxlevel)) / 1000;
     //        printf("SET: %i, %i/%i\n", level, curlevel, maxlevel);
             snprintf(buf, sizeof(buf), "%s/brightness", f);
             return _bl_write_file(buf, curlevel);
          }
        eina_stringshare_del(f);
     }

   return -1;
}
