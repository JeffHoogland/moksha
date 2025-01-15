/* Language chooser */
#include "e_wizard.h"
#define MAX_LEN 256

typedef struct _E_Intl_Pair E_Intl_Pair;

static int _basic_lang_list_sort(const void *data1, const void *data2);

struct _E_Intl_Pair
{
   const char *locale_key;
   const char *locale_icon;
   const char *locale_translation;
};

const E_Intl_Pair basic_language_predefined_pairs[] =
{
   {"ar_AE.UTF-8", "ara_flag.png", "العربية"},
   {"be_BY.UTF-8", "by_flag.png", "Беларуская"},
   {"bg_BG.UTF-8", "bg_flag.png", "Български"},
   {"ca_ES.UTF-8", "cat_flag.png", "Català"},
   {"cs_CZ.UTF-8", "cz_flag.png", "Čeština"},
   {"da_DK.UTF-8", "dk_flag.png", "Dansk"},
   {"de_DE.UTF-8", "de_flag.png", "Deutsch"},
   {"en_US.UTF-8", "us_flag.png", "English (US)"},
   {"en_GB.UTF-8", "gb_flag.png", "British English"},
   {"hi_IN.UTF-8", "IN_flag.png", "हिन्दी"}
   {"mr_IN.UTF-8", "IN_flag.png", "मराठी"}
   {"el_GR.UTF-8", "gr_flag.png", "Ελληνικά"},
   {"eo_US.UTF-8", "epo_flag.png", "Esperanto"},
   {"eo.UTF-8", "epo_flag.png", "Esperanto"},
   {"es_ES.UTF-8", "es_flag.png", "Español"},
   {"es_AR.UTF-8", "ar_flag.png", "Español (Argentina)"},
   {"es_MX.UTF-8", "mx_flag.png", "Español (Mexico)"},
   {"et_ET.UTF-8", "ee_flag.png", "Eesti keel"},
   {"fi_FI.UTF-8", "fi_flag.png", "Suomi"},
   {"fo_FO.UTF-8", "fo_flag.png", "Føroyskt"},
   {"fr_CH.UTF-8", "ch_flag.png", "Français (Suisse)"},
   {"fr_FR.UTF-8", "fr_flag.png", "Français"},
   {"he_HE.UTF-8", "il_flag.png", "עברית"},
   {"hr_HR.UTF-8", "hr_flag.png", "Hrvatski"},
   {"hu_HU.UTF-8", "hu_flag.png", "Magyar"},
   {"it_IT.UTF-8", "it_flag.png", "Italiano"},
   {"ja_JP.UTF-8", "jp_flag.png", "日本語"},
   {"km_KM.UTF-8", "kh_flag.png", "ភាសាខ្មែរ"},
   {"ko_KR.UTF-8", "kr_flag.png", "한국어"},
   {"ku.UTF-8", "ku_flag.png", "یدروك"},
   {"lt_LT.UTF-8", "lt_flag.png", "Lietuvių kalba"},
   {"ms_MY.UTF-8", "my_flag.png", "Bahasa Melayu"},
   {"nb_NO.UTF-8", "no_flag.png", "Norsk Bokmål"},
   {"nl_NL.UTF-8", "nl_flag.png", "Nederlands"},
   {"pl_PL.UTF-8", "pl_flag.png", "Polski"},
   {"pt_BR.UTF-8", "br_flag.png", "Português (Brasil)"},
   {"pt_PT.UTF-8", "pt_flag.png", "Português"},
   {"ru_RU.UTF-8", "ru_flag.png", "Русский"},
   {"sk_SK.UTF-8", "sk_flag.png", "Slovenčina"},
   {"sl_SI.UTF-8", "si_flag.png", "Slovenščina"},
   {"sv_SE.UTF-8", "se_flag.png", "Svenska"},
   {"tr_TR.UTF-8", "tr_flag.png", "Türkçe"},
   {"uk_UA.UTF-8", "ua_flag.png", "Українська мова"},
   {"zh_CN.UTF-8", "cn_flag.png", "中文 (简体)"},
   {"zh_TW.UTF-8", "tw_flag.png", "中文 (繁體)"},
   { NULL, NULL, NULL }
};

static const char *lang = NULL;
static Eina_List *blang_list = NULL;
static char cur_lang[MAX_LEN]="System Default";

static void
_set_currrent(void)
{
   FILE *output;
   char buffer[MAX_LEN];
   char *ch;
   
   if ((output = popen("locale", "r")))
      {
      while (fgets(buffer, MAX_LEN - 1, output))
         {
          // Remove trailing newline and trailing quote
          buffer[strcspn(buffer, "\n")] = 0;
          if ((ch = strstr( buffer, "LANG=" )))
           snprintf(cur_lang, MAX_LEN - 1, "%s", (char *) ch + 5);
         }
      pclose(output); 
    }
}

static int
_basic_lang_list_sort(const void *data1, const void *data2)
{
   E_Intl_Pair *ln1, *ln2;
   const char *trans1;
   const char *trans2;

   if (!data1) return 1;
   if (!data2) return -1;

   ln1 = (E_Intl_Pair *)data1;
   ln2 = (E_Intl_Pair *)data2;

   if (!ln1->locale_translation) return 1;
   trans1 = ln1->locale_translation;

   if (!ln2->locale_translation) return -1;
   trans2 = ln2->locale_translation;

   return strcmp(trans1, trans2);
}

EAPI int
wizard_page_init(E_Wizard_Page *pg __UNUSED__, Eina_Bool *need_xdg_desktops __UNUSED__, Eina_Bool *need_xdg_icons __UNUSED__)
{
   FILE *output;

#ifdef __OpenBSD__
   output = popen("ls /usr/share/locale", "r");
#else
   output = popen("locale -a", "r");
#endif
   if (output)
     {
        char line[32];

        while (fscanf(output, "%[^\n]\n", line) == 1)
          {
             E_Locale_Parts *locale_parts;

             locale_parts = e_intl_locale_parts_get(line);
             if (locale_parts)
               {
                  char *basic_language = NULL;

                  if (locale_parts->mask & E_INTL_LOC_REGION)
                    basic_language = e_intl_locale_parts_combine(locale_parts,
                      E_INTL_LOC_LANG | E_INTL_LOC_REGION);
                  else if (locale_parts->lang)
                    basic_language = strdup(locale_parts->lang);
                  if (basic_language)
                    {
                       int i = 0;

                       while (basic_language_predefined_pairs[i].locale_key)
                         {
                            /* if basic language is supported by E and System*/
                            if (!strncmp
                                  (basic_language_predefined_pairs[i].locale_key,
                                  basic_language, strlen(basic_language)))
                              {
                                 if (!eina_list_data_find
                                       (blang_list,
                                       &basic_language_predefined_pairs[i]))
                                   blang_list = eina_list_append
                                       (blang_list,
                                       &basic_language_predefined_pairs[i]);
                                 break;
                              }
                            i++;
                         }
                    }
                  E_FREE(basic_language);
                  e_intl_locale_parts_free(locale_parts);
               }
          }
        /* Sort basic languages */
        blang_list = eina_list_sort(blang_list, eina_list_count(blang_list), _basic_lang_list_sort);
        pclose(output);
     }
   return 1;
}
/*
EAPI int
wizard_page_shutdown(E_Wizard_Page *pg __UNUSED__)
{
   // FIXME: free blang_list
   return 1;
}
*/
EAPI int
wizard_page_show(E_Wizard_Page *pg)
{
   Evas_Object *o, *of, *ob, *ic;
   Eina_List *l;
   int i, sel = -1;
   char buf[PATH_MAX];

   o = e_widget_list_add(pg->evas, 1, 0);
   if (ecore_file_exists("/etc/bodhi/iso"))
      e_wizard_title_set(_("Language"));
   of = e_widget_framelist_add(pg->evas, _("Select one"), 0);
   ob = e_widget_ilist_add(pg->evas, 32 * e_scale, 32 * e_scale, &lang);
   e_widget_size_min_set(ob, 140 * e_scale, 140 * e_scale);

   e_widget_ilist_freeze(ob);

   e_prefix_data_snprintf(buf, sizeof(buf), "data/flags/%s", "lang-system.png");
   ic = e_util_icon_add(buf, pg->evas);
   e_widget_ilist_append(ob, ic, _("System Default"),
                         NULL, NULL, NULL);
   for (i = 1, l = blang_list; l; l = l->next, i++)
     {
        E_Intl_Pair *pair;

        pair = l->data;
        if (pair->locale_icon)
          {
             e_prefix_data_snprintf(buf, sizeof(buf), "data/flags/%s", pair->locale_icon);
             ic = e_util_icon_add(buf, pg->evas);
          }
        else
          ic = NULL;
        e_widget_ilist_append(ob, ic, _(pair->locale_translation),
                              NULL, NULL, pair->locale_key);
        if (e_intl_language_get())
          {
             if (!strcmp(pair->locale_key, e_intl_language_get())) sel = i;
          }
     }
   e_widget_ilist_go(ob);
   e_widget_ilist_thaw(ob);
   if (sel >= 0)
     {
        e_widget_ilist_selected_set(ob, sel);
        e_widget_ilist_nth_show(ob, sel, 0);
     }
   else if (e_widget_ilist_count(ob) == 2) // default and one other
     e_widget_ilist_selected_set(ob, 1);
   else
     e_widget_ilist_selected_set(ob, 0);

   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   evas_object_show(ob);
   evas_object_show(of);
   e_wizard_page_show(o);
//   pg->data = o;

   /* On an installed system we do not want to show this wizard
    *    But Bodhi wants this displayed for users running the ISO
    */
   if (! ecore_file_exists("/etc/bodhi/iso"))
      {
         _set_currrent();
         // Should always be set so this is just paranoia
         lang = strcmp(cur_lang, "System Default") ? NULL : cur_lang ;
         eina_stringshare_del(e_config->language);
         e_config->language = eina_stringshare_ref(lang);
         e_intl_language_set(e_config->language);
         e_wizard_labels_update();
         if (ecore_file_exists("/usr/bin/xdg-user-dirs-update"))
            e_util_exe_safe_run("/usr/bin/xdg-user-dirs-update --force", NULL);

         return 0;
      }
   return 1; /* 1 == show ui, and wait for user, 0 == just continue */
}

EAPI int
wizard_page_hide(E_Wizard_Page *pg __UNUSED__)
{
   //evas_object_del(pg->data);
/* special - language inits its stuff the moment it goes away */
  // eina_stringshare_del(e_config->language);
  // e_config->language = eina_stringshare_ref(lang);
   /* TODO:
    * This should be on lang select,
    * so if next page needs xdg we can't press next */
   //e_wizard_xdg_desktops_reset();
   //e_intl_language_set(e_config->language);
   //e_wizard_labels_update();
   return 1;
}

EAPI int
wizard_page_apply(E_Wizard_Page *pg __UNUSED__)
{
   // do this again as we want it to apply to the new profile
   eina_stringshare_del(e_config->language);
   e_config->language = eina_stringshare_ref(lang);
   e_intl_language_set(e_config->language);
   e_wizard_labels_update();
   if (ecore_file_exists("/usr/bin/xdg-user-dirs-update"))
      e_util_exe_safe_run("/usr/bin/xdg-user-dirs-update --force", NULL);
   return 1;
}
