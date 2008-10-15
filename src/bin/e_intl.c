/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

static Ecore_Exe *_e_intl_input_method_exec = NULL;
static Ecore_Event_Handler *_e_intl_exit_handler = NULL;

static char *_e_intl_orig_lc_messages = NULL;
static char *_e_intl_orig_language = NULL;
static char *_e_intl_orig_lc_all = NULL;
static char *_e_intl_orig_lang = NULL;
static char *_e_intl_language = NULL;

static char *_e_intl_language_alias = NULL;

static char *_e_intl_orig_xmodifiers = NULL;
static char *_e_intl_orig_qt_im_module = NULL;
static char *_e_intl_orig_gtk_im_module = NULL;

static const char *_e_intl_imc_personal_path = NULL;
static const char *_e_intl_imc_system_path = NULL;

#define E_EXE_STOP(EXE) if (EXE != NULL) { ecore_exe_terminate(EXE); ecore_exe_free(EXE); EXE = NULL; }
#define E_EXE_IS_VALID(EXE) (!((EXE == NULL) || (EXE[0] == 0)))

/* All locale parts */
#define E_INTL_LOC_ALL		E_INTL_LOC_LANG | \
				E_INTL_LOC_REGION | \
				E_INTL_LOC_CODESET | \
				E_INTL_LOC_MODIFIER

/* Locale parts wich are significant when Validating */
#define E_INTL_LOC_SIGNIFICANT	E_INTL_LOC_LANG | \
				E_INTL_LOC_REGION | \
				E_INTL_LOC_CODESET

/* Language Setting and Listing */
static char		*_e_intl_language_path_find(char *language);
static Evas_List 	*_e_intl_language_dir_scan(const char *dir);
static int 		 _e_intl_language_list_find(Evas_List *language_list, char *language);

/* Locale Validation and Discovery */
static Evas_Hash	*_e_intl_locale_alias_hash_get(void);
static char		*_e_intl_locale_alias_get(const char *language);
static Evas_List	*_e_intl_locale_system_locales_get(void);
static Evas_List	*_e_intl_locale_search_order_get(const char *locale);
static int		 _e_intl_locale_validate(const char *locale);
static void 		 _e_intl_locale_hash_free(Evas_Hash *language_hash);
static Evas_Bool 	 _e_intl_locale_hash_free_cb(const Evas_Hash *hash, const char *key, void *data, void *fdata);

/* Input Method Configuration and Management */
static int 		 _e_intl_cb_exit(void *data, int type, void *event);
static Evas_List 	*_e_intl_imc_dir_scan(const char *dir);

EAPI int
e_intl_init(void)
{
   char *s;

   e_intl_data_init();

   if ((s = getenv("LC_MESSAGES"))) _e_intl_orig_lc_messages = strdup(s);
   if ((s = getenv("LANGUAGE"))) _e_intl_orig_language = strdup(s);
   if ((s = getenv("LC_ALL"))) _e_intl_orig_lc_all = strdup(s);
   if ((s = getenv("LANG"))) _e_intl_orig_lang = strdup(s);

   if ((s = getenv("GTK_IM_MODULE"))) _e_intl_orig_gtk_im_module = strdup(s);
   if ((s = getenv("QT_IM_MODULE"))) _e_intl_orig_qt_im_module = strdup(s);
   if ((s = getenv("XMODIFIERS"))) _e_intl_orig_xmodifiers = strdup(s);

   return 1;
}

EAPI int
e_intl_shutdown(void)
{
   E_FREE(_e_intl_language);
   E_FREE(_e_intl_orig_lc_messages);
   E_FREE(_e_intl_orig_language);
   E_FREE(_e_intl_orig_lc_all);
   E_FREE(_e_intl_orig_lang);

   E_FREE(_e_intl_orig_gtk_im_module);
   E_FREE(_e_intl_orig_qt_im_module);
   E_FREE(_e_intl_orig_xmodifiers);

   if (_e_intl_imc_personal_path)
     eina_stringshare_del(_e_intl_imc_personal_path);
   if (_e_intl_imc_system_path)
     eina_stringshare_del(_e_intl_imc_system_path);

   e_intl_data_shutdown();

   return 1;
}

/* Setup configuration settings and start services */
EAPI int
e_intl_post_init(void)
{
   if ((e_config->language) && (e_config->language[0] != 0))
     e_intl_language_set(e_config->language);
   else
     e_intl_language_set(NULL);

   if ((e_config->input_method) && (e_config->input_method[0] != 0))
     e_intl_input_method_set(e_config->input_method);

   _e_intl_exit_handler = ecore_event_handler_add(ECORE_EXE_EVENT_DEL, 
						  _e_intl_cb_exit, NULL);
   return 1;
}

EAPI int
e_intl_post_shutdown(void)
{
   if (_e_intl_exit_handler)
     {
	ecore_event_handler_del(_e_intl_exit_handler);
	_e_intl_exit_handler = NULL;
     }

   e_intl_input_method_set(NULL);

   e_intl_language_set(NULL);
   E_FREE(_e_intl_language_alias);

   E_EXE_STOP(_e_intl_input_method_exec);
   return 1;
}

/*
 * TODO
 * - Add error dialogs explaining any errors while setting the locale
 *      * Locale aliases need to be configured
 *      * Locale is invalid
 *      * Message files are not found for this locale, then we have (en_US, POSIX, C)
 * - Add support of compound locales i.e. (en_US;zh_CN;C) ==Defer==
 * - Add Configuration for to-be-set environment variables
 */
EAPI void
e_intl_language_set(const char *lang)
{
   int set_envars;

   set_envars = 1;
   /* NULL lang means set everything back to the original environment
    * defaults
    */
   if (!lang)
     {
	e_util_env_set("LC_MESSAGES", _e_intl_orig_lc_messages);
	e_util_env_set("LANGUAGE", _e_intl_orig_language);
	e_util_env_set("LC_ALL", _e_intl_orig_lc_all);
	e_util_env_set("LANG", _e_intl_orig_lang);

	lang = getenv("LC_MESSAGES");
	if (!lang) lang = getenv("LANGUAGE");
	if (!lang) lang = getenv("LC_ALL");
	if (!lang) lang = getenv("LANG");

	set_envars = 0;
     }

   E_FREE(_e_intl_language_alias);
   _e_intl_language_alias = _e_intl_locale_alias_get(lang);
   E_FREE(_e_intl_language);

   if (lang)
     _e_intl_language = strdup(lang);
   else
     _e_intl_language = NULL;

   if ((!_e_intl_locale_validate(_e_intl_language_alias)) &&
	 (strcmp(_e_intl_language_alias, "C")))
     {
	fprintf(stderr, "The locale '%s' cannot be found on your "
	       "system. Please install this locale or try "
               "something else.", _e_intl_language_alias);
     }
   else
     {
	/* Only set env vars is a non NULL locale was passed */
	if (set_envars)
	  {
	     e_util_env_set("LANGUAGE", _e_intl_language);
	     e_util_env_set("LANG", _e_intl_language);
	     e_util_env_set("LC_ALL", _e_intl_language);
	     e_util_env_set("LC_MESSAGES", _e_intl_language);
	  }

    	setlocale(LC_ALL, _e_intl_language);
        if (_e_intl_language)
	  {
             char *locale_path;

             locale_path = _e_intl_language_path_find(_e_intl_language_alias);
             if (locale_path == NULL)
	       {
		  E_Locale_Parts *locale_parts;

		  locale_parts = e_intl_locale_parts_get(_e_intl_language_alias);

		  /* If locale is C or some form of en don't report an error */
		  if ((locale_parts == NULL) && 
		      (strcmp(_e_intl_language_alias, "C")))
		    {
		       fprintf(stderr,
			     "An error occurred setting your locale. \n\n"

			     "The locale you have chosen '%s' appears to \n"
			     "be an alias, however, it can not be resloved.\n"
			     "Please make sure you have a 'locale.alias' \n"
			     "file in your 'messages' path which can resolve\n"
			     "this alias.\n\n"

			     "Enlightenment will not be translated.\n",
			     _e_intl_language_alias);
		    }
		  else if ((locale_parts) && (locale_parts->lang) && 
			   (strcmp(locale_parts->lang, "en")))
		    {
		       fprintf(stderr,
			     "An error occurred setting your locale. \n\n"

			     "The translation files for the locale you \n"
			     "have chosen (%s) cannot be found in your \n"
			     "'messages' path. \n\n"

			     "Enlightenment will not be translated.\n",
			     _e_intl_language_alias);
		    }
		  e_intl_locale_parts_free(locale_parts);
	       }
	     else
	       {
		  bindtextdomain(PACKAGE, locale_path);
		  textdomain(PACKAGE);
		  bind_textdomain_codeset(PACKAGE, "UTF-8");
		  free(locale_path);
               }
	  }
     }
}

EAPI const char *
e_intl_language_get(void)
{
   return _e_intl_language;
}

EAPI const char	*
e_intl_language_alias_get(void)
{
   return _e_intl_language_alias;
}

EAPI Evas_List *
e_intl_language_list(void)
{
   Evas_List *next;
   Evas_List *dir_list;
   Evas_List *all_languages;

   all_languages = NULL;
   dir_list = e_path_dir_list_get(path_messages);
   for (next = dir_list ; next ; next = next->next)
     {
	E_Path_Dir *epd;
	Evas_List *dir_languages;

	epd = next->data;
	dir_languages = _e_intl_language_dir_scan(epd->dir);
	while (dir_languages)
	  {
	     char *language;

	     language = dir_languages->data;
	     dir_languages = evas_list_remove_list(dir_languages, dir_languages);

	     if ((_e_intl_language_list_find(all_languages, language)) || 
		 ((strlen(language) > 2) && (!_e_intl_locale_validate(language))))
	       {
		  free(language);
	       }
	     else
	       all_languages = evas_list_append(all_languages, language);
	  }
     }

   e_path_dir_list_free(dir_list);

   return all_languages;
}

static int
_e_intl_language_list_find(Evas_List *language_list, char *language)
{
   Evas_List *l;

   if (!language_list) return 0;
   if (!language) return 0;

   for (l = language_list; l; l = l->next)
     {
	char *lang;

	lang = l->data;
	if (!strcmp(lang, language)) return 1;
     }

   return 0;
}

EAPI void
e_intl_input_method_set(const char *imc_path)
{
   if (!imc_path)
     {
	E_EXE_STOP(_e_intl_input_method_exec);
	e_util_env_set("GTK_IM_MODULE", _e_intl_orig_gtk_im_module);
        e_util_env_set("QT_IM_MODULE", _e_intl_orig_qt_im_module);
        e_util_env_set("XMODIFIERS", _e_intl_orig_xmodifiers);
     }

   if (imc_path)
     {
	Eet_File *imc_ef;
	E_Input_Method_Config *imc;

	imc_ef = eet_open(imc_path, EET_FILE_MODE_READ);
	if (imc_ef)
	  {
	     imc = e_intl_input_method_config_read(imc_ef);
	     eet_close(imc_ef);

	     if (imc)
	       {
	          e_util_env_set("GTK_IM_MODULE", imc->gtk_im_module);
	          e_util_env_set("QT_IM_MODULE", imc->qt_im_module);
	          e_util_env_set("XMODIFIERS", imc->xmodifiers);

		  E_EXE_STOP(_e_intl_input_method_exec);

		  if (E_EXE_IS_VALID(imc->e_im_exec))
		    {
		       e_util_library_path_strip();
		       _e_intl_input_method_exec = ecore_exe_run(imc->e_im_exec, NULL);
		       e_util_library_path_restore();
		       ecore_exe_tag_set(_e_intl_input_method_exec,"E/im_exec");

		       if ((!_e_intl_input_method_exec) ||
			   (!ecore_exe_pid_get(_e_intl_input_method_exec)))
			 e_util_dialog_show(_("Input Method Error"),
					    _( "Error starting the input method executable<br><br>"

					       "please make sure that your input<br>"
					       "method configuration is correct and<br>"
					       "that your configuration's<br>"
					       "executable is in your PATH<br>"));
		    }
		  e_intl_input_method_config_free(imc);
	       }
	  }
     }
}

EAPI Evas_List *
e_intl_input_method_list(void)
{
   Evas_List *input_methods;
   Evas_List *im_list;
   Evas_List *l;
   char *imc_path;

   im_list = NULL;

   /* Personal Path */
   input_methods = _e_intl_imc_dir_scan(e_intl_imc_personal_path_get());
   for (l = input_methods; l; l = l->next)
     {
	imc_path = l->data;
	im_list = evas_list_append(im_list, imc_path);
     }

   while (input_methods)
     input_methods = evas_list_remove_list(input_methods, input_methods);

   /* System Path */
   input_methods = _e_intl_imc_dir_scan(e_intl_imc_system_path_get());
   for (l = input_methods; l; l = l->next)
     {
	imc_path = l->data;
	im_list = evas_list_append(im_list, imc_path);
     }

   while (input_methods)
     input_methods = evas_list_remove_list(input_methods, input_methods);

   return im_list;
}

const char *
e_intl_imc_personal_path_get(void)
{
   if (_e_intl_imc_personal_path == NULL)
     {
	char buf[4096];

	snprintf(buf, sizeof(buf), "%s/.e/e/input_methods", e_user_homedir_get());
	_e_intl_imc_personal_path = eina_stringshare_add(buf);
     }
   return _e_intl_imc_personal_path;
}

const char *
e_intl_imc_system_path_get(void)
{
   if (_e_intl_imc_system_path == NULL)
     {
	char buf[4096];

	snprintf(buf, sizeof(buf), "%s/data/input_methods", e_prefix_data_get());
	_e_intl_imc_system_path = eina_stringshare_add(buf);
     }
   return _e_intl_imc_system_path;
}

static int
_e_intl_cb_exit(void *data, int type, void *event)
{
   Ecore_Exe_Event_Del *ev;

   ev = event;
   if (!ev->exe) return 1;

   if (!(ecore_exe_tag_get(ev->exe) &&
	(!strcmp(ecore_exe_tag_get(ev->exe), "E/im_exec")))) return 1;

   _e_intl_input_method_exec = NULL;
   return 1;
}

static void
_e_intl_locale_hash_free(Evas_Hash *locale_hash)
{
   if (!locale_hash) return;
   evas_hash_foreach(locale_hash, _e_intl_locale_hash_free_cb, NULL);
   evas_hash_free(locale_hash);
}

static Evas_Bool
_e_intl_locale_hash_free_cb(const Evas_Hash *hash __UNUSED__, const char *key __UNUSED__, void *data, void *fdata __UNUSED__)
{
   free(data);
   return 1;
}


/*
 * get the directory associated with the language. Language Must be valid alias
 * i.e. Already validated and already de-aliased.
 *
 * NULL means:
 *  1) The user does not have an enlightenment translation for this locale
 *  2) The user does not have their locale.aliases configured correctly
 *
 * @return NULL if not found.
 */
static char *
_e_intl_language_path_find(char *language)
{
   char		*directory;
   Evas_List	*dir_list;
   Evas_List	*search_list;
   Evas_List	*next_dir;
   Evas_List	*next_search;
   int		 found;

   search_list = _e_intl_locale_search_order_get(language);

   if (search_list == NULL) return NULL;

   directory = NULL;
   found = 0;
   dir_list = e_path_dir_list_get(path_messages);

   /* For each directory in the path */
   for (next_dir = dir_list; next_dir && !found; next_dir = next_dir->next)
     {
	E_Path_Dir *epd;
	epd = next_dir->data;

	/* Match canonicalized locale against each possible search */
	for (next_search = search_list; next_search && !found; next_search = next_search->next)
	  {
	     char *search_locale;
	     char message_path[PATH_MAX];

	     search_locale = next_search->data;
	     snprintf(message_path, sizeof(message_path), "%s/%s/LC_MESSAGES/%s.mo", epd->dir, search_locale, PACKAGE);

	     if ((ecore_file_exists(message_path)) && (!ecore_file_is_dir(message_path)))
	       {
		  directory = strdup(epd->dir);
		  found = 1;
	       }
	  }
     }

   e_path_dir_list_free(dir_list);

   while (search_list)
     {
	char *data;
	data = search_list->data;
	free(data);
	search_list = evas_list_remove_list(search_list, search_list);
     }

   return directory;
}

static Evas_List *
_e_intl_language_dir_scan(const char *dir)
{
   Evas_List *languages;
   Ecore_List *files;
   char *file;

   languages = NULL;

   files = ecore_file_ls(dir);
   if (!files) return NULL;

   ecore_list_first_goto(files);
   if (files)
     {
	while ((file = ecore_list_next(files)))
	  {
	     char file_path[PATH_MAX];

	     snprintf(file_path, sizeof(file_path),"%s/%s/LC_MESSAGES/%s.mo",
		   dir, file, PACKAGE);
	     if (ecore_file_exists(file_path) && !ecore_file_is_dir(file_path))
	       languages = evas_list_append(languages, strdup(file));
	  }
	ecore_list_destroy(files);
     }
   return languages;
}

/* get the alias for a locale
 *
 * return pointer to allocated alias string. never returns NULL
 * String will be the same if its a valid locale already or there
 * is no alias.
 */
static char *
_e_intl_locale_alias_get(const char *language)
{
   Evas_Hash *alias_hash;
   char *alias;
   char *lower_language;
   int i;

   if ((language == NULL) || (!strncmp(language, "POSIX", strlen("POSIX"))))
     return strdup("C");

   alias_hash = _e_intl_locale_alias_hash_get();
   if (alias_hash == NULL) /* No alias file available */
     return strdup(language);

   lower_language = malloc(strlen(language) + 1);
   for (i = 0; i < strlen(language); i++)
     lower_language[i] = tolower(language[i]);
   lower_language[i] = 0;

   alias = evas_hash_find(alias_hash, lower_language);
   free(lower_language);

   if (alias)
     alias = strdup(alias);
   else
     alias = strdup(language);

   _e_intl_locale_hash_free(alias_hash);

   return alias;
}

static Evas_Hash *
_e_intl_locale_alias_hash_get(void)
{
   Evas_List *next;
   Evas_List *dir_list;
   Evas_Hash *alias_hash;

   dir_list = e_path_dir_list_get(path_messages);
   alias_hash = NULL;

   for (next = dir_list; next; next = next->next)
     {
	char buf[4096];
	E_Path_Dir *epd;
	FILE *f;

	epd = next->data;

	snprintf(buf, sizeof(buf), "%s/locale.alias", epd->dir);
	f = fopen(buf, "r");
	if (f)
	  {
	     char alias[4096], locale[4096];

	     /* read locale alias lines */
	     while (fscanf(f, "%4090s %[^\n]\n", alias, locale) == 2)
	       {
		  /* skip comments */
		  if ((alias[0] == '!') || (alias[0] == '#'))
		    continue;

		  /* skip dupes */
		  if (evas_hash_find(alias_hash, alias))
		    continue;

		  alias_hash = evas_hash_add(alias_hash, alias, strdup(locale));
	       }
	     fclose (f);
	  }
     }
   e_path_dir_list_free(dir_list);

   return alias_hash;
}

/* return parts of the locale that are requested in the mask
 * return null if the locale looks to be invalid (Does not have
 * ll_DD)
 *
 * the returned string needs to be freed
 */
/*
 * Not canonic, just gets the parts
 */
EAPI E_Locale_Parts *
e_intl_locale_parts_get(const char *locale)
{
   /* Parse Results */
   E_Locale_Parts *locale_parts;
   char  language[4];
   char  territory[4];
   char  codeset[32];
   char  modifier[32];

   /* Parse State */
   int   state = 0; /* start out looking for the language */
   int   locale_idx;
   int   tmp_idx = 0;

   /* Parse Loop - Seperators are _ . @ */
   for (locale_idx = 0; locale_idx < strlen(locale); locale_idx++)
     {
	char locale_char;
	locale_char = locale[locale_idx];

	/* we have finished scanning the locale string */
	if (!locale_char)
	  break;

	/* scan this character based on the current start */
	switch (state)
	  {
	   case 0: /* Gathering Language */
	     if (tmp_idx == 2 && locale_char == '_')
	       {
		  state++;
		  language[tmp_idx] = 0;
		  tmp_idx = 0;
	       }
	     else if ((tmp_idx < 2) && (islower(locale_char)))
	       language[tmp_idx++] = locale_char;
	     else
	       return NULL;
	     break;
	   case 1: /* Gathering Territory */
	     if (tmp_idx == 2 && locale_char == '.')
	       {
		  state++;
		  territory[tmp_idx] = 0;
		  tmp_idx = 0;
	       }
	     else if ((tmp_idx == 2) && (locale_char == '@'))
	       {
		  state += 2;
		  territory[tmp_idx] = 0;
		  codeset[0] = 0;
		  tmp_idx = 0;
	       }
	     else if ((tmp_idx < 2) && isupper(locale_char))
	       territory[tmp_idx++] = locale_char;
	     else
	       return NULL;
	     break;
	   case 2: /* Gathering Codeset */
	     if (locale_char == '@')
	       {
		  state++;
		  codeset[tmp_idx] = 0;
		  tmp_idx = 0;
	       }
	     else if (tmp_idx < 32)
	       codeset[tmp_idx++] = locale_char;
	     else
	       return NULL;
	     break;
	   case 3: /* Gathering modifier */
	     if (tmp_idx < 32)
	       modifier[tmp_idx++] = locale_char;
	     else
	       return NULL;
	     break;
	   default:
	     break;
	  }
     }

   /* set end-of-string \0 */
   /* There are no breaks here on purpose */
   switch (state)
     {
      case 0:
	language[tmp_idx] = 0;
	tmp_idx = 0;
      case 1:
	territory[tmp_idx] = 0;
	tmp_idx = 0;
      case 2:
	codeset[tmp_idx] = 0;
	tmp_idx = 0;
      case 3:
	modifier[tmp_idx] = 0;
      default:
	break;
     }

   locale_parts = E_NEW(E_Locale_Parts, 1);

   /* Put the parts of the string together */
   if (language[0] != 0)
     {
	locale_parts->mask |= E_INTL_LOC_LANG;
	locale_parts->lang = eina_stringshare_add(language);
     }
   if (territory[0] != 0)
     {
	locale_parts->mask |= E_INTL_LOC_REGION;
	locale_parts->region = eina_stringshare_add(territory);
     }
   if (codeset[0] != 0)
     {
	locale_parts->mask |= E_INTL_LOC_CODESET;
	locale_parts->codeset = eina_stringshare_add(codeset);
     }
   if (modifier[0] != 0)
     {
	locale_parts->mask |= E_INTL_LOC_MODIFIER;
	locale_parts->modifier = eina_stringshare_add(modifier);
     }

   return locale_parts;
}

EAPI void
e_intl_locale_parts_free(E_Locale_Parts *locale_parts)
{
   if (locale_parts != NULL)
     {
	if (locale_parts->lang) eina_stringshare_del(locale_parts->lang);
	if (locale_parts->region) eina_stringshare_del(locale_parts->region);
	if (locale_parts->codeset) eina_stringshare_del(locale_parts->codeset);
	if (locale_parts->modifier) eina_stringshare_del(locale_parts->modifier);
	E_FREE(locale_parts);
     }
}

EAPI char *
e_intl_locale_parts_combine(E_Locale_Parts *locale_parts, int mask)
{
   int locale_size;
   char *locale;

   if (!locale_parts) return NULL;

   if ((mask & locale_parts->mask) != mask) return NULL;

   /* Construct the clean locale string */
   locale_size = 0;

   /* determine the size */
   if (mask & E_INTL_LOC_LANG)
     locale_size =  strlen(locale_parts->lang) + 1;

   if (mask & E_INTL_LOC_REGION)
     locale_size += strlen(locale_parts->region) + 1;

   if (mask & E_INTL_LOC_CODESET)
     locale_size += strlen(locale_parts->codeset) + 1;

   if (mask & E_INTL_LOC_MODIFIER)
     locale_size += strlen(locale_parts->modifier) + 1;

   /* Allocate memory */
   locale = (char *) malloc(locale_size);
   locale[0] = 0;

   if (mask & E_INTL_LOC_LANG)
     strcat(locale, locale_parts->lang);

   if (mask & E_INTL_LOC_REGION)
     {
	if (locale[0] != 0) strcat(locale, "_");
	strcat(locale, locale_parts->region);
      }

   if (mask & E_INTL_LOC_CODESET)
      {
	 if (locale[0] != 0) strcat(locale, ".");
	 strcat(locale, locale_parts->codeset);
      }

   if (mask & E_INTL_LOC_MODIFIER)
     {
	if (locale[0] != 0) strcat(locale, "@");
	strcat(locale, locale_parts->modifier);
     }

   return locale;
}

EAPI char *
e_intl_locale_charset_canonic_get(const char *charset)
{
   char charset_canonic[32];
   char c;
   int i, i_tmp;

   i = 0;
   i_tmp = 0;
   while ((c = charset[i++]) != 0)
     {
	if (isalnum(c))
	  charset_canonic[i_tmp++] = tolower(c);
     }
   charset_canonic[i_tmp] = 0;

   if (!strcmp(charset, charset_canonic))
     return NULL;

   return strdup(charset_canonic);
}

static Evas_List *
_e_intl_locale_system_locales_get(void)
{
   Evas_List	*locales;
   FILE		*output;

   locales = NULL;
   output = popen("locale -a", "r");
   if (output)
     {
	char line[32];
	while (fscanf(output, "%[^\n]\n", line) == 1)
	  locales = evas_list_append(locales, strdup(line));

	pclose(output);
     }
   return locales;
}

/*
 * must be an un aliased locale;
 */
static int
_e_intl_locale_validate(const char *locale)
{
   Evas_List *all_locales;
   E_Locale_Parts *locale_parts;
   char *locale_lr;
   char *locale_cs_canonic;
   int   found;

   found = 0;

   locale_parts = e_intl_locale_parts_get(locale);

   /* Gather the search information */
   locale_lr = 
     e_intl_locale_parts_combine(locale_parts, 
				 E_INTL_LOC_LANG | E_INTL_LOC_REGION);

   if (locale_lr == NULL)
     {
	/* Not valid locale, maybe its an alias */
	locale_lr = strdup(locale);
	locale_cs_canonic = NULL;
     }
   else
     {
	if (locale_parts && locale_parts->codeset)
	  locale_cs_canonic = e_intl_locale_charset_canonic_get(locale_parts->codeset);
	else
	  locale_cs_canonic = NULL;
     }

   /* Get list of all available locales and aliases */
   all_locales = _e_intl_locale_system_locales_get();

   /* Match locale with one from the list */
   while (all_locales)
     {
	char *locale_next;
	locale_next = all_locales->data;

	if (found == 0)
	  {
	     E_Locale_Parts *locale_parts_next;
	     char * locale_lr_next;

	     locale_parts_next = e_intl_locale_parts_get(locale_next);
	     locale_lr_next = e_intl_locale_parts_combine(locale_parts_next,
		   E_INTL_LOC_LANG | E_INTL_LOC_REGION);

	     if ((locale_parts) && (locale_lr_next) && 
		 (!strcmp(locale_lr, locale_lr_next)))
	       {
		  /* Matched lang/region part, now if CS matches */
		  if ((locale_parts->codeset == NULL) && (locale_parts_next->codeset == NULL))
		    {
		       /* Lang/Region parts match and no charsets,
			* we have a match
			*/
		       found = 1;
		    }
		  else if (locale_parts->codeset && locale_parts_next->codeset)
		    {
		       if (!strcmp(locale_parts->codeset, locale_parts_next->codeset))
			 {
			    /* Lang/Region and charsets match */
			    found = 1;
			 }
		       else if (locale_cs_canonic)
			 {
			    char *locale_cs_canonic_next;
			    /* try to match charsets in canonic form */

			    locale_cs_canonic_next =
			       e_intl_locale_charset_canonic_get(locale_parts_next->codeset);

			    if (locale_cs_canonic_next)
			      {
				 if (!strcmp(locale_cs_canonic, locale_cs_canonic_next))
				   {
				      /* Lang/Resion and charsets in canonic
				       * form match
				       */
				      found = 1;
				   }
				 free(locale_cs_canonic_next);
			      }
			    else
			      {
				 if (!strcmp(locale_cs_canonic, locale_parts_next->codeset))
				   {
				      /* Lang/Resion and charsets in canonic
				       * form match
				       */
				      found = 1;
				   }
			      }
			 }
		    }
	       }
	     else
	       {
		  /* Its an alias */
		  if (!strcmp(locale_lr, locale_next)) found = 1;
	       }
	     e_intl_locale_parts_free(locale_parts_next);
	     E_FREE(locale_lr_next);
	  }

	all_locales = evas_list_remove_list(all_locales, all_locales);
	free(locale_next);
     }
   e_intl_locale_parts_free(locale_parts);
   free(locale_lr);
   E_FREE(locale_cs_canonic);
   return found;
}

/*
 *  arg local must be an already validated and unaliased locale
 *  returns the locale search order e.g.
 *  en_US.UTF-8 ->
 *   Mask (b) Locale (en_US.UTF-8)
 *   Mask (a) Locale (en_US)
 *   Mask (9) Locale (en.UTF-8)
 *   Mask (8) Locale (en)
 */
static Evas_List *
_e_intl_locale_search_order_get(const char *locale)
{
   Evas_List *search_list;
   E_Locale_Parts *locale_parts;
   char *masked_locale;
   int mask;

   locale_parts = e_intl_locale_parts_get(locale);
   if (locale_parts == NULL) return NULL;

   search_list = NULL;
   for (mask = E_INTL_LOC_ALL; mask >= E_INTL_LOC_LANG; mask--)
     {
	if ((mask & locale_parts->mask) == mask)
	  {
	     /* Only append if the mask we need is available */
	     masked_locale = e_intl_locale_parts_combine(locale_parts, mask);
	     search_list = evas_list_append(search_list, masked_locale);
	  }
     }
   e_intl_locale_parts_free(locale_parts);
   return search_list;
}

static Evas_List *
_e_intl_imc_dir_scan(const char *dir)
{
   Evas_List *imcs;
   Ecore_List *files;
   char *file;

   imcs = NULL;

   files = ecore_file_ls(dir);
   if (!files) return NULL;

   ecore_list_first_goto(files);
   if (files)
     {
	while ((file = ecore_list_next(files)))
	  {
	     if (strstr(file, ".imc") != NULL)
	       {
		  char buf[PATH_MAX];

		  snprintf(buf, sizeof(buf), "%s/%s", dir, file);
		  imcs = evas_list_append(imcs, strdup(buf));
	       }
	  }
	ecore_list_destroy(files);
     }
   return imcs;
}
