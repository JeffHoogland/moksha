#include "e.h"
#include "e_mod_main.h"

int
temperature_udev_get(Tempthread *tth)
{
   Eina_List *l;
   double cur, temp;
   char *syspath;
   const char *test;
   char buf[256];
   int x, y, cpus = 0;

   temp = -999;

   if (!tth->tempdevs)
     tth->tempdevs =
       eeze_udev_find_by_type(EEZE_UDEV_TYPE_IS_IT_HOT_OR_IS_IT_COLD_SENSOR,
                              NULL);
   if (tth->tempdevs)
     {
        temp = 0;
        EINA_LIST_FOREACH(tth->tempdevs, l, syspath)
          {
             for (x = 1, y = 0; x < 15; x++)
               {
                  if (y >= 2) break;
                  sprintf(buf, "temp%d_input", x);
                  if ((test = eeze_udev_syspath_get_sysattr(syspath, buf)))
                    {
                       y = 0;
                       cur = atoi(test);
                       if (cur > 0)
                         {
                            /* udev reports temp in (celsius * 1000) */
                            temp += (cur / 1000);
                            cpus++;
                         }
                    }
                  /* keep checking for sensors until 2 in a row don't exist */
                  else y++;
               }
          }
        if (cpus > 0)
          {
             temp /= (double)cpus;
             return temp;
          }
     }
   return -999;
}
