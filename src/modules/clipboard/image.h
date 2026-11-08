#ifndef CLIPBOARD_IMAGE_H
#define CLIPBOARD_IMAGE_H

#include "common.h"

/* Images copied to the clipboard are stored as PNG files in
 *   $XDG_DATA_HOME/clipboard/images/<id>.png
 * (directory mode 0700, files 0600 - screenshots may contain secrets).
 * The history file only stores the <id>, a 16 digit lower case hex string
 * derived from the image data, which also gives us de-duplication. */
#define IMG_ID_LEN 16

Eina_Bool clip_image_id_valid(const char *id);
/* returns a newly allocated id or NULL */
char     *clip_image_id_new(const void *data, size_t len);
Eina_Bool clip_image_path_get(const char *id, char *buf, size_t n);
/* Checks the PNG signature and reads the size from the IHDR chunk */
Eina_Bool clip_image_png_info(const void *data, size_t len,
                              unsigned int *w, unsigned int *h);
Eina_Bool clip_image_store(const char *id, const void *data, size_t len);
Eina_Bool clip_image_load_info(const char *id, unsigned int *w,
                               unsigned int *h, size_t *size);
void      clip_image_remove(const char *id);
/* Removes cached files which are not referenced by 'items' */
void      clip_image_cleanup(const Eina_List *items);
/* Newly allocated label, for example "Image 1920x1080 (245 kB)" */
char     *clip_image_label_new(unsigned int w, unsigned int h, size_t size);

#endif
