/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "config.h"

/* TODO List:
 * 
 * * add more language names to the language name list list in e_intl_language_name_get()
 * * as we get translations add languages to the simplified lang list (C and en are currently the same, ja is a test translation - incomplete)
 */

static char *_e_intl_orig_lc_messages = NULL;
static char *_e_intl_orig_language = NULL;
static char *_e_intl_orig_lc_all = NULL;
static char *_e_intl_orig_lang = NULL;
static char *_e_intl_language = NULL;
static Evas_List *_e_intl_languages = NULL;

static char *_e_intl_orig_gtk_im_module_file = NULL;
static char *_e_intl_orig_xmodifiers = NULL;
static char *_e_intl_orig_qt_im_module = NULL; 
static char *_e_intl_orig_gtk_im_module = NULL;
static char *_e_intl_input_method = NULL;
static Evas_List *_e_intl_input_methods = NULL;

#define ADD_LANG(lang) _e_intl_languages = evas_list_append(_e_intl_languages, lang)
#define ADD_IM(method) _e_intl_input_methods = evas_list_append(_e_intl_input_methods, method)

int
e_intl_init(void)
{
   char *s;
   E_Language_Pack *elp;
   
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
   ADD_LANG("nb_NO.UTF-8");

   if ((s = getenv("LC_MESSAGES"))) _e_intl_orig_lc_messages = strdup(s);
   if ((s = getenv("LANGUAGE"))) _e_intl_orig_language = strdup(s);
   if ((s = getenv("LC_ALL"))) _e_intl_orig_lc_all = strdup(s);
   if ((s = getenv("LANG"))) _e_intl_orig_lang = strdup(s);

   if ((s = getenv("GTK_IM_MODULE"))) _e_intl_orig_gtk_im_module = strdup(s);
   if ((s = getenv("QT_IM_MODULE"))) _e_intl_orig_qt_im_module = strdup(s);
   if ((s = getenv("XMODIFIERS"))) _e_intl_orig_xmodifiers = strdup(s);
   if ((s = getenv("GTK_IM_MODULE_FILE"))) _e_intl_orig_gtk_im_module_file = strdup(s);
   
   
   /* Exception: NULL == use LANG. this will get setup in e_config */
   e_intl_language_set(NULL);

   elp = malloc(sizeof(E_Language_Pack));
   elp->version = 1;
   elp->e_im_name = strdup("scim");
   elp->gtk_im_module = strdup("scim");
   elp->qt_im_module = strdup("scim");
   elp->xmodifiers = strdup("@im=SCIM");
   elp->e_im_exec = strdup("scim");
   elp->gtk_im_module_file = NULL;

   ADD_IM(elp);

   elp = malloc(sizeof(E_Language_Pack));
   elp->version = 1;
   elp->e_im_name = strdup("uim");
   elp->gtk_im_module = strdup("uim");
   elp->qt_im_module = strdup("uim");
   elp->xmodifiers = strdup("@im=uim");
   elp->gtk_im_module_file = NULL;
   elp->e_im_exec = strdup("uim-xim");

   ADD_IM(elp);
   
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

void
e_intl_input_method_set(const char *method)
{
   E_Language_Pack *elp;
   Evas_List *next;

   if (_e_intl_input_method) free(_e_intl_input_method);

   if (!method)
     {
	e_util_env_set("GTK_IM_MODULE", _e_intl_orig_gtk_im_module);
        e_util_env_set("QT_IM_MODULE", _e_intl_orig_qt_im_module);
        e_util_env_set("XMODIFIERS", _e_intl_orig_xmodifiers);
        e_util_env_set("GTK_IM_MODULE_FILE", _e_intl_orig_gtk_im_module_file);	 	
     }	
   
   if (method) 
     {   
	_e_intl_input_method = strdup(method);   
	for (next = _e_intl_input_methods; next; next = next->next)     
	  {	
	     elp = next->data;	
	     if (!strcmp(elp->e_im_name, _e_intl_input_method)) 	  
	       {	     
	          e_util_env_set("GTK_IM_MODULE", elp->gtk_im_module);
	          e_util_env_set("QT_IM_MODULE", elp->qt_im_module);
	          e_util_env_set("XMODIFIERS", elp->xmodifiers);
	          e_util_env_set("GTK_IM_MODULE_FILE", elp->gtk_im_module_file);
		  if (elp->e_im_exec != NULL) 
		    {
		       /* FIXME: first check ok exec availability */
		       ecore_exe_run(elp->e_im_exec, NULL);
		    }
		  break; 
	       }	
	  }     
     }   
   else
     {
	_e_intl_input_method = NULL;
     }   
}

const char *
e_intl_input_method_get(void)
{
   return _e_intl_input_method;   
}

const Evas_List *
e_intl_input_method_list(void)
{
   Evas_List *im_list;
   Evas_List *next;
   E_Language_Pack *elp;

   im_list = NULL;
   
   for (next = _e_intl_input_methods; next; next = next->next)
     {
	elp = next->data;
	im_list = evas_list_append(im_list, elp->e_im_name);
     }

   return im_list;
}
 
