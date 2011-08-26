#include "e_fm_device.h"
#include "e_fm_shared_codec.h"
#include "e_fm_shared_device.h"

#define TEBIBYTE_SIZE 1099511627776LL
#define GIBIBYTE_SIZE 1073741824
#define MEBIBYTE_SIZE 1048576
#define KIBIBYTE_SIZE 1024

static void _e_fm2_volume_write(E_Volume *v) EINA_ARG_NONNULL(1);
static void _e_fm2_volume_erase(E_Volume *v) EINA_ARG_NONNULL(1);
static void _e_fm2_device_mount_free(E_Fm2_Mount *m); EINA_ARG_NONNULL(1);
static void _e_fm2_device_mount_ok(E_Fm2_Mount *m) EINA_ARG_NONNULL(1);
static void _e_fm2_device_mount_fail(E_Fm2_Mount *m) EINA_ARG_NONNULL(1);
static void _e_fm2_device_unmount_ok(E_Fm2_Mount *m) EINA_ARG_NONNULL(1);
static void _e_fm2_device_unmount_fail(E_Fm2_Mount *m) EINA_ARG_NONNULL(1);

static Eina_List *_e_stores = NULL;
static Eina_List *_e_vols = NULL;

static void
_e_fm2_device_volume_setup(E_Volume *v)
{
   char label[1024] = {0};
   char size[256] = {0};
   const char *icon = NULL;
   unsigned long long sz;

   /* Compute the size in a readable form */
   if (v->size)
     {
        if ((sz = (v->size / TEBIBYTE_SIZE)) > 0)
          snprintf(size, sizeof(size) - 1, _("%llu TiB"), sz);
        else if ((sz = (v->size / GIBIBYTE_SIZE)) > 0)
          snprintf(size, sizeof(size) - 1, _("%llu GiB"), sz);
        else if ((sz = (v->size / MEBIBYTE_SIZE)) > 0)
          snprintf(size, sizeof(size) - 1, _("%llu MiB"), sz);
        else if ((sz = (v->size / KIBIBYTE_SIZE)) > 0)
          snprintf(size, sizeof(size) - 1, _("%llu KiB"), sz);
        else
          snprintf(size, sizeof(size) - 1, _("%llu B"), v->size);
     }

   /* Choose the label */
   if ((v->label) && (v->label[0]))
     {}
   else if ((v->partition_label) && (v->partition_label[0]))
     snprintf(label, sizeof(label) - 1, "%s", v->partition_label);
   else if (((v->storage->vendor) && (v->storage->vendor[0])) &&
            ((v->storage->model) && (v->storage->model[0])))
     {
        if (size[0] != '\0')
          snprintf(label, sizeof(label) - 1, "%s %s - %s", v->storage->vendor, v->storage->model, size);
        else
          snprintf(label, sizeof(label) - 1, "%s %s", v->storage->vendor, v->storage->model);
     }
   else if ((v->storage->model) && (v->storage->model[0]))
     {
        if (size[0] != '\0')
          snprintf(label, sizeof(label) - 1, "%s - %s", v->storage->model, size);
        else
          snprintf(label, sizeof(label) - 1, "%s", v->storage->model);
     }
   else if ((v->storage->vendor) && (v->storage->vendor[0]))
     {
        if (size[0] != '\0')
          snprintf(label, sizeof(label) - 1, "%s - %s", v->storage->vendor, size);
        else
          snprintf(label, sizeof(label) - 1, "%s", v->storage->vendor);
     }
   else if (v->storage->drive_type && (!strcmp(v->storage->drive_type, "sd_mmc")))
     snprintf(label, sizeof(label) - 1, "Flash Card - %s", size);
   else
     snprintf(label, sizeof(label), _("Unknown Volume"));

   if (label[0]) eina_stringshare_replace(&v->label, label);

   /* Choose the icon */
   /* http://standards.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html */
   if (v->storage->icon.volume)
     icon = v->storage->icon.volume;
   else
     {
        if ((!v->storage->drive_type) || (!strcmp(v->storage->drive_type, "disk")) || (!strcmp(v->storage->drive_type, "partition")))
          {
             if (v->storage->removable == 0)
               icon = "drive-harddisk";
             else
               icon = "drive-removable-media";
          }
        else if ((!strcmp(v->storage->drive_type, "cdrom")) || (!strncmp(v->storage->drive_type, "optical", 7)))
          icon = "drive-optical";
        else if (!strncmp(v->storage->drive_type, "floppy", 6))
          icon = "media-floppy";
        else if (!strcmp(v->storage->drive_type, "tape"))
          icon = "media-tape";
        else if (!strcmp(v->storage->drive_type, "compact_flash")
                 || !strcmp(v->storage->drive_type, "memory_stick")
                 || !strcmp(v->storage->drive_type, "smart_media")
                 || !strcmp(v->storage->drive_type, "sd_mmc")
                 || !strncmp(v->storage->drive_type, "flash", 5))
          icon = "media-flash";
     }
   if (icon) eina_stringshare_replace(&v->icon, icon);

   if ((!v->mount_point) ||
       (strcmp(v->mount_point, "/") &&
        strcmp(v->mount_point, "/home") &&
        strcmp(v->mount_point, "/tmp")))
     _e_fm2_volume_write(v);
}
     
EAPI void
e_fm2_device_storage_add(E_Storage *s)
{
   Eina_List *l;
   E_Volume *v;
   
   if (e_fm2_device_storage_find(s->udi)) return;

   s->validated = EINA_TRUE;
   _e_stores = eina_list_append(_e_stores, s);
/*
   printf("STO+\n"
          "  udi: %s\n"
          "  bus: %s\n"
          "  drive_type: %s\n"
          "  model: %s\n"
          "  vendor: %s\n"
          "  serial: %s\n"
          "  removable: %i\n"
          "  media_available: %i\n"
          "  media_size: %lli\n"
          "  requires_eject: %i\n"
          "  hotpluggable: %i\n"
          "  media_check_enabled: %i\n"
          "  icon.drive: %s\n"
          "  icon.volume: %s\n\n"
          ,
          s->udi,
          s->bus,
          s->drive_type,
          s->model,
          s->vendor,
          s->serial,
          s->removable,
          s->media_available,
          s->media_size,
          s->requires_eject,
          s->hotpluggable,
          s->media_check_enabled,
          s->icon.drive,
          s->icon.volume);
 */
   if ((s->removable == 0) &&
       (s->media_available == 0) &&
       (s->media_size == 0) &&
       (s->requires_eject == 0) &&
       (s->hotpluggable == 0) &&
       (s->media_check_enabled == 0))
     {
//	printf("      Ignore this storage\n");
     }
   else
     {
        s->trackable = EINA_TRUE;
     }
     
   EINA_LIST_FOREACH(_e_vols, l, v) /* catch volumes which were added before their storage */
     {
        if ((!v->storage) && (s->udi == v->parent))
          {
             v->storage = s;
             _e_fm2_device_volume_setup(v);
          }
     }
}

EAPI void
e_fm2_device_storage_del(E_Storage *s)
{
//   printf("STO- %s\n", s->udi);
     _e_stores = eina_list_remove(_e_stores, s);
     _e_fm_shared_device_storage_free(s);
}

EAPI E_Storage *
e_fm2_device_storage_find(const char *udi)
{
   Eina_List *l;
   E_Storage *s;

   if (!udi) return NULL;
   EINA_LIST_FOREACH(_e_stores, l, s)
     {
        if (!strcmp(udi, s->udi)) return s;
     }
   return NULL;
}


EAPI void
e_fm2_device_volume_add(E_Volume *v)
{
   E_Storage *s;

   if (e_fm2_device_volume_find(v->udi)) return;

   v->validated = EINA_TRUE;
   _e_vols = eina_list_append(_e_vols, v);
/*
   printf("VOL+\n"
          "  udi: %s\n"
          "  uuid: %s\n"
          "  fstype: %s\n"
          "  size: %llu\n"
          "  label: %s\n"
          "  partition: %d\n"
          "  partition_number: %d\n"
          "  partition_label: %s\n"
          "  mounted: %d\n"
          "  mount_point: %s\n"
          "  parent: %s\n"
          ,
          v->udi,
          v->uuid,
          v->fstype,
          v->size,
          v->label,
          v->partition,
          v->partition_number,
          v->partition ? v->partition_label : "(not a partition)",
          v->mounted,
          v->mount_point,
          v->parent);
 */
/* Check mount point */
   if ((v->efm_mode == EFM_MODE_USING_HAL_MOUNT) &&
       ((!v->mount_point) || (!v->mount_point[0])))
     {
        if (v->mount_point) eina_stringshare_del(v->mount_point);
        v->mount_point = NULL;
        v->mount_point = e_fm2_device_volume_mountpoint_get(v);
        if ((!v->mount_point) || (!v->mount_point[0]))
          {
             char buf[PATH_MAX];
             const char *id;

             if (v->mount_point) eina_stringshare_del(v->mount_point);
             v->mount_point = NULL;
             id = "disk";
             if ((v->uuid) && (v->uuid[0])) id = v->uuid;
             if (ecore_file_is_dir("/media"))
               snprintf(buf, sizeof(buf), "/media/%s", id);
             else if (ecore_file_is_dir("/mnt"))
               snprintf(buf, sizeof(buf), "/mnt/%s", id);
             else if (ecore_file_is_dir("/tmp"))
               snprintf(buf, sizeof(buf), "/tmp/%s", id);
             else
               buf[0] = 0;
             v->mount_point = eina_stringshare_add(buf);
          }
     }

   /* Search parent storage */
   if ((s = e_fm2_device_storage_find(v->parent)))
     {
        v->storage = s;
        s->volumes = eina_list_append(s->volumes, v);
     }

   if (v->storage) _e_fm2_device_volume_setup(v);
     
}

EAPI void
e_fm2_device_volume_del(E_Volume *v)
{
   E_Fm2_Mount *m;

//   printf("VOL- %s\n", v->udi);
   _e_vols = eina_list_remove(_e_vols, v);
   _e_fm2_volume_erase(v);
   if (v->storage)
     {
        v->storage->volumes = eina_list_remove(v->storage->volumes, v);
        v->storage = NULL;
     }
   EINA_LIST_FREE(v->mounts, m)
     {
        _e_fm2_device_unmount_ok(m);
        _e_fm2_device_mount_free(m);
     }
   _e_fm_shared_device_volume_free(v);
}

static void
_e_fm2_volume_write(E_Volume *v)
{
   char buf[PATH_MAX], buf2[PATH_MAX];
   FILE *f;
   const char *id;

   if (!v->storage) return;
   id = ecore_file_file_get(v->storage->udi);
//   printf("vol write %s\n", id);
   e_user_dir_snprintf(buf, sizeof(buf), "fileman/favorites/|%s_%d.desktop",
                       id, v->partition_number);

   f = fopen(buf, "w");
   if (f)
     {
        fprintf(f,
                "[Desktop Entry]\n"
                "Encoding=UTF-8\n"
                "Type=Link\n"
                "X-Enlightenment-Type=Removable\n"
                "X-Enlightenment-Removable-State=Full\n"
                "Name=%s\n"
                "Icon=%s\n"
                "Comment=%s\n"
                "URL=file:/%s\n"
                ,
                v->label,
                v->icon,
                _("Removable Device"),
                v->udi);
        fclose(f);

        if (e_config->device_desktop)
          {
             e_user_homedir_snprintf(buf2, sizeof(buf2),
                                     "%s/|%s_%d.desktop",
                                     _("Desktop"), id, v->partition_number);
             ecore_file_symlink(buf, buf2);
          }

        /* FIXME: manipulate icon directly */
        _e_fm2_file_force_update(buf);
        //_e_fm2_file_force_update(buf2);
     }
}

#undef TEBIBYTE_SIZE
#undef GIBIBYTE_SIZE
#undef MEBIBYTE_SIZE
#undef KIBIBYTE_SIZE

static void
_e_fm2_volume_erase(E_Volume *v)
{
   char buf[PATH_MAX] = {0};
   const char *id;

   if (!v->storage) return;
   id = ecore_file_file_get(v->storage->udi);
   e_user_homedir_snprintf(buf, sizeof(buf), "%s/|%s_%d.desktop",
                           _("Desktop"), id, v->partition_number);
   ecore_file_unlink(buf);
   _e_fm2_file_force_update(buf);

   if (e_config->device_desktop)
     {
        e_user_dir_snprintf(buf, sizeof(buf),
                            "fileman/favorites/|%s_%d.desktop",
                            id, v->partition_number);
        ecore_file_unlink(buf);
        _e_fm2_file_force_update(buf);
     }
}

EAPI E_Volume *
e_fm2_device_volume_find(const char *udi)
{
   Eina_List *l;
   E_Volume *v;

   if (!udi) return NULL;

   EINA_LIST_FOREACH(_e_vols, l, v)
     {
        if (!v->udi) continue;
        if (!strcmp(udi, v->udi)) return v;
     }

   return NULL;
}

EAPI const char *
e_fm2_device_volume_mountpoint_get(E_Volume *v)
{
   char buf[PATH_MAX] = {0};

   if (!v) return NULL;
   if (v->mount_point)
     {
        //	printf("GET MOUNTPOINT = %s\n", v->mount_point);
          return eina_stringshare_add(v->mount_point);
     }
   else if (v->efm_mode != EFM_MODE_USING_HAL_MOUNT)
     return NULL;

   if (v->label && v->label[0] != '\0')
     snprintf(buf, sizeof(buf) - 1, "/media/%s", v->label);
   else if (v->uuid && v->uuid[0] != '\0')
     snprintf(buf, sizeof(buf) - 1, "/media/%s", v->uuid);
   else if ((v->storage) && (v->storage->serial) && v->storage->serial[0] != '\0')
     snprintf(buf, sizeof(buf) - 1, "/media/%s", v->storage->serial);
   else
     {
        static int mount_count = 1;
        snprintf(buf, sizeof(buf) - 1, "/media/unknown-%i", mount_count++);
     }
//   printf("GET MOUNTPOINT = %s\n", buf);
   return eina_stringshare_add(buf);
}

EAPI void
e_fm2_device_mount_add(E_Volume   *v,
                       const char *mountpoint)
{
   Eina_List *l;
   E_Fm2_Mount *m;

   v->mounted = EINA_TRUE;
   if (mountpoint && (mountpoint[0]))
     eina_stringshare_replace(&v->mount_point, mountpoint);

   EINA_LIST_FOREACH(v->mounts, l, m)
     _e_fm2_device_mount_ok(m);

//   printf("MOUNT %s %s\n", v->udi, v->mount_point);
}

EAPI void
e_fm2_device_mount_del(E_Volume *v)
{
   E_Fm2_Mount *m;

   v->mounted = EINA_FALSE;
   if (v->mount_point)
     {
        eina_stringshare_del(v->mount_point);
        v->mount_point = NULL;
     }

   EINA_LIST_FREE(v->mounts, m)
     {
        _e_fm2_device_unmount_ok(m);
        _e_fm2_device_mount_free(m);
     }
}

EAPI void
_e_fm2_device_mount_free(E_Fm2_Mount *m)
{
   if (!m) return;

   if (m->udi) eina_stringshare_del(m->udi);
   if (m->mount_point) eina_stringshare_del(m->mount_point);

   free(m);
}

EAPI E_Fm2_Mount *
e_fm2_device_mount_find(const char *path)
{
   Eina_List *l;
   E_Volume *v;

   EINA_LIST_FOREACH(_e_vols, l, v)
     {
        if (v->mounted
            && v->mount_point
            && !strncmp(path, v->mount_point, strlen(v->mount_point))
            && v->mounts)
          return eina_list_data_get(v->mounts);
     }
   return NULL;
}

EAPI E_Fm2_Mount *
e_fm2_device_mount(E_Volume *v,
                   Ecore_Cb  mount_ok,
                   Ecore_Cb  mount_fail,
                   Ecore_Cb  unmount_ok,
                   Ecore_Cb  unmount_fail,
                   void     *data)
{
   E_Fm2_Mount *m;

   if (!v) return NULL;

   m = calloc(1, sizeof(E_Fm2_Mount));
   if (!m) return NULL;
   m->udi = eina_stringshare_add(v->udi);
   m->mount_ok = mount_ok;
   m->mount_fail = mount_fail;
   m->unmount_ok = unmount_ok;
   m->unmount_fail = unmount_fail;
   m->data = data;
   m->volume = v;
   m->mounted = v->mounted;

   v->mounts = eina_list_prepend(v->mounts, m);

//   printf("BEGIN MOUNT %p '%s'\n", m, v->mount_point);

   if (!v->mounted)
     {
        v->auto_unmount = EINA_TRUE;
        _e_fm2_client_mount(v->udi, v->mount_point);
     }
   else
     {
        v->auto_unmount = EINA_FALSE;
        m->mount_point = eina_stringshare_add(v->mount_point);
     }

   return m;
}

EAPI void
e_fm2_device_mount_fail(E_Volume *v)
{
   E_Fm2_Mount *m;

   v->mounted = EINA_FALSE;
   if (v->mount_point)
     {
        eina_stringshare_del(v->mount_point);
        v->mount_point = NULL;
     }

   EINA_LIST_FREE(v->mounts, m)
     {
        _e_fm2_device_mount_fail(m);
        _e_fm2_device_mount_free(m);
     }
}

EAPI void
e_fm2_device_unmount(E_Fm2_Mount *m)
{
   E_Volume *v;

   if (!(v = m->volume)) return;
   v->mounts = eina_list_remove(v->mounts, m);
   _e_fm2_device_mount_free(m);

   if (v->auto_unmount && v->mounted && !eina_list_count(v->mounts))
     _e_fm2_client_unmount(v->udi);
}

EAPI void
e_fm2_device_unmount_fail(E_Volume *v)
{
   Eina_List *l;
   E_Fm2_Mount *m;

   v->mounted = EINA_TRUE;

   EINA_LIST_FOREACH(v->mounts, l, m)
     _e_fm2_device_unmount_fail(m);
}

static void
_e_fm2_device_mount_ok(E_Fm2_Mount *m)
{
   if (m->mounted) return;
   m->mounted = EINA_TRUE;
   if (m->volume)
     m->mount_point = eina_stringshare_add(m->volume->mount_point);
   if (m->mount_ok)
     m->mount_ok(m->data);
//   printf("MOUNT OK '%s'\n", m->mount_point);
}

static void
_e_fm2_device_mount_fail(E_Fm2_Mount *m)
{
   m->mounted = EINA_FALSE;
   if (m->mount_point)
     {
        eina_stringshare_del(m->mount_point);
        m->mount_point = NULL;
     }
   if (m->mount_fail)
     m->mount_fail(m->data);
}

static void
_e_fm2_device_unmount_ok(E_Fm2_Mount *m)
{
   if (!m->mounted) return;
   m->mounted = EINA_FALSE;
   if (m->mount_point)
     {
        eina_stringshare_del(m->mount_point);
        m->mount_point = NULL;
     }
   if (m->unmount_ok)
     m->unmount_ok(m->data);
}

static void
_e_fm2_device_unmount_fail(E_Fm2_Mount *m)
{
   if (m->mounted) return;
   m->mounted = EINA_TRUE;
   if (m->unmount_fail)
     m->unmount_fail(m->data);
}

EAPI void
e_fm2_device_show_desktop_icons(void)
{
   Eina_List *l;
   E_Volume *v;
   char buf[PATH_MAX] = {0};
   char buf2[PATH_MAX] = {0};
   const char *id;

   EINA_LIST_FOREACH(_e_vols, l, v)
     {
        if (!v) continue;
        if (!v->storage) continue;

        id = ecore_file_file_get(v->storage->udi);

        e_user_dir_snprintf(buf, sizeof(buf),
                            "fileman/favorites/|%s_%d.desktop",
                            id, v->partition_number);

        e_user_homedir_snprintf(buf2, sizeof(buf2),
                                "%s/|%s_%d.desktop",
                                _("Desktop"), id, v->partition_number);

        if (ecore_file_exists(buf) && !ecore_file_exists(buf2))
          {
             ecore_file_symlink(buf, buf2);
             _e_fm2_file_force_update(buf2);
          }
     }
}

EAPI void
e_fm2_device_hide_desktop_icons(void)
{
   Eina_List *l;
   E_Volume *v;
   char buf[PATH_MAX] = {0};
   const char *id;

   EINA_LIST_FOREACH(_e_vols, l, v)
     {
        if (!v) continue;
        if (!v->storage) continue;

        id = ecore_file_file_get(v->storage->udi);

        e_user_homedir_snprintf(buf, sizeof(buf),
                                "%s/|%s_%d.desktop",
                                _("Desktop"), id, v->partition_number);

        if (ecore_file_exists(buf))
          {
             ecore_file_unlink(buf);
             _e_fm2_file_force_update(buf);
          }
     }
}

EAPI Eina_List *
e_fm2_device_volume_list_get(void)
{
   return _e_vols;
}

