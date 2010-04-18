#include "Evry.h"

#define MAX_FUZZ 100
#define MAX_WORDS 5

static const char  *home_dir = NULL;
static int home_dir_len;
static char dir_buf[1024];

EAPI void
evry_util_file_detail_set(Evry_Item_File *file)
{
   const char *path;

   if (EVRY_ITEM(file)->detail)
     return;
   
   if (!home_dir)
     {
	home_dir = e_user_homedir_get();
	home_dir_len = strlen(home_dir);
     }
   
   path = ecore_file_dir_get(file->path);
	  
   if (path && !strncmp(path, home_dir, home_dir_len))
     {
	if (*(path + home_dir_len) == '\0')
	  snprintf(dir_buf, sizeof(dir_buf), "~%s", path + home_dir_len);
	else
	  snprintf(dir_buf, sizeof(dir_buf), "~%s/", path + home_dir_len);
	
	EVRY_ITEM(file)->detail = eina_stringshare_add(dir_buf);
     }
   else
     {
	if (!strncmp(path, "//", 2)) path++;
	
	EVRY_ITEM(file)->detail = eina_stringshare_add(path);
     }
}

EAPI int
evry_fuzzy_match(const char *str, const char *match)
{
   const char *p, *m, *next;
   int sum = 0;

   unsigned int last = 0;
   unsigned int offset = 0;
   unsigned int min = 0;
   unsigned char first = 0;
   /* ignore punctuation */
   unsigned char ip = 1;

   unsigned int cnt = 0;
   /* words in match */
   unsigned int m_num = 0;
   unsigned int m_cnt = 0;
   unsigned int m_min[MAX_WORDS];
   unsigned int m_len = 0;

   if (!match || !str) return 0;

   /* remove white spaces at the beginning */
   for (; (*match != 0) && isspace(*match); match++);
   for (; (*str != 0)   && isspace(*str);   str++);

   /* count words in match */
   for (m = match; (*m != 0) && (m_num < MAX_WORDS);)
     {
	for (; (*m != 0) && !isspace(*m); m++);
	for (; (*m != 0) &&  isspace(*m); m++);
	m_min[m_num++] = MAX_FUZZ;
     }
   for (m = match; ip && (*m != 0); m++)
     if (ip && ispunct(*m)) ip = 0;

   m_len = strlen(match);

   /* with less than 3 chars match must be a prefix */
   if (m_len < 3) m_len = 0;

   next = str;
   m = match;

   while((m_cnt < m_num) && (*next != 0))
     {
	/* reset match */
	if (m_cnt == 0) m = match;

	/* end of matching */
	if (*m == 0) break;

	offset = 0;
	last = 0;
	min = 1;
	first = 0;

	/* match current word of string against current match */
	for (p = next; *next != 0; p++)
	  {
	     /* new word of string begins */
	     if ((*p == 0) || isspace(*p) || (ip && ispunct(*p)))
	       {
		  if (m_cnt < m_num - 1)
		    {
		       /* test next match */
		       for (; (*m != 0) && !isspace(*m); m++);
		       for (; (*m != 0) &&  isspace(*m); m++);
		       m_cnt++;
		       break;
		    }
		  else
		    {
		       /* go to next word */
		       for (; (*p != 0) && ((isspace(*p) || (ip && ispunct(*p)))); p++);
		       cnt++;
		       next = p;
		       m_cnt = 0;
		       break;
		    }
	       }

	     /* current char matches? */
	     if (tolower(*p) != tolower(*m))
	       {
		  if (!first)
		    offset += 1;
		  else
		    offset += 3;

		  if (offset <= m_len * 3)
		    continue;
	       }

	     if (min < MAX_FUZZ && offset <= m_len * 3)
	       {
		  /* first offset of match in word */
		  if (!first)
		    {
		       first = 1;
		       last = offset;
		    }

		  min += offset + (offset - last) * 5;
		  last = offset;

		  /* try next char of match */
		  if (*(++m) != 0 && !isspace(*m))
		    continue;

		  /* end of match: store min weight of match */
		  min += (cnt - m_cnt) > 0 ? (cnt - m_cnt) : 0;

		  if (min < m_min[m_cnt])
		    m_min[m_cnt] = min;
	       }
	     else
	       {
		  /* go to next match */
		  for (; (*m != 0) && !isspace(*m); m++);
	       }

	     if (m_cnt < m_num - 1)
	       {
		  /* test next match */
		  for (; (*m != 0) && isspace(*m); m++);
		  m_cnt++;
		  break;
	       }
	     else if(*p != 0)
	       {
		  /* go to next word */
		  for (; (*p != 0) && !((isspace(*p) || (ip && ispunct(*p)))); p++);
		  for (; (*p != 0) &&  ((isspace(*p) || (ip && ispunct(*p)))); p++);
		  cnt++;
		  next = p;
		  m_cnt = 0;
		  break;
	       }
	     else
	       {
		  next = p;
		  break;
	       }
	  }
     }

   for (m_cnt = 0; m_cnt < m_num; m_cnt++)
     {
	sum += m_min[m_cnt];

	if (sum >= MAX_FUZZ)
	  {
	     sum = 0;
	     break;
	  }
     }

   if (sum > 0)
     {
	/* exact match ? */
	if (strlen(str) != m_len)
	  sum += 10;
     }
   
   return sum;
}

static int
_evry_fuzzy_match_sort_cb(const void *data1, const void *data2)
{
   const Evry_Item *it1 = data1;
   const Evry_Item *it2 = data2;

   if (it1->priority - it2->priority)
     return (it1->priority - it2->priority);

   if (it1->fuzzy_match || it2->fuzzy_match)
     {
	if (it1->fuzzy_match && !it2->fuzzy_match)
	  return -1;

	if (!it1->fuzzy_match && it2->fuzzy_match)
	  return 1;

	if (it1->fuzzy_match - it2->fuzzy_match)
	  return (it1->fuzzy_match - it2->fuzzy_match);
     }

   return 0;
}

EAPI Eina_List *
evry_fuzzy_match_sort(Eina_List *items)
{
   return eina_list_sort(items, eina_list_count(items), _evry_fuzzy_match_sort_cb);
}
 

/* taken from e_utils. just changed 48 to 72.. we need
   evry_icon_theme_set(Evas_Object *obj, const char *icon,
   size:small, mid, large) imo */
static int
_evry_icon_theme_set(Evas_Object *obj, const char *icon)
{
   const char *file;
   char buf[4096];

   if ((!icon) || (!icon[0])) return 0;
   snprintf(buf, sizeof(buf), "e/icons/%s", icon);
   file = e_theme_edje_file_get("base/theme/icons", buf);
   if (file[0])
     {
	e_icon_file_edje_set(obj, file, buf);
	return 1;
     }
   return 0;
}

static int
_evry_icon_fdo_set(Evas_Object *obj, const char *icon)
{
   char *path = NULL;
   unsigned int size;

   if ((!icon) || (!icon[0])) return 0;
   size = e_util_icon_size_normalize(128 * e_scale);
   path = efreet_icon_path_find(e_config->icon_theme, icon, size);

   if (!path) return 0;
   e_icon_file_set(obj, path);
   E_FREE(path);
   return 1;
}

EAPI Evas_Object *
evry_icon_theme_get(const char *icon, Evas *e)
{
   Evas_Object *o = e_icon_add(e);

   if (e_config->icon_theme_overrides)
     {
	if (_evry_icon_fdo_set(o, icon) ||
	    _evry_icon_theme_set(o, icon))
	  return o;
     }
   else
     {
	if (_evry_icon_theme_set(o, icon) ||
	    _evry_icon_fdo_set(o, icon))
	  return o;
     }

   evas_object_del(o);

   return NULL;
}

EAPI Evas_Object *
evry_icon_mime_get(const char *mime, Evas *e)
{
   Evas_Object *o = NULL;
   char *icon;

   icon = efreet_mime_type_icon_get(mime, e_config->icon_theme, 64);

   if (icon)
     {
	o = e_util_icon_add(icon, e);
	free(icon);
     }
   else
     {
	o = evry_icon_theme_get("none", e);
     }

   return o;
}

EAPI int
evry_util_exec_app(const Evry_Item *it_app, const Evry_Item *it_file)
{
   E_Zone *zone;
   Eina_List *files = NULL;
   char *exe = NULL;

   if (!it_app) return 0;
   ITEM_APP(app, it_app);
   
   zone = e_util_zone_current_get(e_manager_current_get());

   if (app->desktop)
     {
	if (it_file)
	  {
	     ITEM_FILE(file, it_file);

	     Eina_List *l;
	     char *mime;
	     char *path = NULL;
	     int open_folder = 0;

	     if (!EVRY_ITEM(file)->browseable)
	       {
		  EINA_LIST_FOREACH(app->desktop->mime_types, l, mime)
		    {
		       if (!strcmp(mime, "x-directory/normal"))
			 {
			    open_folder = 1;
			    break;
			 }
		    }
	       }

	     if (open_folder)
	       {
		  path = ecore_file_dir_get(file->path);
		  files = eina_list_append(files, path);
	       }
	     else
	       {
		  files = eina_list_append(files, file->path);
	       }

	     e_exec(zone, app->desktop, NULL, files, NULL);

	     if (file && file->mime && !open_folder)
	       e_exehist_mime_desktop_add(file->mime, app->desktop);

	     if (files)
	       eina_list_free(files);

	     if (open_folder && path)
	       free(path);
	  }
	else if (app->file)
	  {
	     files = eina_list_append(files, app->file);
	     e_exec(zone, app->desktop, NULL, files, NULL);
	     eina_list_free(files);
	  }
	else
	  {
	     e_exec(zone, app->desktop, NULL, NULL, "everything");
	  }
     }
   else if (app->file)
     {
	if (it_file)
	  {
	     ITEM_FILE(file, it_file);

	     /* files = eina_list_append(files, file->path);
	      * 
	      * e_exec(zone, NULL, app->file, files, NULL);
	      * 
	      * if (files)
	      *   eina_list_free(files); */

	     char *tmp;
	     int len;
	     tmp = eina_str_escape(file->path);
	     len = strlen(app->file) + strlen(tmp) + 2;
	     exe = malloc(len);
	     snprintf(exe, len, "%s %s", app->file, tmp);
	     e_exec(zone, NULL, exe, NULL, NULL);
	     E_FREE(exe);
	     E_FREE(tmp);
	  }
	else
	  {
	     exe = (char *) app->file;
	     e_exec(zone, NULL, exe, NULL, NULL);
	  }
     }

   return 1;
}

/* taken from curl:
 *
 * Copyright (C) 1998 - 2010, Daniel Stenberg, <daniel@haxx.se>, et
 * al.
 *
 * Unescapes the given URL escaped string of given length. Returns a
 * pointer to a malloced string with length given in *olen.
 * If length == 0, the length is assumed to be strlen(string).
 * If olen == NULL, no output length is stored.
 */
#define ISXDIGIT(x) (isxdigit((int) ((unsigned char)x)))

EAPI char *
evry_util_unescape(const char *string, int length)
{
   int alloc = (length?length:(int)strlen(string))+1;
   char *ns = malloc(alloc);
   unsigned char in;
   int strindex=0;
   unsigned long hex;

   if( !ns )
     return NULL;

   while(--alloc > 0) {
      in = *string;
      if(('%' == in) && ISXDIGIT(string[1]) && ISXDIGIT(string[2])) {
	 /* this is two hexadecimal digits following a '%' */
	 char hexstr[3];
	 char *ptr;
	 hexstr[0] = string[1];
	 hexstr[1] = string[2];
	 hexstr[2] = 0;

	 hex = strtoul(hexstr, &ptr, 16);
	 in = (unsigned char)(hex & (unsigned long) 0xFF);
	 // in = ultouc(hex); /* this long is never bigger than 255 anyway */

	 string+=2;
	 alloc-=2;
      }

      ns[strindex++] = in;
      string++;
   }
   ns[strindex]=0; /* terminate it */

   return ns;
}

#undef ISXDIGIT



static int
_conf_timer(void *data)
{
   /* e_util_dialog_internal(title,  */
   e_util_dialog_internal(_("Configuration Updated"), data);
   return 0;
}
  
EAPI Eina_Bool
evry_util_module_config_check(const char *module_name, int conf, int epoch, int version)
{
   if ((conf >> 16) < epoch) 
     {
	char *too_old =
	  _("%s Configuration data needed "
	    "upgrading. Your old configuration<br> has been"
	    " wiped and a new set of defaults initialized. "
	    "This<br>will happen regularly during "
	    "development, so don't report a<br>bug. "
	    "This simply means the module needs "
	    "new configuration<br>data by default for "
	    "usable functionality that your old<br>"
	    "configuration simply lacks. This new set of "
	    "defaults will fix<br>that by adding it in. "
	    "You can re-configure things now to your<br>"
	    "liking. Sorry for the inconvenience.<br>");

	char buf[4096];
	snprintf(buf, sizeof(buf), too_old, module_name);
	ecore_timer_add(1.0, _conf_timer, buf);
	return EINA_FALSE;
     }
   else if (conf > version) 
     {
	char *too_new =
	  _("Your %s Module configuration is NEWER "
	    "than the module version. This is "
	    "very<br>strange. This should not happen unless"
	    " you downgraded<br>the module or "
	    "copied the configuration from a place where"
	    "<br>a newer version of the module "
	    "was running. This is bad and<br>as a "
	    "precaution your configuration has been now "
	    "restored to<br>defaults. Sorry for the "
	    "inconvenience.<br>");

	char buf[4096];
	snprintf(buf, sizeof(buf), too_new, module_name);
	ecore_timer_add(1.0, _conf_timer, buf);
	return EINA_FALSE;
     }

   return EINA_TRUE;
}
