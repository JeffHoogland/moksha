#include "image.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#define IMG_MAX_DIM 100000

static Eina_Bool
_image_dir_get(char *buf, size_t n)
{
   const char *home = efreet_data_home_get();
   int r;

   if (!home || home[0] != '/') return EINA_FALSE;
   r = snprintf(buf, n, "%s/clipboard/images", home);
   return (r > 0) && ((size_t) r < n);
}

static Eina_Bool
_image_dir_ensure(char *buf, size_t n)
{
   if (!_image_dir_get(buf, n)) return EINA_FALSE;
   if (!ecore_file_is_dir(buf))
     {
        if (!ecore_file_mkpath(buf)) return EINA_FALSE;
        if (chmod(buf, 0700) != 0)
          WRN("Unable to restrict permissions of %s", buf);
     }
   return EINA_TRUE;
}

Eina_Bool
clip_image_id_valid(const char *id)
{
   size_t i;

   if (!id || strlen(id) != IMG_ID_LEN) return EINA_FALSE;
   for (i = 0; i < IMG_ID_LEN; i++)
     {
        char c = id[i];
        if (!(((c >= '0') && (c <= '9')) || ((c >= 'a') && (c <= 'f'))))
          return EINA_FALSE;
     }
   return EINA_TRUE;
}

char *
clip_image_id_new(const void *data, size_t len)
{
   /* 64 bit FNV-1a, mixed with the length. Good enough for de-duplication. */
   const unsigned char *p = data;
   unsigned long long h = 14695981039346656037ULL;
   char buf[IMG_ID_LEN + 1];
   size_t i;

   if (!p || !len) return NULL;
   for (i = 0; i < len; i++)
     {
        h ^= p[i];
        h *= 1099511628211ULL;
     }
   h ^= (unsigned long long) len;
   h *= 1099511628211ULL;
   snprintf(buf, sizeof(buf), "%016llx", h);
   return strdup(buf);
}

Eina_Bool
clip_image_path_get(const char *id, char *buf, size_t n)
{
   char dir[PATH_MAX];
   int r;

   if (!clip_image_id_valid(id)) return EINA_FALSE;
   if (!_image_dir_get(dir, sizeof(dir))) return EINA_FALSE;
   r = snprintf(buf, n, "%s/%s.png", dir, id);
   return (r > 0) && ((size_t) r < n);
}

static Eina_Bool
_png_header(const unsigned char *b, size_t len, unsigned int *w, unsigned int *h)
{
   static const unsigned char sig[8] = { 0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a };
   unsigned int ww, hh;

   if (!b || len < 24) return EINA_FALSE;
   if (memcmp(b, sig, 8) || memcmp(b + 12, "IHDR", 4)) return EINA_FALSE;
   ww = ((unsigned int) b[16] << 24) | ((unsigned int) b[17] << 16) |
        ((unsigned int) b[18] << 8)  |  (unsigned int) b[19];
   hh = ((unsigned int) b[20] << 24) | ((unsigned int) b[21] << 16) |
        ((unsigned int) b[22] << 8)  |  (unsigned int) b[23];
   if (!ww || !hh || (ww > IMG_MAX_DIM) || (hh > IMG_MAX_DIM)) return EINA_FALSE;
   if (w) *w = ww;
   if (h) *h = hh;
   return EINA_TRUE;
}

Eina_Bool
clip_image_png_info(const void *data, size_t len, unsigned int *w, unsigned int *h)
{
   return _png_header(data, len, w, h);
}

Eina_Bool
clip_image_store(const char *id, const void *data, size_t len)
{
   char dir[PATH_MAX], path[PATH_MAX], tmp[PATH_MAX + 8];
   size_t done = 0;
   int fd;

   if (!data || !len) return EINA_FALSE;
   if (!clip_image_path_get(id, path, sizeof(path))) return EINA_FALSE;
   if (!_image_dir_ensure(dir, sizeof(dir))) return EINA_FALSE;
   /* Same id == same image, nothing to do */
   if (ecore_file_exists(path)) return EINA_TRUE;

   if (snprintf(tmp, sizeof(tmp), "%s.tmp", path) >= (int) sizeof(tmp))
     return EINA_FALSE;
   fd = open(tmp, O_WRONLY | O_CREAT | O_TRUNC, 0600);
   if (fd < 0)
     {
        ERR("Unable to create %s: %s", tmp, strerror(errno));
        return EINA_FALSE;
     }
   while (done < len)
     {
        ssize_t w = write(fd, (const char *) data + done, len - done);
        if (w < 0)
          {
             if (errno == EINTR) continue;
             break;
          }
        done += (size_t) w;
     }
   if ((close(fd) != 0) || (done != len))
     {
        ERR("Unable to write %s", tmp);
        unlink(tmp);
        return EINA_FALSE;
     }
   /* write to a temporary file first so a crash never leaves a half image */
   if (rename(tmp, path) != 0)
     {
        ERR("Unable to rename %s: %s", tmp, strerror(errno));
        unlink(tmp);
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

Eina_Bool
clip_image_load_info(const char *id, unsigned int *w, unsigned int *h, size_t *size)
{
   unsigned char hdr[24];
   char path[PATH_MAX];
   long long fsize;
   Eina_Bool ok;
   FILE *f;

   if (!clip_image_path_get(id, path, sizeof(path))) return EINA_FALSE;
   fsize = ecore_file_size(path);
   if (fsize <= 0) return EINA_FALSE;
   f = fopen(path, "rb");
   if (!f) return EINA_FALSE;
   ok = (fread(hdr, 1, sizeof(hdr), f) == sizeof(hdr));
   fclose(f);
   if (!ok || !_png_header(hdr, sizeof(hdr), w, h)) return EINA_FALSE;
   if (size) *size = (size_t) fsize;
   return EINA_TRUE;
}

void
clip_image_remove(const char *id)
{
   char path[PATH_MAX];

   if (clip_image_path_get(id, path, sizeof(path)))
     ecore_file_unlink(path);
}

static Eina_Bool
_id_referenced(const Eina_List *items, const char *id)
{
   const Eina_List *l;
   const Clip_Data *cd;

   EINA_LIST_FOREACH(items, l, cd)
     if ((cd->type == CLIP_TYPE_IMAGE) && cd->content && !strcmp(cd->content, id))
       return EINA_TRUE;
   return EINA_FALSE;
}

void
clip_image_cleanup(const Eina_List *items)
{
   char dir[PATH_MAX], path[PATH_MAX], id[IMG_ID_LEN + 1];
   Eina_List *files;
   char *f;

   if (!_image_dir_get(dir, sizeof(dir)) || !ecore_file_is_dir(dir)) return;
   files = ecore_file_ls(dir);
   EINA_LIST_FREE(files, f)
     {
        size_t len = strlen(f);

        if ((len == IMG_ID_LEN + 4) && !strcmp(f + IMG_ID_LEN, ".png"))
          {
             memcpy(id, f, IMG_ID_LEN);
             id[IMG_ID_LEN] = 0;
             if (clip_image_id_valid(id) && !_id_referenced(items, id))
               clip_image_remove(id);
          }
        else if ((len > 4) && !strcmp(f + len - 4, ".tmp"))
          {
             if (snprintf(path, sizeof(path), "%s/%s", dir, f) < (int) sizeof(path))
               ecore_file_unlink(path);
          }
        free(f);
     }
}

char *
clip_image_label_new(unsigned int w, unsigned int h, size_t size)
{
   char buf[128];

   if (size >= 1024 * 1024)
     snprintf(buf, sizeof(buf), "%s %ux%u (%.1f MB)", _("Image"), w, h,
              (double) size / (1024.0 * 1024.0));
   else
     snprintf(buf, sizeof(buf), "%s %ux%u (%u kB)", _("Image"), w, h,
              (unsigned int) ((size + 1023) / 1024));
   return strdup(buf);
}
