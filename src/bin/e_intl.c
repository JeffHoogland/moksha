/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "config.h"

/* TODO List:
 * 
 * * load/save language in config so u can change language runtime via a gui and/or ipc
 * * add ipc to get/set/list languages, get language name, simplified language string, etc. (so a config tool can be written to display supported languages and be able to select from them)
 * * add more language names to the language name list list in e_intl_language_name_get()
 * * as we get translations add languages to the simplified lang list (C and en are currently the same, ja is a test translation - incomplete)
 */

static char *_e_intl_orig_lc_messages = NULL;
static char *_e_intl_orig_language = NULL;
static char *_e_intl_orig_lc_all = NULL;
static char *_e_intl_orig_lang = NULL;
static char *_e_intl_language = NULL;
static Evas_List *_e_intl_languages = NULL;

#define ADD_LANG(lang) _e_intl_languages = evas_list_append(_e_intl_languages, lang)

int
e_intl_init(void)
{
   char *s;
   
   if (_e_intl_languages) return 1;

   /* supporeted languages - as we get translations - add them here
    * 
    * if you add a language:
    * 
    * NOTE: add a language NAME for this in e_intl_language_name_get() if
    *       there isn't one yet (use the english name - then we will add
    *       translations of the language names to the .po file)
    * NOTE: add a translation logic list to take all possible ways to address
    *       a language locale and convert it to a simplified one that is in
    *       the list here below. languages can often have multiple ways of
    *       being addressed (same language spoken in multiple countries or
    *       many variants of the language). this translation allows all the
    *       variants to be used and mapped to a simple "single" name for that
    *       language. if the differences in variants are large (eg simplified
    *       vs. traditional chinese) we may refer to them as separate languages
    *       entirely.
    */
   /* FIXME: remove this - hunt locale dirs (a user one in ~/.e/e/ too for
    * user installed locale support
    */
   ADD_LANG("");
   ADD_LANG("en_US.UTF-8");
   ADD_LANG("ja_JP.UTF-8");
   ADD_LANG("fr_FR.UTF-8");
   ADD_LANG("es_AR.UTF-8");
   ADD_LANG("pt_BR.UTF-8");
   ADD_LANG("fi_FI.UTF-8");
   ADD_LANG("ru_RU.UTF-8");
   ADD_LANG("bg_BG.UTF-8");
   ADD_LANG("de_DE.UTF-8");
   ADD_LANG("pl_PL.UTF-8");
   ADD_LANG("zh_CN.UTF-8");
   ADD_LANG("hu_HU.UTF-8");
   ADD_LANG("sl_SI.UTF-8");
   ADD_LANG("it_IT.UTF-8");
   ADD_LANG("cs_CS.UTF-8");
   ADD_LANG("da_DK.UTF-8");
   ADD_LANG("sk_SK.UTF-8");
   ADD_LANG("sv_SV.UTF-8");

   if ((s = getenv("LC_MESSAGES"))) _e_intl_orig_lc_messages = strdup(s);
   if ((s = getenv("LANGUAGE"))) _e_intl_orig_language = strdup(s);
   if ((s = getenv("LC_ALL"))) _e_intl_orig_lc_all = strdup(s);
   if ((s = getenv("LANG"))) _e_intl_orig_lang = strdup(s);
   
   /* FIXME: NULL == use LANG. make this read a config value if it exists */
   e_intl_language_set(NULL);
   return 1;
}

int
e_intl_shutdown(void)
{
   E_FREE(_e_intl_language);
   E_FREE(_e_intl_orig_lc_messages);
   E_FREE(_e_intl_orig_language);
   E_FREE(_e_intl_orig_lc_all);
   E_FREE(_e_intl_orig_lang);
   evas_list_free(_e_intl_languages);
   return 1;
}

/* FIXME: finish this */
static Evas_List *
_e_intl_dir_scan(char *dir)
{
   Ecore_List *files;
   char *file;
   
   files = ecore_file_ls(dir);
   if (!files) return NULL;
   
   ecore_list_goto_first(files);
   if (files)
     {
	while ((file = ecore_list_next(files)))
	  {
	     /* Do something! */
	  }
	ecore_list_destroy(files);
     }
   return NULL;
}

void
e_intl_language_set(const char *lang)
{
   /* 1 list ~/.e/e/locale contents */
   /* 2 list e_preifx_locale_get() contents */
   
   /* FIXME: determine if in user or system locale dir */
   if (_e_intl_language) free(_e_intl_language);
   /* NULL lang means set everything back to the original environemtn defaults */
   if (!lang)
     {
	if (_e_intl_orig_lc_messages) e_util_env_set("LC_MESSAGES", _e_intl_orig_lc_messages);
	if (_e_intl_orig_language) e_util_env_set("LANGUAGE", _e_intl_orig_language);
	if (_e_intl_orig_lc_all) e_util_env_set("LC_ALL", _e_intl_orig_lc_all);
	if (_e_intl_orig_lang) e_util_env_set("LANG", _e_intl_orig_lang);
     }
   if (!lang) lang = getenv("LC_MESSAGES");
   if (!lang) lang = getenv("LANGUAGE");
   if (!lang) lang = getenv("LC_ALL");
   if (!lang) lang = getenv("LANG");
   if (lang)
     {
	_e_intl_language = strdup(lang);
	e_util_env_set("LANGUAGE", _e_intl_language);
	if (getenv("LANG"))        e_util_env_set("LANG", _e_intl_language);
	if (getenv("LC_ALL"))      e_util_env_set("LC_ALL", _e_intl_language);
	if (getenv("LC_MESSAGES")) e_util_env_set("LC_MESSAGES", _e_intl_language);
     }
   else
     {
	_e_intl_language = NULL;
     }
   if (setlocale(LC_ALL, _e_intl_language) == NULL)
     {
	perror("setlocale() :");
	if (_e_intl_language)
	  printf("An error occured when trying to use the locale: %s\nDetails:\n",
		 _e_intl_language);
	else
	  printf("An error occured trying to use the default locale\n");
     }
   bindtextdomain(PACKAGE, e_prefix_locale_get());
   textdomain(PACKAGE);
//   XSetLocaleModifiers("");
   bind_textdomain_codeset(PACKAGE, "UTF-8");
}

const char *
e_intl_language_get(void)
{
   return _e_intl_language;
}

const Evas_List *
e_intl_language_list(void)
{
   /* FIXME: hunt dirs for locales */
   return _e_intl_languages;
}
