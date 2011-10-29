#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

/* local subsystem functions */
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
   int maxlevel = 0, curlevel = -1;
   char file[4096] = "";
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

   for (i = 0; i < (int)(sizeof(search) / sizeof(Bl_Entry)); i++)
     {
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
                       snprintf(file, sizeof(file), "%s/%s", b->base, b->max);
                       free(valstr);
                       valstr = _bl_read_file(file);
                       if (valstr)
                         {
                            maxlevel = atoi(valstr);
                            free(valstr);
                            valstr = NULL;
                         }
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
                                      snprintf(file, sizeof(file), "%s/%s/%s",
                                                b->base, dp->d_name, b->max);
                                      free(valstr);
                                      valstr = _bl_read_file(file);
                                      if (valstr)
                                        {
                                           maxlevel = atoi(valstr);
                                           free(valstr);
                                           valstr = NULL;
                                        }
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
   if (maxlevel <= 0) maxlevel = 255;
   printf("curlevel = %i\n", curlevel);
   printf("maxlevel = %i\n", maxlevel);
   printf("file = %s\n", file);
   printf("buf = %s\n", buf);
   if (curlevel >= 0)
     {
        curlevel = ((maxlevel * level) + (500 / maxlevel)) / 1000;
        printf("SET: %i, %i/%i\n", level, curlevel, maxlevel);
        return _bl_write_file(buf, curlevel);
     }
   return -1;
}
