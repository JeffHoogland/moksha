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
static Evas_Bool _e_fm_mime_icon_foreach(Evas_Hash *hash, const char *key, void *data, void *fdata);
static void _e_fm_mime_all_free(void);
static void _e_fm_mime_update(void);
static int _e_fm_mime_glob_remove(const char *glob);
static void _e_fm_mime_mime_types_load(char *file);
static void _e_fm_mime_shared_mimeinfo_globs_load(char *file);

static Evas_List *mimes = NULL;
static Evas_Hash *icon_map = NULL;

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

/* returns:
 * NULL == don't know
 * "THUMB" == generate a thumb
 * "e/icons/fileman/mime/..." == theme icon
 * "/path/to/file....edj" = explicit icon edje file
 * "/path/to/file..." = explicit image file to use
 */
EAPI const char *
e_fm_mime_icon_get(const char *mime)
{
   char buf[4096], buf2[4096], *homedir = NULL, *val;
   Evas_List *l;
   E_Config_Mime_Icon *mi;
   Evas_List *freelist = NULL;
   
   /* 0.0 clean out hash cache once it has mroe than 256 entried in it */
   if (evas_hash_size(icon_map) > 256)
     {
	evas_hash_foreach(icon_map, _e_fm_mime_icon_foreach, &freelist);
	while (freelist)
	  {
	     evas_stringshare_del(freelist->data);
	     freelist = evas_list_remove_list(freelist, freelist);
	  }
	evas_hash_free(icon_map);
	icon_map = NULL;
     }
   /* 0. look in mapping cache */
   val = evas_hash_find(icon_map, mime);
   if (val) return val;
   
   strncpy(buf2, mime, sizeof(buf2) - 1);
   buf2[sizeof(buf2) - 1] = 0;
   val = strchr(buf2, '/');
   if (val) *val = 0;
   
   /* 1. look up in mapping to file or thumb (thumb has flag)*/
   for (l = e_config->mime_icons; l; l = l->next)
     {
	mi = l->data;
	if (e_util_glob_match(mi->mime, mime))
	  {
	     strncpy(buf, mi->icon, sizeof(buf) - 1);
	     buf[sizeof(buf) - 1] = 0;
	     goto ok;
	  }
     }
   
   /* 2. look up in ~/.e/e/icons */
   homedir = e_user_homedir_get();
   if (!homedir) return NULL;
   
   snprintf(buf, sizeof(buf), "%s/.e/e/icons/%s.edj", homedir, mime);
   if (ecore_file_exists(buf)) goto ok;
   snprintf(buf, sizeof(buf), "%s/.e/e/icons/%s.svg", homedir, mime);
   if (ecore_file_exists(buf)) goto ok;
   snprintf(buf, sizeof(buf), "%s/.e/e/icons/%s.png", homedir, mime);
   if (ecore_file_exists(buf)) goto ok;
   snprintf(buf, sizeof(buf), "%s/.e/e/icons/%s.xpm", homedir, mime);
   if (ecore_file_exists(buf)) goto ok;
   snprintf(buf, sizeof(buf), "%s/.e/e/icons/%s.edj", homedir, buf2);
   if (ecore_file_exists(buf)) goto ok;
   snprintf(buf, sizeof(buf), "%s/.e/e/icons/%s.svg", homedir, buf2);
   if (ecore_file_exists(buf)) goto ok;
   snprintf(buf, sizeof(buf), "%s/.e/e/icons/%s.png", homedir, buf2);
   if (ecore_file_exists(buf)) goto ok;
   snprintf(buf, sizeof(buf), "%s/.e/e/icons/%s.xpm", homedir, buf2);
   if (ecore_file_exists(buf)) goto ok;
   
   /* 3. look up icon in theme */
   snprintf(buf, sizeof(buf), "e/icons/fileman/mime/%s", mime);
   val = (char *)e_theme_edje_file_get("base/theme/fileman", buf);
   if ((val) && (e_util_edje_collection_exists(val, buf))) goto ok;
   snprintf(buf, sizeof(buf), "e/icons/fileman/mime/%s", buf2);
   val = (char *)e_theme_edje_file_get("base/theme/fileman", buf);
   if ((val) && (e_util_edje_collection_exists(val, buf))) goto ok;
   
   /* 4. look up icon in PREFIX/share/enlightent/data/icons */
   snprintf(buf, sizeof(buf), "%s/data/icons/%s.edj", e_prefix_data_get(), mime);
   if (ecore_file_exists(buf)) goto ok;
   snprintf(buf, sizeof(buf), "%s/data/icons/%s.svg", e_prefix_data_get(), mime);
   if (ecore_file_exists(buf)) goto ok;
   snprintf(buf, sizeof(buf), "%s/data/icons/%s.png", e_prefix_data_get(), mime);
   if (ecore_file_exists(buf)) goto ok;
   snprintf(buf, sizeof(buf), "%s/data/icons/%s.xpm", e_prefix_data_get(), mime);
   if (ecore_file_exists(buf)) goto ok;
   snprintf(buf, sizeof(buf), "%s/data/icons/%s.edj", e_prefix_data_get(), buf2);
   if (ecore_file_exists(buf)) goto ok;
   snprintf(buf, sizeof(buf), "%s/data/icons/%s.svg", e_prefix_data_get(), buf2);
   if (ecore_file_exists(buf)) goto ok;
   snprintf(buf, sizeof(buf), "%s/data/icons/%s.png", e_prefix_data_get(), buf2);
   if (ecore_file_exists(buf)) goto ok;
   snprintf(buf, sizeof(buf), "%s/data/icons/%s.xpm", e_prefix_data_get(), buf2);
   if (ecore_file_exists(buf)) goto ok;
   
   if (homedir) free(homedir);
   return NULL;
   
   ok:
   val = (char *)evas_stringshare_add(buf);
   icon_map = evas_hash_add(icon_map, mime, val);
   if (homedir) free(homedir);
   return val;
   
}

/* local subsystem functions */
static Evas_Bool
_e_fm_mime_icon_foreach(Evas_Hash *hash, const char *key, void *data, void *fdata)
{
   Evas_List **freelist;
   
   freelist = fdata;
   *freelist = evas_list_append(*freelist, data);
   return 1;
}

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
	
	snprintf(buf, sizeof(buf), "/usr/local/etc/mime.types");
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
	
	snprintf(buf, sizeof(buf), "/usr/local/share/mime/globs");
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
   char buf[4096], mimetype[4096], ext[4096], *p, *pp;
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
	     mime->glob = evas_stringshare_add(ext);
	     if ((!mime->mime) || (!mime->glob))
	       {
		  if (mime->mime) evas_stringshare_del(mime->mime);
		  if (mime->glob) evas_stringshare_del(mime->glob);
		  free(mime);
	       }
	     else
	       {
		  _e_fm_mime_glob_remove(ext);
		  mimes = evas_list_append(mimes, mime);
	       }
	  }
     }
   fclose(f);
}
