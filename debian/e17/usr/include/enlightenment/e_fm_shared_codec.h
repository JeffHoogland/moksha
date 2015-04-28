#ifndef E_FM_SHARED_CODEC
#define E_FM_SHARED_CODEC

#include <Eet.h>

void *_e_fm_shared_codec_storage_encode(E_Storage *s, int *size);
E_Storage *_e_fm_shared_codec_storage_decode(void *s, int size);
void *_e_fm_shared_codec_volume_encode(E_Volume *v, int *size);
E_Volume *_e_fm_shared_codec_volume_decode(void *v, int size);

Eet_Data_Descriptor *_e_volume_edd_new(void);
Eet_Data_Descriptor *_e_storage_edd_new(void);
void _e_storage_volume_edd_init(void);
void _e_storage_volume_edd_shutdown(void);


#endif
