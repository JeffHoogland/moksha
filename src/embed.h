#ifndef E_EMBED_H
#define E_EMBED_H

#include "e.h"
#include "text.h"

typedef void * Embed;

Embed e_embed_text(Ebits_Object o, char *bit_name, Evas evas, E_Text *text_obj, int clip_x, int clip_y);
Embed e_embed_image_object(Ebits_Object o, char *bit_name, Evas evas, Evas_Object image_obj);
void  e_embed_free(Embed em);

#endif
