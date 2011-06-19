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
read_file(const char *file)
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
write_file(const char *file, int val)
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

/* externally accessible functions */
int
main(int argc,
     char **argv)
{
   int i;
   int level;
   char *maxstr;
   int maxlevel = 0, curlevel;
   char file[4096] = "";
   
   for (i = 1; i < argc; i++)
     {
        if ((!strcmp(argv[i], "-h")) ||
            (!strcmp(argv[i], "-help")) ||
            (!strcmp(argv[i], "--help")))
          {
             printf(
               "This is an internal tool for Enlightenment.\n"
               "do not use it.\n"
               );
             exit(0);
          }
     }
   if (argc == 2)
     {
        level = atoi(argv[1]);
     }
   else
     {
        exit(1);
     }

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
   
   maxstr = read_file("/sys/devices/virtual/backlight/acpi_video0/max_brightness_max");
   if (maxstr)
     {
        maxlevel = atoi(maxstr);
        if (maxlevel <= 0)
          {
             free(maxstr);
             maxstr = NULL;
          }
        else
          {
             snprintf(file, sizeof(file), "/sys/devices/virtual/backlight/acpi_video0/brightness");
             free(maxstr);
             maxstr = NULL;
          }
     }
   if (maxlevel <= 0)
     {
        DIR *dirp = opendir("/sys/devices/virtual/backlight");
        struct dirent *dp;
        
        if (!dirp) return 1;
        while ((dp = readdir(dirp)))
          {
             if ((strcmp(dp->d_name, ".")) && (strcmp(dp->d_name, "..")))
               {
                  char buf[4096];
                  
                  snprintf(buf, sizeof(buf), "/sys/devices/virtual/backlight/%s/max_brightness", dp->d_name);
                  maxstr = read_file(buf);
                  if (maxstr)
                    {
                       maxlevel = atoi(maxstr);
                       if (maxlevel <= 0)
                         {
                            free(maxstr);
                            maxstr = NULL;
                         }
                       else
                         {
                            snprintf(file, sizeof(file), "/sys/devices/virtual/backlight/%s/brightness", dp->d_name);
                            free(maxstr);
                            maxstr = NULL;
                            break;
                         }
                    }
               }
          }
        closedir(dirp);
     }
   if (maxlevel > 0)
     {
        curlevel = ((maxlevel * level) + (500 / maxlevel)) / 1000;
        return write_file(file, curlevel);
     }
   return -1;
}
