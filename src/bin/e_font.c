/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "config.h"

/* TODO List:
 * - export to libe
 * - use e_path to search for available fonts
 */

static Evas_List *_e_font_defaults;	/* MRU <E_Font_Default> */
static Evas_List *_e_font_fallbacks;	/* <E_Font_Fallback> */

static Evas_List *_e_font_font_dir_available_get (Evas_List * available_fonts, const char *font_dir);

int
e_font_init(void)
{
   /* just get the pointers into the config */
   _e_font_defaults = e_config->font_defaults;
   _e_font_fallbacks = e_config->font_fallbacks;
   return 1;
}

int
e_font_shutdown(void)
{
   /* e_config will do this */
   return 1;
}

void
e_font_apply(void)
{
   char buf[PATH_MAX];
   Evas_List *next;
   E_Font_Fallback *eff;
   E_Font_Default *efd;
   
   /* setup edje fallback list */
   next = _e_font_fallbacks;
   if (next)
     {
	eff = evas_list_data(next);
	strncpy(buf, eff->name, PATH_MAX - 1);
	next = evas_list_next(next);
     }
   else
     {
	edje_fontset_append_set(NULL);
     }
   
   while (next)
     {
	eff = evas_list_data(next);
	strcat(buf, ",");
	strcat(buf, eff->name);
	next = evas_list_next(next);
     }
   edje_fontset_append_set(buf);
   
   /* setup edje text classes */
   next = _e_font_defaults;
   while (next)
     {
	efd = evas_list_data(next);
	edje_text_class_set(efd->text_class, efd->font, efd->size);
	next = evas_list_next(next);
     }
}

Evas_List *
e_font_available_list(void)
{
   Evas_List *available;
   
   available = NULL;
   /* use e_path for this */
   available = _e_font_font_dir_available_get(available, "~/.e/e/fonts");
   available = _e_font_font_dir_available_get(available, PACKAGE_DATA_DIR "/data/fonts");
   return available;
}

void
e_font_available_list_free(Evas_List * available)
{
   char *font_name;
   Evas_List *l;
   
   for (l = available; l; l = l->next)
     {
	font_name = evas_list_data(l);
	available = evas_list_remove(available, l);
	free(font_name);
     }
}

void
e_font_fallback_clear(void)
{
   Evas_List *next;
   
   next = _e_font_fallbacks;
   while (next)
     {
	E_Font_Fallback *eff;
	
	eff = evas_list_data(next);
	_e_font_fallbacks = evas_list_remove_list(_e_font_fallbacks, next);
	E_FREE(eff->name);
	E_FREE(eff);
	next = evas_list_next(next);
    }
}

void
e_font_fallback_append(const char *font)
{
   E_Font_Fallback *eff;
   
   e_font_fallback_remove (font);
   
   eff = E_NEW(E_Font_Fallback, 1);
   eff->name = strdup(font);
   _e_font_fallbacks = evas_list_append(_e_font_fallbacks, eff);
}

void
e_font_fallback_prepend(const char *font)
{
   E_Font_Fallback *eff;
   
   e_font_fallback_remove (font);
   
   eff = E_NEW(E_Font_Fallback, 1);
   eff->name = strdup(font);
   _e_font_fallbacks = evas_list_prepend(_e_font_fallbacks, eff);
}

void
e_font_fallback_remove(const char *font)
{
   Evas_List *next;
   
   next = _e_font_fallbacks;
   while (next)
     {
	E_Font_Fallback *eff;
	
	eff = evas_list_data(next);
	if (!strcmp(eff->name, font))
	  {
	     _e_font_fallbacks = evas_list_remove_list(_e_font_fallbacks, next);
	     E_FREE(eff->name);
	     E_FREE(eff);
	     break;
	  }
	next = evas_list_next(next);
     }
}

Evas_List *
e_font_fallback_list(void)
{
   return _e_font_fallbacks;
}

void
e_font_default_set(const char *text_class, const char *font, int size)
{
   E_Font_Default *efd;
   Evas_List *next;

   /* search for the text class */
   next = _e_font_defaults;
   while (next)
     {
	efd = evas_list_data(next);
	if (!strcmp(efd->text_class, text_class))
	  {
	     E_FREE(efd->font);
	     efd->font = strdup(font);
	     efd->size = size;
	     /* move to the front of the list */
	     _e_font_defaults = evas_list_remove_list(_e_font_defaults, next);
	     _e_font_defaults = evas_list_prepend(_e_font_defaults, efd);
	     return;
	  }
	next = evas_list_next(next);
     }

   /* the text class doesnt exist */
   efd = E_NEW(E_Font_Default, 1);
   efd->text_class = strdup(text_class);
   efd->font = strdup(font);
   efd->size = size;
   
   _e_font_defaults = evas_list_prepend(_e_font_defaults, efd);
}

/*
 * returns a pointer to the data, return null if nothing if found.
 */
E_Font_Default *
e_font_default_get(const char *text_class)
{
   E_Font_Default *efd;
   Evas_List *next;

   /* search for the text class */
   next = _e_font_defaults;
   while (next)
     {
	efd = evas_list_data(next);
	if (!strcmp(efd->text_class, text_class))
	  {
	     /* move to the front of the list */
	     _e_font_defaults = evas_list_remove_list(_e_font_defaults, next);
	     _e_font_defaults = evas_list_prepend(_e_font_defaults, efd);
	     return efd;
	  }
	next = evas_list_next(next);
     }
   return NULL;
}

void
e_font_default_remove(const char *text_class)
{
   E_Font_Default *efd;
   Evas_List *next;
   
   /* search for the text class */
   next = _e_font_defaults;
   while (next)
     {
	efd = evas_list_data(next);
	if (!strcmp(efd->text_class, text_class))
	  {
	     _e_font_defaults = evas_list_remove_list(_e_font_defaults, next);
	     E_FREE(efd->text_class);
	     E_FREE(efd->font);
	     E_FREE(efd);
	     return;
	  }
	next = evas_list_next(next);
    }
}

Evas_List *
e_font_default_list(void)
{
   return _e_font_defaults;
}

static Evas_List *
_e_font_font_dir_available_get(Evas_List * available_fonts, const char *font_dir)
{
   char buf[4096];
   FILE *f;
   
   sprintf (buf, "%s/fonts.alias", font_dir);
   f = fopen (buf, "r");
   if (f)
     {
	char fname[4096], fdef[4096];
	Evas_List *next;
	
	/* read font alias lines */
	while (fscanf(f, "%4090s %[^\n]\n", fname, fdef) == 2)
	  {
	     /* skip comments */
	     if ((fdef[0] == '!') || (fdef[0] == '#'))
	       continue;
	     
	     /* skip duplicates */
	     next = available_fonts;
	     while (next)
	       {
		  if (!strcmp((char *)evas_list_data(next), fname))
		    continue;
		  next = evas_list_next(next);
	       }
	     available_fonts = evas_list_append(available_fonts, strdup(fname));
	  }
	fclose (f);
     }
   return available_fonts;
}
