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

static Ecore_Exe *_e_intl_input_method_exec = NULL;
static Ecore_Event_Handler *_e_intl_exit_handler = NULL;

static char *_e_intl_orig_lc_messages = NULL;
static char *_e_intl_orig_language = NULL;
static char *_e_intl_orig_lc_all = NULL;
static char *_e_intl_orig_lang = NULL;
static char *_e_intl_language = NULL;
static Evas_List *_e_intl_languages = NULL;

static char *_e_intl_orig_xmodifiers = NULL;
static char *_e_intl_orig_qt_im_module = NULL; 
static char *_e_intl_orig_gtk_im_module = NULL;
static char *_e_intl_input_method = NULL;

static Eet_Data_Descriptor *_e_intl_input_method_config_edd = NULL;

#define ADD_LANG(lang) _e_intl_languages = evas_list_append(_e_intl_languages, lang)

#define E_EXE_STOP(EXE) if (EXE != NULL) { ecore_exe_terminate(EXE); ecore_exe_free(EXE); EXE = NULL; }
#define E_EXE_IS_VALID(EXE) (!((EXE == NULL) || (EXE[0] == 0)))

static int _e_intl_cb_exit(void *data, int type, void *event);
static Evas_List *_e_intl_imc_path_scan(E_Path *path);
static Evas_List *_e_intl_imc_dir_scan(char *dir);
static E_Input_Method_Config *_e_intl_imc_find(Evas_List *imc_list, char *name);


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
   ADD_LANG("cs_CZ.UTF-8");
   ADD_LANG("da_DK.UTF-8");
   ADD_LANG("sk_SK.UTF-8");
   ADD_LANG("sv_SV.UTF-8");
   ADD_LANG("nb_NO.UTF-8");
   ADD_LANG("nl_NL.UTF-8");
   ADD_LANG("zh_TW.UTF-8");

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

int
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
   
   evas_list_free(_e_intl_languages);
   
   E_CONFIG_DD_FREE(_e_intl_input_method_config_edd);
  
   return 1;
}

/* Setup configuration settings and start services */
int
e_intl_post_init(void)
{
   if ((e_config->language) && (e_config->language[0] != 0))
     e_intl_language_set(e_config->language);
   
   if ((e_config->input_method) && (e_config->input_method[0] != 0))
     e_intl_input_method_set(e_config->input_method); 

   _e_intl_exit_handler = ecore_event_handler_add(ECORE_EVENT_EXE_EXIT, _e_intl_cb_exit, NULL);
   return 1;
}

int
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
_e_intl_imc_dir_scan(char *dir)
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
	e_util_env_set("LC_MESSAGES", _e_intl_orig_lc_messages);
	e_util_env_set("LANGUAGE", _e_intl_orig_language);
	e_util_env_set("LC_ALL", _e_intl_orig_lc_all);
	e_util_env_set("LANG", _e_intl_orig_lang);
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
			 e_error_dialog_show(_("Input Method Error"),
					_( "Error starting the input method "
					   "executable\n\n"
					   
					"please make sure that your input\n"
					"method configuration is correct and\n"
					"that your configuration's\n" 
					"executable is in your PATH\n"));  
		    }
	       }	

	/* Need to free up the directory listing */
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

const char *
e_intl_input_method_get(void)
{
   return _e_intl_input_method;   
}

Evas_List *
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
E_Input_Method_Config *
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
int
e_intl_input_method_config_write (Eet_File * imc_file, E_Input_Method_Config * imc)
{
   int ok = 0;

   if (imc_file)
     {
	ok = eet_data_write(imc_file, _e_intl_input_method_config_edd, "imc", imc, 0);
     }
   return ok;
}

void
e_intl_input_method_config_free (E_Input_Method_Config *imc)
{
   if (imc != NULL)
     {
	E_FREE(imc->e_im_name);
	E_FREE(imc->gtk_im_module);
	E_FREE(imc->qt_im_module);
	E_FREE(imc->xmodifiers);
	E_FREE(imc->e_im_exec);
	E_FREE(imc);
     }
}

static int
_e_intl_cb_exit(void *data, int type, void *event)
{
   Ecore_Event_Exe_Exit *ev;
   
   ev = event;
   if (!ev->exe) return 1;
   
   if (!(ecore_exe_tag_get(ev->exe) && 
	 (!strcmp(ecore_exe_tag_get(ev->exe), "E/im_exec")))) return 1;

   _e_intl_input_method_exec = NULL;
   return 1;
}
