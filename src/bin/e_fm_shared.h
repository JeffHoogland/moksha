#ifdef E_FM_SHARED_DATATYPES

# define E_DEVICE_TYPE_STORAGE 1
# define E_DEVICE_TYPE_VOLUME  2
typedef struct _E_Storage E_Storage;
typedef struct _E_Volume  E_Volume;
typedef struct _E_Fm2_Mount  E_Fm2_Mount;

struct _E_Storage
{
   int type;
   char *udi;
   char *bus;
   char *drive_type;
   
   char *model;
   char *vendor;
   char *serial;
   
   char removable;
   char media_available;
   unsigned long long media_size;
   
   char requires_eject;
   char hotpluggable;
   char media_check_enabled;
   
   struct {
      char *drive;
      char *volume;
   } icon;
   
   Eina_List *volumes;
   
   unsigned char validated;
   unsigned char trackable;
};

struct _E_Volume
{
   int type;
   char *udi;
   char *uuid;
   char *label;
   char *fstype;
   unsigned long long size;
   
   char  partition;
   int   partition_number;
   char *partition_label;
   char  mounted;
   char *mount_point;
   
   char *parent;
   E_Storage *storage;
   void *prop_handler;
   Eina_List *mounts;
   
   unsigned char validated;
};

struct _E_Fm2_Mount
{
   const char   *udi;
   const char   *mount_point;
   
   Ecore_Timer  *timeout;
   void        (*mount_ok) (void *data);
   void        (*mount_fail) (void *data);
   void        (*unmount_ok) (void *data);
   void        (*unmount_fail) (void *data);
   void         *data;

   E_Volume *volume;

   unsigned char mounted : 1;
};

#endif

#ifdef E_FM_SHARED_CODEC
static Eet_Data_Descriptor *_e_volume_edd = NULL;
static Eet_Data_Descriptor *_e_storage_edd = NULL;

static void
_e_volume_free(E_Volume *v)
{
   if (v->storage)
     {
	v->storage->volumes = eina_list_remove(v->storage->volumes, v);
	v->storage = NULL;
     }
   if (v->udi) free(v->udi);
   if (v->uuid) free(v->uuid);
   if (v->label) free(v->label);
   if (v->fstype) free(v->fstype);
   if (v->partition_label) free(v->partition_label);
   if (v->mount_point) free(v->mount_point);
   if (v->parent) free(v->parent);
   free(v);
}

static void
_e_storage_free(E_Storage *s)
{
   while (s->volumes)
     {
	E_Volume *v;
	
	v = s->volumes->data;
	_e_volume_free(v);
	s->volumes = eina_list_remove_list(s->volumes, s->volumes);
     }
   if (s->udi) free(s->udi);
   if (s->bus) free(s->bus);
   if (s->drive_type) free(s->drive_type);
   
   if (s->model) free(s->model);
   if (s->vendor) free(s->vendor);
   if (s->serial) free(s->serial);
   if (s->icon.drive) free(s->icon.drive);
   if (s->icon.volume) free(s->icon.volume);
   free(s);
}

static Eet_Data_Descriptor *
_e_volume_edd_new(void)
{
   Eet_Data_Descriptor *edd;
   Eet_Data_Descriptor_Class eddc;

   eddc.version = EET_DATA_DESCRIPTOR_CLASS_VERSION;
   eddc.func.mem_alloc = NULL;
   eddc.func.mem_free = NULL;
   eddc.func.str_alloc = (char *(*)(const char *)) strdup;
   eddc.func.str_free = (void (*)(const char *)) free;
   eddc.func.list_next = (void *(*)(void *)) eina_list_next;
   eddc.func.list_append = (void *(*)(void *l, void *d)) eina_list_append;
   eddc.func.list_data = (void *(*)(void *)) eina_list_data_get;
   eddc.func.hash_foreach =
     (void  (*) (void *, int (*) (void *, const char *, void *, void *), void *))
     eina_hash_foreach;
   eddc.func.hash_add = (void* (*) (void *, const char *, void *)) eet_eina_hash_add_alloc;
   eddc.func.hash_free = (void  (*) (void *)) eina_hash_free;
   eddc.name = "e_volume";
   eddc.size = sizeof(E_Volume);

   edd = eet_data_descriptor2_new(&eddc);
#define DAT(x, y, z) EET_DATA_DESCRIPTOR_ADD_BASIC(edd, E_Volume, x, y, z)
   DAT("type", type, EET_T_INT);
   DAT("udi", udi, EET_T_STRING);
   DAT("uuid", uuid, EET_T_STRING);
   DAT("label", label, EET_T_STRING);
   DAT("fstype", fstype, EET_T_STRING);
   DAT("size", size, EET_T_ULONG_LONG);
   DAT("partition", partition, EET_T_CHAR);
   DAT("partition_number", partition_number, EET_T_INT);
   DAT("partition_label", partition_label, EET_T_STRING);
   DAT("mounted", mounted, EET_T_CHAR);
   DAT("mount_point", mount_point, EET_T_STRING);
   DAT("parent", parent, EET_T_STRING);
#undef DAT
   return edd;
}

static Eet_Data_Descriptor *
_e_storage_edd_new(void)
{
   Eet_Data_Descriptor *edd;
   Eet_Data_Descriptor_Class eddc;

   eddc.version = EET_DATA_DESCRIPTOR_CLASS_VERSION;
   eddc.func.mem_alloc = NULL;
   eddc.func.mem_free = NULL;
   eddc.func.str_alloc = (char *(*)(const char *)) strdup;
   eddc.func.str_free = (void (*)(const char *)) free;
   eddc.func.list_next = (void *(*)(void *)) eina_list_next;
   eddc.func.list_append = (void *(*)(void *l, void *d)) eina_list_append;
   eddc.func.list_data = (void *(*)(void *)) eina_list_data_get;
   eddc.func.hash_foreach =
     (void  (*) (void *, int (*) (void *, const char *, void *, void *), void *))
     eina_hash_foreach;
   eddc.func.hash_add = (void* (*) (void *, const char *, void *)) eet_eina_hash_add_alloc;
   eddc.func.hash_free = (void  (*) (Eina_Hash *)) eina_hash_free;
   eddc.name = "e_storage";
   eddc.size = sizeof(E_Storage);

   edd = eet_data_descriptor2_new(&eddc);
#define DAT(x, y, z) EET_DATA_DESCRIPTOR_ADD_BASIC(edd, E_Storage, x, y, z)
   DAT("type", type, EET_T_INT);
   DAT("udi", udi, EET_T_STRING);
   DAT("bus", bus, EET_T_STRING);
   DAT("drive_type", drive_type, EET_T_STRING);
   DAT("model", model, EET_T_STRING);
   DAT("vendor", vendor, EET_T_STRING);
   DAT("serial", serial, EET_T_STRING);
   DAT("removable", removable, EET_T_CHAR);
   DAT("media_available", media_available, EET_T_CHAR);
   DAT("media_size", media_size, EET_T_ULONG_LONG);
   DAT("requires_eject", requires_eject, EET_T_CHAR);
   DAT("hotpluggable", hotpluggable, EET_T_CHAR);
   DAT("media_check_enabled", media_check_enabled, EET_T_CHAR);
   DAT("icon.drive", icon.drive, EET_T_STRING);
   DAT("icon.volume", icon.volume, EET_T_STRING);
#undef DAT
   return edd;
}

static void
_e_storage_volume_edd_init(void)
{
   _e_volume_edd = _e_volume_edd_new();
   _e_storage_edd = _e_storage_edd_new();
}

static void
_e_storage_volume_edd_shutdown(void)
{
   if (_e_volume_edd)
     {
	eet_data_descriptor_free(_e_volume_edd);
	_e_volume_edd = NULL;
     }
   if (_e_storage_edd)
     {
	eet_data_descriptor_free(_e_storage_edd);
	_e_storage_edd = NULL;
     }
}

#endif
