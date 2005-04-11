/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "config.h"

/* TODO List:
 */

static char *_e_intl_language = NULL;
static Evas_List *_e_intl_languages = NULL;

#define ADD_LANG(lang) _e_intl_languages = evas_list_append(_e_intl_languages, lang)

int
e_intl_init(void)
{
   if (_e_intl_languages) return 1;

   /* supporeted languages - as we get translations - add them here */
   ADD_LANG("C");
   ADD_LANG("en");
   ADD_LANG("jp");

   /* FIXME: NULL == use LANG. make this read a config value if it exists */
   e_intl_language_set(NULL);
   return 1;
}

int
e_intl_shutdown(void)
{
   return 1;
}

void
e_intl_language_set(const char *lang)
{
   char buf[4096];
   
   if (_e_intl_language) free(_e_intl_language);
   if (!lang) lang = getenv("LANG");
   _e_intl_language = strdup(lang);
   snprintf(buf, sizeof(buf), "LANG=%s", _e_intl_language);
   putenv(buf);
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
   /* FIXME: add as many as we can to this */
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
   IFL("jp") "Japanese";
   IFL("kr") "Korean";
   IFL("lt") "Lithuanian";
   IFL("lv") "Latvian";
   IFL("nl") "Dutch";
   IFL("nn") "Norwegian Bokmall";
   IFL("no") "Norwegian Nynorsk";
   IFL("pl") "Polish";
   IFL("pt") "Portuguese";
   IFL("pt_BR") "Portuguese (Brazil)";
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
   IFL("zh") "Chinese (Simplified)";
   IFL("zh_TW") "Chinese (Traditional)";
   return "Unknown";
}

#define ISL(l) (!strcasecmp(buf, l))
const char *
e_intl_language_simple_get(const char *lang)
{
   char buf[4096], *p;
   
   if (!lang) return "C";
   strncpy(buf, lang, sizeof(buf) - 1);
   p = strchr(buf, '.');
   if (p) *p = 0;
   if (ISL("en") || ISL("en_US") || ISL("en_GB") || ISL("en_CA") ||
       ISL("en_AU") || ISL("en_NZ") || ISL("en_RN"))
     return "en";
   if (ISL("ja") || ISL("ja_JP") || ISL("JP") || ISL("en_CA"))
     return "ja";
   /* FIXME: add all sorts of fuzzy matching here */
   return lang;
}

const Evas_List *
e_intl_language_list(void)
{
   return _e_intl_languages;
}
