#include "e_fm_shared_types.h"
#include "e_fm_shared_codec.h"

static Eet_Data_Descriptor *_e_volume_edd = NULL;
static Eet_Data_Descriptor *_e_storage_edd = NULL;

Eet_Data_Descriptor *
_e_volume_edd_new(void)
{
   Eet_Data_Descriptor *edd;
   Eet_Data_Descriptor_Class eddc;

   if (!eet_eina_stream_data_descriptor_class_set(&eddc, sizeof(eddc), "e_volume", sizeof(E_Volume)))
     return NULL;

//   eddc.func.str_alloc = (char *(*)(const char *)) strdup;
//   eddc.func.str_free = (void (*)(const char *)) free;

   edd = eet_data_descriptor_stream_new(&eddc);
#define DAT(MEMBER, TYPE) EET_DATA_DESCRIPTOR_ADD_BASIC(edd, E_Volume, #MEMBER, MEMBER, EET_T_##TYPE)
   DAT(type, INT);
   DAT(udi, STRING);
   DAT(uuid, STRING);
   DAT(label, STRING);
   DAT(fstype, STRING);
   DAT(size, ULONG_LONG);
   DAT(partition, CHAR);
   DAT(partition_number, INT);
   DAT(partition_label, STRING);
   DAT(mounted, CHAR);
   DAT(mount_point, STRING);
   DAT(parent, STRING);
   DAT(first_time, CHAR);
   DAT(encrypted, CHAR);
   DAT(unlocked, CHAR);
   DAT(efm_mode, UINT);
#undef DAT
   return edd;
}

Eet_Data_Descriptor *
_e_storage_edd_new(void)
{
   Eet_Data_Descriptor *edd;
   Eet_Data_Descriptor_Class eddc;

   if (!eet_eina_stream_data_descriptor_class_set(&eddc, sizeof (eddc), "e_storage", sizeof (E_Storage)))
     return NULL;

//   eddc.func.str_alloc = (char *(*)(const char *)) strdup;
//   eddc.func.str_free = (void (*)(const char *)) free;

   edd = eet_data_descriptor_stream_new(&eddc);
#define DAT(MEMBER, TYPE) EET_DATA_DESCRIPTOR_ADD_BASIC(edd, E_Storage, #MEMBER, MEMBER, EET_T_##TYPE)
   DAT(type, INT);
   DAT(udi, STRING);
   DAT(bus, STRING);
   DAT(drive_type, STRING);
   DAT(model, STRING);
   DAT(vendor, STRING);
   DAT(serial, STRING);
   DAT(removable, CHAR);
   DAT(media_available, CHAR);
   DAT(media_size, ULONG_LONG);
   DAT(requires_eject, CHAR);
   DAT(hotpluggable, CHAR);
   DAT(media_check_enabled, CHAR);
   DAT(icon.drive, STRING);
   DAT(icon.volume, STRING);
#undef DAT
   return edd;
}

void *
_e_fm_shared_codec_storage_encode(E_Storage *s, int *size)
{
   return eet_data_descriptor_encode(_e_storage_edd, s, size);
}

E_Storage *
_e_fm_shared_codec_storage_decode(void *s, int size)
{
   return eet_data_descriptor_decode(_e_storage_edd, s, size);
}


void *
_e_fm_shared_codec_volume_encode(E_Volume *v, int *size)
{
   return eet_data_descriptor_encode(_e_volume_edd, v, size);
}

E_Volume *
_e_fm_shared_codec_volume_decode(void *v, int size)
{
   return eet_data_descriptor_decode(_e_volume_edd, v, size);
}

void
_e_storage_volume_edd_init(void)
{
   _e_volume_edd = _e_volume_edd_new();
   _e_storage_edd = _e_storage_edd_new();
}

void
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
