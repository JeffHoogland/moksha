#include "e_fm_shared_types.h"
#include "e_fm_shared_device.h"

void
_e_fm_shared_device_storage_free(E_Storage *s)
{
   E_Volume *v;
   EINA_LIST_FREE(s->volumes, v)
     {
        v->storage = NULL;
	       _e_fm_shared_device_volume_free(v);
     }
   if (s->udi) eina_stringshare_del(s->udi);
   if (s->bus) eina_stringshare_del(s->bus);
   if (s->drive_type) eina_stringshare_del(s->drive_type);
   if (s->model) eina_stringshare_del(s->model);
   if (s->vendor) eina_stringshare_del(s->vendor);
   if (s->serial) eina_stringshare_del(s->serial);
   if (s->icon.drive) eina_stringshare_del(s->icon.drive);
   if (s->icon.volume) eina_stringshare_del(s->icon.volume);
#ifdef HAVE_EEZE_MOUNT
   if (s->disk) eeze_disk_free(s->disk);
#endif
   free(s);
}


void
_e_fm_shared_device_volume_free(E_Volume *v)
{
   if (v->storage)
     {
								v->storage->volumes = eina_list_remove(v->storage->volumes, v);
								v->storage = NULL;
     }
   if (v->udi) eina_stringshare_del(v->udi);
   if (v->uuid) eina_stringshare_del(v->uuid);
   if (v->label) eina_stringshare_del(v->label);
   if (v->icon) eina_stringshare_del(v->icon);
   if (v->fstype) eina_stringshare_del(v->fstype);
   if (v->partition_label) eina_stringshare_del(v->partition_label);
   if (v->mount_point) eina_stringshare_del(v->mount_point);
   if (v->parent) eina_stringshare_del(v->parent);
   free(v);
}
