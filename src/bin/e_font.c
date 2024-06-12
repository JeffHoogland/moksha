#include "e.h"

#define E_TOK_STYLE ":style="

static Eina_Bool          _font_hash_free_cb(const Eina_Hash *hash __UNUSED__, const void *key __UNUSED__, void *data, void *fdata __UNUSED__);
static Eina_Hash         *_e_font_available_hash_add(Eina_Hash *font_hash, const char *full_name);
static E_Font_Properties *_e_font_fontconfig_name_parse(Eina_Hash **font_hash, E_Font_Properties *efp, const char *font);
static char _fn_buf[1024];

EINTERN int
e_font_init(void)
{
   /* all init stuff is in e_config */
   return 1;
}

EINTERN int
e_font_shutdown(void)
{
   /* e_config will do this */
   return 1;
}

EAPI void
e_font_apply(void)
{
   char buf[1024];
   Eina_List *l;
   E_Border *bd;
   E_Font_Default *efd;
   E_Font_Fallback *eff;
   int blen, len;

   /* setup edje fallback list */
   blen = sizeof(buf) - 1;
   buf[0] = 0;
   buf[blen] = 0;
   l = e_config->font_fallbacks;
   if (l)
     {
        eff = eina_list_data_get(l);
        len = strlen(eff->name);
        if (len < blen)
          {
             strcpy(buf, eff->name);
             blen -= len;
          }
        EINA_LIST_FOREACH(eina_list_next(l), l, eff)
          {
             len = 1;
             if (len < blen)
               {
                  strcat(buf, ",");
                  blen -= len;
               }
             len = strlen(eff->name);
             if (len < blen)
               {
                  strcat(buf, eff->name);
                  blen -= len;
               }
          }
        edje_fontset_append_set(buf);
     }
   else
     edje_fontset_append_set(NULL);

   /* setup edje text classes */
   EINA_LIST_FOREACH(e_config->font_defaults, l, efd)
     {
        edje_text_class_set(efd->text_class, efd->font, efd->size);
     }

   /* Update borders */
   EINA_LIST_FOREACH(e_border_client_list(), l, bd)
     {
        e_border_frame_recalc(bd);
     }
}

EAPI Eina_List *
e_font_available_list(void)
{
   Eina_List *evas_fonts;
   Eina_List *e_fonts;
   Eina_List *l;
   const char *evas_font;
   E_Manager *man;
   E_Container *con;

   man = e_manager_current_get();
   if (!man) return NULL;
   con = e_container_current_get(man);
   if (!con) con = e_container_number_get(man, 0);
   if (!con) return NULL;

   evas_fonts = evas_font_available_list(con->bg_evas);

   e_fonts = NULL;
   EINA_LIST_FOREACH(evas_fonts, l, evas_font)
     {
        E_Font_Available *efa;

        efa = E_NEW(E_Font_Available, 1);
        efa->name = eina_stringshare_add(evas_font);
        e_fonts = eina_list_append(e_fonts, efa);
     }

   evas_font_available_list_free(con->bg_evas, evas_fonts);

   return e_fonts;
}

EAPI void
e_font_available_list_free(Eina_List *available)
{
   E_Font_Available *efa;

   EINA_LIST_FREE(available, efa)
     {
        if (efa->name) eina_stringshare_del(efa->name);
        E_FREE(efa);
     }
}

EAPI void
e_font_properties_free(E_Font_Properties *efp)
{
   const char *str;

   EINA_LIST_FREE(efp->styles, str)
     {
        if (str) eina_stringshare_del(str);
     }
   if (efp->name) eina_stringshare_del(efp->name);
   free(efp);
}

static Eina_Bool
_font_hash_free_cb(const Eina_Hash *hash __UNUSED__, const void *key __UNUSED__, void *data, void *fdata __UNUSED__)
{
   E_Font_Properties *efp;

   efp = data;
   e_font_properties_free(efp);
   return 1;
}

EAPI void
e_font_available_hash_free(Eina_Hash *hash)
{
   eina_hash_foreach(hash, _font_hash_free_cb, NULL);
   eina_hash_free(hash);
}

EAPI E_Font_Properties *
e_font_fontconfig_name_parse(const char *font)
{
   if (!font) return NULL;
   return _e_font_fontconfig_name_parse(NULL, NULL, font);
}

static E_Font_Properties *
_e_font_fontconfig_name_parse(Eina_Hash **font_hash, E_Font_Properties *efp, const char *font)
{
   char *s1;

   s1 = strchr(font, ':');
   if (s1)
     {
        char *s2, *name, *style;
        int len;

        len = s1 - font;
        name = calloc(len + 1, sizeof(char));
        strncpy(name, font, len);

        /* Get subname (should be english)  */
        s2 = strchr(name, ',');
        if (s2)
          {
             len = s2 - name;
             name = realloc(name, sizeof(char) * len + 1);
             memset(name, 0, sizeof(char) * len + 1);
             strncpy(name, font, len);
          }

        if (strncmp(s1, E_TOK_STYLE, strlen(E_TOK_STYLE)) == 0)
          {
             style = s1 + strlen(E_TOK_STYLE);

             if (font_hash) efp = eina_hash_find(*font_hash, name);
             if (!efp)
               {
                  efp = calloc(1, sizeof(E_Font_Properties));
                  efp->name = eina_stringshare_add(name);
                  if (font_hash)
                    {
                       if (!*font_hash) *font_hash = eina_hash_string_superfast_new(NULL);
                       eina_hash_add(*font_hash, name, efp);
                    }
               }
             s2 = strchr(style, ',');
             if (s2)
               {
                  char *style_old;

                  len = s2 - style;
                  style_old = style;
                  style = calloc(len + 1, sizeof(char));
                  strncpy(style, style_old, len);
                  efp->styles = eina_list_append(efp->styles, eina_stringshare_add(style));
                  free(style);
               }
             else
               efp->styles = eina_list_append(efp->styles, eina_stringshare_add(style));
          }
        free(name);
     }
   else
     {
        if (font_hash) efp = eina_hash_find(*font_hash, font);
        if (!efp)
          {
             efp = calloc(1, sizeof(E_Font_Properties));
             efp->name = eina_stringshare_add(font);
             if (font_hash)
               {
                  if (!*font_hash) *font_hash = eina_hash_string_superfast_new(NULL);
                  eina_hash_add(*font_hash, font, efp);
               }
          }
     }
   return efp;
}

static Eina_Hash *
_e_font_available_hash_add(Eina_Hash *font_hash, const char *full_name)
{
   _e_font_fontconfig_name_parse(&font_hash, NULL, full_name);
   return font_hash;
}

EAPI Eina_Hash *
e_font_available_list_parse(Eina_List *list)
{
   Eina_Hash *font_hash;
   Eina_List *next;
   void *key;

   font_hash = NULL;

   /* Populate Default Font Families */
   font_hash = _e_font_available_hash_add(font_hash, "Sans:style=Regular");
   font_hash = _e_font_available_hash_add(font_hash, "Sans:style=Bold");
   font_hash = _e_font_available_hash_add(font_hash, "Sans:style=Oblique");
   font_hash = _e_font_available_hash_add(font_hash, "Sans:style=Bold Oblique");

   font_hash = _e_font_available_hash_add(font_hash, "Serif:style=Regular");
   font_hash = _e_font_available_hash_add(font_hash, "Serif:style=Bold");
   font_hash = _e_font_available_hash_add(font_hash, "Serif:style=Oblique");
   font_hash = _e_font_available_hash_add(font_hash, "Serif:style=Bold Oblique");

   font_hash = _e_font_available_hash_add(font_hash, "Monospace:style=Regular");
   font_hash = _e_font_available_hash_add(font_hash, "Monospace:style=Bold");
   font_hash = _e_font_available_hash_add(font_hash, "Monospace:style=Oblique");
   font_hash = _e_font_available_hash_add(font_hash, "Monospace:style=Bold Oblique");

   EINA_LIST_FOREACH(list, next, key)
     font_hash = _e_font_available_hash_add(font_hash, key);

   return font_hash;
}

EAPI const char *
e_font_fontconfig_name_get(const char *name, const char *style)
{
   char buf[256];

   if (!name) return NULL;
   if (!style || style[0] == 0) return eina_stringshare_add(name);
   snprintf(buf, 256, "%s"E_TOK_STYLE "%s", name, style);
   return eina_stringshare_add(buf);
}

EAPI void
e_font_fallback_clear(void)
{
   E_Font_Fallback *eff;

   EINA_LIST_FREE(e_config->font_fallbacks, eff)
     {
        if (eff->name) eina_stringshare_del(eff->name);
        E_FREE(eff);
     }
}

EAPI void
e_font_fallback_append(const char *font)
{
   E_Font_Fallback *eff;

   e_font_fallback_remove(font);

   eff = E_NEW(E_Font_Fallback, 1);
   eff->name = eina_stringshare_add(font);
   e_config->font_fallbacks = eina_list_append(e_config->font_fallbacks, eff);
}

EAPI void
e_font_fallback_prepend(const char *font)
{
   E_Font_Fallback *eff;

   e_font_fallback_remove(font);

   eff = E_NEW(E_Font_Fallback, 1);
   eff->name = eina_stringshare_add(font);
   e_config->font_fallbacks = eina_list_prepend(e_config->font_fallbacks, eff);
}

EAPI void
e_font_fallback_remove(const char *font)
{
   Eina_List *next;
   E_Font_Fallback *eff;

   EINA_LIST_FOREACH(e_config->font_fallbacks, next, eff)
     {
        if (!e_util_strcmp(eff->name, font))
          {
             e_config->font_fallbacks =
               eina_list_remove_list(e_config->font_fallbacks, next);
             if (eff->name) eina_stringshare_del(eff->name);
             E_FREE(eff);
             break;
          }
     }
}

EAPI Eina_List *
e_font_fallback_list(void)
{
   return e_config->font_fallbacks;
}

EAPI void
e_font_default_set(const char *text_class, const char *font, Evas_Font_Size size)
{
   E_Font_Default *efd;
   Eina_List *next;

   /* search for the text class */
   EINA_LIST_FOREACH(e_config->font_defaults, next, efd)
     {
        if (!strcmp(efd->text_class, text_class))
          {
             if (efd->font) eina_stringshare_del(efd->font);
             efd->font = eina_stringshare_add(font);
             efd->size = size;
             /* move to the front of the list */
             e_config->font_defaults =
               eina_list_remove_list(e_config->font_defaults, next);
             e_config->font_defaults =
               eina_list_prepend(e_config->font_defaults, efd);
             return;
          }
     }

   /* the text class doesn't exist */
   efd = E_NEW(E_Font_Default, 1);
   efd->text_class = eina_stringshare_add(text_class);
   efd->font = eina_stringshare_add(font);
   efd->size = size;

   e_config->font_defaults = eina_list_prepend(e_config->font_defaults, efd);
}

/*
 * returns a pointer to the data, return null if nothing if found.
 */
EAPI E_Font_Default *
e_font_default_get(const char *text_class)
{
   E_Font_Default *efd = NULL, *defd = NULL;
   Eina_List *next;

   /* search for the text class */
   EINA_LIST_FOREACH(e_config->font_defaults, next, efd)
     {
        if (!strcmp(efd->text_class, text_class))
          {
             /* move to the front of the list */
             e_config->font_defaults =
               eina_list_remove_list(e_config->font_defaults, next);
             e_config->font_defaults =
               eina_list_prepend(e_config->font_defaults, efd);
             return efd;
          }
        if (!strcmp(efd->text_class, "default")) defd = efd;
     }
   if (!defd) defd = efd;
   return defd;
}

EAPI void
e_font_default_remove(const char *text_class)
{
   E_Font_Default *efd;
   Eina_List *next;

   /* search for the text class */
   EINA_LIST_FOREACH(e_config->font_defaults, next, efd)
     {
        if (!e_util_strcmp(efd->text_class, text_class))
          {
             e_config->font_defaults =
               eina_list_remove_list(e_config->font_defaults, next);
             if (efd->text_class) eina_stringshare_del(efd->text_class);
             if (efd->font) eina_stringshare_del(efd->font);
             E_FREE(efd);
             edje_text_class_del(text_class);
             return;
          }
     }
}

EAPI Eina_List *
e_font_default_list(void)
{
   return e_config->font_defaults;
}

/* return the default font name with fallbacks, font size is returned
 * in size_ret. This function is needed when all hell breaks loose and
 * we need a font name and size.
 */
EAPI const char *
e_font_default_string_get(const char *text_class, Evas_Font_Size *size_ret)
{
   E_Font_Default *efd;
   Eina_List *next;
   E_Font_Fallback *eff;
   int blen, len;

   _fn_buf[0] = 0;
   efd = e_font_default_get(text_class);
   if (!efd)
     {
        if (size_ret) *size_ret = 0;
        return "";
     }
   blen = sizeof(_fn_buf) - 1;

   len = strlen(efd->font);
   if (len < blen)
     {
        strcpy(_fn_buf, efd->font);
        blen -= len;
     }

   EINA_LIST_FOREACH(e_config->font_fallbacks, next, eff)
     {
        len = 1;
        if (len < blen)
          {
             strcat(_fn_buf, ",");
             blen -= len;
          }
        len = strlen(eff->name);
        if (len < blen)
          {
             strcat(_fn_buf, eff->name);
             blen -= len;
          }
     }

   if (size_ret) *size_ret = efd->size;
   return _fn_buf;
}

