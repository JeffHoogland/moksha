/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Intl_Pair E_Intl_Pair;
typedef struct _E_Intl_Langauge_Node E_Intl_Language_Node;
typedef struct _E_Intl_Region_Node E_Intl_Region_Node;

static void        *_create_data             (E_Config_Dialog *cfd);
static void         _free_data               (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int          _advanced_apply_data     (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_advanced_create_widgets (E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int          _basic_apply_data        (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets    (E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

static void _ilist_basic_language_cb_change (void *data, Evas_Object *obj);
static void _ilist_language_cb_change       (void *data, Evas_Object *obj);
static void _ilist_region_cb_change         (void *data, Evas_Object *obj);
static void _ilist_codeset_cb_change        (void *data, Evas_Object *obj);
static void _ilist_modifier_cb_change       (void *data, Evas_Object *obj);
static int  _lang_list_sort                 (const void *data1, const void *data2);
static void _lang_list_load                 (void *data);
static int  _region_list_sort               (const void *data1, const void *data2);
static void _region_list_load               (void *data);
static int  _basic_lang_list_sort           (const void *data1, const void *data2);

/* Fill the clear lists, fill with language, select */
/* Update lanague */
static void      _cfdata_language_go        (const char *lang, const char *region, const char *codeset, const char *modifier, E_Config_Dialog_Data *cfdata);
static Eina_Bool _lang_hash_cb              (const Eina_Hash *hash __UNUSED__, const void *key __UNUSED__, void *data, void *fdata);
static Eina_Bool _region_hash_cb            (const Eina_Hash *hash __UNUSED__, const void *key __UNUSED__, void *data, void *fdata);
static Eina_Bool _language_hash_free_cb     (const Eina_Hash *hash __UNUSED__, const void *key __UNUSED__, void *data, void *fdata __UNUSED__);
static Eina_Bool _region_hash_free_cb       (const Eina_Hash *hash __UNUSED__, const void *key __UNUSED__, void *data, void *fdata __UNUSED__);
static void      _intl_current_locale_setup (E_Config_Dialog_Data *cfdata);
static const char *_intl_charset_upper_get  (const char *charset);

struct _E_Intl_Pair
{
   const char *locale_key;
   const char *locale_translation;
};

/* We need to store a map of languages -> Countries -> Extra
 *
 * Extra:
 * Each region has its own Encodings
 * Each region has its own Modifiers
 */

struct _E_Intl_Langauge_Node
{
   const char *lang_code;		/* en */
   const char *lang_name;		/* English (trans in ilist) */
   int	       lang_available;		/* defined in e translation */
   Eina_Hash  *region_hash;	        /* US->utf8 */
};

struct _E_Intl_Region_Node
{
   const char *region_code;		/* US */
   const char *region_name;		/* United States */
   Eina_List  *available_codesets;
   Eina_List  *available_modifiers;
};

struct _E_Config_Dialog_Data
{
   E_Config_Dialog *cfd;
   Evas *evas;

   /* Current data */
   char	*cur_language;

   const char *cur_blang;

   const char *cur_lang;
   const char *cur_reg;
   const char *cur_cs;
   const char *cur_mod;

   int  lang_dirty;

   Eina_Hash *locale_hash;
   Eina_List *lang_list;
   Eina_List *region_list;
   Eina_List *blang_list;

   struct
     {
	Evas_Object     *blang_list;

	Evas_Object	*lang_list;
	Evas_Object     *reg_list;
	Evas_Object	*cs_list;
	Evas_Object	*mod_list;
	
	Evas_Object     *locale_entry;
     } 
   gui;
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
     {"el_GR.UTF-8", "Ελληνικά"},
     { NULL, NULL }
};

const E_Intl_Pair language_predefined_pairs[ ] = {
       {"aa", "Qafár af"},
       {"af", "Afrikaans"},
       {"ak", "Akan"},
       {"am", "አማርኛ"},
       {"an", "Aragonés"},
       {"ar", "ةيبرعلا"},
       {"as", "অসমীয়া"},
       {"az", "Azərbaycan dili"},
       {"be", "Беларуская мова"},
       {"bg", "Български"},
       {"bn", "বাংলা"},
       {"br", "Brezhoneg"},
       {"bs", "Bosanski"},
       {"byn", "ብሊና"},
       {"ca", "Català"},
       {"cch", "Atsam"},
       {"cs", "Čeština"},
       {"cy", "Cymraeg"},
       {"da", "Dansk"},
       {"de", "Deutsch"},
       {"dv", "ދިވެހި"},
       {"dz", "Dzongkha"},
       {"ee", "Eʋegbe"},
       {"el", "Ελληνικά"},
       {"en", "English"},
       {"eo", "Esperanto"},
       {"es", "Español"},
       {"et", "Eesti keel"},
       {"eu", "Euskara"},
       {"fa", "یسراف"},
       {"fi", "Suomi"},
       {"fo", "Føroyskt"},
       {"fr", "Français"},
       {"fur", "Furlan"},
       {"ga", "Gaeilge"},
       {"gaa", "Gã"},
       {"gez", "ግዕዝ"},
       {"gl", "Galego"},
       {"gu", "Gujarati"},
       {"gv", "Yn Ghaelg"},
       {"ha", "Hausa"},
       {"haw", "ʻŌlelo Hawaiʻi"},
       {"he", "תירבע"},
       {"hi", "Hindi"},
       {"hr", "Hrvatski"},
       {"hu", "Magyar"},
       {"hy", "Հայերեն"},
       {"ia", "Interlingua"},
       {"id", "Indonesian"},
       {"ig", "Igbo"},
       {"is", "Íslenska"},
       {"it", "Italiano"},
       {"iu", "ᐃᓄᒃᑎᑐᑦ"},
       {"iw", "תירבע"},
       {"ja", "日本語"},
       {"ka", "ქართული"},
       {"kaj", "Jju"},
       {"kam", "Kikamba"},
       {"kcg", "Tyap"},
       {"kfo", "Koro"},
       {"kk", "Qazaq"},
       {"kl", "Kalaallisut"},
       {"km", "Khmer"},
       {"kn", "ಕನ್ನಡ"},
       {"ko", "한국어"},
       {"kok", "Konkani"},
       {"ku", "یدروك"},
       {"kw", "Kernowek"},
       {"ky", "Кыргыз тили"},
       {"ln", "Lingála"},
       {"lo", "ພາສາລາວ"},
       {"lt", "Lietuvių kalba"},
       {"lv", "Latviešu"},
       {"mi", "Te Reo Māori"},
       {"mk", "Македонски"},
       {"ml", "മലയാളം"},
       {"mn", "Монгол"},
       {"mr", "मराठी"},
       {"ms", "Bahasa Melayu"},
       {"mt", "Malti"},
       {"nb", "Norsk Bokmål"},
       {"ne", "नेपाली"},
       {"nl", "Nederlands"},
       {"nn", "Norsk Nynorsk"},
       {"no", "Norsk"},
       {"nr", "isiNdebele"},
       {"nso", "Sesotho sa Leboa"},
       {"ny", "Chicheŵa"},
       {"oc", "Occitan"},
       {"om", "Oromo"},
       {"or", "ଓଡ଼ିଆ"},
       {"pa", "ਪੰਜਾਬੀ"},
       {"pl", "Polski"},
       {"ps", "وتښپ"},
       {"pt", "Português"},
       {"ro", "Română"},
       {"ru", "Русский"},
       {"rw", "Kinyarwanda"},
       {"sa", "संस्कृतम्"},
       {"se", "Davvisápmi"},
       {"sh", "Srpskohrvatski/Српскохрватски"},
       {"sid", "Sidámo 'Afó"},
       {"sk", "Slovenčina"},
       {"sl", "Slovenščina"},
       {"so", "af Soomaali"},
       {"sq", "Shqip"},
       {"sr", "Српски"},
       {"ss", "Swati"},
       {"st", "Southern Sotho"},
       {"sv", "Svenska"},
       {"sw", "Swahili"},
       {"syr", "Syriac"},
       {"ta", "தமிழ்"},
       {"te", "తెలుగు"},
       {"tg", "Тоҷикӣ"},
       {"th", "ภาษาไทย"},
       {"ti", "ትግርኛ"},
       {"tig", "ቲግሬ"},
       {"tl", "Tagalog"},
       {"tn", "Setswana"},
       {"tr", "Türkçe"},
       {"ts", "Tsonga"},
       {"tt", "Татарча"},
       {"uk", "Українська мова"},
       {"ur", "ودراُ"},
       {"uz", "O‘zbek"},
       {"ve", "Venda"},
       {"vi", "Tiếng Việt"},
       {"wa", "Walon"},
       {"wal", "Walamo"},
       {"xh", "Xhosa"},
       {"yi", "שידיִי"},
       {"yo", "èdèe Yorùbá"},
       {"zh", "汉语/漢語"},
       {"zu", "Zulu"},
       { NULL, NULL}
};

const E_Intl_Pair region_predefined_pairs[ ] = {
       { "AF", "Afghanistan"},
       { "AX", "Åland"},
       { "AL", "Shqipëria"},
       { "DZ", "Algeria"},
       { "AS", "Amerika Sāmoa"},
       { "AD", "Andorra"},
       { "AO", "Angola"},
       { "AI", "Anguilla"},
       { "AQ", "Antarctica"},
       { "AG", "Antigua and Barbuda"},
       { "AR", "Argentina"},
       { "AM", "Հայաստան"},
       { "AW", "Aruba"},
       { "AU", "Australia"},
       { "AT", "Österreich"},
       { "AZ", "Azərbaycan"},
       { "BS", "Bahamas"},
       { "BH", "Bahrain"},
       { "BD", "বাংলাদেশ"},
       { "BB", "Barbados"},
       { "BY", "Беларусь"},
       { "BE", "Belgium"},
       { "BZ", "Belize"},
       { "BJ", "Bénin"},
       { "BM", "Bermuda"},
       { "BT", "Bhutan"},
       { "BO", "Bolivia"},
       { "BA", "Bosnia and Herzegovina"},
       { "BW", "Botswana"},
       { "BV", "Bouvetøya"},
       { "BR", "Brazil"},
       { "IO", "British Indian Ocean Territory"},
       { "BN", "Brunei Darussalam"},
       { "BG", "България"},
       { "BF", "Burkina Faso"},
       { "BI", "Burundi"},
       { "KH", "Cambodia"},
       { "CM", "Cameroon"},
       { "CA", "Canada"},
       { "CV", "Cape Verde"},
       { "KY", "Cayman Islands"},
       { "CF", "Central African Republic"},
       { "TD", "Chad"},
       { "CL", "Chile"},
       { "CN", "中國"},
       { "CX", "Christmas Island"},
       { "CC", "Cocos (keeling) Islands"},
       { "CO", "Colombia"},
       { "KM", "Comoros"},
       { "CG", "Congo"},
       { "CD", "Congo"},
       { "CK", "Cook Islands"},
       { "CR", "Costa Rica"},
       { "CI", "Cote d'Ivoire"},
       { "HR", "Hrvatska"},
       { "CU", "Cuba"},
       { "CY", "Cyprus"},
       { "CZ", "Česká republika"},
       { "DK", "Danmark"},
       { "DJ", "Djibouti"},
       { "DM", "Dominica"},
       { "DO", "República Dominicana"},
       { "EC", "Ecuador"},
       { "EG", "Egypt"},
       { "SV", "El Salvador"},
       { "GQ", "Equatorial Guinea"},
       { "ER", "Eritrea"},
       { "EE", "Eesti"},
       { "ET", "Ethiopia"},
       { "FK", "Falkland Islands (malvinas)"},
       { "FO", "Faroe Islands"},
       { "FJ", "Fiji"},
       { "FI", "Finland"},
       { "FR", "France"},
       { "GF", "French Guiana"},
       { "PF", "French Polynesia"},
       { "TF", "French Southern Territories"},
       { "GA", "Gabon"},
       { "GM", "Gambia"},
       { "GE", "Georgia"},
       { "DE", "Deutschland"},
       { "GH", "Ghana"},
       { "GI", "Gibraltar"},
       { "GR", "Greece"},
       { "GL", "Greenland"},
       { "GD", "Grenada"},
       { "GP", "Guadeloupe"},
       { "GU", "Guam"},
       { "GT", "Guatemala"},
       { "GG", "Guernsey"},
       { "GN", "Guinea"},
       { "GW", "Guinea-Bissau"},
       { "GY", "Guyana"},
       { "HT", "Haiti"},
       { "HM", "Heard Island and Mcdonald Islands"},
       { "VA", "Holy See (Vatican City State)"},
       { "HN", "Honduras"},
       { "HK", "Hong Kong"},
       { "HU", "Magyarország"},
       { "IS", "Iceland"},
       { "IN", "India"},
       { "ID", "Indonesia"},
       { "IR", "Iran"},
       { "IQ", "Iraq"},
       { "IE", "Éire"},
       { "IM", "Isle Of Man"},
       { "IL", "Israel"},
       { "IT", "Italia"},
       { "JM", "Jamaica"},
       { "JP", "日本"},
       { "JE", "Jersey"},
       { "JO", "Jordan"},
       { "KZ", "Kazakhstan"},
       { "KE", "Kenya"},
       { "KI", "Kiribati"},
       { "KP", "Korea"},
       { "KR", "Korea"},
       { "KW", "Kuwait"},
       { "KG", "Kyrgyzstan"},
       { "LA", "Lao People's Democratic Republic"},
       { "LV", "Latvija"},
       { "LB", "Lebanon"},
       { "LS", "Lesotho"},
       { "LR", "Liberia"},
       { "LY", "Libyan Arab Jamahiriya"},
       { "LI", "Liechtenstein"},
       { "LT", "Lietuva"},
       { "LU", "Lëtzebuerg"},
       { "MO", "Macao"},
       { "MK", "Македонија"},
       { "MG", "Madagascar"},
       { "MW", "Malawi"},
       { "MY", "Malaysia"},
       { "MV", "Maldives"},
       { "ML", "Mali"},
       { "MT", "Malta"},
       { "MH", "Marshall Islands"},
       { "MQ", "Martinique"},
       { "MR", "Mauritania"},
       { "MU", "Mauritius"},
       { "YT", "Mayotte"},
       { "MX", "Mexico"},
       { "FM", "Micronesia"},
       { "MD", "Moldova"},
       { "MC", "Monaco"},
       { "MN", "Mongolia"},
       { "MS", "Montserrat"},
       { "MA", "Morocco"},
       { "MZ", "Mozambique"},
       { "MM", "Myanmar"},
       { "NA", "Namibia"},
       { "NR", "Nauru"},
       { "NP", "Nepal"},
       { "NL", "Nederland"},
       { "AN", "Netherlands Antilles"},
       { "NC", "New Caledonia"},
       { "NZ", "New Zealand"},
       { "NI", "Nicaragua"},
       { "NE", "Niger"},
       { "NG", "Nigeria"},
       { "NU", "Niue"},
       { "NF", "Norfolk Island"},
       { "MP", "Northern Mariana Islands"},
       { "NO", "Norge"},
       { "OM", "Oman"},
       { "PK", "Pakistan"},
       { "PW", "Palau"},
       { "PS", "Palestinian Territory"},
       { "PA", "Panama"},
       { "PG", "Papua New Guinea"},
       { "PY", "Paraguay"},
       { "PE", "Peru"},
       { "PH", "Philippines"},
       { "PN", "Pitcairn"},
       { "PL", "Poland"},
       { "PT", "Portugal"},
       { "PR", "Puerto Rico"},
       { "QA", "Qatar"},
       { "RE", "Reunion"},
       { "RO", "Romania"},
       { "RU", "Russian Federation"},
       { "RW", "Rwanda"},
       { "SH", "Saint Helena"},
       { "KN", "Saint Kitts and Nevis"},
       { "LC", "Saint Lucia"},
       { "PM", "Saint Pierre and Miquelon"},
       { "VC", "Saint Vincent and the Grenadines"},
       { "WS", "Samoa"},
       { "SM", "San Marino"},
       { "ST", "Sao Tome and Principe"},
       { "SA", "Saudi Arabia"},
       { "SN", "Senegal"},
       { "CS", "Serbia and Montenegro"},
       { "SC", "Seychelles"},
       { "SL", "Sierra Leone"},
       { "SG", "Singapore"},
       { "SK", "Slovakia"},
       { "SI", "Slovenia"},
       { "SB", "Solomon Islands"},
       { "SO", "Somalia"},
       { "ZA", "South Africa"},
       { "GS", "South Georgia and the South Sandwich Islands"},
       { "ES", "Spain"},
       { "LK", "Sri Lanka"},
       { "SD", "Sudan"},
       { "SR", "Suriname"},
       { "SJ", "Svalbard and Jan Mayen"},
       { "SZ", "Swaziland"},
       { "SE", "Sweden"},
       { "CH", "Switzerland"},
       { "SY", "Syrian Arab Republic"},
       { "TW", "Taiwan"},
       { "TJ", "Tajikistan"},
       { "TZ", "Tanzania"},
       { "TH", "Thailand"},
       { "TL", "Timor-Leste"},
       { "TG", "Togo"},
       { "TK", "Tokelau"},
       { "TO", "Tonga"},
       { "TT", "Trinidad and Tobago"},
       { "TN", "Tunisia"},
       { "TR", "Turkey"},
       { "TM", "Turkmenistan"},
       { "TC", "Turks and Caicos Islands"},
       { "TV", "Tuvalu"},
       { "UG", "Uganda"},
       { "UA", "Ukraine"},
       { "AE", "United Arab Emirates"},
       { "GB", "United Kingdom"},
       { "US", "United States"},
       { "UM", "United States Minor Outlying Islands"},
       { "UY", "Uruguay"},
       { "UZ", "Uzbekistan"},
       { "VU", "Vanuatu"},
       { "VE", "Venezuela"},
       { "VN", "Viet Nam"},
       { "VG", "Virgin Islands"},
       { "VI", "Virgin Islands"},
       { "WF", "Wallis and Futuna"},
       { "EH", "Western Sahara"},
       { "YE", "Yemen"},
       { "ZM", "Zambia"},
       { "ZW", "Zimbabwe"},
       { NULL, NULL}
};

/* This comes from 
   $ man charsets
 * and 
   $ locale -a | grep -v @ | grep "\." | cut -d . -f 2 | sort -u
 *
 * On some machines is complains if codesets don't look like this
 * On linux its not really a problem but BSD has issues. So we neet to 
 * make sure that locale -a output gets converted to upper-case form in
 * all situations just to be safe. 
 */
const E_Intl_Pair charset_predefined_pairs[ ] = {
       /* These are in locale -a but not in charsets */
       {"cp1255", "CP1255"},
       {"euc", "EUC"},
       {"georgianps", "GEORGIAN-PS"},
       {"iso885914", "ISO-8859-14"},
       {"koi8t", "KOI8-T"},
       {"tcvn", "TCVN"},
       {"ujis", "UJIS"},

       /* These are from charsets man page */
       {"big5", "BIG5"},
       {"big5hkscs", "BIG5-HKSCS"},
       {"cp1251", "CP1251"},
       {"eucjp", "EUC-JP"},
       {"euckr", "EUC-KR"},
       {"euctw", "EUC-TW"},
       {"gb18030", "GB18030"},
       {"gb2312", "GB2312"},
       {"gbk", "GBK"},
       {"iso88591", "ISO-8859-1"},
       {"iso885913", "ISO-8859-13"},
       {"iso885915", "ISO-8859-15"},
       {"iso88592", "ISO-8859-2"},
       {"iso88593", "ISO-8859-3"},
       {"iso88595", "ISO-8859-5"},
       {"iso88596", "ISO-8859-6"},
       {"iso88597", "ISO-8859-7"},
       {"iso88598", "ISO-8859-8"},
       {"iso88599", "ISO-8859-9"},
       {"koi8r", "KOI8-R"},
       {"koi8u", "KOI8-U"},
       {"tis620", "TIS-620"},
       {"utf8", "UTF-8"},
       { NULL, NULL }
};



EAPI E_Config_Dialog *
e_int_config_intl(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "_config_intl_dialog")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);
   
   v->create_cfdata           = _create_data;
   v->free_cfdata             = _free_data;
   v->advanced.create_widgets = _advanced_create_widgets;
   v->advanced.apply_cfdata   = _advanced_apply_data;
   v->basic.create_widgets    = _basic_create_widgets;
   v->basic.apply_cfdata      = _basic_apply_data;
   
   cfd = e_config_dialog_new(con,
			     _("Language Settings"),
			    "E", "_config_intl_dialog",
			     "preferences-desktop-locale", 0, v, NULL);
   return cfd;
}

/* Build hash tables used for locale navigation. The locale information is 
 * gathered using the locale -a command. 
 *
 * Below the following terms are used:
 * ll - Locale Language Code (Example en)
 * RR - Locale Region code (Example US)
 * enc - Locale Encoding (Example UTF-8)
 * mod - Locale Modifier (Example EURO)
 */
static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   Eina_List    *e_lang_list;
   FILE		*output;
   
   e_lang_list = e_intl_language_list();
   
   /* Get list of all locales and start making map */
   output = popen("locale -a", "r");
   if ( output ) 
     {
	char line[32];
	while (fscanf(output, "%[^\n]\n", line) == 1)
	  {
	     E_Locale_Parts *locale_parts;

	     locale_parts = e_intl_locale_parts_get(line);

	     if (locale_parts)
	       {
		  char *basic_language;

		  basic_language = e_intl_locale_parts_combine(locale_parts, E_INTL_LOC_LANG | E_INTL_LOC_REGION);
		  if (basic_language)
		    {
		       int i;
		       
		       i = 0;
		       while (basic_language_predefined_pairs[i].locale_key)
			 {
			    /* if basic language is supported by E and System*/
			    if (!strncmp(basic_language_predefined_pairs[i].locale_key, 
				     basic_language, strlen(basic_language)))
			      {
				 if (!eina_list_data_find(cfdata->blang_list, &basic_language_predefined_pairs[i]))
				   cfdata->blang_list = eina_list_append(cfdata->blang_list, &basic_language_predefined_pairs[i]);
				 break;
			      }
			    i++;
			 }
		    }
		  E_FREE(basic_language);

		  /* If the language is a valid ll_RR[.enc[@mod]] locale add it to the hash */
		  if (locale_parts->lang)
		    {
		       E_Intl_Language_Node *lang_node;
		       E_Intl_Region_Node  *region_node;

		       /* Add the language to the new locale properties to the hash */
		       /* First check if the LANGUAGE exists in there already */

		       lang_node  = eina_hash_find(cfdata->locale_hash, locale_parts->lang);
		       if (lang_node == NULL)
			 {
			    Eina_List *next;
			    int i;

			    /* create new node */
			    lang_node = E_NEW(E_Intl_Language_Node, 1);

		            lang_node->lang_code = eina_stringshare_add(locale_parts->lang);

		            /* Check if the language list exists */
		            /* Linear Search */
		            for (next = e_lang_list; next; next = next->next) 
			      {
				 char *e_lang;

				 e_lang = next->data;
     				 if (!strncmp(e_lang, locale_parts->lang, 2) || !strcmp("en", locale_parts->lang)) 
				   {
				      lang_node->lang_available = 1;
				      break;
				   }
			      }
		       
			    /* Search for translation */
			    /* Linear Search */
			    i = 0;
			    while (language_predefined_pairs[i].locale_key)
			      {
				 if (!strcmp(language_predefined_pairs[i].locale_key, locale_parts->lang))
				   {
				      lang_node->lang_name = _(language_predefined_pairs[i].locale_translation);
				      break;
				   }
				 i++;
			      }

			    if (!cfdata->locale_hash)
			      cfdata->locale_hash = eina_hash_string_superfast_new(NULL);
			    eina_hash_add(cfdata->locale_hash, locale_parts->lang, lang_node);
			 }

		       /* We now have the current language hash node, lets see if there is
			  region data that needs to be added.
			*/

		       if (locale_parts->region)
			 {
			    region_node = eina_hash_find(lang_node->region_hash, locale_parts->region);

			    if (region_node == NULL)
			      {
				 int i;

				 /* create new node */
				 region_node = E_NEW(E_Intl_Region_Node, 1);
				 region_node->region_code = eina_stringshare_add(locale_parts->region);

				 /* Get the region translation */
				 /* Linear Search */
				 i = 0;
				 while (region_predefined_pairs[i].locale_key)
				   {
				      if (!strcmp(region_predefined_pairs[i].locale_key, locale_parts->region))
					{
					   region_node->region_name = _(region_predefined_pairs[i].locale_translation);
					   break;
					}
				      i++;
				   }
				 if (!lang_node->region_hash)
				   lang_node->region_hash = eina_hash_string_superfast_new(NULL);
				 eina_hash_add(lang_node->region_hash, locale_parts->region, region_node);
			      }

			    /* We now have the current region hash node */
		            /* Add codeset to the region hash node if it exists */
			    if (locale_parts->codeset)
			      {
				 const char * cs;
				 const char * cs_trans;
			    
				 cs_trans = _intl_charset_upper_get(locale_parts->codeset);
				 if (cs_trans == NULL) 
				   cs = eina_stringshare_add(locale_parts->codeset);
				 else 
				   cs = eina_stringshare_add(cs_trans);
			    
				 /* Exclusive */
				 /* Linear Search */
				 if (!eina_list_data_find(region_node->available_codesets, cs))
				   region_node->available_codesets = eina_list_append(region_node->available_codesets, cs);
			      }

			    /* Add modifier to the region hash node if it exists */
			    if (locale_parts->modifier)
			      {
				 const char *mod;

				 mod = eina_stringshare_add(locale_parts->modifier);
				 /* Find only works here because we are using stringshare*/
			    
				 /* Exclusive */
				 /* Linear Search */
				 if (!eina_list_data_find(region_node->available_modifiers, mod))
				   region_node->available_modifiers = eina_list_append(region_node->available_modifiers, mod);
			      }
			 }
		    }
		  e_intl_locale_parts_free(locale_parts);
	       }
	  }

	/* Sort basic languages */
	cfdata->blang_list = eina_list_sort(cfdata->blang_list,
	      eina_list_count(cfdata->blang_list),
	      _basic_lang_list_sort);

        while (e_lang_list)
	  {
	     free(e_lang_list->data);
	     e_lang_list = eina_list_remove_list(e_lang_list, e_lang_list);
	  }
	pclose(output);
     }

   /* Make sure we know the currently configured locale */
   if (e_config->language)
     cfdata->cur_language = strdup(e_config->language);

   return;
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->cfd = cfd;
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   E_FREE(cfdata->cur_language);

   eina_stringshare_del(cfdata->cur_blang);
   eina_stringshare_del(cfdata->cur_lang);
   eina_stringshare_del(cfdata->cur_reg);
   eina_stringshare_del(cfdata->cur_cs);
   eina_stringshare_del(cfdata->cur_mod);

   eina_hash_foreach(cfdata->locale_hash, _language_hash_free_cb, NULL);
   eina_hash_free(cfdata->locale_hash);

   cfdata->lang_list = eina_list_free(cfdata->lang_list);
   cfdata->region_list = eina_list_free(cfdata->region_list);
   cfdata->blang_list = eina_list_free(cfdata->blang_list);

   E_FREE(cfdata);
}

static Eina_Bool
_language_hash_free_cb(const Eina_Hash *hash __UNUSED__, const void *key __UNUSED__, void *data, void *fdata __UNUSED__)
{
   E_Intl_Language_Node *node;

   node = data;
   if (node->lang_code) eina_stringshare_del(node->lang_code);
   eina_hash_foreach(node->region_hash, _region_hash_free_cb, NULL);
   eina_hash_free(node->region_hash);
   free(node);

   return 1;
}

static Eina_Bool
_region_hash_free_cb(const Eina_Hash *hash __UNUSED__, const void *key __UNUSED__, void *data, void *fdata __UNUSED__)
{
   E_Intl_Region_Node *node;

   node = data;
   if (node->region_code) eina_stringshare_del(node->region_code);
   while (node->available_codesets)
     {
	const char *str;

	str = node->available_codesets->data;
	if (str) eina_stringshare_del(str);
	node->available_codesets =
          eina_list_remove_list(node->available_codesets, node->available_codesets);
     }

   while (node->available_modifiers)
     {
	const char *str;

	str = node->available_modifiers->data;
	if (str) eina_stringshare_del(str);
	node->available_modifiers =
          eina_list_remove_list(node->available_modifiers, node->available_modifiers);
     }

   free(node);
   return 1;
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   if (cfdata->cur_language)
     {
	if (e_config->language) eina_stringshare_del(e_config->language);
	e_config->language = eina_stringshare_add(cfdata->cur_language);
	e_intl_language_set(e_config->language);
     }

   e_config_save_queue();
   return 1;
}

static int
_advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   if (cfdata->cur_language)
     {
	if (e_config->language) eina_stringshare_del(e_config->language);
	e_config->language = eina_stringshare_add(cfdata->cur_language);
	e_intl_language_set(e_config->language);
     }

   e_config_save_queue();
   return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ob;
   char *cur_sig_loc;
   Eina_List *next;
   int i = 0;
   
   cfdata->evas = evas;
   o = e_widget_table_add(evas, 0);
   of = e_widget_framelist_add(evas, _("Language Selector"), 0);
   ob = e_widget_ilist_add(evas, 16, 16, &(cfdata->cur_blang));
   e_widget_min_size_set(ob, 175, 175);
   e_widget_on_change_hook_set(ob, _ilist_basic_language_cb_change, cfdata);
   cfdata->gui.blang_list = ob;
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(o, of, 0, 0, 1, 1, 1, 1, 1, 1);

   /* Load languages */
   evas_event_freeze(evas_object_evas_get(ob));
   edje_freeze();
   e_widget_ilist_freeze(ob);
   if (cfdata->cur_language)
     {
	E_Locale_Parts *locale_parts;
	locale_parts = e_intl_locale_parts_get(cfdata->cur_language);
	if (locale_parts)
	  {
	     cur_sig_loc = e_intl_locale_parts_combine(locale_parts,
		   E_INTL_LOC_LANG | E_INTL_LOC_REGION);

	     e_intl_locale_parts_free(locale_parts);
	  }
	else
	  cur_sig_loc = NULL;
     }
   else
     cur_sig_loc = NULL;

   for (next = cfdata->blang_list; next; next = next->next) 
     {
	E_Intl_Pair *pair;
	const char *key;
	const char *trans;

	pair = next->data;
	key = pair->locale_key;
	trans = _(pair->locale_translation);
	e_widget_ilist_append(cfdata->gui.blang_list, NULL, trans, NULL, NULL, key);
	if (cur_sig_loc && !strncmp(key, cur_sig_loc, strlen(cur_sig_loc)))
	  e_widget_ilist_selected_set(cfdata->gui.blang_list, i);
	
	i++;
     }
   E_FREE(cur_sig_loc);   
   e_widget_ilist_go(ob);
   e_widget_ilist_thaw(ob);
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(ob));

   of = e_widget_frametable_add(evas, _("Locale Selected"), 0);
   ob = e_widget_label_add(evas, _("Locale"));
   e_widget_frametable_object_append(of, ob, 0, 0, 1, 1, 1, 0, 1, 0);
   ob = e_widget_entry_add(evas, &(cfdata->cur_language), NULL, NULL, NULL);
   cfdata->gui.locale_entry = ob;
   e_widget_disabled_set(cfdata->gui.locale_entry, 1);
   e_widget_min_size_set(cfdata->gui.locale_entry, 100, 25);
   e_widget_frametable_object_append(of, cfdata->gui.locale_entry, 
				     0, 1, 1, 1, 1, 1, 1, 0);
   e_widget_table_object_append(o, of, 0, 1, 1, 1, 1, 0, 1, 0);

   e_dialog_resizable_set(cfd->dia, 1);
   return o;
}
   
static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ob;
   const char *lang, *reg, *cs, *mod;

   cfdata->evas = evas;

   _intl_current_locale_setup(cfdata);

   o = e_widget_table_add(evas, 0);

   of = e_widget_framelist_add(evas, _("Language Selector"), 1);

   /* Language List */
   ob = e_widget_ilist_add(evas, 16, 16, &(cfdata->cur_lang));
   cfdata->gui.lang_list = ob;

   /* If lang_list already loaded just use it */
   if (cfdata->lang_list == NULL)
     eina_hash_foreach(cfdata->locale_hash, _lang_hash_cb, cfdata);

   if (cfdata->lang_list)
     {
	cfdata->lang_list =
          eina_list_sort(cfdata->lang_list, eina_list_count(cfdata->lang_list),
                         _lang_list_sort);
	_lang_list_load(cfdata);
     }

   e_widget_ilist_go(ob);
   e_widget_min_size_set(ob, 140, 200);
   e_widget_framelist_object_append(of, ob);
   e_widget_ilist_selected_set(ob, e_widget_ilist_selected_get(ob));

   /* Region List */
   ob = e_widget_ilist_add(evas, 0, 0, &(cfdata->cur_reg));
   cfdata->gui.reg_list = ob;

   e_widget_ilist_go(ob);
   e_widget_min_size_set(ob, 100, 100);
   e_widget_framelist_object_append(of, ob);
   e_widget_ilist_selected_set(ob, e_widget_ilist_selected_get(ob));

   /* Codeset List */
   ob = e_widget_ilist_add(evas, 0, 0, &(cfdata->cur_cs));
   cfdata->gui.cs_list = ob;

   e_widget_ilist_go(ob);
   e_widget_min_size_set(ob, 100, 100);
   e_widget_framelist_object_append(of, ob);
   
   /* Modified List */
   ob = e_widget_ilist_add(evas, 0, 0, &(cfdata->cur_mod));
   cfdata->gui.mod_list = ob;

   e_widget_ilist_go(ob);
   e_widget_min_size_set(ob, 100, 100);
   e_widget_framelist_object_append(of, ob);

   e_widget_table_object_append(o, of, 0, 0, 1, 1, 1, 1, 1, 1);
   
   /* Locale selector */
   of = e_widget_frametable_add(evas, _("Locale Selected"), 0);
   ob = e_widget_label_add(evas, _("Locale"));
   e_widget_frametable_object_append(of, ob, 0, 0, 1, 1, 1, 0, 1, 0);
   ob = e_widget_entry_add(evas, &(cfdata->cur_language), NULL, NULL, NULL);
   cfdata->gui.locale_entry = ob;
   e_widget_disabled_set(cfdata->gui.locale_entry, 1);
   e_widget_min_size_set(cfdata->gui.locale_entry, 100, 25);
   e_widget_frametable_object_append(of, cfdata->gui.locale_entry, 
				     0, 1, 1, 1, 1, 1, 1, 0);
   e_widget_table_object_append(o, of, 0, 1, 1, 1, 1, 0, 1, 0);

   /* all these cur_* values are not guaranteed to be const so we need to
    * copy them. 
    */
   lang = eina_stringshare_ref(cfdata->cur_lang);
   reg = eina_stringshare_ref(cfdata->cur_reg);
   cs = eina_stringshare_ref(cfdata->cur_cs);
   mod = eina_stringshare_ref(cfdata->cur_mod);
   
   _cfdata_language_go(lang, reg, cs, mod, cfdata);
   
   eina_stringshare_del(lang);
   eina_stringshare_del(reg);
   eina_stringshare_del(cs);
   eina_stringshare_del(mod);
   
   e_widget_on_change_hook_set(cfdata->gui.lang_list, _ilist_language_cb_change, cfdata);
   e_widget_on_change_hook_set(cfdata->gui.reg_list, _ilist_region_cb_change, cfdata); 
   e_widget_on_change_hook_set(cfdata->gui.cs_list, _ilist_codeset_cb_change, cfdata); 
   e_widget_on_change_hook_set(cfdata->gui.mod_list, _ilist_modifier_cb_change, cfdata); 

   e_dialog_resizable_set(cfd->dia, 1);
   return o;
}

static void
_ilist_basic_language_cb_change(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data * cfdata;
    
   cfdata = data;

   e_widget_entry_text_set(cfdata->gui.locale_entry, cfdata->cur_blang);
}

static void
_ilist_language_cb_change(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data * cfdata;
   
   cfdata = data;
   
   _cfdata_language_go(cfdata->cur_lang, NULL, NULL, NULL, cfdata);
   
   e_widget_entry_text_set(cfdata->gui.locale_entry, cfdata->cur_lang);
   eina_stringshare_del(cfdata->cur_cs);
   eina_stringshare_del(cfdata->cur_mod);
}

static void
_ilist_region_cb_change(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data * cfdata;
   char locale[32];
    
   cfdata = data;
    
   _cfdata_language_go(cfdata->cur_lang, cfdata->cur_reg, NULL, NULL, cfdata);
    
   sprintf(locale, "%s_%s", cfdata->cur_lang, cfdata->cur_reg);
   e_widget_entry_text_set(cfdata->gui.locale_entry, locale);
   eina_stringshare_del(cfdata->cur_cs);
   eina_stringshare_del(cfdata->cur_mod);
}

static void 
_ilist_codeset_cb_change(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data * cfdata;
   char locale[32];
   
   cfdata = data;
   
   if (cfdata->cur_mod)
     sprintf(locale, "%s_%s.%s@%s", cfdata->cur_lang, cfdata->cur_reg, cfdata->cur_cs, cfdata->cur_mod);
   else
     sprintf(locale, "%s_%s.%s", cfdata->cur_lang, cfdata->cur_reg, cfdata->cur_cs);
   
   e_widget_entry_text_set(cfdata->gui.locale_entry, locale);
}

static void 
_ilist_modifier_cb_change(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data * cfdata;
   char locale[32];
   
   cfdata = data;
   if (cfdata->cur_cs)
     sprintf(locale, "%s_%s.%s@%s", cfdata->cur_lang, cfdata->cur_reg, cfdata->cur_cs, cfdata->cur_mod);
   else
     sprintf(locale, "%s_%s@%s", cfdata->cur_lang, cfdata->cur_reg, cfdata->cur_mod);
   
   e_widget_entry_text_set(cfdata->gui.locale_entry, locale);
}

static void 
_cfdata_language_go(const char *lang, const char *region, const char *codeset, const char *modifier, E_Config_Dialog_Data *cfdata)
{
   E_Intl_Language_Node *lang_node;	
   int lang_update;
   int region_update;
   
   /* Check what changed */
   lang_update = 0;
   region_update = 0;

   evas_event_freeze(evas_object_evas_get(cfdata->gui.cs_list));
   evas_event_freeze(evas_object_evas_get(cfdata->gui.mod_list));
   edje_freeze();
   e_widget_ilist_freeze(cfdata->gui.cs_list);
   e_widget_ilist_freeze(cfdata->gui.mod_list);

   if (cfdata->lang_dirty || (lang && !region))
     {
	lang_update = 1;
	region_update = 1;
	e_widget_ilist_clear(cfdata->gui.cs_list);
	e_widget_ilist_clear(cfdata->gui.mod_list);
     }
   if (lang && region)
     {
	region_update = 1;
	e_widget_ilist_clear(cfdata->gui.cs_list);
	e_widget_ilist_clear(cfdata->gui.mod_list);
     }

   cfdata->lang_dirty = 0;

   if (lang)
     {
	lang_node = eina_hash_find(cfdata->locale_hash, lang);

	if (lang_node)
	  {
	     if (lang_update)
	       {
		  e_widget_ilist_clear(cfdata->gui.reg_list);
		  cfdata->region_list = eina_list_free(cfdata->region_list);
		  eina_hash_foreach(lang_node->region_hash,
                                    _region_hash_cb, cfdata);
		  cfdata->region_list =
                    eina_list_sort(cfdata->region_list,
                                   eina_list_count(cfdata->region_list),
                                   _region_list_sort);
		  _region_list_load(cfdata);
	       }

	     if (region && region_update)
	       {
		  E_Intl_Region_Node *reg_node;

		  reg_node = eina_hash_find(lang_node->region_hash, region);
		  if (reg_node)
		    {
		       Eina_List *next;

		       for (next = reg_node->available_codesets; next; next = next->next)
			 {
			    const char * cs;

			    cs = next->data;
			    e_widget_ilist_append(cfdata->gui.cs_list, NULL, cs, NULL, NULL, cs);
			    if (codeset && !strcmp(cs, codeset))
			      {
				 int count;

				 count = e_widget_ilist_count(cfdata->gui.cs_list);
				 e_widget_ilist_selected_set(cfdata->gui.cs_list, count - 1);
			      }
			 }
		       
		       for (next = reg_node->available_modifiers; next; next = next->next) 
			 {
			    const char * mod;
			    
			    mod = next->data;
			    e_widget_ilist_append(cfdata->gui.mod_list, NULL, mod, NULL, NULL, mod);
			    if (modifier && !strcmp(mod, modifier)) 
			      {
				 int count;

				 count = e_widget_ilist_count(cfdata->gui.mod_list);
				 e_widget_ilist_selected_set(cfdata->gui.mod_list, count - 1);
			      }

			 }
		    }
		  e_widget_ilist_go(cfdata->gui.cs_list);
		  e_widget_ilist_go(cfdata->gui.mod_list);
	       }
	  }
     }
   e_widget_ilist_thaw(cfdata->gui.cs_list);
   e_widget_ilist_thaw(cfdata->gui.mod_list);
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(cfdata->gui.cs_list));
   evas_event_thaw(evas_object_evas_get(cfdata->gui.mod_list));

   e_widget_ilist_go(cfdata->gui.reg_list);
}

static Eina_Bool
_lang_hash_cb(const Eina_Hash *hash __UNUSED__, const void *key __UNUSED__, void *data, void *fdata)
{
   E_Config_Dialog_Data *cfdata;
   E_Intl_Language_Node *lang_node;

   cfdata = fdata;
   lang_node = data;

   cfdata->lang_list = eina_list_append(cfdata->lang_list, lang_node);
   return 1;
}

static Eina_Bool
_region_hash_cb(const Eina_Hash *hash __UNUSED__, const void *key __UNUSED__, void *data, void *fdata)
{
   E_Config_Dialog_Data *cfdata;
   E_Intl_Region_Node *reg_node;

   cfdata = fdata;
   reg_node = data;

   cfdata->region_list = eina_list_append(cfdata->region_list, reg_node);
   return 1;
}

void 
_intl_current_locale_setup(E_Config_Dialog_Data *cfdata)
{
   eina_stringshare_del(cfdata->cur_lang);
   eina_stringshare_del(cfdata->cur_reg);
   eina_stringshare_del(cfdata->cur_cs);
   eina_stringshare_del(cfdata->cur_mod);

   cfdata->cur_lang = NULL;
   cfdata->cur_reg = NULL;
   cfdata->cur_cs = NULL;
   cfdata->cur_mod = NULL;
 
   if (cfdata->cur_language)
     {
	E_Locale_Parts *locale_parts;
 
	locale_parts = e_intl_locale_parts_get(cfdata->cur_language);
	if (locale_parts)
	  {
	     cfdata->cur_lang = eina_stringshare_add(locale_parts->lang);
	     cfdata->cur_reg = eina_stringshare_add(locale_parts->region);
	     if (locale_parts->codeset) 
	       {
		  const char *cs_trans;
	
		  cs_trans = _intl_charset_upper_get(locale_parts->codeset);
		  if (cs_trans == NULL)
		    cfdata->cur_cs = eina_stringshare_add(locale_parts->codeset);
		  else
		    cfdata->cur_cs = eina_stringshare_add(cs_trans);
	       }
	     cfdata->cur_mod = eina_stringshare_add(locale_parts->modifier);
	  }
	e_intl_locale_parts_free(locale_parts);
     }
   cfdata->lang_dirty = 1;
}

static int 
_lang_list_sort(const void *data1, const void *data2) 
{
   const E_Intl_Language_Node *ln1, *ln2;
   const char *trans1;
   const char *trans2;
   
   if (!data1) return 1;
   if (!data2) return -1;
   
   ln1 = data1;
   ln2 = data2;

   if (!ln1->lang_name) return 1;
   trans1 = ln1->lang_name;

   if (!ln2->lang_name) return -1;
   trans2 = ln2->lang_name;
   
   return (strcmp(trans1, trans2));
}

static void 
_lang_list_load(void *data) 
{
   E_Config_Dialog_Data *cfdata;
   Eina_List *l;
   
   if (!data) return;

   cfdata = data;
   if (!cfdata->lang_list) return;

   evas_event_freeze(evas_object_evas_get(cfdata->gui.lang_list));
   edje_freeze();
   e_widget_ilist_freeze(cfdata->gui.lang_list);
   
   for (l = cfdata->lang_list; l; l = l->next) 
     {
	E_Intl_Language_Node *ln;
	const char *trans;
	
	ln = l->data;
	if (!ln) continue;
	if (ln->lang_name)
	  trans = ln->lang_name;
	else 
	  trans = ln->lang_code;
	
	if (ln->lang_available)
	  {
	     Evas_Object *ic;
	
	     ic = e_icon_add(cfdata->evas);
	     e_util_icon_theme_set(ic, "dialog-ok-apply");
	     e_widget_ilist_append(cfdata->gui.lang_list, ic, trans, NULL, NULL, ln->lang_code);
	  }
	else
	  e_widget_ilist_append(cfdata->gui.lang_list, NULL, trans, NULL, NULL, ln->lang_code);

	if (cfdata->cur_lang && !strcmp(cfdata->cur_lang, ln->lang_code)) 
	  {
	     int count;
	     
	     count = e_widget_ilist_count(cfdata->gui.lang_list);
	     e_widget_ilist_selected_set(cfdata->gui.lang_list, count - 1);
	  }
     }
   e_widget_ilist_thaw(cfdata->gui.lang_list);
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(cfdata->gui.lang_list));
}

static int 
_region_list_sort(const void *data1, const void *data2) 
{
   const E_Intl_Region_Node *rn1, *rn2;
   const char *trans1;
   const char *trans2;
   
   if (!data1) return 1;
   if (!data2) return -1;
   
   rn1 = data1;
   rn2 = data2;

   if (!rn1->region_name) return 1;
   trans1 = rn1->region_name;

   if (!rn2->region_name) return -1;
   trans2 = rn2->region_name;
   
   return (strcmp(trans1, trans2));
}

static void 
_region_list_load(void *data) 
{
   E_Config_Dialog_Data *cfdata;
   Eina_List *l;
   
   if (!data) return;

   cfdata = data;
   if (!cfdata->region_list) return;

   evas_event_freeze(evas_object_evas_get(cfdata->gui.reg_list));
   edje_freeze();
   e_widget_ilist_freeze(cfdata->gui.reg_list);
   
   for (l = cfdata->region_list; l; l = l->next) 
     {
	E_Intl_Region_Node *rn;
	const char *trans;
	
	rn = l->data;
	if (!rn) continue;
	if (rn->region_name)
	  trans = rn->region_name;
	else 
	  trans = rn->region_code;
	
	e_widget_ilist_append(cfdata->gui.reg_list, NULL, trans, NULL, NULL, rn->region_code);
  
	if (cfdata->cur_reg && !strcmp(cfdata->cur_reg, rn->region_code)) 
	  {
	     int count;
	     
	     count = e_widget_ilist_count(cfdata->gui.reg_list);
	     e_widget_ilist_selected_set(cfdata->gui.reg_list, count - 1);
	  }
     }
   e_widget_ilist_thaw(cfdata->gui.reg_list);
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(cfdata->gui.reg_list));
}

static int 
_basic_lang_list_sort(const void *data1, const void *data2) 
{
   const E_Intl_Pair *ln1, *ln2;
   const char *trans1;
   const char *trans2;
   
   if (!data1) return 1;
   if (!data2) return -1;
   
   ln1 = data1;
   ln2 = data2;

   if (!ln1->locale_translation) return 1;
   trans1 = ln1->locale_translation;

   if (!ln2->locale_translation) return -1;
   trans2 = ln2->locale_translation;
   
   return (strcmp(trans1, trans2));
}

const char *
_intl_charset_upper_get(const char *charset)
{
   int i;
   
   i = 0;
   while (charset_predefined_pairs[i].locale_key)
     {
	if (!strcmp(charset_predefined_pairs[i].locale_key, charset))
	  {
	     return charset_predefined_pairs[i].locale_translation;
	  }			 
	i++;			      
     }
   return NULL;
}
