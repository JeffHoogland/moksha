/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

typedef struct _E_Intl_Pair E_Intl_Pair;

static int  _basic_lang_list_sort(const void *data1, const void *data2);

struct _E_Intl_Pair
{
   const char *locale_key;
   const char *locale_translation;
};

const E_Intl_Pair basic_language_predefined_pairs[ ] = {
     {"bg_BG.UTF-8", "Български"},
     {"ca_ES.UTF-8", "Català"},
     {"zh_CN.UTF-8", "Chinese (Simplified)"},
     {"zh_TW.UTF-8", "Chinese (Traditional)"},
     {"cs_CZ.UTF-8", "Čeština"},
     {"da_DK.UTF-8", "Dansk"},
     {"nl_NL.UTF-8", "Nederlands"},
     {"en_US.UTF-8", "English"},
     {"fi_FI.UTF-8", "Suomi"},
     {"fr_FR.UTF-8", "Français"},
     {"de_DE.UTF-8", "Deutsch"},
     {"hu_HU.UTF-8", "Magyar"},
     {"it_IT.UTF-8", "Italiano"},
     {"ja_JP.UTF-8", "日本語"},
     {"ko_KR.UTF-8", "한국어"},
     {"nb_NO.UTF-8", "Norsk Bokmål"},
     {"pl_PL.UTF-8", "Polski"},
     {"pt_BR.UTF-8", "Português"},
     {"ru_RU.UTF-8", "Русский"},
     {"sk_SK.UTF-8", "Slovenčina"},
     {"sl_SI.UTF-8", "Slovenščina"},
     {"es_AR.UTF-8", "Español"},
     {"sv_SE.UTF-8", "Svenska"},
     { NULL, NULL }
};

static char *lang = NULL;
static Eina_List *blang_list = NULL;

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
   
   return (strcmp(trans1, trans2));
}

EAPI int
wizard_page_init(E_Wizard_Page *pg)
{
   Eina_List    *e_lang_list;
   FILE		*output;
   
   e_lang_list = e_intl_language_list();
   
   printf("init\n");
   output = popen("locale -a", "r");
   if (output) 
     {
	char line[32];
	
	while (fscanf(output, "%[^\n]\n", line) == 1)
	  {
	     E_Locale_Parts *locale_parts;

	     locale_parts = e_intl_locale_parts_get(line);
	     if (locale_parts)
	       {
		  char *basic_language;

		  basic_language = 
		    e_intl_locale_parts_combine
		    (locale_parts, E_INTL_LOC_LANG | E_INTL_LOC_REGION);
		  if (basic_language)
		    {
		       int i;
		       
		       i = 0;
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

        while (e_lang_list)
	  {
	     free(e_lang_list->data);
	     e_lang_list = eina_list_remove_list(e_lang_list, e_lang_list);
	  }	     
	pclose(output);
     }
   return 1;
}
EAPI int
wizard_page_shutdown(E_Wizard_Page *pg)
{
   // FIXME: free blang_list
   return 1;
}
EAPI int
wizard_page_show(E_Wizard_Page *pg)
{
   Evas_Object *o, *of, *ob;
   Eina_List *l;
   int i, sel = -1;
   
   o = e_widget_list_add(pg->evas, 1, 0);
   e_wizard_title_set(_("Language"));
   of = e_widget_framelist_add(pg->evas, _("Available"), 0);
   ob = e_widget_ilist_add(pg->evas, 32 * e_scale, 32 * e_scale, &lang);
   e_widget_min_size_set(ob, 140 * e_scale, 140 * e_scale);
   
   e_widget_ilist_freeze(ob);
   
   for (i = 0, l = blang_list; l; l = l->next, i++)
     {
	E_Intl_Pair *pair;
	
	pair = l->data;
	e_widget_ilist_append(ob, NULL, _(pair->locale_translation), 
			      NULL, NULL, pair->locale_key);
	if (e_intl_language_get())
	  {
	     if (!strcmp(pair->locale_key, e_intl_language_get())) sel = i;
	  }
     }
   e_widget_ilist_go(ob);
   e_widget_ilist_thaw(ob);
   if (sel >= 0) e_widget_ilist_selected_set(ob, sel);
   
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 0, 0, 0.5);
   evas_object_show(ob);
   evas_object_show(of);
   e_wizard_page_show(o);
   pg->data = of;
   return 1; /* 1 == show ui, and wait for user, 0 == just continue */
}
EAPI int
wizard_page_hide(E_Wizard_Page *pg)
{
   evas_object_del(pg->data);
   /* special - language inits its stuff the moment it goes away */
   if (e_config->language) eina_stringshare_del(e_config->language);
   e_config->language = NULL;
   if (lang) e_config->language = eina_stringshare_add(lang);
   e_intl_language_set(e_config->language);
   e_wizard_labels_update();
   return 1;
}
EAPI int
wizard_page_apply(E_Wizard_Page *pg)
{
   return 1;
}
