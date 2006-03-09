/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* TODO List:
 * 
 */

static Ecore_Exe *_e_intl_input_method_exec = NULL;
static Ecore_Event_Handler *_e_intl_exit_handler = NULL;

static char *_e_intl_orig_lc_messages = NULL;
static char *_e_intl_orig_language = NULL;
static char *_e_intl_orig_lc_all = NULL;
static char *_e_intl_orig_lang = NULL;
static char *_e_intl_language = NULL;

static char *_e_intl_orig_xmodifiers = NULL;
static char *_e_intl_orig_qt_im_module = NULL; 
static char *_e_intl_orig_gtk_im_module = NULL;
static char *_e_intl_input_method = NULL;

static Eet_Data_Descriptor *_e_intl_input_method_config_edd = NULL;
static int loc_mask;

#define E_EXE_STOP(EXE) if (EXE != NULL) { ecore_exe_terminate(EXE); ecore_exe_free(EXE); EXE = NULL; }
#define E_EXE_IS_VALID(EXE) (!((EXE == NULL) || (EXE[0] == 0)))

#define E_LOC_CODESET	E_INTL_LOC_CODESET
#define E_LOC_REGION	E_INTL_LOC_REGION
#define E_LOC_MODIFIER	E_INTL_LOC_MODIFIER
#define E_LOC_LANG	E_INTL_LOC_LANG

#define E_LOC_ALL	E_LOC_LANG | E_LOC_REGION | E_LOC_CODESET | E_LOC_MODIFIER
#define E_LOC_SIGNIFICANT E_LOC_LANG | E_LOC_REGION | E_LOC_CODESET

/* Language Setting and Listing */
static char		*_e_intl_language_path_find(char *language);
static Evas_List 	*_e_intl_language_dir_scan(const char *dir);
static int 		 _e_intl_language_list_find(Evas_List *language_list, char *language);

/* Locale Validation and Discovery */
static char		*_e_intl_locale_alias_get(char *language); 
static Evas_Hash	*_e_intl_locale_alias_hash_get(void);
static Evas_List	*_e_intl_locale_system_locales_get(void);
static Evas_List	*_e_intl_locale_search_order_get(char *locale);
static int		 _e_intl_locale_validate(char *locale);
static void 		 _e_intl_locale_hash_free(Evas_Hash *language_hash);
Evas_Bool 		 _e_intl_locale_hash_free_cb(Evas_Hash *hash, const char *key, void *data, void *fdata);

/* Input Method Configuration and Management */
static int 		 _e_intl_cb_exit(void *data, int type, void *event);
static Evas_List 	*_e_intl_imc_path_scan(E_Path *path);
static Evas_List 	*_e_intl_imc_dir_scan(const char *dir);
static E_Input_Method_Config *_e_intl_imc_find(Evas_List *imc_list, char *name);


EAPI int
e_intl_init(void)
{
   char *s;
   
   _e_intl_input_method_config_edd = E_CONFIG_DD_NEW("input_method_config", E_Input_Method_Config);
   E_CONFIG_VAL(_e_intl_input_method_config_edd, E_Input_Method_Config, version, INT);
   E_CONFIG_VAL(_e_intl_input_method_config_edd, E_Input_Method_Config, e_im_name, STR);
   E_CONFIG_VAL(_e_intl_input_method_config_edd, E_Input_Method_Config, gtk_im_module, STR);
   E_CONFIG_VAL(_e_intl_input_method_config_edd, E_Input_Method_Config, qt_im_module, STR);
   E_CONFIG_VAL(_e_intl_input_method_config_edd, E_Input_Method_Config, xmodifiers, STR);
   E_CONFIG_VAL(_e_intl_input_method_config_edd, E_Input_Method_Config, e_im_exec, STR);
   

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
   
   E_CONFIG_DD_FREE(_e_intl_input_method_config_edd);
  
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

   _e_intl_exit_handler = ecore_event_handler_add(ECORE_EXE_EVENT_DEL, _e_intl_cb_exit, NULL);
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
   char *alias_locale;
   int set_envars;
   
   if (_e_intl_language) free(_e_intl_language);
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
    
   if (lang)
     _e_intl_language = strdup(lang);
   else
     _e_intl_language = NULL;

   alias_locale = _e_intl_locale_alias_get(_e_intl_language);
   if (!_e_intl_locale_validate(alias_locale))
     {
	fprintf(stderr, "The locale '%s' cannot be found on your "
	       "system. Please install this locale or try "
               "something else.\n", alias_locale);
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
             
             locale_path = _e_intl_language_path_find(alias_locale);
             if (locale_path == NULL)
	       {
		  char * match_lang;

		  match_lang = e_intl_locale_canonic_get(alias_locale, E_LOC_LANG);
		  
		  /* If locale is C or some form of en don't report an error */
		  if ( match_lang == NULL && strcmp (alias_locale, "C") )
		    {
		       fprintf(stderr, "The locale you have chosen '%s' "
			     "appears to be an alias, however, it can not be "
			     "resloved. Please make sure you have a "
			     "'locale.aliases' file in your 'messages' path "
			     "which can resolve this alias.\n"
			     "\n"
			     "Enlightenment will not be translated.\n", 
			     alias_locale);
		    }
		  else if ( match_lang != NULL && strcmp(match_lang, "en") ) 
		    {
		       fprintf(stderr, "The translation files for the "
			     "locale you have chosen (%s) cannot be found in "
			     "your 'messages' path.\n"
			     "\n"
			     "Enlightenment will not be translated.\n", 
			     alias_locale);
		    }
		  E_FREE(match_lang);
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
   free(alias_locale);
}

EAPI const char *
e_intl_language_get(void)
{
   return _e_intl_language;
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

	     if (  _e_intl_language_list_find(all_languages, language) || (strlen(language) > 2 && 
		      !_e_intl_locale_validate(language)))
	       {
		  free(language);
	       }
	     else
	       {
		  all_languages = evas_list_append(all_languages, language);
	       }
	  }
     }
   
   e_path_dir_list_free(dir_list);

   return all_languages;
}

static int
_e_intl_language_list_find(Evas_List *language_list, char * language)
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
e_intl_input_method_set(const char *method)
{
   if (_e_intl_input_method) free(_e_intl_input_method);

   if (!method)
     {
	E_EXE_STOP(_e_intl_input_method_exec); 
	e_util_env_set("GTK_IM_MODULE", _e_intl_orig_gtk_im_module);
        e_util_env_set("QT_IM_MODULE", _e_intl_orig_qt_im_module);
        e_util_env_set("XMODIFIERS", _e_intl_orig_xmodifiers);
     }	
   
   if (method) 
     {   
	Evas_List * input_methods;
	E_Input_Method_Config *imc;

	input_methods = _e_intl_imc_path_scan(path_input_methods);
	_e_intl_input_method = strdup(method);
	
	imc = _e_intl_imc_find (input_methods, _e_intl_input_method);	
	
	     if (imc) 	  
	       {	     
	          e_util_env_set("GTK_IM_MODULE", imc->gtk_im_module);
	          e_util_env_set("QT_IM_MODULE", imc->qt_im_module);
	          e_util_env_set("XMODIFIERS", imc->xmodifiers);
		 
		  E_EXE_STOP(_e_intl_input_method_exec); 
		  
		  if (E_EXE_IS_VALID(imc->e_im_exec)) 
		    {
		       _e_intl_input_method_exec = ecore_exe_run(imc->e_im_exec, NULL);
		       ecore_exe_tag_set(_e_intl_input_method_exec,"E/im_exec");
	       	       
		       if (  !_e_intl_input_method_exec || 
			     !ecore_exe_pid_get(_e_intl_input_method_exec))    
			 e_util_dialog_show(_("Input Method Error"),
					    _( "Error starting the input method executable<br><br>"
					       
					       "please make sure that your input<br>"
					       "method configuration is correct and<br>"
					       "that your configuration's<br>"
					       "executable is in your PATH<br>"));  
		    }
	       }	

	/* Free up the directory listing */
       	while (input_methods)
	  {
	     E_Input_Method_Config *imc;
	     
	     imc = input_methods->data;	     
	     input_methods = evas_list_remove_list(input_methods,input_methods);
	     e_intl_input_method_config_free (imc); 
	  }
     }   
   else
     {
	_e_intl_input_method = NULL;
     }   
}

EAPI const char *
e_intl_input_method_get(void)
{
   return _e_intl_input_method;   
}

EAPI Evas_List *
e_intl_input_method_list(void)
{
   Evas_List *input_methods;
   Evas_List *im_list;
   Evas_List *l;
   E_Input_Method_Config *imc;

   im_list = NULL;
   
   input_methods = _e_intl_imc_path_scan(path_input_methods);
   for (l = input_methods; l; l = l->next)
     {
	imc = l->data;
	im_list = evas_list_append(im_list, strdup(imc->e_im_name));
     }

   /* Need to free up the directory listing */
   while (input_methods)
     {
	E_Input_Method_Config *imc;
	     
	imc = input_methods->data;	     
	input_methods = evas_list_remove_list(input_methods, input_methods);
	e_intl_input_method_config_free (imc);
     }
   return im_list;
}

/* Get the input method configuration from the file */
EAPI E_Input_Method_Config *
e_intl_input_method_config_read (Eet_File * imc_file)
{
   E_Input_Method_Config *imc;
   
   imc = NULL;
   if (imc_file)	  
     {     
	imc = (E_Input_Method_Config *) eet_data_read(imc_file, _e_intl_input_method_config_edd, "imc");
     }
   return imc;
}

/* Write the input method configuration to the file */
EAPI int
e_intl_input_method_config_write (Eet_File * imc_file, E_Input_Method_Config * imc)
{
   int ok = 0;

   if (imc_file)
     {
	ok = eet_data_write(imc_file, _e_intl_input_method_config_edd, "imc", imc, 0);
     }
   return ok;
}

EAPI void
e_intl_input_method_config_free (E_Input_Method_Config *imc)
{
   if (imc != NULL)
     {
	if (imc->e_im_name) evas_stringshare_del(imc->e_im_name);
	if (imc->gtk_im_module) evas_stringshare_del(imc->gtk_im_module);
	if (imc->qt_im_module) evas_stringshare_del(imc->qt_im_module);
	if (imc->xmodifiers) evas_stringshare_del(imc->xmodifiers);
	if (imc->e_im_exec) evas_stringshare_del(imc->e_im_exec);
	E_FREE(imc);
     }
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
   if(!locale_hash) return;
   evas_hash_foreach(locale_hash, _e_intl_locale_hash_free_cb, NULL);
   evas_hash_free(locale_hash);
}

Evas_Bool
_e_intl_locale_hash_free_cb(Evas_Hash *hash __UNUSED__, const char *key __UNUSED__, void *data, void *fdata __UNUSED__)
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
   for (next_dir = dir_list ; next_dir ; next_dir = next_dir->next)
     {
	E_Path_Dir *epd;
	Ecore_List *files;
	epd = next_dir->data;
	
	/* For each directory in the locale path */
	files = ecore_file_ls(epd->dir);
	if (files)
	  {
	     char *file;
	     
	     ecore_list_goto_first(files);
	     while ((file = ecore_list_next(files)) != NULL)
	       {
		  /* Match canonicalized locale against each possible search */
		  for (next_search = search_list ; next_search && !found ; next_search = next_search->next)
		    {
		       char *clean_file;
		       char *search_locale;
		       
		       search_locale = next_search->data;
		       clean_file = e_intl_locale_canonic_get(file, E_LOC_ALL);
		       /* Match directory with the search locale */
		       if (clean_file && !strcmp(clean_file, search_locale))
			 {
			    char message_path[PATH_MAX];
			    snprintf(message_path, sizeof(message_path), "%s/%s/LC_MESSAGES/%s.mo", epd->dir, search_locale, PACKAGE);
			    if (ecore_file_exists(message_path) && !ecore_file_is_dir(message_path))
			      {
				 directory = strdup(epd->dir);
			      }
			 }
		       E_FREE(clean_file);
		    } 
	       }
	     ecore_list_destroy(files);
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
  
   ecore_list_goto_first(files);
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
_e_intl_locale_alias_get(char *language)
{
   Evas_Hash *alias_hash;
   char *canonic;
   char *alias;
    
   if (language == NULL || !strncmp(language, "POSIX", strlen("POSIX")))
     return strdup("C");
   
   canonic = e_intl_locale_canonic_get(language, E_LOC_ALL );
   
   alias_hash = _e_intl_locale_alias_hash_get();
   if (alias_hash == NULL) 
     {
	if (canonic == NULL)
	  return strdup(language);
	else 
	  return canonic;
     }

   if (canonic == NULL) /* not not a locale */
     {
	char *lower_language;
	int i;
	
	lower_language = (char *) malloc(strlen(language) + 1);
	for (i = 0; i < strlen(language); i++)
	     lower_language[i] = tolower(language[i]);
	lower_language[i] = 0;     
	
	alias = (char *) evas_hash_find(alias_hash, lower_language);
	free(lower_language);
     }
   else
     {
	alias = (char *) evas_hash_find(alias_hash, canonic);
     }
   
   _e_intl_locale_hash_free(alias_hash);
   
   if (alias) 
     {
	alias = strdup(alias);
	if (canonic) free(canonic);
     }
   else if (canonic) 
     {
	alias = canonic;
     }
   else 
     {
	alias = strdup(language);
     }
   
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
   
   for (next = dir_list ; next ; next = next->next)
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
	     
	     /* read font alias lines */
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
EAPI char *
e_intl_locale_canonic_get(char *locale, int ret_mask)
{
   char *clean_locale;
   int   clean_e_intl_locale_size;
   char  language[4];
   char  territory[4];
   char  codeset[32];
   char  modifier[32];

   int   state = 0; /* start out looking for the language */
   int   locale_idx;
   int   tmp_idx = 0;
  
   loc_mask = 0; 
   /* Seperators are _ . @ */

   for ( locale_idx = 0; locale_idx < strlen(locale); locale_idx++ )
     {
	char locale_char;
	locale_char = locale[locale_idx];
	
	/* we have finished scanning the locale string */
	if(locale_char == 0)
	  break;
	
	/* scan this character based on the current start */
	switch(state)
	  {
	   case 0: /* Gathering Language */
	      if (tmp_idx == 2 && locale_char == '_')
		{
		   state++;
		   language[tmp_idx] = 0;
		   tmp_idx = 0;
		}
	      else if (tmp_idx < 2 && islower(locale_char))
		{
		   language[tmp_idx++] = locale_char;
		}
	      else
		{
		   return NULL;
		}
	      break;
	   case 1: /* Gathering Territory */
	      if (tmp_idx == 2 && locale_char == '.')
		{
		   state++;
		   territory[tmp_idx] = 0;
		   tmp_idx = 0;
		}
	      else if(tmp_idx == 2 && locale_char == '@')
		{
		   state += 2;
		   territory[tmp_idx] = 0;
		   codeset[0] = 0;
		   tmp_idx = 0;
		}
	      else if(tmp_idx < 2 && isupper(locale_char))
		{
		   territory[tmp_idx++] = locale_char;
		}
	      else 
		{
		   return NULL;
		}
	      break;
	   case 2: /* Gathering Codeset */
	      if (locale_char == '@')
		{
		   state++;
		   codeset[tmp_idx] = 0;
		   tmp_idx = 0;
		}
	      else if (isalnum(locale_char))
		{
		   codeset[tmp_idx++] = tolower(locale_char);
		}
	     break;
	   case 3: /* Gathering modifier */
	     modifier[tmp_idx++] = locale_char;
	  }
     }

   /* set end-of-string \0 */
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
     }

   /* Construct the clean locale string */

   /* determine the size */
   clean_e_intl_locale_size =	strlen(language) + 1;
   
   if ((ret_mask & E_LOC_REGION) && territory[0] != 0)
     clean_e_intl_locale_size += strlen(territory) + 1;
   
   if ((ret_mask & E_LOC_CODESET) && codeset[0] != 0)
     clean_e_intl_locale_size += strlen(codeset) + 1;
	
   if ((ret_mask & E_LOC_MODIFIER) && (modifier[0] != 0))
     clean_e_intl_locale_size += strlen(modifier) + 1;     

   /* Allocate memory */
   clean_locale = (char *) malloc(clean_e_intl_locale_size);
   clean_locale[0] = 0;
   
   /* Put the parts of the string together */
   strcat(clean_locale, language);
   loc_mask |= E_LOC_LANG;
   
   if ((ret_mask & E_LOC_REGION) && territory[0] != 0)
     {
	loc_mask |= E_LOC_REGION;
	strcat(clean_locale, "_");
	strcat(clean_locale, territory);
     }	
   if ((ret_mask & E_LOC_CODESET) && codeset[0] != 0)	  
     {
	loc_mask |= E_LOC_CODESET;
	strcat(clean_locale, ".");
	strcat(clean_locale, codeset);	  
     }	
   if ((ret_mask & E_LOC_MODIFIER) && (modifier[0] != 0))     
     {	       
	loc_mask |= E_LOC_MODIFIER;
	strcat(clean_locale, "@");
	strcat(clean_locale, modifier);     
     }

   return clean_locale;
	 
}

static Evas_List *
_e_intl_locale_system_locales_get(void)
{
   Evas_List	*locales;
   FILE		*output;

   locales = NULL;
   output = popen("locale -a", "r");
   if ( output ) 
     {
	char line[32];
	while ( fscanf(output, "%[^\n]\n", line) == 1)
	  {
	     locales = evas_list_append(locales, strdup(line));
	  }
	          
	pclose(output);
     }
   return locales;
}

/*
 must be an un aliased locale;
 */
static int
_e_intl_locale_validate(char *locale)
{
   Evas_List *all_locales;
   char *search_locale;
   int found;
   
   found = 0;
   search_locale = e_intl_locale_canonic_get(locale, E_LOC_SIGNIFICANT );
   if ( search_locale == NULL )
     search_locale = strdup(locale); 
     /* If this validates their is probably an alias issue */
   
   all_locales = _e_intl_locale_system_locales_get();

   while(all_locales)
     {
	char *test_locale;
	test_locale = all_locales->data;

	if (found == 0)
	  {
	     char *clean_test_locale;
	     
	     /* FOR BSD, need to canonicalize the locale from "locale -a" */ 
	     clean_test_locale = e_intl_locale_canonic_get(test_locale, E_LOC_ALL);
	     if (clean_test_locale)
	       {
		  if (!strcmp(clean_test_locale, search_locale)) found = 1;
		  free(clean_test_locale);
	       }
	     else
	       {
		  if (!strcmp(test_locale, search_locale)) found = 1;
	       }
	  }
	
	all_locales = evas_list_remove_list(all_locales, all_locales);
	free(test_locale);
     }
   free(search_locale);
   return found; 
}

/* 
 *  arg local must be an already validated and unaliased locale
 *  
 */
static Evas_List *
_e_intl_locale_search_order_get(char *locale)
{
   Evas_List *search_list;
   char *masked_locale;
   int mask;
   
   search_list = NULL;
   for ( mask = E_LOC_ALL; mask >= E_LOC_LANG; mask-- )
       {
	  masked_locale = e_intl_locale_canonic_get(locale, mask);
	  
	  if (loc_mask == mask) 
	    search_list = evas_list_append(search_list, masked_locale);
	  else
	    free(masked_locale);
       } 
   return search_list;
}

static Evas_List *
_e_intl_imc_path_scan(E_Path *path)
{

   Evas_List *next;
   Evas_List *dir_list;
   Evas_List *all_imcs;
  
   if (!path) return NULL; 
   
   all_imcs = NULL; 
   dir_list = e_path_dir_list_get(path);
   
   for (next = dir_list ; next ; next = next->next)
     {
	E_Path_Dir *epd;
	Evas_List *dir_imcs;
	
	epd = next->data;

	dir_imcs = _e_intl_imc_dir_scan(epd->dir);
	
	while (dir_imcs)
	  {
	     E_Input_Method_Config *imc;

	     imc = dir_imcs->data;
	     dir_imcs = evas_list_remove_list(dir_imcs, dir_imcs);

	     if (_e_intl_imc_find(all_imcs, imc->e_im_name))
	       {
		  e_intl_input_method_config_free(imc);
	       }
	     else
	       {
		  all_imcs = evas_list_append(all_imcs, imc);
	       }
	  }
     }
   
   e_path_dir_list_free(dir_list);  

   return all_imcs;
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
  
   ecore_list_goto_first(files);
   if (files)
     {
	while ((file = ecore_list_next(files)))
	  {
	     E_Input_Method_Config *imc;
	     Eet_File *imc_file;
	     char buf[PATH_MAX]; 
	     
	     snprintf(buf, sizeof(buf), "%s/%s", dir, file);	     
	     imc_file = eet_open(buf, EET_FILE_MODE_READ);
	     if (imc_file)
	       {
		  imc = e_intl_input_method_config_read (imc_file);
		  if (imc)
		    {
		       imcs = evas_list_append(imcs, imc);
		    }
	       }
	  }
	ecore_list_destroy(files);
     }
   return imcs;
}

static E_Input_Method_Config *
_e_intl_imc_find(Evas_List *imc_list, char * name)
{
   Evas_List *l;
   
   if (!imc_list) return NULL;
   if (!name) return NULL;

   for (l = imc_list; l; l = l->next)
     {
	E_Input_Method_Config *imc;

	imc = l->data;
	if (!strcmp(imc->e_im_name, name)) return imc;
     }
   
   return NULL;
}
