
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_EEZE

#include <e.h>
#include "e_mod_places.h"
#include <Eeze.h>
// #include <Eeze_Disk.h>

/* Local Function Prototypes */


/* Local Variables */



/* Implementation */
Eina_Bool
places_eeze_init(void)
{
   printf("PLACES: eeze: init()\n");

   if (!eeze_init() /*|| !eeze_disk_function()*/)
     {
        printf("Impossible to setup eeze.\n");
        return EINA_FALSE;
     }

/*
   Eina_List *disks;
   const char *syspath;
   disks = eeze_udev_find_by_type(EEZE_UDEV_TYPE_DRIVE_MOUNTABLE, NULL);
   printf("Found the following mountable disks:\n");
   EINA_LIST_FREE(disks, syspath)
     {
        Eeze_Disk *disk;

        disk = eeze_disk_new(syspath);
        printf("\t%s - %s:%s\n", syspath, eeze_disk_devpath_get(disk), eeze_disk_mount_point_get(disk));
        eeze_disk_free(disk);
        eina_stringshare_del(syspath);
     }
*/

   return EINA_TRUE;
}

void
places_eeze_shutdown(void)
{
   eeze_shutdown();
}


/* Internals */

#endif
