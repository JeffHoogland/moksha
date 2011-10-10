#include "e_mod_main.h"
#include "md5.h"

#define MAX_FUZZ 100
#define MAX_WORDS 5

static const char  *home_dir = NULL;
static int home_dir_len;
static char dir_buf[1024];
static char thumb_buf[4096];

void
evry_util_file_detail_set(Evry_Item_File *file)
{
   char *dir = NULL;
   const char *tmp;

   if (EVRY_ITEM(file)->detail)
     return;

   if (!home_dir)
     {
	home_dir = e_user_homedir_get();
	home_dir_len = strlen(home_dir);
     }

   dir = ecore_file_dir_get(file->path);
   if (!dir || !home_dir) return;

   if (!strncmp(dir, home_dir, home_dir_len))
     {
	tmp = dir + home_dir_len;

	if (*(tmp) == '\0')
	  snprintf(dir_buf, sizeof(dir_buf), "~%s", tmp);
	else
	  snprintf(dir_buf, sizeof(dir_buf), "~%s/", tmp);

	EVRY_ITEM(file)->detail = eina_stringshare_add(dir_buf);
     }
   else
     {
	if (!strncmp(dir, "//", 2))
	  EVRY_ITEM(file)->detail = eina_stringshare_add(dir + 1);
	else
	  EVRY_ITEM(file)->detail = eina_stringshare_add(dir);
     }

   E_FREE(dir);
}

int
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

   if (!match || !str || !match[0] || !str[0])
     return 0;

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
	/* m_len = 0; */

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

		  /* m_len++; */

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
	if (strcmp(match, str))
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

Eina_List *
evry_fuzzy_match_sort(Eina_List *items)
{
   return eina_list_sort(items, -1, _evry_fuzzy_match_sort_cb);
}


static int _sort_flags = 0;

static int
_evry_items_sort_func(const void *data1, const void *data2)
{
   const Evry_Item *it1 = data1;
   const Evry_Item *it2 = data2;

   /* if (!((!_sort_flags) &&
    *       (it1->type == EVRY_TYPE_ACTION) &&
    *       (it2->type == EVRY_TYPE_ACTION)))
    *   { */
	/* only sort actions when there is input otherwise show default order */

	if (((it1->type == EVRY_TYPE_ACTION) || (it1->subtype == EVRY_TYPE_ACTION)) &&
	    ((it2->type == EVRY_TYPE_ACTION) || (it2->subtype == EVRY_TYPE_ACTION)))
	  {
	     const Evry_Action *act1 = data1;
	     const Evry_Action *act2 = data2;

	     /* sort actions that match the specific type before
		those matching general type */
	     if (act1->it1.item && act2->it1.item)
	       {
		  if ((act1->it1.type == act1->it1.item->type) &&
		      (act2->it1.type != act2->it1.item->type))
		    return -1;

		  if ((act1->it1.type != act1->it1.item->type) &&
		      (act2->it1.type == act2->it1.item->type))
		    return 1;
	       }

	     /* sort context specific actions before
		general actions */
	     if (act1->remember_context)
	       {
		  if (!act2->remember_context)
		    return -1;
	       }
	     else
	       {
		  if (act2->remember_context)
		    return 1;
	       }
	  }
     /* } */
   
   if (_sort_flags)
     {
	/* when there is no input sort items with higher
	 * plugin priority first */
	if (it1->type != EVRY_TYPE_ACTION &&
	    it2->type != EVRY_TYPE_ACTION)
	  {
	     int prio1 = it1->plugin->config->priority;
	     int prio2 = it2->plugin->config->priority;

	     if (prio1 - prio2)
	       return (prio1 - prio2);
	  }
     }
   
   /* sort items which match input or which
      match much better first */
   if (it1->fuzzy_match > 0 || it2->fuzzy_match > 0)
     {
   	if (it2->fuzzy_match <= 0)
   	  return -1;

   	if (it1->fuzzy_match <= 0)
   	  return 1;

   	if (abs (it1->fuzzy_match - it2->fuzzy_match) > 5)
   	  return (it1->fuzzy_match - it2->fuzzy_match);
     }

   /* sort recently/most frequently used items first */
   if (it1->usage > 0.0 || it2->usage > 0.0)
     {
	return (it1->usage > it2->usage ? -1 : 1);
     }

   /* sort items which match input better first */
   if (it1->fuzzy_match > 0 || it2->fuzzy_match > 0)
     {
	if (it1->fuzzy_match - it2->fuzzy_match)
	  return (it1->fuzzy_match - it2->fuzzy_match);
     }

   /* sort itemswith higher priority first */
   if ((it1->plugin == it2->plugin) &&
       (it1->priority - it2->priority))
     return (it1->priority - it2->priority);

   /* sort items with higher plugin priority first */
   if (it1->type != EVRY_TYPE_ACTION &&
       it2->type != EVRY_TYPE_ACTION)
     {
	int prio1 = it1->plugin->config->priority;
	int prio2 = it2->plugin->config->priority;

	if (prio1 - prio2)
	  return (prio1 - prio2);
     }

   return strcasecmp(it1->label, it2->label);
}

void
evry_util_items_sort(Eina_List **items, int flags)
{
   _sort_flags = flags;
   *items = eina_list_sort(*items, -1, _evry_items_sort_func);   
   _sort_flags = 0;
}

int
evry_util_plugin_items_add(Evry_Plugin *p, Eina_List *items, const char *input,
		      int match_detail, int set_usage)
{
   Eina_List *l;
   Evry_Item *it;
   int match = 0;

   EINA_LIST_FOREACH(items, l, it)
     {
	it->fuzzy_match = 0;

	if (set_usage)
	  evry_history_item_usage_set(it, input, NULL);

	if (!input)
	  {
	     p->items = eina_list_append(p->items, it);
	     continue;
	  }

	it->fuzzy_match = evry_fuzzy_match(it->label, input);

	if (match_detail)
	  {
	     match = evry_fuzzy_match(it->detail, input);

	     if (!(it->fuzzy_match) || (match && (match < it->fuzzy_match)))
	       it->fuzzy_match = match;
	  }

	if (it->fuzzy_match)
	  p->items = eina_list_append(p->items, it);
     }

   p->items = eina_list_sort(p->items, -1, _evry_items_sort_func);

   return !!(p->items);
}

Evas_Object *
evry_icon_theme_get(const char *icon, Evas *e)
{
   Evas_Object *o = NULL;

   if (!icon)
     return NULL;
   
   o = e_icon_add(e);
   e_icon_scale_size_set(o, 128); 
   e_icon_preload_set(o, 1);

   if (icon[0] == '/')
     {
	if (!e_icon_file_set(o, icon))
	  {
	     evas_object_del(o);
	     o = NULL;
	  }
     }
   else if (!e_util_icon_theme_set(o, icon))
     {
	evas_object_del(o);
	o = NULL;
     }
   
   return o;
}


Evas_Object *
evry_util_icon_get(Evry_Item *it, Evas *e)
{
   Evas_Object *o = NULL;

   if (it->icon_get)
     o = it->icon_get(it, e);
   if (o) return o;

   if ((it->icon) && (it->icon[0] == '/'))
     o = evry_icon_theme_get(it->icon, e);
   if (o) return o;
   
   if (CHECK_TYPE(it, EVRY_TYPE_FILE))
     {
	const char *icon;
	char *sum;
	
	GET_FILE(file, it);

	if (it->browseable)
	  o = evry_icon_theme_get("folder", e);
	if (o) return o;
	
	if ((!it->icon) && (file->mime) &&
	    (/*(!strncmp(file->mime, "image/", 6)) || */
	     (!strncmp(file->mime, "video/", 6)) ||
	     (!strncmp(file->mime, "application/pdf", 15))) &&
	    (evry_file_url_get(file)))
	  {
	     sum = evry_util_md5_sum(file->url);

	     snprintf(thumb_buf, sizeof(thumb_buf),
		      "%s/.thumbnails/normal/%s.png",
		      e_user_homedir_get(), sum);
	     free(sum);

	     if ((o = evry_icon_theme_get(thumb_buf, e)))
	       {
		  it->icon = eina_stringshare_add(thumb_buf);
		  return o;
	       }
	  }

	if ((!it->icon) && (file->mime))
	  {
	     icon = efreet_mime_type_icon_get(file->mime, e_config->icon_theme, 128);
	     /* XXX can do _ref ?*/
	     if ((o = evry_icon_theme_get(icon, e)))
	       {
		  /* it->icon = eina_stringshare_add(icon); */
		  return o;
	       }
	  }

	if ((icon = efreet_mime_type_icon_get("unknown", e_config->icon_theme, 128)))
	  it->icon = eina_stringshare_add(icon);
	else
	  it->icon = eina_stringshare_add("");
     }
   
   if (CHECK_TYPE(it, EVRY_TYPE_APP))
     {
	GET_APP(app, it);
	
	o = e_util_desktop_icon_add(app->desktop, 128, e); 
	if (o) return o;
	
	o = evry_icon_theme_get("system-run", e);
	if (o) return o;
     }

   if (it->icon)
     o = evry_icon_theme_get(it->icon, e);
   if (o) return o;
   
   if (it->browseable)
     o = evry_icon_theme_get("folder", e);
   if (o) return o;

   o = evry_icon_theme_get("unknown", e);
   
   return o;
}

int
evry_util_exec_app(const Evry_Item *it_app, const Evry_Item *it_file)
{
   E_Zone *zone;
   Eina_List *files = NULL;
   char *exe = NULL;
   char *tmp = NULL;

   if (!it_app) return 0;
   GET_APP(app, it_app);
   GET_FILE(file, it_file);

   zone = e_util_zone_current_get(e_manager_current_get());

   if (app->desktop)
     {
	if (file && evry_file_path_get(file))
	  {
	     Eina_List *l;
	     char *mime;
	     int open_folder = 0;

	     /* when the file is no a directory and the app
		opens folders, pass only the dir */
	     if (!IS_BROWSEABLE(file))
	       {
		  EINA_LIST_FOREACH(app->desktop->mime_types, l, mime)
		    {
		       if (!mime)
			 continue;

		       if (!strcmp(mime, "x-directory/normal"))
			 open_folder = 1;

		       if (file->mime && !strcmp(mime, file->mime))
			 {
			    open_folder = 0;
			    break;
			 }
		    }
	       }

	     if (open_folder)
	       {
		  tmp = ecore_file_dir_get(file->path);
		  files = eina_list_append(files, tmp);
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

	     E_FREE(tmp);
	  }
	else if (app->file)
	  {
	     files = eina_list_append(files, app->file);
	     e_exec(zone, app->desktop, NULL, files, NULL);
	     eina_list_free(files);
	  }
	else
	  {
	     e_exec(zone, app->desktop, NULL, NULL, NULL);
	  }
     }
   else if (app->file)
     {
	if (file && evry_file_path_get(file))
	  {
	     int len;
	     len = strlen(app->file) + strlen(file->path) + 4;
	     exe = malloc(len);
	     snprintf(exe, len, "%s \'%s\'", app->file, file->path);
	     e_exec(zone, NULL, exe, NULL, NULL);
	     E_FREE(exe);
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

char *
evry_util_url_unescape(const char *string, int length)
{
   int alloc = (length?length:(int)strlen(string))+1;
   char *ns = malloc(alloc);
   unsigned char in;
   int strindex=0;
   unsigned long hex;

   if( !ns )
     return NULL;

   while(--alloc > 0)
     {
	in = *string;
	if(('%' == in) && ISXDIGIT(string[1]) && ISXDIGIT(string[2]))
	  {
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

static Eina_Bool
_isalnum(unsigned char in)
{
   switch (in)
     {
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
      case 'a': case 'b': case 'c': case 'd': case 'e':
      case 'f': case 'g': case 'h': case 'i': case 'j':
      case 'k': case 'l': case 'm': case 'n': case 'o':
      case 'p': case 'q': case 'r': case 's': case 't':
      case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
      case 'A': case 'B': case 'C': case 'D': case 'E':
      case 'F': case 'G': case 'H': case 'I': case 'J':
      case 'K': case 'L': case 'M': case 'N': case 'O':
      case 'P': case 'Q': case 'R': case 'S': case 'T':
      case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
	 return EINA_TRUE;
      default:
	 break;
     }
   return EINA_FALSE;
}

char *
evry_util_url_escape(const char *string, int inlength)
{
   size_t alloc = (inlength?(size_t)inlength:strlen(string))+1;
   char *ns;
   char *testing_ptr = NULL;
   unsigned char in; /* we need to treat the characters unsigned */
   size_t newlen = alloc;
   int strindex=0;
   size_t length;

   ns = malloc(alloc);
   if(!ns)
     return NULL;

   length = alloc-1;
   while(length--)
     {
	in = *string;

	if (_isalnum(in))
	  {
	     /* just copy this */
	     ns[strindex++]=in;
	  }
	else {
	   /* encode it */
	   newlen += 2; /* the size grows with two, since this'll become a %XX */
	   if(newlen > alloc)
	     {
		alloc *= 2;
		testing_ptr = realloc(ns, alloc);
		if(!testing_ptr)
		  {
		     free( ns );
		     return NULL;
		  }
		else
		  {
		     ns = testing_ptr;
		  }
	     }

	   snprintf(&ns[strindex], 4, "%%%02X", in);

	   strindex+=3;
	}
	string++;
     }
   ns[strindex]=0; /* terminate it */
   return ns;
}

const char*
evry_file_path_get(Evry_Item_File *file)
{
   const char *tmp;
   char *path;

   if (file->path)
     return file->path;

   if (!file->url)
     return NULL;

   if (!strncmp(file->url, "file://", 7))
     tmp = file->url + 7;
   else return NULL;

   if (!(path = evry_util_url_unescape(tmp, 0)))
     return NULL;

   file->path = eina_stringshare_add(path);

   E_FREE(path);

   return file->path;
}

const char*
evry_file_url_get(Evry_Item_File *file)
{
   char dest[PATH_MAX * 3 + 7];
   const char *p;
   int i;

   if (file->url)
     return file->url;

   if (!file->path)
     return NULL;

   memset(dest, 0, PATH_MAX * 3 + 7);

   snprintf(dest, 8, "file://");

   /* Most app doesn't handle the hostname in the uri so it's put to NULL */
   for (i = 7, p = file->path; *p != '\0'; p++, i++)
     {
	if (isalnum(*p) || strchr("/$-_.+!*'()", *p))
	  dest[i] = *p;
	else
	  {
	     snprintf(&(dest[i]), 4, "%%%02X", (unsigned char)*p);
	     i += 2;
	  }
     }

   file->url = eina_stringshare_add(dest);

   return file->url;
}

static void
_cb_free_item_changed(void *data __UNUSED__, void *event)
{
   Evry_Event_Item_Changed *ev = event;

   evry_item_free(ev->item);
   E_FREE(ev);
}

void
evry_item_changed(Evry_Item *it, int icon, int selected)
{
   Evry_Event_Item_Changed *ev;
   ev = E_NEW(Evry_Event_Item_Changed, 1);
   ev->item = it;
   ev->changed_selection = selected;
   ev->changed_icon = icon;
   evry_item_ref(it);
   ecore_event_add(_evry_events[EVRY_EVENT_ITEM_CHANGED], ev, _cb_free_item_changed, NULL);
}

static char thumb_buf[4096];
static const char hex[] = "0123456789abcdef";

char *
evry_util_md5_sum(const char *str)
{
   MD5_CTX ctx;
   unsigned char hash[MD5_HASHBYTES];
   int n;
   char md5out[(2 * MD5_HASHBYTES) + 1];
   MD5Init (&ctx);
   MD5Update (&ctx, (unsigned char const*)str,
	      (unsigned)strlen (str));
   MD5Final (hash, &ctx);

   for (n = 0; n < MD5_HASHBYTES; n++)
     {
	md5out[2 * n] = hex[hash[n] >> 4];
	md5out[2 * n + 1] = hex[hash[n] & 0x0f];
     }
   md5out[2 * n] = '\0';

   return strdup(md5out);
}
