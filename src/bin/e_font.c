/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* TODO List:
 * - export to libe
 * - use e_path to search for available fonts
 */

static Evas_List *_e_font_font_dir_available_get (Evas_List * available_fonts, const char *font_dir);

static char _fn_buf[1024];

EAPI int
e_font_init(void)
{
   /* all init stuff is in e_config */
   return 1;
}

EAPI int
e_font_shutdown(void)
{
   /* e_config will do this */
   return 1;
}

EAPI void
e_font_apply(void)
{
   char buf[1024];
   Evas_List *l;
   E_Font_Fallback *eff;
   int blen, len;
   
   /* setup edje fallback list */
   blen = sizeof(buf) - 1;
   buf[0] = 0;
   buf[blen] = 0;
   l = e_config->font_fallbacks;
   if (l)
     {
	eff = evas_list_data(l);
	len = strlen(eff->name);
	if (len < blen)
	  {
	     strcpy(buf, eff->name);
	     blen -= len;
	  }
	for (l = evas_list_next(l); l; l = l->next)
	  {
	     eff = evas_list_data(l);
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
   for (l = e_config->font_defaults; l; l = l->next)
     {
	E_Font_Default *efd;

	efd = evas_list_data(l);
	edje_text_class_set(efd->text_class, efd->font, efd->size);
     }

   /* Update borders */
   for (l = e_border_client_list(); l; l = l->next)
     {
	E_Border *bd;

	bd = l->data;
	e_border_frame_recalc(bd);
     }
}

EAPI Evas_List *
e_font_available_list(void)
{
   Evas_List *dir_list;
   Evas_List *next;
   Evas_List *available;   

   dir_list = e_path_dir_list_get(path_fonts);
   available = NULL;
   for ( next = dir_list; next; next = next->next)
     {
        E_Path_Dir *epd = next->data;
	available = _e_font_font_dir_available_get(available, epd->dir);
     }
   
   e_path_dir_list_free(dir_list);
   return available;
}

EAPI void
e_font_available_list_free(Evas_List *available)
{
   E_Font_Available *efa;
   
   while (available)
     {
	efa = available->data;
	available = evas_list_remove_list(available, available);
	if (efa->name) evas_stringshare_del(efa->name);
	E_FREE(efa);	
     }
}

EAPI void
e_font_fallback_clear(void)
{
   E_Font_Fallback *eff;
    
   while (e_config->font_fallbacks)
     {	
	eff = e_config->font_fallbacks->data;
	e_config->font_fallbacks = evas_list_remove_list(
					e_config->font_fallbacks, 
					e_config->font_fallbacks);
	if (eff->name) evas_stringshare_del(eff->name);
	E_FREE(eff);
    }
}

EAPI void
e_font_fallback_append(const char *font)
{
   E_Font_Fallback *eff;
   
   e_font_fallback_remove (font);
   
   eff = E_NEW(E_Font_Fallback, 1);
   eff->name = evas_stringshare_add(font);
   e_config->font_fallbacks = evas_list_append(e_config->font_fallbacks, eff);
}

EAPI void
e_font_fallback_prepend(const char *font)
{
   E_Font_Fallback *eff;
   
   e_font_fallback_remove (font);
   
   eff = E_NEW(E_Font_Fallback, 1);
   eff->name = evas_stringshare_add(font);
   e_config->font_fallbacks = evas_list_prepend(e_config->font_fallbacks, eff);
}

EAPI void
e_font_fallback_remove(const char *font)
{
   Evas_List *next;

   for (next = e_config->font_fallbacks; next; next = next->next)
     {
	E_Font_Fallback *eff;
	
	eff = evas_list_data(next);
	if (!strcmp(eff->name, font))
	  {
	     e_config->font_fallbacks = evas_list_remove_list(
					e_config->font_fallbacks, next);
	     if (eff->name) evas_stringshare_del(eff->name);
	     E_FREE(eff);
	     break;
	  }
     }
}

EAPI Evas_List *
e_font_fallback_list(void)
{
   return e_config->font_fallbacks;
}

EAPI void
e_font_default_set(const char *text_class, const char *font, int size)
{
   E_Font_Default *efd;
   Evas_List *next;

   /* search for the text class */
   for (next = e_config->font_defaults; next; next = next->next)
     {
	efd = evas_list_data(next);
	if (!strcmp(efd->text_class, text_class))
	  {
	     if (efd->font) evas_stringshare_del(efd->font);
	     efd->font = evas_stringshare_add(font);
	     efd->size = size;
	     /* move to the front of the list */
	     e_config->font_defaults = evas_list_remove_list(
					e_config->font_defaults, next);
	     e_config->font_defaults = evas_list_prepend(
					e_config->font_defaults, efd);
	     return;
	  }
     }

   /* the text class doesnt exist */
   efd = E_NEW(E_Font_Default, 1);
   efd->text_class = evas_stringshare_add(text_class);
   efd->font = evas_stringshare_add(font);
   efd->size = size;
   
   e_config->font_defaults = evas_list_prepend(e_config->font_defaults, efd);
}

/*
 * returns a pointer to the data, return null if nothing if found.
 */
EAPI E_Font_Default *
e_font_default_get(const char *text_class)
{
   E_Font_Default *efd = NULL, *defd = NULL;
   Evas_List *next;

   /* search for the text class */
   for (next = e_config->font_defaults; next; next = next->next)
     {
	efd = evas_list_data(next);
	if (!strcmp(efd->text_class, text_class))
	  {
	     /* move to the front of the list */
	     e_config->font_defaults = evas_list_remove_list(
					e_config->font_defaults, next);
	     e_config->font_defaults = evas_list_prepend(
					e_config->font_defaults, efd);
	     return efd;
	  }
	if (!strcmp(efd->text_class, "default"))
	  defd = efd;
     }
   if (!defd)
     defd  = efd;
   return defd;
}

EAPI void
e_font_default_remove(const char *text_class)
{
   E_Font_Default *efd;
   Evas_List *next;
   
   /* search for the text class */
   for (next = e_config->font_defaults; next; next = next->next)
     {
	efd = evas_list_data(next);
	if (!strcmp(efd->text_class, text_class))
	  {
	     e_config->font_defaults = evas_list_remove_list(
					e_config->font_defaults, next);
	     if (efd->text_class) evas_stringshare_del(efd->text_class);
	     if (efd->font) evas_stringshare_del(efd->font);
	     E_FREE(efd);
	     return;
	  }
    }
}

EAPI Evas_List *
e_font_default_list(void)
{
   return e_config->font_defaults;
}

EAPI const char *
e_font_default_string_get(const char *text_class, int *size_ret)
{
   E_Font_Default *efd;
   Evas_List *next;
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
   
   next = e_config->font_fallbacks;
   while (next)
     {
	eff = evas_list_data(next);
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
	next = evas_list_next(next);
     }
   
   if (size_ret) *size_ret = efd->size;
   return _fn_buf;
}

static Evas_List *
_e_font_font_dir_available_get(Evas_List *available_fonts, const char *font_dir)
{
   char buf[4096];
   FILE *f;
   
   snprintf(buf, sizeof(buf), "%s/fonts.alias", font_dir);
   f = fopen(buf, "r");
   if (f)
     {
	char fname[4096], fdef[4096];
	Evas_List *next;
	
	/* read font alias lines */
	while (fscanf(f, "%4090s %[^\n]\n", fname, fdef) == 2)
	  {
	     E_Font_Available *efa;
	     
	     /* skip comments */
	     if ((fdef[0] == '!') || (fdef[0] == '#'))
	       continue;
	     
	     /* skip duplicates */
	     
	     for (next = available_fonts; next; next = evas_list_next(next))
	       {
	       	  efa = (E_Font_Available *)evas_list_data(next);
	       
		  if (!strcmp(efa->name, fname))
		    continue;		  
	       }
	        
	     efa = malloc(sizeof(E_Font_Available));
	     efa->name = evas_stringshare_add(fname);
	     available_fonts = evas_list_append(available_fonts, efa);
	  }
	fclose (f);
     }
   return available_fonts;
}
