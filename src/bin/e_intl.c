#include "e.h"

static Ecore_Exe *_e_intl_input_method_exec = NULL;
static Ecore_Event_Handler *_e_intl_exit_handler = NULL;

static char *_e_intl_orig_lang = NULL;
static char *_e_intl_orig_language = NULL;
static char *_e_intl_language = NULL;

static char *_e_intl_language_alias = NULL;

static char *_e_intl_orig_xmodifiers = NULL;
static char *_e_intl_orig_qt_im_module = NULL;
static char *_e_intl_orig_gtk_im_module = NULL;
static char *_e_intl_orig_ecore_imf_module = NULL;

static const char *_e_intl_imc_personal_path = NULL;
static const char *_e_intl_imc_system_path = NULL;

#define E_EXE_STOP(EXE)     if (EXE) { ecore_exe_terminate(EXE); ecore_exe_free(EXE); EXE = NULL; }
#define E_EXE_IS_VALID(EXE) (!((!EXE) || (EXE[0] == 0)))

/* All locale parts */
#define E_INTL_LOC_ALL         E_INTL_LOC_LANG | \
  E_INTL_LOC_REGION |                            \
  E_INTL_LOC_CODESET |                           \
  E_INTL_LOC_MODIFIER

/* Locale parts which are significant when Validating */
#define E_INTL_LOC_SIGNIFICANT E_INTL_LOC_LANG | \
  E_INTL_LOC_REGION |                            \
  E_INTL_LOC_CODESET

/* Language Setting and Listing */
static char      *_e_intl_language_path_find(char *language);
static Eina_List *_e_intl_language_dir_scan(const char *dir);
static int        _e_intl_language_list_find(Eina_List *language_list, char *language);

/* Locale Validation and Discovery */
static Eina_Hash *_e_intl_locale_alias_hash_get(void);
static char      *_e_intl_locale_alias_get(const char *language);
static Eina_List *_e_intl_locale_system_locales_get(void);
static Eina_List *_e_intl_locale_search_order_get(const char *locale);
static int        _e_intl_locale_validate(const char *locale);
static void       _e_intl_locale_hash_free(Eina_Hash *language_hash);
static Eina_Bool  _e_intl_locale_hash_free_cb(const Eina_Hash *hash, const void *key, void *data, void *fdata);

/* Input Method Configuration and Management */
static Eina_Bool  _e_intl_cb_exit(void *data, int type, void *event);
static Eina_List *_e_intl_imc_dir_scan(const char *dir);

EINTERN int
e_intl_init(void)
{
   char *s;

   e_intl_data_init();

   if ((s = getenv("LANG"))) _e_intl_orig_lang = strdup(s);
   if ((s = getenv("LANGUAGE"))) _e_intl_orig_language = strdup(s);

   if ((s = getenv("GTK_IM_MODULE"))) _e_intl_orig_gtk_im_module = strdup(s);
   if ((s = getenv("QT_IM_MODULE"))) _e_intl_orig_qt_im_module = strdup(s);
   if ((s = getenv("XMODIFIERS"))) _e_intl_orig_xmodifiers = strdup(s);
   if ((s = getenv("ECORE_IMF_MODULE"))) _e_intl_orig_ecore_imf_module = strdup(s);

   return 1;
}

EINTERN int
e_intl_shutdown(void)
{
   E_FREE(_e_intl_language);
   E_FREE(_e_intl_orig_lang);
   E_FREE(_e_intl_orig_language);

   E_FREE(_e_intl_orig_gtk_im_module);
   E_FREE(_e_intl_orig_qt_im_module);
   E_FREE(_e_intl_orig_xmodifiers);
   E_FREE(_e_intl_orig_ecore_imf_module);

   if (_e_intl_imc_personal_path)
     eina_stringshare_del(_e_intl_imc_personal_path);
   if (_e_intl_imc_system_path)
     eina_stringshare_del(_e_intl_imc_system_path);

   e_intl_data_shutdown();

   return 1;
}

/* Setup configuration settings and start services */
EINTERN int
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

EINTERN int
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
   int ok;

   set_envars = 1;
   /* NULL lang means set everything back to the original environment
    * defaults
    */
   if (!lang)
     {
        e_util_env_set("LANG", _e_intl_orig_lang);

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

   ok = 1;
   if (strcmp(_e_intl_language_alias, "C"))
     {
        ok = _e_intl_locale_validate(_e_intl_language_alias);
        if (!ok)
          {
             char *p, *new_lang;

             new_lang = _e_intl_language_alias;
             p = strchr(new_lang, '.');
             if (p) *p = 0;
             _e_intl_language_alias = strdup(new_lang);
             E_FREE(new_lang);
             ok = _e_intl_locale_validate(_e_intl_language_alias);
          }
     }
   if (!ok)
     {
        fprintf(stderr, "The locale '%s' cannot be found on your "
                        "system. Please install this locale or try "
                        "something else.", _e_intl_language_alias);
        return;
     }
   /* Only set env vars is a non NULL locale was passed */
   if (set_envars)
     {
        e_util_env_set("LANG", lang);
        /* Unset LANGUAGE, apparently causes issues if set */
        e_util_env_set("LANGUAGE", NULL);
        efreet_lang_reset();
        setlocale(LC_ALL, lang);
     }
   else
     {
        setlocale(LC_ALL, "");
     }

   if (_e_intl_language)
     {
        char *locale_path;

        locale_path = _e_intl_language_path_find(_e_intl_language_alias);
        if (!locale_path)
          {
             E_Locale_Parts *locale_parts;

             locale_parts = e_intl_locale_parts_get(_e_intl_language_alias);

             /* If locale is C or some form of en don't report an error */
             if ((!locale_parts) &&
                 (strcmp(_e_intl_language_alias, "C")))
               {
                  fprintf(stderr,
                          "An error occurred setting your locale. \n\n"

                          "The locale you have chosen '%s' appears to \n"
                          "be an alias, however, it can not be resolved.\n"
                          "Please make sure you have a 'locale.alias' \n"
                          "file in your 'messages' path which can resolve\n"
                          "this alias.\n\n"

                          "Moksha will not be translated.\n",
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

                          "Moksha will not be translated.\n",
                          _e_intl_language_alias);
               }
             e_intl_locale_parts_free(locale_parts);
          }
        else
          {
#ifdef HAVE_GETTEXT
             bindtextdomain(PACKAGE, locale_path);
             textdomain(PACKAGE);
             bind_textdomain_codeset(PACKAGE, "UTF-8");
#endif
             free(locale_path);
          }
     }
}

EAPI const char *
e_intl_language_get(void)
{
   return _e_intl_language;
}

EAPI const char *
e_intl_language_alias_get(void)
{
   return _e_intl_language_alias;
}

EAPI Eina_List *
e_intl_language_list(void)
{
   Eina_List *next;
   Eina_List *dir_list;
   Eina_List *all_languages;
   E_Path_Dir *epd;

   all_languages = NULL;
   dir_list = e_path_dir_list_get(path_messages);
   EINA_LIST_FOREACH(dir_list, next, epd)
     {
        Eina_List *dir_languages;
        char *language;

        dir_languages = _e_intl_language_dir_scan(epd->dir);

        EINA_LIST_FREE(dir_languages, language)
          if ((_e_intl_language_list_find(all_languages, language)) ||
              ((strlen(language) > 2) && (!_e_intl_locale_validate(language))))
            {
               free(language);
            }
          else
            all_languages = eina_list_append(all_languages, language);
     }

   e_path_dir_list_free(dir_list);

   return all_languages;
}

static int
_e_intl_language_list_find(Eina_List *language_list, char *language)
{
   Eina_List *l;
   char *lang;

   if ((!language_list) || (!language)) return 0;

   EINA_LIST_FOREACH(language_list, l, lang)
     if (!strcmp(lang, language)) return 1;

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
        e_util_env_set("ECORE_IMF_MODULE", _e_intl_orig_ecore_imf_module);
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
                  e_util_env_set("ECORE_IMF_MODULE", imc->ecore_imf_module);

                  E_EXE_STOP(_e_intl_input_method_exec);

                  if (E_EXE_IS_VALID(imc->e_im_exec))
                    {
                       // if you see valgrind complain about memory
                       // definitely lost here... it's wrong.
                       _e_intl_input_method_exec = e_util_exe_safe_run
                         (imc->e_im_exec, NULL);
                       ecore_exe_tag_set(_e_intl_input_method_exec, "E/im_exec");

                       if ((!_e_intl_input_method_exec) ||
                           (!ecore_exe_pid_get(_e_intl_input_method_exec)))
                         e_util_dialog_show(_("Input Method Error"),
                                            _("Error starting the input method executable<br><br>"
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

EAPI Eina_List *
e_intl_input_method_list(void)
{
   Eina_List *input_methods;
   Eina_List *im_list;

   im_list = NULL;

   /* Personal Path */
   im_list = _e_intl_imc_dir_scan(e_intl_imc_personal_path_get());

   /* System Path */
   input_methods = _e_intl_imc_dir_scan(e_intl_imc_system_path_get());
   im_list = eina_list_merge(im_list, input_methods);

   return im_list;
}

const char *
e_intl_imc_personal_path_get(void)
{
   if (!_e_intl_imc_personal_path)
     {
        char buf[4096];

        e_user_dir_concat_static(buf, "input_methods");
        _e_intl_imc_personal_path = eina_stringshare_add(buf);
     }
   return _e_intl_imc_personal_path;
}

const char *
e_intl_imc_system_path_get(void)
{
   if (!_e_intl_imc_system_path)
     {
        char buf[4096];

        e_prefix_data_concat_static(buf, "data/input_methods");
        _e_intl_imc_system_path = eina_stringshare_add(buf);
     }
   return _e_intl_imc_system_path;
}

static Eina_Bool
_e_intl_cb_exit(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Exe_Event_Del *ev;

   ev = event;
   if (!ev->exe) return ECORE_CALLBACK_PASS_ON;
   if (ev->exe == _e_intl_input_method_exec)
     _e_intl_input_method_exec = NULL;
   return ECORE_CALLBACK_PASS_ON;
}

static void
_e_intl_locale_hash_free(Eina_Hash *locale_hash)
{
   if (!locale_hash) return;
   eina_hash_foreach(locale_hash, _e_intl_locale_hash_free_cb, NULL);
   eina_hash_free(locale_hash);
}

static Eina_Bool
_e_intl_locale_hash_free_cb(const Eina_Hash *hash __UNUSED__, const void *key __UNUSED__, void *data, void *fdata __UNUSED__)
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
   char *directory;
   char *data;
   Eina_List *dir_list;
   Eina_List *search_list;
   Eina_List *next_dir;
   Eina_List *next_search;
   E_Path_Dir *epd;
   char *search_locale;
   int found;

   search_list = _e_intl_locale_search_order_get(language);

   if (!search_list) return NULL;

   directory = NULL;
   found = 0;
   dir_list = e_path_dir_list_get(path_messages);

   /* For each directory in the path */
   EINA_LIST_FOREACH(dir_list, next_dir, epd)
     {
        if (found) break;

        /* Match canonicalized locale against each possible search */
        EINA_LIST_FOREACH(search_list, next_search, search_locale)
          {
             char message_path[PATH_MAX];

             snprintf(message_path, sizeof(message_path), "%s/%s/LC_MESSAGES/%s.mo", epd->dir, search_locale, PACKAGE);

             if ((ecore_file_exists(message_path)) && (!ecore_file_is_dir(message_path)))
               {
                  directory = strdup(epd->dir);
                  found = 1;
                  break;
               }
          }
     }

   e_path_dir_list_free(dir_list);

   EINA_LIST_FREE(search_list, data)
     free(data);

   return directory;
}

static Eina_List *
_e_intl_language_dir_scan(const char *dir)
{
   Eina_List *languages;
   Eina_List *files;
   char *file;

   languages = NULL;

   files = ecore_file_ls(dir);
   if (!files) return NULL;

   if (files)
     {
        EINA_LIST_FREE(files, file)
          {
             char file_path[PATH_MAX];

             snprintf(file_path, sizeof(file_path), "%s/%s/LC_MESSAGES/%s.mo",
                      dir, file, PACKAGE);
             if (ecore_file_exists(file_path) && !ecore_file_is_dir(file_path))
               languages = eina_list_append(languages, file);
             else
               free(file);
          }
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
   Eina_Hash *alias_hash;
   char *alias;
   char lbuf[256];
   char *lower_language = lbuf;

   if ((!language) || (!strncmp(language, "POSIX", strlen("POSIX"))))
     return strdup("C");

   alias_hash = _e_intl_locale_alias_hash_get();
   if (!alias_hash) /* No alias file available */
     return strdup(language);

   strncpy(lbuf, language, sizeof(lbuf) - 1);
   lbuf[sizeof(lbuf) - 1] = '\0';
   eina_str_tolower(&lower_language);
   alias = eina_hash_find(alias_hash, lower_language);

   if (alias)
     alias = strdup(alias);
   else
     alias = strdup(language);

   _e_intl_locale_hash_free(alias_hash);

   return alias;
}

static Eina_Hash *
_e_intl_locale_alias_hash_get(void)
{
   Eina_List *next;
   Eina_List *dir_list;
   E_Path_Dir *epd;
   Eina_Hash *alias_hash;

   dir_list = e_path_dir_list_get(path_messages);
   alias_hash = NULL;

   EINA_LIST_FOREACH(dir_list, next, epd)
     {
        char buf[4096];
        FILE *f;

        snprintf(buf, sizeof(buf), "%s/locale.alias", epd->dir);
        f = fopen(buf, "r");
        if (f)
          {
             char alias[4096], locale[4096];

             /* read locale alias lines */
             while (fscanf(f, "%4095s %4095[^\n]\n", alias, locale) == 2)
               {
                  /* skip comments */
                  if ((alias[0] == '!') || (alias[0] == '#'))
                    continue;

                  /* skip dupes */
                  if (eina_hash_find(alias_hash, alias))
                    continue;

                  if (!alias_hash) alias_hash = eina_hash_string_superfast_new(NULL);
                  eina_hash_add(alias_hash, alias, strdup(locale));
               }
             fclose(f);
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
   char language[4] = {0};
   char territory[4] = {0};
   char codeset[32] = {0};
   char modifier[32] = {0};

   /* Parse State */
   int state = 0;   /* start out looking for the language */
   unsigned int locale_idx;
   int tmp_idx = 0;

   /* Parse Loop - Separators are _ . @ */
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
             else if (locale_char == '.') // no territory
               {
                  state = 2;
                  language[tmp_idx] = 0;
                  tmp_idx = 0;
               }
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
             else if (tmp_idx < 31)
               codeset[tmp_idx++] = locale_char;
             else
               return NULL;
             break;

           case 3: /* Gathering modifier */
             if (tmp_idx < 31)
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
        EINA_FALLTHROUGH;
        /* no break */

      case 1:
        territory[tmp_idx] = 0;
        tmp_idx = 0;
        EINA_FALLTHROUGH;
        /* no break */

      case 2:
        codeset[tmp_idx] = 0;
        tmp_idx = 0;
        EINA_FALLTHROUGH;
        /* no break */

      case 3:
        modifier[tmp_idx] = 0;
        EINA_FALLTHROUGH;
        /* no break */

      default:
        break;
     }

   if ((!language[0]) && (!territory[0]) && (!codeset[0]) && (!modifier[0])) return NULL;
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
   if (locale_parts)
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
     locale_size = strlen(locale_parts->lang) + 1;

   if (mask & E_INTL_LOC_REGION)
     locale_size += strlen(locale_parts->region) + 1;

   if (mask & E_INTL_LOC_CODESET)
     locale_size += strlen(locale_parts->codeset) + 1;

   if (mask & E_INTL_LOC_MODIFIER)
     locale_size += strlen(locale_parts->modifier) + 1;

   if (!locale_size) return NULL;

   /* Allocate memory */
   locale = (char *)malloc(locale_size);
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

static Eina_List *
_e_intl_locale_system_locales_get(void)
{
   Eina_List *locales;
   FILE *output;

   locales = NULL;
   /* FIXME: Maybe needed for other BSD OS, or even Solaris */
#ifdef __OpenBSD__
   output = popen("ls /usr/share/locale", "r");
#else
   output = popen("locale -a", "r");
#endif
   if (output)
     {
        char line[32];
        while (fscanf(output, "%31[^\n]\n", line) == 1)
          locales = eina_list_append(locales, strdup(line));

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
   Eina_List *all_locales;
   E_Locale_Parts *locale_parts;
   char *locale_next;
   char *locale_lr = NULL;
   char *locale_cs_canonic;
   int found;

   found = 0;

   locale_parts = e_intl_locale_parts_get(locale);

   /* Gather the search information */
   if (locale_parts)
     {
        if (locale_parts->mask & E_INTL_LOC_REGION)
          locale_lr = e_intl_locale_parts_combine(locale_parts, E_INTL_LOC_LANG | E_INTL_LOC_REGION);
        else if (locale_parts->lang)
          locale_lr = strdup(locale_parts->lang);
     }
   if (!locale_lr)
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
   EINA_LIST_FREE(all_locales, locale_next)
     {
        if (found == 0)
          {
             E_Locale_Parts *locale_parts_next;
             char *locale_lr_next = NULL;

             locale_parts_next = e_intl_locale_parts_get(locale_next);
             if (locale_parts_next)
               {
                  if (locale_parts_next->mask & E_INTL_LOC_REGION)
                    locale_lr_next = e_intl_locale_parts_combine(locale_parts_next,
                                                                 E_INTL_LOC_LANG | E_INTL_LOC_REGION);
                  else if (locale_parts_next->lang)
                    locale_lr_next = strdup(locale_parts_next->lang);
               }
             if ((locale_parts) && (locale_lr_next) &&
                 (!strcmp(locale_lr, locale_lr_next)))
               {
                  /* Matched lang/region part, now if CS matches */
                  if ((!locale_parts->codeset) && (!locale_parts_next->codeset))
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
static Eina_List *
_e_intl_locale_search_order_get(const char *locale)
{
   Eina_List *search_list;
   E_Locale_Parts *locale_parts;
   char *masked_locale;
   int mask;

   locale_parts = e_intl_locale_parts_get(locale);
   if (!locale_parts) return NULL;

   search_list = NULL;
   for (mask = E_INTL_LOC_ALL; mask >= E_INTL_LOC_LANG; mask--)
     {
        if ((mask & locale_parts->mask) == mask)
          {
             /* Only append if the mask we need is available */
             masked_locale = e_intl_locale_parts_combine(locale_parts, mask);
             search_list = eina_list_append(search_list, masked_locale);
          }
     }
   e_intl_locale_parts_free(locale_parts);
   return search_list;
}

static Eina_List *
_e_intl_imc_dir_scan(const char *dir)
{
   Eina_List *imcs = NULL;
   Eina_List *files;
   char *file;

   files = ecore_file_ls(dir);

   EINA_LIST_FREE(files, file)
     {
        if (strstr(file, ".imc"))
          {
             char buf[PATH_MAX];

             snprintf(buf, sizeof(buf), "%s/%s", dir, file);
             imcs = eina_list_append(imcs, strdup(buf));
          }
        free(file);
     }
   return imcs;
}

