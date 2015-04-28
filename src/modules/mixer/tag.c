#include "pa.h"

uint8_t *
tag_uint32(Pulse_Tag *tag, uint32_t val)
{
   uint8_t *ret;

   ret = tag->data + tag->size;
   *ret = PA_TAG_U32;
   val = htonl(val);
   memcpy(ret + 1, &val, sizeof(val));
   ret += PA_TAG_SIZE_U32;
   tag->size = ret - tag->data;
   return ret;
}

uint8_t *
untag_uint32(Pulse_Tag *tag, uint32_t *val)
{
   uint8_t *ret;

   ret = tag->data + tag->size;
   if ((*ret != PA_TAG_U32) && (*ret != PA_TAG_VOLUME)) return 0;
   memcpy(val, ret + 1, sizeof(uint32_t));
   *val = ntohl(*val);
   ret += PA_TAG_SIZE_U32;
   tag->size = ret - tag->data;
   return ret;
}

uint8_t *
tag_bool(Pulse_Tag *tag, Eina_Bool val)
{
   uint8_t *ret;

   ret = tag->data + tag->size;
   *ret = (uint8_t)(val ? PA_TAG_BOOLEAN_TRUE : PA_TAG_BOOLEAN_FALSE);
   ret += PA_TAG_SIZE_BOOLEAN;
   tag->size = ret - tag->data;
   return ret;
}

uint8_t *
untag_bool(Pulse_Tag *tag, Eina_Bool *val)
{
   uint8_t *ret;

   ret = tag->data + tag->size;
   if ((*ret != PA_TAG_BOOLEAN_TRUE) && (*ret != PA_TAG_BOOLEAN_FALSE)) return 0;
   *val = !!(*ret == PA_TAG_BOOLEAN_TRUE) ? EINA_TRUE : EINA_FALSE;
   ret += PA_TAG_SIZE_BOOLEAN;
   tag->size = ret - tag->data;
   return ret;
}

uint8_t *
tag_string(Pulse_Tag *tag, const char *val)
{
   uint8_t *ret;

   ret = tag->data + tag->size;
   if (val)
     {
        *ret = PA_TAG_STRING;
        strcpy((char*)ret + 1, val);
        ret += PA_TAG_SIZE_STRING + strlen(val);
        tag->size = ret - tag->data;
     }
   else
     *ret = PA_TAG_STRING_NULL, tag->size++;
   return ret;
}

uint8_t *
untag_string(Pulse_Tag *tag, const char **val)
{
   uint8_t *ret;

   ret = tag->data + tag->size;
   switch (*ret)
     {
      case PA_TAG_STRING:
        eina_stringshare_replace(val, (char*)ret + 1);
        ret += PA_TAG_SIZE_STRING + strlen(*val);
        break;
      case PA_TAG_STRING_NULL:
        *val = NULL;
        ret += PA_TAG_SIZE_STRING_NULL;
        break;
      default:
        return 0;
     }
   tag->size = ret - tag->data;
   return ret;
}

uint8_t *
untag_sample(Pulse_Tag *tag, pa_sample_spec *spec)
{
   uint8_t *ret;

   ret = tag->data + tag->size;
   if (*ret != PA_TAG_SAMPLE_SPEC) return 0;

   spec->format = ret[1];
   spec->channels = ret[2];
   memcpy(&spec->rate, ret + 3, sizeof(spec->rate));
   spec->rate = ntohl(spec->rate);
   ret += PA_TAG_SIZE_SAMPLE_SPEC, tag->size += PA_TAG_SIZE_SAMPLE_SPEC;
   return ret;
}

uint8_t *
untag_channel_map(Pulse_Tag *tag, pa_channel_map *map)
{
   uint8_t *ret;
   unsigned int x;

   ret = tag->data + tag->size;
   if (*ret != PA_TAG_CHANNEL_MAP) return 0;

   map->channels = ret[1];
   if (map->channels > PA_CHANNELS_MAX) return 0;
   if (map->channels + 2 + tag->size > tag->dsize) return 0;

   for (ret += 2, x = 0; x < map->channels; ret++, x++)
     map->map[x] = (int8_t)*ret;

   tag->size = ret - tag->data;
   return ret;
}

uint8_t *
untag_usec(Pulse_Tag *tag, uint64_t *val)
{
   uint8_t *ret;
   uint32_t tmp;

   ret = tag->data + tag->size;
   if (*ret != PA_TAG_USEC) return 0;

   memcpy(&tmp, ret + 1, 4);
   *val = (uint64_t)ntohl(tmp) << 32;
   memcpy(&tmp, ret + 5, 4);
   *val |= (uint64_t)ntohl(tmp);
   ret += PA_TAG_SIZE_USEC;
   tag->size = ret - tag->data;
   return ret;
}

uint8_t *
tag_arbitrary(Pulse_Tag *tag, const void *val, uint32_t size)
{
   uint8_t *ret;
   uint32_t tmp;

   ret = tag->data + tag->size;
   *ret = PA_TAG_ARBITRARY;
   tmp = htonl(size);
   memcpy(ret + 1, &tmp, sizeof(uint32_t));
   memcpy(ret + PA_TAG_SIZE_U32, val, size);
   ret += PA_TAG_SIZE_ARBITRARY + size;
   tag->size = ret - tag->data;
   return ret;
}

uint8_t *
untag_arbitrary(Pulse_Tag *tag, Eina_Binbuf **val)
{
   uint8_t *ret;
   uint32_t len;

   if (!untag_uint32(tag, &len)) return 0;
   ret = tag->data + tag->size;
   if (*ret != PA_TAG_ARBITRARY) return 0;
   ret += PA_TAG_SIZE_ARBITRARY;
   
   *val = eina_binbuf_new();
   eina_binbuf_append_length(*val, ret, len);
   ret += len;
   tag->size = ret - tag->data;
   return ret;
}

uint8_t *
tag_simple_init(Pulse *conn, Pulse_Tag *tag, uint32_t val, PA_Tag type)
{
   switch (type)
     {
      case PA_TAG_U32:
         tag_uint32(tag, val);
         return tag_uint32(tag, conn->tag_count++);
      default:
        break;
     }
   return NULL;
}

static Eina_Bool
tag_proplist_foreach(const Eina_Hash *h __UNUSED__, const char *key, const char *val, Pulse_Tag *tag)
{
   size_t size;

   tag_string(tag, key);
   size = strlen(val) + 1;
   tag_uint32(tag, size);
   tag_arbitrary(tag, val, size);
   return EINA_TRUE;
}

uint8_t *
tag_proplist(Pulse_Tag *tag)
{
   uint8_t *ret;

   ret = tag->data + tag->size;
   *ret = PA_TAG_PROPLIST, tag->size++;
   
   eina_hash_foreach(tag->props, (Eina_Hash_Foreach)tag_proplist_foreach, tag);
   return tag_string(tag, NULL);
}

uint8_t *
untag_proplist(Pulse_Tag *tag, Eina_Hash **props)
{
   uint8_t *ret;

   ret = tag->data + tag->size;
   if (*ret != PA_TAG_PROPLIST) return 0;

   *props = eina_hash_string_superfast_new((Eina_Free_Cb)eina_binbuf_free);
   for (++tag->size; *ret != PA_TAG_STRING_NULL && tag->size < tag->dsize - 1;)
     {
        const char *key = NULL;
        Eina_Binbuf *val;
        EINA_SAFETY_ON_FALSE_GOTO(untag_string(tag, &key), error);
        EINA_SAFETY_ON_FALSE_GOTO(untag_arbitrary(tag, &val), error);
#if 0
        {
           char buf[128];
           snprintf(buf, sizeof(buf), "key='%%s', val='%%.%zus'", eina_binbuf_length_get(val));
           DBG(buf, key, eina_binbuf_string_get(val));
        }
#endif
        eina_hash_add(*props, key, val);
        eina_stringshare_del(key);
        ret = tag->data + tag->size;
     }
   tag->size++;
   return ++ret;
error:
   eina_hash_free(*props);
   return 0;
}

uint8_t *
tag_volume(Pulse_Tag *tag, uint8_t channels, double vol)
{
   uint32_t pa_vol;
   uint8_t *ret, x;

   if (vol <= 0.0) pa_vol = PA_VOLUME_MUTED;
   else pa_vol = ((vol * PA_VOLUME_NORM) - (PA_VOLUME_NORM / 2)) / 100;
   pa_vol = htonl(pa_vol);
   ret = tag->data + tag->size;
   *ret++ = PA_TAG_CVOLUME;
   *ret++ = channels;
   for (x = 0; x < channels; x++, ret += sizeof(pa_vol))
     memcpy(ret, &pa_vol, sizeof(pa_vol));
   tag->size = ret - tag->data;
   return ret;
}

uint8_t *
tag_cvol(Pulse_Tag *tag, pa_cvolume *c)
{
   uint8_t *ret, x;
   uint32_t pa_vol;

   ret = tag->data + tag->size;
   *ret++ = PA_TAG_CVOLUME;
   *ret++ = c->channels;
   for (x = 0; x < c->channels; x++, ret += sizeof(pa_vol))
     {
        pa_vol = htonl(c->values[x]);
        memcpy(ret, &pa_vol, sizeof(pa_vol));
     }
   tag->size = ret - tag->data;
   return ret;
}

uint8_t *
untag_cvol(Pulse_Tag *tag, pa_cvolume *cvol)
{
   uint32_t pa_vol;
   uint8_t *ret, x;

   ret = tag->data + tag->size;
   if (*ret != PA_TAG_CVOLUME) return 0;
   cvol->channels = ret[1];
   for (x = 0, ret += 2; x < cvol->channels; x++, ret += sizeof(pa_vol))
     {
        memcpy(&pa_vol, ret, sizeof(pa_vol));
        cvol->values[x] = ntohl(pa_vol);
     }
     
   tag->size = ret - tag->data;
   return ret;
}

void
tag_finish(Pulse_Tag *tag)
{
   EINA_SAFETY_ON_NULL_RETURN(tag);
   tag->header[PA_PSTREAM_DESCRIPTOR_CHANNEL] = htonl((uint32_t) -1);
   tag->header[PA_PSTREAM_DESCRIPTOR_LENGTH] = htonl(tag->dsize);
}

void
pulse_tag_free(Pulse_Tag *tag)
{
   if (!tag) return;
   free(tag->data);
   if (tag->props) eina_hash_free(tag->props);
   free(tag);
}
