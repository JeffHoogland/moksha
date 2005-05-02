/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* TODO List:
 * 
 * * load/save language in config so u can change language runtime via a gui and/or ipc
 * * add ipc to get/set/list languages, get language name, simplified language string, etc. (so a config tool can be written to display supported languages and be able to select from them)
 * * add more language names to the language name list list in e_intl_language_name_get()
 * * as we get translations add languages to the simplified lang list (C and en are currently the same, ja is a test translation - incomplete)
 */

static char *_e_intl_language = NULL;
static Evas_List *_e_intl_languages = NULL;

#define ADD_LANG(lang) _e_intl_languages = evas_list_append(_e_intl_languages, lang)

int
e_intl_init(void)
{
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
   ADD_LANG("C");
   ADD_LANG("en");
   ADD_LANG("ja");
   ADD_LANG("fr");
   ADD_LANG("es");
   ADD_LANG("pt");
   ADD_LANG("fi");
   ADD_LANG("ru");
   ADD_LANG("bg");
   ADD_LANG("de");
   ADD_LANG("pl");
   ADD_LANG("zh_CN");
   ADD_LANG("hu");
   ADD_LANG("sl");
   ADD_LANG("it");

   /* FIXME: NULL == use LANG. make this read a config value if it exists */
   e_intl_language_set(getenv("LANG"));
   return 1;
}

int
e_intl_shutdown(void)
{
   free(_e_intl_language);
   _e_intl_language = NULL;
   evas_list_free(_e_intl_languages);
   return 1;
}

void
e_intl_language_set(const char *lang)
{
   if (_e_intl_language) free(_e_intl_language);
   if (!lang) lang = getenv("LANG");
   if (lang)
     {
	_e_intl_language = strdup(lang);
	e_util_env_set("LANG", _e_intl_language);
     }
   else
     {
	_e_intl_language = NULL;
     }
   setlocale(LC_ALL, "");
   bindtextdomain(PACKAGE, LOCALE_DIR);
   textdomain(PACKAGE);
//   XSetLocaleModifiers("");
   bind_textdomain_codeset(PACKAGE, "UTF-8");
}

const char *
e_intl_language_get(void)
{
   return _e_intl_language;
}

#define IFL(l) if (!strcmp(lang, l)) return
const char *
e_intl_language_name_get(const char *lang)
{
   if (!lang) return "None";
   /* this is a list of DISTINCT languages. some languages have variants that
    * are different enough to justify being listed separately as distinct
    * languages here. this is intended for use in a gui that lets you select
    * a language, with e being able to give a name for the given language
    * encoding (once simplfied)
    */
   /* FIXME: add as many as we can to this */
   IFL("") "None";
   IFL("C") "None";
   IFL("bg") "Bulgarian";
   IFL("bs") "Bosnian";
   IFL("ca") "Catalan";
   IFL("cs") "Czech";
   IFL("cy") "Welsh";
   IFL("da") "Danish";
   IFL("de") "German";
   IFL("el") "Greek";
   IFL("en") "English"; 
   IFL("es") "Spanish"; 
   IFL("eu") "Basque";
   IFL("fa") "Persian";
   IFL("fr") "French";
   IFL("fi") "Finnish";
   IFL("gl") "Galician";
   IFL("hi") "Hindi";
   IFL("hr") "Croatian";
   IFL("hu") "Hungarian";
   IFL("id") "Indonesian";
   IFL("it") "Italian";
   IFL("ja") "Japanese";
   IFL("kr") "Korean";
   IFL("lt") "Lithuanian";
   IFL("lv") "Latvian";
   IFL("nl") "Dutch";
   IFL("no") "Norwegian";
   IFL("nb") "Norwegian Bokmal";
   IFL("nn") "Norwegian Nynorsk";
   IFL("pl") "Polish";
   IFL("pt") "Portuguese";
   IFL("ro") "Romanian";
   IFL("ru") "Russian";
   IFL("sk") "Slovak";
   IFL("sl") "Slovenian";
   IFL("sq") "Albanian";
   IFL("sr") "Serbian";
   IFL("sv") "Swedish";
   IFL("tr") "Tuirkish";
   IFL("uk") "Ukrainian";
   IFL("vi") "Vietnamese";
   /* must keep both - politically sensitive */
   IFL("zh_CN") "Chinese (Simplified)";
   IFL("zh_TW") "Chinese (Traditional)";
   return "Unknown";
}

#define ISL(l) (!strcasecmp(buf, l))
const char *
e_intl_language_simple_get(const char *lang)
{
   static char buf[128];
   char *p;

   if (!lang) return "C";
   /* strip off the charset stuff after any "." eg: "en_US.UTF-8" -> "en_US" */
   strncpy(buf, lang, sizeof(buf) - 1);
   p = strchr(buf, '.');
   if (p) *p = 0;
   /* do we want to split this inot the different forms of english?
    * ie american vs british? or australian? etc.
    */
   /* for known specific mappings - do them first here */
   if (ISL("en") || ISL("en_US") || ISL("en_GB") || ISL("en_GB@euro") || 
       ISL("en_CA") || ISL("en_AU") || ISL("en_NZ") || ISL("en_RN"))
     return "en";
   if (ISL("ja") || ISL("ja_JP") || ISL("JP"))
     return "ja";
   if (ISL("fr") || ISL("fr_FR") || ISL("FR") || ISL("fr_FR@euro"))
     return "fr";
   if (ISL("es") || ISL("es_ES") || ISL("ES") || ISL("es_ES@euro") ||
       ISL("es_AR") || ISL("AR"))
     return "es";
   if (ISL("pt") || ISL("pt_PT") || ISL("PT") || ISL("pt_PT@euro") ||
       ISL("pt_BR") || ISL("BR"))
     return "pt";
   if (ISL("fi") || ISL("fi_FI") || ISL("FI") || ISL("fi_FI@euro"))
     return "fi";
   if (ISL("ru") || ISL("ru_RU") || ISL("RU"))
     return "ru";
   if (ISL("bg") || ISL("bg_BG") || ISL("BG"))
     return "bg";
   if (ISL("de") || ISL("de_DE") || ISL("DE") || ISL("de_DE@euro") ||
       ISL("de_AT") || ISL("AT") || ISL("de_AT@euro"))
     return "de";
   if (ISL("pl") || ISL("pl_PL") || ISL("PL") || ISL("pl_PL@euro"))
     return "pl";
   if (ISL("zh") || ISL("zh_CN") || ISL("CN"))
     return "zh_CN";
   if (ISL("zh") || ISL("zh_TW") || ISL("TW"))
     return "zh_TW";
   if (ISL("hu") || ISL("hu_HU") || ISL("HU"))
     return "hu";
   if (ISL("sl") || ISL("sl_SL") || ISL("SL"))
     return "sl";
   if (ISL("it") || ISL("it_IT") || ISL("IT"))
     return "it";
   /* this is the default fallback - we have no special cases for this lang
    * so just strip off anything after and including the _ for country region
    * and just return the language encoding
    */
   /* strip off anything after a "_" eg: "en_US" -> "en" */
   p = strchr(buf, '_');
   if (p) *p = 0;
   /* we can safely retunr buf because its a static - BUT its contents will
    * change if we call e_intl_language_simple_get() again - so its only
    * intended for immediate use and de-reference, not for storage
    */
   return buf;
}

const Evas_List *
e_intl_language_list(void)
{
   return _e_intl_languages;
}
