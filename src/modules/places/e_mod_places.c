#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <e.h>
#include <sys/statvfs.h>
#include "e_mod_main.h"
#include "e_mod_places.h"

#ifdef PLACES_HAVE_SYSTEMD
# include "backend_systemd.h"
#endif

#ifdef HAVE_EEZE
# include "backend_eeze.h"
#endif

#ifdef PLACES_HAVE_UDISKS2
# include "backend_udisks2.h"
#endif

#ifdef PLACES_HAVE_MOUNT
# include "backend_mount.h"
#endif

/* Enable/Disable debug messages */
// #define PDBG(...) do {} while (0)
#define PDBG(...) printf("PLACES: "__VA_ARGS__)


/* Local Typedefs */
typedef struct _FreespaceThreadData FreespaceThreadData;
struct _FreespaceThreadData
{
   const char *id;
   const char *mount_point;
   unsigned long long size;
   unsigned long long free_space;
};

/* Local Function Prototypes */
static Eina_Bool _places_freespace_timer_cb(void *data);
static const char *_places_human_size_get(unsigned long long size);
static void _places_volume_object_update(Volume *vol, Evas_Object *obj);
static void _places_run_fm_external(const char *fm, const char *directory);
static void places_run_fm(const char *directory);

/* EcoreEvent callbacks */
static Eina_Bool _places_screensaver_on_cb(void *data, int type, void *event);
static Eina_Bool _places_screensaver_off_cb(void *data, int type, void *event);

/* Edje callbacks */
void _places_icon_activated_cb(void *data, Evas_Object *o __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__);
void _places_eject_activated_cb(void *data, Evas_Object *o, const char *emission, const char *source);
void _places_header_activated_cb(void *data __UNUSED__, Evas_Object *o __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__);

/* Local Variables */
// static Ecore_Timer *poller = NULL;
static char theme_file[PATH_MAX];
static Eina_List *volumes = NULL;
static Ecore_Timer *freespace_timer = NULL;
static Ecore_Thread *freespace_thread = NULL;
static Ecore_Event_Handler *places_screensaver_on_handler = NULL;
static Ecore_Event_Handler *places_screensaver_off_handler = NULL;
#define PLACES_FREESPACE_INTERVAL 3.0


/* Implementation */
void
places_init(void)
{
   volumes = NULL;

   PDBG("Init\n");

#ifdef PLACES_HAVE_SYSTEMD
   places_systemd_init();
#endif
#ifdef HAVE_EEZE
   places_eeze_init();
#endif
#ifdef PLACES_HAVE_UDISKS2
   places_udisks2_init();
#endif
#ifdef PLACES_HAVE_MOUNT
   places_mount_init();
#endif

   snprintf(theme_file, PATH_MAX, "%s/e-module-places.edj",
            places_conf->module->dir);

   places_screensaver_on_handler =
      ecore_event_handler_add(E_EVENT_SCREENSAVER_ON,
                              _places_screensaver_on_cb, NULL);
   places_screensaver_off_handler =
      ecore_event_handler_add(E_EVENT_SCREENSAVER_OFF,
                              _places_screensaver_off_cb, NULL);

   freespace_timer = ecore_timer_add(PLACES_FREESPACE_INTERVAL,
                                     _places_freespace_timer_cb, NULL);

}

void
places_shutdown(void)
{
    if (places_screensaver_on_handler)
     {
        ecore_event_handler_del(places_screensaver_on_handler);
        places_screensaver_on_handler = NULL;
     }
   if (places_screensaver_off_handler)
     {
        ecore_event_handler_del(places_screensaver_off_handler);
        places_screensaver_off_handler = NULL;
     }
   if (freespace_timer)
     {
        ecore_timer_del(freespace_timer);
        freespace_timer = NULL;
     }
   if (freespace_thread)
     {
        ecore_thread_cancel(freespace_thread);
        freespace_thread = NULL;
     }

   while (volumes)
     places_volume_del((Volume*)volumes->data);

#ifdef PLACES_HAVE_SYSTEMD
   places_systemd_shutdown();
#endif
#ifdef HAVE_EEZE
   places_eeze_shutdown();
#endif
#ifdef PLACES_HAVE_UDISKS2
   places_udisks2_shutdown();
#endif
#ifdef PLACES_HAVE_MOUNT
   places_mount_shutdown();
#endif

}

Evas_Object *
places_main_obj_create(Evas *canvas)
{
   Evas_Object *o;

   o = edje_object_add(canvas);
   edje_object_file_set(o, theme_file, "modules/places/main");

   return o;
}

Eina_List *
places_volume_list_get(void)
{
   return volumes;
}

Volume *
places_volume_add(const char *id, Eina_Bool first_time)
{
   Volume *v;
   if (!id) return NULL;

   v = E_NEW(Volume, 1);
   if (!v) return NULL;

   // safe defaults
   v->id = eina_stringshare_add(id);
   v->perc_backup = 0;
   v->valid = EINA_FALSE;
   v->objs = NULL;
   v->icon = eina_stringshare_add("");
   v->device = eina_stringshare_add("");
   v->label = eina_stringshare_add("");
   v->mount_point = eina_stringshare_add("");
   v->fstype = eina_stringshare_add("");
   v->drive_type = eina_stringshare_add("");
   v->model = eina_stringshare_add("");
   v->serial = eina_stringshare_add("");
   v->vendor = eina_stringshare_add("");
   v->bus = eina_stringshare_add("");
   v->to_mount = ((places_conf->auto_mount && !first_time) ||
                  (first_time && places_conf->boot_mount));
   v->force_open = (places_conf->auto_open && !first_time);

   volumes = eina_list_append(volumes, v);

   return v;
}
void
places_volume_del(Volume *v)
{
   Evas_Object *o;
   Evas_Object *swal;
   Eina_List *l;
   Instance *inst;

   if (v->free_func)
      v->free_func(v);

   volumes = eina_list_remove(volumes, v);
   EINA_LIST_FREE(v->objs, o)
     {
        swal = edje_object_part_swallow_get(o, "icon");
        if (swal)
          {
             edje_object_part_unswallow(o, swal);
             evas_object_del(swal);
          }

        EINA_LIST_FOREACH(instances, l, inst)
          edje_object_part_box_remove(inst->o_main, "box", o);

        evas_object_del(o);
     }
   if (v->id)         eina_stringshare_del(v->id);
   if (v->label)       eina_stringshare_del(v->label);
   if (v->icon)        eina_stringshare_del(v->icon);
   if (v->mount_point) eina_stringshare_del(v->mount_point);
   if (v->device)      eina_stringshare_del(v->device);
   if (v->fstype)      eina_stringshare_del(v->fstype);
   if (v->bus)         eina_stringshare_del(v->bus);
   if (v->drive_type)  eina_stringshare_del(v->drive_type);
   if (v->model)       eina_stringshare_del(v->model);
   if (v->vendor)      eina_stringshare_del(v->vendor);
   if (v->serial)      eina_stringshare_del(v->serial);

   E_FREE(v);
}

Volume *
places_volume_by_id_get(const char *id)
{
   Volume *v;
   Eina_List *l;

   EINA_LIST_FOREACH(volumes, l, v)
     if (eina_streq(v->id, id))
       return v;

   return NULL;
}

static int
_places_volume_sort_cb(const void *d1, const void *d2)
{
   const Volume *v1 = d1;
   const Volume *v2 = d2;

   if(!v1) return 1;
   if(!v2) return -1;

   // removable after interal
   if (v1->removable && !v2->removable) return 1;
   if (v2->removable && !v1->removable) return -1;
   // network after local
   if (v1->remote && !v2->remote) return 1;
   if (v2->remote && !v1->remote) return -1;
   // filesystem root on top
   if (eina_streq(v1->mount_point, "/")) return -1;
   if (eina_streq(v2->mount_point, "/")) return 1;
   // order by label
   if(!v1->label) return 1;
   if(!v2->label) return -1;
   return strcmp(v1->label, v2->label);
}

void
places_update_all_gadgets(void)
{
   Eina_List *l;
   Instance *inst;

   volumes = eina_list_sort(volumes, 0, _places_volume_sort_cb);
   EINA_LIST_FOREACH(instances, l, inst)
     places_fill_box(inst->o_main, inst->horiz);
}

void
places_volume_update(Volume *vol)
{
   Evas_Object *obj;
   Eina_List *l;

   if (eina_str_has_prefix(vol->fstype, "nfs") ||  // nfs, nfs3, nfs4, etc..
       eina_streq(vol->fstype, "cifs"))
      vol->remote = EINA_TRUE;
   else
      vol->remote = EINA_FALSE;

   EINA_LIST_FOREACH(vol->objs, l, obj)
     _places_volume_object_update(vol, obj);

   // mount the volume if needed
   if (vol->to_mount && !vol->mounted)
   {
      places_volume_mount(vol);
      vol->to_mount = EINA_FALSE;
   }

   // the volume has been mounted as requested, open the fm
   if (vol->force_open && vol->mounted && vol->mount_point)
   {
      places_run_fm(vol->mount_point);
      vol->force_open = EINA_FALSE;
   }

   // debug:
   places_print_volume(vol);
}

void
places_fill_box(Evas_Object *main, Eina_Bool horiz)
{
   Eina_List *l;
   int min_w, min_h, max_w, max_h, found;
   Evas_Object *o, *icon;
   char *f1, *f2, *f3;
   char buf[128];

   if (!main) return;

   places_empty_box(main);

   // orient the edje box
   if (horiz)
      edje_object_signal_emit(main, "box,set,horiz", "places");
   else
      edje_object_signal_emit(main, "box,set,vert", "places");

   // header (or just a separator if header is not wanted)
   o = edje_object_add(evas_object_evas_get(main));
   if (places_conf->hide_header)
     edje_object_file_set(o, theme_file, "modules/places/separator");
   else
     edje_object_file_set(o, theme_file, "modules/places/header");

    edje_object_part_text_set(o, "label", _("Places"));
   if (horiz)
       edje_object_signal_emit(o, "separator,set,vert", "places");
   else
      edje_object_signal_emit(o, "separator,set,horiz", "places");

   edje_object_size_min_get(o, &min_w, &min_h);
   edje_object_size_max_get(o, &max_w, &max_h);
   evas_object_size_hint_min_set(o, min_w, min_h);
   evas_object_size_hint_max_set(o, max_w, max_h);
   // evas_object_size_hint_align_set(o, EVAS_HINT_FILL, 0.0);
   edje_object_part_box_append(main, "box", o);
   evas_object_show(o);

   edje_object_signal_callback_add(o, "header,activated", "places",
                                   _places_header_activated_cb, NULL);

   // volume objects
   for (l = volumes; l; l = l->next)
     {
        Volume *vol = l->data;

        if (!vol->valid) continue;

        //volume object
        o = edje_object_add(evas_object_evas_get(main));
        edje_object_file_set(o, theme_file, "modules/places/volume");
        vol->objs = eina_list_append(vol->objs, o);

        //choose the right icon
        icon = e_icon_add(evas_object_evas_get(main));
        f1 = f2 = f3 = NULL;
        /* optical discs */
        if (eina_str_has_prefix(vol->drive_type, "optical_bd"))
          {
             f1 = "media"; f2 = "optical"; f3 = "bd";
          }
        else if (eina_str_has_prefix(vol->drive_type, "optical_dvd"))
          {
             f1 = "media"; f2 = "optical"; f3 = "dvd";
          }
        // WD SMARTWARE HACK
        else if (eina_str_has_prefix(vol->drive_type, "optical_cd") &&
                 eina_streq(vol->fstype, "udf")) // NOTE WD Smartware hack
          {
             f1 = "media"; f2 = "optical"; f3 = "cd";
          }
        else if (eina_str_has_prefix(vol->drive_type, "optical_cd") &&
                 !eina_streq(vol->fstype, "iso9660"))
          {
             f1 = "media"; f2 = "optical"; f3 = "audio";
          }
        else if (eina_str_has_prefix(vol->drive_type, "cdrom") ||
                 eina_str_has_prefix(vol->drive_type, "optical"))
          {
             f1 = "media"; f2 = "optical";
          }
        /* flash cards */
        else if (eina_streq(vol->drive_type, "flash_mmc") ||
                 eina_streq(vol->drive_type, "sd_mmc") ||
                 eina_streq(vol->model, "SD/MMC"))
          {
             f1 = "media"; f2 = "flash"; f3 = "sdmmc"; // NOTE sd-mmc in Oxigen :(
          }
        else if (eina_streq(vol->drive_type, "flash_ms") ||
                 eina_streq(vol->drive_type, "memory_stick") ||
                 eina_streq(vol->model, "MS/MS-Pro"))
          {
             f1 = "media"; f2 = "flash"; f3 = "ms"; // NOTE memory-stick in Oxigen :(
          }
        else if (eina_streq(vol->drive_type, "flash_cf"))
          {
             f1 = "media"; f2 = "flash"; f3 = "cf";
          }
        else if (eina_streq(vol->drive_type, "flash_sd"))
          {
             f1 = "media"; f2 = "flash"; f3 = "sd";
          }
        else if (eina_streq(vol->drive_type, "flash_sm"))
          {
             f1 = "media"; f2 = "flash"; f3 = "sm";
          }
        else if (eina_str_has_prefix(vol->drive_type, "flash"))
          {
             f1 = "media"; f2 = "flash";
          }
        /* iPods */
        else if (eina_streq(vol->model, "iPod"))
          {
             f1 = "multimedia-player"; f2 = "apple"; f3 = "ipod";
          }
        /* Floppy disks */
        else if (eina_str_has_prefix(vol->drive_type, "floppy"))
          {
             f1 = "media"; f2 = "floppy";
          }
        /* generic usb drives */
        else if (eina_streq(vol->bus, "usb"))
          {
             f1 = "drive"; f2 = "removable-media"; f3 = "usb";
          }
        /* network filesystem */
        else if (eina_streq(vol->fstype, "nfs") ||
                 eina_streq(vol->fstype, "cifs"))
          {
             f1 = "folder"; f2 = "remote";
          }

        // search the icon, following freedesktop fallback system
        found = 0;
        if (f1 && f2 && f3)
          {
             snprintf(buf, sizeof(buf), "%s-%s-%s", f1, f2, f3);
             found = e_util_icon_theme_set(icon, buf);
          }
        if (!found && f1 && f2)
          {
             snprintf(buf, sizeof(buf), "%s-%s", f1, f2);
             found = e_util_icon_theme_set(icon, buf);
          }
        if (!found)
          {
             snprintf(buf, sizeof(buf), "drive-harddisk");
             found = e_util_icon_theme_set(icon, buf);
          }
        if (found)
          {
             edje_object_part_swallow(o, "icon", icon);
             vol->icon = eina_stringshare_add(buf);
          }
        else evas_object_del(icon);

        //set partition type tag
        if (eina_streq(vol->fstype, "ext2") ||
            eina_streq(vol->fstype, "ext3") ||
            eina_streq(vol->fstype, "ext4") ||
            eina_streq(vol->fstype, "reiserfs") ||
            eina_streq(vol->fstype, "nfs"))
          edje_object_signal_emit(o, "icon,tag,ext3", "places");
        else if (eina_streq(vol->fstype, "ufs") ||
                 eina_streq(vol->fstype, "zfs"))
          edje_object_signal_emit(o, "icon,tag,ufs", "places");
        else if (eina_streq(vol->fstype, "vfat") ||
                 eina_streq(vol->fstype, "ntfs") ||
                 eina_streq(vol->fstype, "ntfs-3g") ||
                 eina_streq(vol->fstype, "cifs"))
          edje_object_signal_emit(o, "icon,tag,fat", "places");
        else if (eina_streq(vol->fstype, "hfs") ||
                 eina_streq(vol->fstype, "hfsplus"))
          edje_object_signal_emit(o, "icon,tag,hfs", "places");

        // update labels, gauge and button
        _places_volume_object_update(vol, o);

        // orient the separator
        if (horiz)
           edje_object_signal_emit(o, "separator,set,vert", "places");
        else
          edje_object_signal_emit(o, "separator,set,horiz", "places");

        // connect signals from edje
        edje_object_signal_callback_add(o, "icon,activated", "places",
                                        _places_icon_activated_cb, vol);
        edje_object_signal_callback_add(o, "eject,activated", "places",
                                        _places_eject_activated_cb, vol);

        // pack the volume in the box
        edje_object_size_min_get(o, &min_w, &min_h);
        edje_object_size_max_get(o, &max_w, &max_h);
        evas_object_size_hint_min_set(o, min_w, min_h);
        evas_object_size_hint_max_set(o, max_w, max_h);
        // evas_object_size_hint_align_set(o, EVAS_HINT_FILL, 0.0);
        // evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        edje_object_part_box_append(main, "box", o);
        evas_object_show(o);
     }
   edje_object_calc_force(main);
   edje_object_size_min_restricted_calc(main, &min_w, &min_h, 99, 1);
   PDBG("SIZE: %d %d\n", min_w, min_h);
   evas_object_size_hint_min_set(main, min_w, min_h);
}

void
places_empty_box(Evas_Object *main)
{
   Evas_Object *o;

   while ((o = edje_object_part_box_remove_at(main, "box", 0)))
   {
      Volume *vol;
      Evas_Object *swal;
      Eina_List *l;

      swal = edje_object_part_swallow_get(o, "icon");
      if (swal)
      {
         edje_object_part_unswallow(o, swal);
         evas_object_del(swal);
      }

      EINA_LIST_FOREACH(volumes, l, vol)
         vol->objs = eina_list_remove(vol->objs, o);

      evas_object_del(o);
   }
}

void
places_volume_mount(Volume *vol)
{
   Eina_List *opts = NULL;
   char buf[256];

   if (!vol || !vol->mount_func || vol->mounted)
     return;

   if ((eina_streq(vol->fstype, "vfat")) || (eina_streq(vol->fstype, "ntfs")))
     {
        snprintf(buf, sizeof(buf), "uid=%i", (int)getuid());
        opts = eina_list_append(opts, buf);
     }

   vol->mount_func(vol, opts);
   vol->to_mount = EINA_FALSE;

   eina_list_free(opts);
}

void
places_volume_unmount(Volume *vol)
{
   if (vol && vol->unmount_func)
     vol->unmount_func(vol, NULL);
}

void
places_volume_eject(Volume *vol)
{
   if (vol && vol->eject_func)
     vol->eject_func(vol, NULL);
}

void
places_print_volume(Volume *v)
{
   const char *size, *free;

   printf("Got volume %s (totals: %d)\n", v->id, eina_list_count(volumes));
   printf("  valid: %d\n", v->valid);
   printf("  label: %s\n",v->label);
   printf("  mounted: %d\n", v->mounted);
   printf("  m_point: %s\n", v->mount_point);
   printf("  device: %s\n", v->device);
   printf("  fstype: %s\n", v->fstype);
   printf("  bus: %s\n", v->bus);
   printf("  drive_type: %s\n", v->drive_type);
   printf("  model: %s\n", v->model);
   printf("  vendor: %s\n", v->vendor);
   printf("  serial: %s\n", v->serial);
   printf("  removable: %d\n", v->removable);
   printf("  requires eject: %d\n", v->requires_eject);
   printf("  media_available: %d\n", v->media_available);
   printf("  remote: %d\n", v->remote);
   size = _places_human_size_get(v->size);
   free = _places_human_size_get(v->free_space);
   printf("  size: %s\n", size);
   printf("  free_space: %s\n", free);
   eina_stringshare_del(size);
   eina_stringshare_del(free);
   printf("\n");
}

static void
places_run_fm(const char *directory)
{
   //const char *directory = data;

   if (places_conf->fm && places_conf->fm[0])
     {
        _places_run_fm_external(places_conf->fm, directory);
     }
   else
     {
        E_Action *act = e_action_find("fileman");
        Eina_List *managers = e_manager_list();

        if (act && act->func.go && managers && managers->data)
          act->func.go(E_OBJECT(managers->data), directory);
        else
          e_util_dialog_internal(_("Warning"),
                      _("<b>Cannot run the Enlightenment FileManager.</b><br>"
                         "Please choose a custom file manager in<br>"
                         "the gadget configuration."));
     }
}

/* Currently unused
static unsigned long long
_places_free_space_get(const char *mount)
{
   struct statvfs s;

   if (!mount) return 0;
   if (statvfs(mount, &s) != 0)
     return 0;

   return (unsigned long long)s.f_bavail * (unsigned long long)s.f_frsize;
}*/

/* Internals */
static void
_places_freespace_thread_run(void *data, Ecore_Thread *thread)
{
   /* statvfs can block, so we run it in a thread to not stall the E mainloop */
   Eina_List *l;
   FreespaceThreadData *td = NULL;
   struct statvfs s;

   // PDBG("RUNNING IN THREAD %p\n", thread);

   if (ecore_thread_check(thread) == EINA_TRUE)  // thread cancelled
     return;

   EINA_LIST_FOREACH(data, l, td)
     {
        if (td->mount_point && (statvfs(td->mount_point, &s) == 0))
          {
             td->size = (unsigned long long)s.f_blocks * (unsigned long long)s.f_frsize;
             td->free_space = (unsigned long long)s.f_bavail * (unsigned long long)s.f_frsize;
          }
        if (ecore_thread_check(thread) == EINA_TRUE)
          return;
     }
}

static void
_places_freespace_thread_done(void *data, Ecore_Thread *thread __UNUSED__)
{
   FreespaceThreadData *td = NULL;

   int percent;
   char diskname[200];
   char diskpercent[200];
   Volume *vol;

   EINA_LIST_FREE(data, td)
     {
        vol = places_volume_by_id_get(td->id);
        if (vol)
          {
             // skip check if cdrom
             if (strstr(vol->device, "/dev/scd") || strstr(vol->device, "/dev/sr")) return;
             // redraw only if size or free_space has changed more than 1Mb
             if ((labs((long long)td->free_space - (long long)vol->free_space) > 1024 * 1024) ||
                 (labs((long long)td->size - (long long)vol->size) > 1024 * 1024))
               {
                  vol->size = td->size;
                  vol->free_space = td->free_space;

                   percent = 100 - (((long double)vol->free_space / (long double)vol->size) * 100);
                   if ((places_conf->alert_p > 0) && (percent > places_conf->alert_p)
                                                  && (percent > vol->perc_backup))
                     {
                        // FIXME: has to support old API for config option
                        E_Notification_Notify n;
                        sprintf(diskname,_("Places warning!"));
                        sprintf(diskpercent,_("Disk ''%s'' usage is %d %%!"), vol->label, percent);

                        memset(&n, 0, sizeof(E_Notification_Notify));
                        n.app_name = _("Places");
                        n.replaces_id = 0;
                        n.icon.icon = "dialog_error";
                        n.summary = diskname;
                        n.body = diskpercent;
                        n.timeout = places_conf->alert_timeout * 1000;
                        e_notification_client_send(&n,  NULL, NULL);
                      }
                  vol->perc_backup = percent;

                  places_volume_update(vol);
               }
          }
        eina_stringshare_del(td->id);
        eina_stringshare_del(td->mount_point);
        E_FREE(td);
     }

   // PDBG("    THREAD DONE %p\n", thread);
   freespace_thread = NULL;
}

static void
_places_freespace_thread_cancel(void *data, Ecore_Thread *thread __UNUSED__)
{
   FreespaceThreadData *td = NULL;

   EINA_LIST_FREE(data, td)
     {
        eina_stringshare_del(td->id);
        eina_stringshare_del(td->mount_point);
        E_FREE(td);
     }
   freespace_thread = NULL;
}

static Eina_Bool
_places_freespace_timer_cb(void *data EINA_UNUSED)
{
   Volume *vol;
   Eina_List *l, *tdl = NULL;
   FreespaceThreadData *td = NULL;

   // PDBG("TIMER %.1f\n", ecore_time_get());
   if (freespace_thread)
     {
        // PDBG("*** SOMETHING WRONG *** thread:%p still running...\n", freespace_thread);
        return ECORE_CALLBACK_RENEW;
     }

   EINA_LIST_FOREACH(volumes, l, vol)
     {
        if (vol->valid && vol->mounted)
          {
             td = E_NEW(FreespaceThreadData, 1);
             if (!td) return ECORE_CALLBACK_RENEW;

             td->id = eina_stringshare_add(vol->id);
             td->mount_point = eina_stringshare_add(vol->mount_point);
             tdl = eina_list_append(tdl, td);
          }
     }

   if (tdl)
     freespace_thread = ecore_thread_run(_places_freespace_thread_run,
                                         _places_freespace_thread_done,
                                         _places_freespace_thread_cancel,
                                         tdl);

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_places_screensaver_on_cb(void *data EINA_UNUSED, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   if (freespace_timer)
     {
        ecore_timer_del(freespace_timer);
        freespace_timer = NULL;
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_places_screensaver_off_cb(void *data EINA_UNUSED, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   if (!freespace_timer)
     freespace_timer = ecore_timer_add(PLACES_FREESPACE_INTERVAL,
                                       _places_freespace_timer_cb, NULL);

   return ECORE_CALLBACK_PASS_ON;
}

static const char *
_places_human_size_get(unsigned long long size)
{
   double dsize;
   char hum[32], *suffix;

   dsize = (double)size;
   if (dsize < 1024)
     snprintf(hum, sizeof(hum), "%.0f%s", dsize, _("b"));
   else
     {
        dsize /= 1024.0;
        if (dsize < 1024)
          suffix = N_("KB");
        else
          {
             dsize /= 1024.0;
             if (dsize < 1024)
               suffix = N_("MB");
             else
               {
                  dsize /= 1024.0;
                  if(dsize < 1024)
                    suffix = N_("GB");
                  else
                    {
                       dsize /= 1024.0;
                       suffix = N_("TB");
                    }
               }
          }
        snprintf(hum, sizeof(hum), "%.1f%s", dsize, _(suffix));
     }

   return eina_stringshare_add(hum);
}

static void
_places_run_fm_external(const char *fm, const char *directory)
{
   char exec[PATH_MAX];
   E_Zone *zone = NULL;
   zone = e_util_zone_current_get (e_manager_current_get ());
   snprintf(exec, PATH_MAX, "%s \"%s\"", (char*)fm, (char*)directory);
   e_exec(zone, NULL, exec, NULL, NULL);
}

static void
_places_volume_object_update(Volume *vol, Evas_Object *obj)
{
   const char *tot_h;
   const char *free_h;
   char buf[256];
   char buf2[16];

   // PDBG("Object update for vol %s\n", vol->id);

   // the volume label
   if (eina_streq(vol->mount_point, "/"))
     edje_object_part_text_set(obj, "volume_label", _("Filesystem"));
   else if (vol->label && vol->label[0])
     edje_object_part_text_set(obj, "volume_label", vol->label);
   else
     edje_object_part_text_set(obj, "volume_label", _("No Name"));

   // the free label
   tot_h = _places_human_size_get(vol->size);
   if (vol->mounted)
     {
        free_h = _places_human_size_get(vol->free_space);
        snprintf(buf, sizeof(buf), "%s %s %s", free_h, _("free of"), tot_h);
        edje_object_part_text_set(obj, "size_label", buf);
        eina_stringshare_del(free_h);
     }
   else
     {
        snprintf(buf, sizeof(buf), _("%s Not Mounted"), tot_h);
        edje_object_part_text_set(obj, "size_label", buf);
     }
   eina_stringshare_del(tot_h);

   // the gauge
   int percent;
   Edje_Message_Float msg_float;
   if (vol->mounted)
     {
        percent = 100 - (((long double)vol->free_space / (long double)vol->size) * 100);
        snprintf(buf2, sizeof(buf2), "%d%%", percent);
        edje_object_part_text_set(obj, "percent_label", buf2);
        msg_float.val = (float)percent / 100;
        edje_object_message_send(obj, EDJE_MESSAGE_FLOAT, 1, &msg_float);
        edje_object_signal_emit(obj, "gauge,show", "places");
     }
   else
     {
        edje_object_signal_emit(obj, "gauge,hide", "places");
        edje_object_part_text_set(obj, "percent_label", "");
     }

   // the mount/eject icon
   if (vol->mounted  && vol->mount_point && vol->unmount_func &&
       !eina_streq(vol->mount_point, "/"))
     {
        edje_object_signal_emit(obj, "icon,eject,show", "places");
        edje_object_part_text_set(obj, "eject_label", _("unmount"));
     }
   else if (!vol->mounted && vol->eject_func &&
            (vol->requires_eject || vol->removable))
     {
        edje_object_signal_emit(obj, "icon,eject,show", "places");
        edje_object_part_text_set(obj, "eject_label", _("eject"));
     }
   else
     edje_object_signal_emit(obj, "icon,eject,hide", "places");
}


/* EDJE Callbacks */
void
_places_icon_activated_cb(void *data, Evas_Object *o __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Volume *vol = data;

   if (vol->mounted)
     places_run_fm(vol->mount_point);
   else if (vol->mount_func)
     {
        vol->force_open = EINA_TRUE;
        places_volume_mount(vol);
     }
}

void
_places_eject_activated_cb(void *data, Evas_Object *o __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Volume *vol = data;

   if (vol->mounted)
     places_volume_unmount(vol);
   else
     places_volume_eject(vol);
}

void
_places_header_activated_cb(void *data __UNUSED__, Evas_Object *o __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   places_run_fm(e_user_homedir_get());
}

/* E17 menu augmentation */
static Eina_List *_menu_strings = NULL;

void
_places_menu_folder_cb(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   const char *folder = data;

   places_run_fm(folder);
}

void
_places_menu_volume_cb(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   Volume *vol = data;

   if (vol->mounted)
     places_run_fm(vol->mount_point);
   else if (vol->mount_func)
     {
        vol->force_open = EINA_TRUE;
        places_volume_mount(vol);
     }
}

static void
_places_bookmarks_parse(E_Menu *em)
{
   char line[PATH_MAX];
   char buf[PATH_MAX];
   const char *s;
   E_Menu_Item *mi;
   Efreet_Uri *uri;
   char *alias;
   FILE* fp;

   EINA_LIST_FREE(_menu_strings, s)
     eina_stringshare_del(s);

   snprintf(buf, sizeof(buf), "%s/gtk-3.0/bookmarks", efreet_config_home_get());
   fp = fopen(buf, "r");
   if (!fp)
     {
        snprintf(buf, sizeof(buf), "%s/.gtk-bookmarks", e_user_homedir_get());
        fp = fopen(buf, "r");
     }
   if (fp)
     {
        while(fgets(line, sizeof(line), fp))
          {
             alias = NULL;
             line[strlen(line) - 1] = '\0';
             alias = strchr(line, ' ');
             if (alias)
               {
                  line[alias-line] =  '\0';
                  alias++;
               }
             uri = efreet_uri_decode(line);
             if (uri && uri->path && ecore_file_exists(uri->path))
               {
                  mi = e_menu_item_new(em);
                  e_menu_item_label_set(mi, alias ? alias :
                                        ecore_file_file_get(uri->path));
                  e_util_menu_item_theme_icon_set(mi, "user-bookmarks");
                  s = eina_stringshare_add(uri->path);
                  e_menu_item_callback_set(mi, _places_menu_folder_cb, s);
                  _menu_strings = eina_list_append(_menu_strings, s);
               }
             if (uri) efreet_uri_free(uri);
          }
        fclose(fp);
     }
}

void
places_generate_menu(void *data __UNUSED__, E_Menu *em)
{
   E_Menu_Item *mi;

   // Home
   if (places_conf->show_home)
     {
        mi = e_menu_item_new(em);
        e_menu_item_label_set(mi, _("Home"));
        e_util_menu_item_theme_icon_set(mi, "user-home");
        e_menu_item_callback_set(mi, _places_menu_folder_cb, e_user_homedir_get());
     }

   // Desktop
   if (places_conf->show_desk)
     {
        mi = e_menu_item_new(em);
        e_menu_item_label_set(mi, _("Desktop"));
        e_util_menu_item_theme_icon_set(mi, "user-desktop");
        e_menu_item_callback_set(mi, _places_menu_folder_cb, efreet_desktop_dir_get());
     }

   // Trash
   if (places_conf->show_trash)
     {
        mi = e_menu_item_new(em);
        e_menu_item_label_set(mi, _("Trash"));
        e_util_menu_item_theme_icon_set(mi, "user-trash");
        e_menu_item_callback_set(mi, _places_menu_folder_cb, "trash:///");
     }

   // File System
   if (places_conf->show_root)
     {
        mi = e_menu_item_new(em);
        e_menu_item_label_set(mi, _("Filesystem"));
        e_util_menu_item_theme_icon_set(mi, "drive-harddisk");
        e_menu_item_callback_set(mi, _places_menu_folder_cb, "/");
     }

   // Temp
   if (places_conf->show_temp)
     {
        mi = e_menu_item_new(em);
        e_menu_item_label_set(mi, _("Temp"));
        e_util_menu_item_theme_icon_set(mi, "user-temp");
        e_menu_item_callback_set(mi, _places_menu_folder_cb, "/tmp");
     }

   // Separator
   if (places_conf->show_home || places_conf->show_desk ||
       places_conf->show_trash || places_conf->show_root ||
       places_conf->show_temp)
     {
        mi = e_menu_item_new(em);
        e_menu_item_separator_set(mi, 1);
     }

   // Volumes
   Eina_Bool volumes_visible = 0;
   const Eina_List *l;
   Volume *vol;
   EINA_LIST_FOREACH(volumes, l, vol)
     {
        if (!vol->valid) continue;
        if (eina_streq(vol->mount_point, "/")) continue;

        mi = e_menu_item_new(em);
        if ((vol->label) && (vol->label[0] != '\0'))
          e_menu_item_label_set(mi, vol->label);
        else
          e_menu_item_label_set(mi, ecore_file_file_get(vol->mount_point));

        if (vol->icon)
          e_util_menu_item_theme_icon_set(mi, vol->icon);

        //e_menu_item_callback_set(mi, places_menu_click_cb, vol);
        e_menu_item_callback_set(mi, _places_menu_volume_cb, vol);
        volumes_visible = 1;
     }

   // Favorites
   if (places_conf->show_bookm)
     {
        if (volumes_visible)
          {
             mi = e_menu_item_new(em);
             e_menu_item_separator_set(mi, 1);
          }
        _places_bookmarks_parse(em);
     }

   e_menu_pre_activate_callback_set(em, NULL, NULL);
}

void
places_augmentation(void *data __UNUSED__, E_Menu *em)
{
   E_Menu_Item *mi;
   E_Menu *m;

   mi = e_menu_item_new(em);
   e_menu_item_label_set(mi, _("Places"));
   e_util_menu_item_theme_icon_set(mi, "system-file-manager");

   m = e_menu_new();
   e_menu_item_submenu_set(mi, m);
   // e_object_unref(E_OBJECT(m));
   e_menu_pre_activate_callback_set(m, places_generate_menu, NULL);
}


#undef PDBG
