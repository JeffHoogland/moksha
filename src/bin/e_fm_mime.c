/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Mime E_Mime;

struct _E_Mime
{
   const char *glob;
   const char *mime;
};

/* local subsystem functions */
static void _e_fm_mime_all_free(void);
static void _e_fm_mime_update(void);
static int _e_fm_mime_glob_remove(const char *glob);
static void _e_fm_mime_mime_types_load(char *file);
static void _e_fm_mime_shared_mimeinfo_globs_load(char *file);

static Evas_List *mimes = NULL;

/* externally accessible functions */
EAPI const char *
e_fm_mime_filename_get(const char *fname)
{
   Evas_List *l;
   E_Mime *mime;
   
   _e_fm_mime_update();
   /* case senstive match first */
   for (l = mimes; l; l = l->next)
     {
	mime = l->data;
	if (e_util_glob_match(fname, mime->glob))
	  {
	     mimes = evas_list_remove_list(mimes, l);
	     mimes = evas_list_prepend(mimes, mime);
	     return mime->mime;
	  }
     }
   /* case insenstive match second */
   for (l = mimes; l; l = l->next)
     {
	mime = l->data;
	if (e_util_glob_case_match(fname, mime->glob))
	  {
	     mimes = evas_list_remove_list(mimes, l);
	     mimes = evas_list_prepend(mimes, mime);
	     return mime->mime;
	  }
     }
   return NULL;
}

/* local subsystem functions */
static void
_e_fm_mime_all_free(void)
{
   E_Mime *mime;
	
   while (mimes)
     {
	mime = mimes->data;
	mimes = evas_list_remove_list(mimes, mimes);
	evas_stringshare_del(mime->glob);
	evas_stringshare_del(mime->mime);
	free(mime);
     }
}

static void
_e_fm_mime_update(void)
{
   static double last_t = 0.0, t;
   char buf[4096];
   char *homedir;
   int reload = 0;
   
   /* load /etc/mime.types
    * load /usr/share/mime/
    * 
    * load ~/.mime.types
    * load ~/.local/share/mime/
    */
   t = ecore_time_get();
   if ((t - last_t) < 1.0) return;

   homedir = e_user_homedir_get();
   if (!homedir) return;
   
     {
	static time_t last_changed = 0;
	time_t ch;
	
	snprintf(buf, sizeof(buf), "/etc/mime.types");
	ch = ecore_file_mod_time(buf);
	if ((ch != last_changed) || (reload))
	  {
	     _e_fm_mime_all_free();
	     last_changed = ch;
	     _e_fm_mime_mime_types_load(buf);
	     reload = 1;
	  }
     }

     {
	static time_t last_changed = 0;
	time_t ch;
	
	snprintf(buf, sizeof(buf), "/usr/share/mime/globs");
	ch = ecore_file_mod_time(buf);
	if ((ch != last_changed) || (reload))
	  {
	     last_changed = ch;
	     _e_fm_mime_shared_mimeinfo_globs_load(buf);
	  }
     }

     {
	static time_t last_changed = 0;
	time_t ch;
	
	snprintf(buf, sizeof(buf), "%s/.mime.types", homedir);
	ch = ecore_file_mod_time(buf);
	if ((ch != last_changed) || (reload))
	  {
	     last_changed = ch;
	     _e_fm_mime_mime_types_load(buf);
	  }
     }
   
     {
	static time_t last_changed = 0;
	time_t ch;
	
	snprintf(buf, sizeof(buf), "%s/.local/share/mime/globs", homedir);
	ch = ecore_file_mod_time(buf);
	if ((ch != last_changed) || (reload))
	  {
	     last_changed = ch;
	     _e_fm_mime_shared_mimeinfo_globs_load(buf);
	  }
     }
   free(homedir);
}

static int
_e_fm_mime_glob_remove(const char *glob)
{
   Evas_List *l;
   E_Mime *mime;
   
   for (l = mimes; l; l = l->next)
     {
	mime = l->data;
	if (!strcmp(glob, mime->glob))
	  {
	     mimes = evas_list_remove_list(mimes, l);
	     evas_stringshare_del(mime->glob);
	     evas_stringshare_del(mime->mime);
	     free(mime);
	     return 1;
	  }
     }
   return 0;
}

static void
_e_fm_mime_mime_types_load(char *file)
{
   /* format:

    #  type of encoding.
    #
    ###############################################################################

    application/msaccess                            mdb
    application/msword                              doc dot
    application/news-message-id

    */
   FILE *f;
   char buf[4096], buf2[4096], mimetype[4096], ext[4096], *p, *pp;
   E_Mime *mime;
   
   f = fopen(file, "rb");
   if (!f) return;
   while (fgets(buf, sizeof(buf), f))
     {
	p = buf;
	while (isblank(*p) && (*p != 0) && (*p != '\n')) p++;
	if (*p == '#') continue;
	if ((*p == '\n') || (*p == 0)) continue;
	pp = p;
	while (!isblank(*p) && (*p != 0) && (*p != '\n')) p++;
	if ((*p == '\n') || (*p == 0)) continue;
	strncpy(mimetype, pp, (p - pp));
	mimetype[p - pp] = 0;
	do
	  {
	     while (isblank(*p) && (*p != 0) && (*p != '\n')) p++;
	     if ((*p == '\n') || (*p == 0)) break;
	     pp = p;
	     while (!isblank(*p) && (*p != 0) && (*p != '\n')) p++;
	     strncpy(ext, pp, (p - pp));
	     ext[p - pp] = 0;
	     
	     mime = E_NEW(E_Mime, 1);
	     if (mime)
	       {
		  mime->mime = evas_stringshare_add(mimetype);
		  snprintf(buf2, sizeof(buf2), "*.%s", ext);
		  mime->glob = evas_stringshare_add(buf2);
		  if ((!mime->mime) || (!mime->glob))
		    {
		       if (mime->mime) evas_stringshare_del(mime->mime);
		       if (mime->glob) evas_stringshare_del(mime->glob);
		       free(mime);
		    }
		  else
		    {
		       _e_fm_mime_glob_remove(buf2);
		       mimes = evas_list_append(mimes, mime);
		    }
	       }
	  }
	while ((*p != '\n') && (*p != 0));
     }
   fclose(f);
}

static void
_e_fm_mime_shared_mimeinfo_globs_load(char *file)
{
   /* format:

    # This file was automatically generated by the
    # update-mime-database command. DO NOT EDIT!
    text/vnd.wap.wml:*.wml
    application/x-7z-compressed:*.7z
    application/vnd.corel-draw:*.cdr
    text/spreadsheet:*.sylk

    */
   FILE *f;
   char buf[4096], buf2[4096], mimetype[4096], ext[4096], *p, *pp;
   E_Mime *mime;
   
   f = fopen(file, "rb");
   if (!f) return;
   while (fgets(buf, sizeof(buf), f))
     {
	p = buf;
	while (isblank(*p) && (*p != 0) && (*p != '\n')) p++;
	if (*p == '#') continue;
	if ((*p == '\n') || (*p == 0)) continue;
	pp = p;
	while ((*p != ':') && (*p != 0) && (*p != '\n')) p++;
	if ((*p == '\n') || (*p == 0)) continue;
	strncpy(mimetype, pp, (p - pp));
	mimetype[p - pp] = 0;
	p++;
	pp = ext;
	while ((*p != 0) && (*p != '\n'))
	  {
	     *pp = *p;
	     pp++;
	     p++;
	  }
	*pp = 0;
	
	mime = E_NEW(E_Mime, 1);
	if (mime)
	  {
	     mime->mime = evas_stringshare_add(mimetype);
	     snprintf(buf2, sizeof(buf2), "*.%s", ext);
	     mime->glob = evas_stringshare_add(buf2);
	     if ((!mime->mime) || (!mime->glob))
	       {
		  if (mime->mime) evas_stringshare_del(mime->mime);
		  if (mime->glob) evas_stringshare_del(mime->glob);
		  free(mime);
	       }
	     else
	       {
		  _e_fm_mime_glob_remove(buf2);
		  mimes = evas_list_append(mimes, mime);
	       }
	  }
     }
   fclose(f);
}
