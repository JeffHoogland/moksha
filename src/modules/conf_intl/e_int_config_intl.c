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
   const char *locale_icon;
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
       {"bg_BG.UTF-8", "lang-bg_BG.png", "Български"},
       {"ca_ES.UTF-8", "lang-ca_ES.png", "Català"},
       {"zh_CN.UTF-8", "lang-zh_CN.png", "Chinese (Simplified)"},
       {"zh_TW.UTF-8", "lang-zh_TW.png", "Chinese (Traditional)"},
       {"cs_CZ.UTF-8", "lang-cs_CZ.png", "Čeština"},
       {"da_DK.UTF-8", "lang-da_DK.png", "Dansk"},
       {"nl_NL.UTF-8", "lang-nl_NL.png", "Nederlands"},
       {"en_US.UTF-8", "lang-en_US.png", "English"},
       {"en_GB.UTF-8", NULL,             "British English"},
       {"fi_FI.UTF-8", "lang-fi_FI.png", "Suomi"},
       {"fr_FR.UTF-8", "lang-fr_FR.png", "Français"},
       {"de_DE.UTF-8", "lang-de_DE.png", "Deutsch"},
       {"hu_HU.UTF-8", "lang-hu_HU.png", "Magyar"},
       {"it_IT.UTF-8", "lang-it_IT.png", "Italiano"},
       {"ja_JP.UTF-8", "lang-ja_JP.png", "日本語"},
       {"ko_KR.UTF-8", "lang-ko_KR.png", "한국어"},
       {"nb_NO.UTF-8", "lang-nb_NO.png", "Norsk Bokmål"},
       {"pl_PL.UTF-8", "lang-pl_PL.png", "Polski"},
       {"pt_BR.UTF-8", "lang-pt_BR.png", "Português"},
       {"ru_RU.UTF-8", "lang-ru_RU.png", "Русский"},
       {"sk_SK.UTF-8", "lang-sk_SK.png", "Slovenčina"},
       {"sl_SI.UTF-8", "lang-sl_SI.png", "Slovenščina"},
       {"es_AR.UTF-8", "lang-es_AR.png", "Español"},
       {"sv_SE.UTF-8", "lang-sv_SE.png", "Svenska"},
       {"el_GR.UTF-8", "lang-el_GR.png", "Ελληνικά"},
       { NULL, NULL, NULL }
};

const E_Intl_Pair language_predefined_pairs[ ] = {
       {"aa", NULL, "Qafár af"},
       {"af", NULL, "Afrikaans"},
       {"ak", NULL, "Akan"},
       {"am", NULL, "አማርኛ"},
       {"an", NULL, "Aragonés"},
       {"ar", NULL, "ةيبرعلا"},
       {"as", NULL, "অসমীয়া"},
       {"az", NULL, "Azərbaycan dili"},
       {"be", NULL, "Беларуская мова"},
       {"bg", NULL, "Български"},
       {"bn", NULL, "বাংলা"},
       {"br", NULL, "Brezhoneg"},
       {"bs", NULL, "Bosanski"},
       {"byn", NULL, "ብሊና"},
       {"ca", NULL, "Català"},
       {"cch", NULL, "Atsam"},
       {"cs", NULL, "Čeština"},
       {"cy", NULL, "Cymraeg"},
       {"da", NULL, "Dansk"},
       {"de", NULL, "Deutsch"},
       {"dv", NULL, "ދިވެހި"},
       {"dz", NULL, "Dzongkha"},
       {"ee", NULL, "Eʋegbe"},
       {"el", NULL, "Ελληνικά"},
       {"en", NULL, "English"},
       {"eo", NULL, "Esperanto"},
       {"es", NULL, "Español"},
       {"et", NULL, "Eesti keel"},
       {"eu", NULL, "Euskara"},
       {"fa", NULL, "یسراف"},
       {"fi", NULL, "Suomi"},
       {"fo", NULL, "Føroyskt"},
       {"fr", NULL, "Français"},
       {"fur", NULL, "Furlan"},
       {"ga", NULL, "Gaeilge"},
       {"gaa", NULL, "Gã"},
       {"gez", NULL, "ግዕዝ"},
       {"gl", NULL, "Galego"},
       {"gu", NULL, "Gujarati"},
       {"gv", NULL, "Yn Ghaelg"},
       {"ha", NULL, "Hausa"},
       {"haw", NULL, "ʻŌlelo Hawaiʻi"},
       {"he", NULL, "עברית"},
       {"hi", NULL, "Hindi"},
       {"hr", NULL, "Hrvatski"},
       {"hu", NULL, "Magyar"},
       {"hy", NULL, "Հայերեն"},
       {"ia", NULL, "Interlingua"},
       {"id", NULL, "Indonesian"},
       {"ig", NULL, "Igbo"},
       {"is", NULL, "Íslenska"},
       {"it", NULL, "Italiano"},
       {"iu", NULL, "ᐃᓄᒃᑎᑐᑦ"},
       {"iw", NULL, "עברית"},
       {"ja", NULL, "日本語"},
       {"ka", NULL, "ქართული"},
       {"kaj", NULL, "Jju"},
       {"kam", NULL, "Kikamba"},
       {"kcg", NULL, "Tyap"},
       {"kfo", NULL, "Koro"},
       {"kk", NULL, "Qazaq"},
       {"kl", NULL, "Kalaallisut"},
       {"km", NULL, "Khmer"},
       {"kn", NULL, "ಕನ್ನಡ"},
       {"ko", NULL, "한국어"},
       {"kok", NULL, "Konkani"},
       {"ku", NULL, "یدروك"},
       {"kw", NULL, "Kernowek"},
       {"ky", NULL, "Кыргыз тили"},
       {"ln", NULL, "Lingála"},
       {"lo", NULL, "ພາສາລາວ"},
       {"lt", NULL, "Lietuvių kalba"},
       {"lv", NULL, "Latviešu"},
       {"mi", NULL, "Te Reo Māori"},
       {"mk", NULL, "Македонски"},
       {"ml", NULL, "മലയാളം"},
       {"mn", NULL, "Монгол"},
       {"mr", NULL, "मराठी"},
       {"ms", NULL, "Bahasa Melayu"},
       {"mt", NULL, "Malti"},
       {"nb", NULL, "Norsk Bokmål"},
       {"ne", NULL, "नेपाली"},
       {"nl", NULL, "Nederlands"},
       {"nn", NULL, "Norsk Nynorsk"},
       {"no", NULL, "Norsk"},
       {"nr", NULL, "isiNdebele"},
       {"nso", NULL, "Sesotho sa Leboa"},
       {"ny", NULL, "Chicheŵa"},
       {"oc", NULL, "Occitan"},
       {"om", NULL, "Oromo"},
       {"or", NULL, "ଓଡ଼ିଆ"},
       {"pa", NULL, "ਪੰਜਾਬੀ"},
       {"pl", NULL, "Polski"},
       {"ps", NULL, "وتښپ"},
       {"pt", NULL, "Português"},
       {"ro", NULL, "Română"},
       {"ru", NULL, "Русский"},
       {"rw", NULL, "Kinyarwanda"},
       {"sa", NULL, "संस्कृतम्"},
       {"se", NULL, "Davvisápmi"},
       {"sh", NULL, "Srpskohrvatski/Српскохрватски"},
       {"sid", NULL, "Sidámo 'Afó"},
       {"sk", NULL, "Slovenčina"},
       {"sl", NULL, "Slovenščina"},
       {"so", NULL, "af Soomaali"},
       {"sq", NULL, "Shqip"},
       {"sr", NULL, "Српски"},
       {"ss", NULL, "Swati"},
       {"st", NULL, "Southern Sotho"},
       {"sv", NULL, "Svenska"},
       {"sw", NULL, "Swahili"},
       {"syr", NULL, "Syriac"},
       {"ta", NULL, "தமிழ்"},
       {"te", NULL, "తెలుగు"},
       {"tg", NULL, "Тоҷикӣ"},
       {"th", NULL, "ภาษาไทย"},
       {"ti", NULL, "ትግርኛ"},
       {"tig", NULL, "ቲግሬ"},
       {"tl", NULL, "Tagalog"},
       {"tn", NULL, "Setswana"},
       {"tr", NULL, "Türkçe"},
       {"ts", NULL, "Tsonga"},
       {"tt", NULL, "Татарча"},
       {"uk", NULL, "Українська мова"},
       {"ur", NULL, "ودراُ"},
       {"uz", NULL, "O‘zbek"},
       {"ve", NULL, "Venda"},
       {"vi", NULL, "Tiếng Việt"},
       {"wa", NULL, "Walon"},
       {"wal", NULL, "Walamo"},
       {"xh", NULL, "Xhosa"},
       {"yi", NULL, "שידיִי"},
       {"yo", NULL, "èdèe Yorùbá"},
       {"zh", NULL, "汉语/漢語"},
       {"zu", NULL, "Zulu"},
       { NULL, NULL, NULL}
};

const E_Intl_Pair region_predefined_pairs[ ] = {
       { "AF", NULL, "Afghanistan"},
       { "AX", NULL, "Åland"},
       { "AL", NULL, "Shqipëria"},
       { "DZ", NULL, "Algeria"},
       { "AS", NULL, "Amerika Sāmoa"},
       { "AD", NULL, "Andorra"},
       { "AO", NULL, "Angola"},
       { "AI", NULL, "Anguilla"},
       { "AQ", NULL, "Antarctica"},
       { "AG", NULL, "Antigua and Barbuda"},
       { "AR", NULL, "Argentina"},
       { "AM", NULL, "Հայաստան"},
       { "AW", NULL, "Aruba"},
       { "AU", NULL, "Australia"},
       { "AT", NULL, "Österreich"},
       { "AZ", NULL, "Azərbaycan"},
       { "BS", NULL, "Bahamas"},
       { "BH", NULL, "Bahrain"},
       { "BD", NULL, "বাংলাদেশ"},
       { "BB", NULL, "Barbados"},
       { "BY", NULL, "Беларусь"},
       { "BE", NULL, "Belgium"},
       { "BZ", NULL, "Belize"},
       { "BJ", NULL, "Bénin"},
       { "BM", NULL, "Bermuda"},
       { "BT", NULL, "Bhutan"},
       { "BO", NULL, "Bolivia"},
       { "BA", NULL, "Bosnia and Herzegovina"},
       { "BW", NULL, "Botswana"},
       { "BV", NULL, "Bouvetøya"},
       { "BR", NULL, "Brazil"},
       { "IO", NULL, "British Indian Ocean Territory"},
       { "BN", NULL, "Brunei Darussalam"},
       { "BG", NULL, "България"},
       { "BF", NULL, "Burkina Faso"},
       { "BI", NULL, "Burundi"},
       { "KH", NULL, "Cambodia"},
       { "CM", NULL, "Cameroon"},
       { "CA", NULL, "Canada"},
       { "CV", NULL, "Cape Verde"},
       { "KY", NULL, "Cayman Islands"},
       { "CF", NULL, "Central African Republic"},
       { "TD", NULL, "Chad"},
       { "CL", NULL, "Chile"},
       { "CN", NULL, "中國"},
       { "CX", NULL, "Christmas Island"},
       { "CC", NULL, "Cocos (keeling) Islands"},
       { "CO", NULL, "Colombia"},
       { "KM", NULL, "Comoros"},
       { "CG", NULL, "Congo"},
       { "CD", NULL, "Congo"},
       { "CK", NULL, "Cook Islands"},
       { "CR", NULL, "Costa Rica"},
       { "CI", NULL, "Cote d'Ivoire"},
       { "HR", NULL, "Hrvatska"},
       { "CU", NULL, "Cuba"},
       { "CY", NULL, "Cyprus"},
       { "CZ", NULL, "Česká republika"},
       { "DK", NULL, "Danmark"},
       { "DJ", NULL, "Djibouti"},
       { "DM", NULL, "Dominica"},
       { "DO", NULL, "República Dominicana"},
       { "EC", NULL, "Ecuador"},
       { "EG", NULL, "Egypt"},
       { "SV", NULL, "El Salvador"},
       { "GQ", NULL, "Equatorial Guinea"},
       { "ER", NULL, "Eritrea"},
       { "EE", NULL, "Eesti"},
       { "ET", NULL, "Ethiopia"},
       { "FK", NULL, "Falkland Islands (malvinas)"},
       { "FO", NULL, "Faroe Islands"},
       { "FJ", NULL, "Fiji"},
       { "FI", NULL, "Finland"},
       { "FR", NULL, "France"},
       { "GF", NULL, "French Guiana"},
       { "PF", NULL, "French Polynesia"},
       { "TF", NULL, "French Southern Territories"},
       { "GA", NULL, "Gabon"},
       { "GM", NULL, "Gambia"},
       { "GE", NULL, "Georgia"},
       { "DE", NULL, "Deutschland"},
       { "GH", NULL, "Ghana"},
       { "GI", NULL, "Gibraltar"},
       { "GR", NULL, "Greece"},
       { "GL", NULL, "Greenland"},
       { "GD", NULL, "Grenada"},
       { "GP", NULL, "Guadeloupe"},
       { "GU", NULL, "Guam"},
       { "GT", NULL, "Guatemala"},
       { "GG", NULL, "Guernsey"},
       { "GN", NULL, "Guinea"},
       { "GW", NULL, "Guinea-Bissau"},
       { "GY", NULL, "Guyana"},
       { "HT", NULL, "Haiti"},
       { "HM", NULL, "Heard Island and Mcdonald Islands"},
       { "VA", NULL, "Holy See (Vatican City State)"},
       { "HN", NULL, "Honduras"},
       { "HK", NULL, "Hong Kong"},
       { "HU", NULL, "Magyarország"},
       { "IS", NULL, "Iceland"},
       { "IN", NULL, "India"},
       { "ID", NULL, "Indonesia"},
       { "IR", NULL, "Iran"},
       { "IQ", NULL, "Iraq"},
       { "IE", NULL, "Éire"},
       { "IM", NULL, "Isle Of Man"},
       { "IL", NULL, "Israel"},
       { "IT", NULL, "Italia"},
       { "JM", NULL, "Jamaica"},
       { "JP", NULL, "日本"},
       { "JE", NULL, "Jersey"},
       { "JO", NULL, "Jordan"},
       { "KZ", NULL, "Kazakhstan"},
       { "KE", NULL, "Kenya"},
       { "KI", NULL, "Kiribati"},
       { "KP", NULL, "Korea"},
       { "KR", NULL, "Korea"},
       { "KW", NULL, "Kuwait"},
       { "KG", NULL, "Kyrgyzstan"},
       { "LA", NULL, "Lao People's Democratic Republic"},
       { "LV", NULL, "Latvija"},
       { "LB", NULL, "Lebanon"},
       { "LS", NULL, "Lesotho"},
       { "LR", NULL, "Liberia"},
       { "LY", NULL, "Libyan Arab Jamahiriya"},
       { "LI", NULL, "Liechtenstein"},
       { "LT", NULL, "Lietuva"},
       { "LU", NULL, "Lëtzebuerg"},
       { "MO", NULL, "Macao"},
       { "MK", NULL, "Македонија"},
       { "MG", NULL, "Madagascar"},
       { "MW", NULL, "Malawi"},
       { "MY", NULL, "Malaysia"},
       { "MV", NULL, "Maldives"},
       { "ML", NULL, "Mali"},
       { "MT", NULL, "Malta"},
       { "MH", NULL, "Marshall Islands"},
       { "MQ", NULL, "Martinique"},
       { "MR", NULL, "Mauritania"},
       { "MU", NULL, "Mauritius"},
       { "YT", NULL, "Mayotte"},
       { "MX", NULL, "Mexico"},
       { "FM", NULL, "Micronesia"},
       { "MD", NULL, "Moldova"},
       { "MC", NULL, "Monaco"},
       { "MN", NULL, "Mongolia"},
       { "MS", NULL, "Montserrat"},
       { "MA", NULL, "Morocco"},
       { "MZ", NULL, "Mozambique"},
       { "MM", NULL, "Myanmar"},
       { "NA", NULL, "Namibia"},
       { "NR", NULL, "Nauru"},
       { "NP", NULL, "Nepal"},
       { "NL", NULL, "Nederland"},
       { "AN", NULL, "Netherlands Antilles"},
       { "NC", NULL, "New Caledonia"},
       { "NZ", NULL, "New Zealand"},
       { "NI", NULL, "Nicaragua"},
       { "NE", NULL, "Niger"},
       { "NG", NULL, "Nigeria"},
       { "NU", NULL, "Niue"},
       { "NF", NULL, "Norfolk Island"},
       { "MP", NULL, "Northern Mariana Islands"},
       { "NO", NULL, "Norge"},
       { "OM", NULL, "Oman"},
       { "PK", NULL, "Pakistan"},
       { "PW", NULL, "Palau"},
       { "PS", NULL, "Palestinian Territory"},
       { "PA", NULL, "Panama"},
       { "PG", NULL, "Papua New Guinea"},
       { "PY", NULL, "Paraguay"},
       { "PE", NULL, "Peru"},
       { "PH", NULL, "Philippines"},
       { "PN", NULL, "Pitcairn"},
       { "PL", NULL, "Poland"},
       { "PT", NULL, "Portugal"},
       { "PR", NULL, "Puerto Rico"},
       { "QA", NULL, "Qatar"},
       { "RE", NULL, "Reunion"},
       { "RO", NULL, "Romania"},
       { "RU", NULL, "Russian Federation"},
       { "RW", NULL, "Rwanda"},
       { "SH", NULL, "Saint Helena"},
       { "KN", NULL, "Saint Kitts and Nevis"},
       { "LC", NULL, "Saint Lucia"},
       { "PM", NULL, "Saint Pierre and Miquelon"},
       { "VC", NULL, "Saint Vincent and the Grenadines"},
       { "WS", NULL, "Samoa"},
       { "SM", NULL, "San Marino"},
       { "ST", NULL, "Sao Tome and Principe"},
       { "SA", NULL, "Saudi Arabia"},
       { "SN", NULL, "Senegal"},
       { "CS", NULL, "Serbia and Montenegro"},
       { "SC", NULL, "Seychelles"},
       { "SL", NULL, "Sierra Leone"},
       { "SG", NULL, "Singapore"},
       { "SK", NULL, "Slovakia"},
       { "SI", NULL, "Slovenia"},
       { "SB", NULL, "Solomon Islands"},
       { "SO", NULL, "Somalia"},
       { "ZA", NULL, "South Africa"},
       { "GS", NULL, "South Georgia and the South Sandwich Islands"},
       { "ES", NULL, "Spain"},
       { "LK", NULL, "Sri Lanka"},
       { "SD", NULL, "Sudan"},
       { "SR", NULL, "Suriname"},
       { "SJ", NULL, "Svalbard and Jan Mayen"},
       { "SZ", NULL, "Swaziland"},
       { "SE", NULL, "Sweden"},
       { "CH", NULL, "Switzerland"},
       { "SY", NULL, "Syrian Arab Republic"},
       { "TW", NULL, "Taiwan"},
       { "TJ", NULL, "Tajikistan"},
       { "TZ", NULL, "Tanzania"},
       { "TH", NULL, "Thailand"},
       { "TL", NULL, "Timor-Leste"},
       { "TG", NULL, "Togo"},
       { "TK", NULL, "Tokelau"},
       { "TO", NULL, "Tonga"},
       { "TT", NULL, "Trinidad and Tobago"},
       { "TN", NULL, "Tunisia"},
       { "TR", NULL, "Turkey"},
       { "TM", NULL, "Turkmenistan"},
       { "TC", NULL, "Turks and Caicos Islands"},
       { "TV", NULL, "Tuvalu"},
       { "UG", NULL, "Uganda"},
       { "UA", NULL, "Ukraine"},
       { "AE", NULL, "United Arab Emirates"},
       { "GB", NULL, "United Kingdom"},
       { "US", NULL, "United States"},
       { "UM", NULL, "United States Minor Outlying Islands"},
       { "UY", NULL, "Uruguay"},
       { "UZ", NULL, "Uzbekistan"},
       { "VU", NULL, "Vanuatu"},
       { "VE", NULL, "Venezuela"},
       { "VN", NULL, "Viet Nam"},
       { "VG", NULL, "Virgin Islands"},
       { "VI", NULL, "Virgin Islands"},
       { "WF", NULL, "Wallis and Futuna"},
       { "EH", NULL, "Western Sahara"},
       { "YE", NULL, "Yemen"},
       { "ZM", NULL, "Zambia"},
       { "ZW", NULL, "Zimbabwe"},
       { NULL, NULL, NULL}
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
       {"cp1255", NULL, "CP1255"},
       {"euc", NULL, "EUC"},
       {"georgianps", NULL, "GEORGIAN-PS"},
       {"iso885914", NULL, "ISO-8859-14"},
       {"koi8t", NULL, "KOI8-T"},
       {"tcvn", NULL, "TCVN"},
       {"ujis", NULL, "UJIS"},

       /* These are from charsets man page */
       {"big5", NULL, "BIG5"},
       {"big5hkscs", NULL, "BIG5-HKSCS"},
       {"cp1251", NULL, "CP1251"},
       {"eucjp", NULL, "EUC-JP"},
       {"euckr", NULL, "EUC-KR"},
       {"euctw", NULL, "EUC-TW"},
       {"gb18030", NULL, "GB18030"},
       {"gb2312", NULL, "GB2312"},
       {"gbk", NULL, "GBK"},
       {"iso88591", NULL, "ISO-8859-1"},
       {"iso885913", NULL, "ISO-8859-13"},
       {"iso885915", NULL, "ISO-8859-15"},
       {"iso88592", NULL, "ISO-8859-2"},
       {"iso88593", NULL, "ISO-8859-3"},
       {"iso88595", NULL, "ISO-8859-5"},
       {"iso88596", NULL, "ISO-8859-6"},
       {"iso88597", NULL, "ISO-8859-7"},
       {"iso88598", NULL, "ISO-8859-8"},
       {"iso88599", NULL, "ISO-8859-9"},
       {"koi8r", NULL, "KOI8-R"},
       {"koi8u", NULL, "KOI8-U"},
       {"tis620", NULL, "TIS-620"},
       {"utf8", NULL, "UTF-8"},
       { NULL, NULL, NULL }
};

E_Config_Dialog *
e_int_config_intl(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "language/language_settings")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);
   
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->advanced.create_widgets = _advanced_create_widgets;
   v->advanced.apply_cfdata = _advanced_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->basic.apply_cfdata = _basic_apply_data;
   
   cfd = e_config_dialog_new(con,
			     _("Language Settings"),
			     "E", "language/language_settings",
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
		       if (!lang_node)
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

			    if (!region_node)
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
				 const char * cs = NULL;
				 const char * cs_trans;
			    
				 cs_trans = _intl_charset_upper_get(locale_parts->codeset);
				 if (!cs_trans) 
				   cs = eina_stringshare_add(locale_parts->codeset);
				 else 
				   cs = eina_stringshare_add(cs_trans);
			    
				 /* Exclusive */
				 /* Linear Search */
				 if (!eina_list_data_find(region_node->available_codesets, cs))
				   region_node->available_codesets = eina_list_append(region_node->available_codesets, cs);
                                 else eina_stringshare_del(cs);
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
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
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

static void
_lc_check(void)
{
   char buf[8192];
   
   buf[0] = 0;
   
   if (getenv("LC_CTYPE"))    strcat(buf, "LC_CTYPE<br>");
   if (getenv("LC_NUMERIC"))  strcat(buf, "LC_NUMERIC<br>");
   if (getenv("LC_TIME"))     strcat(buf, "LC_TIME<br>");
   if (getenv("LC_COLLATE"))  strcat(buf, "LC_COLLATE<br>");
   if (getenv("LC_MONETARY")) strcat(buf, "LC_MONETARY<br>");
   if (getenv("LC_MESSAGES")) strcat(buf, "LC_MESSAGES<br>");
   if (getenv("LC_ALL"))      strcat(buf, "LC_ALL<br>");
   
   if (buf[0] != 0)
      e_util_dialog_show(_("Possible Locale problems"), 
                         _("You have some extra locale environment<br>"
                           "variables set that may interfere with<br>"
                           "correct display of your chosen language.<br>"
                           "If you don't want these affected, use the<br>"
                           "Enviornment variable settings to unset them.<br>"
                           "The variables that may affect you are<br>"
                           "as follows:<br>"
                           "%s"), buf);
}

static int
_basic_apply_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   if (cfdata->cur_language)
     {
	if (e_config->language) eina_stringshare_del(e_config->language);
        e_config->language = NULL;
        if ((cfdata->cur_language) && (cfdata->cur_language[0]))
           e_config->language = eina_stringshare_add(cfdata->cur_language);
	e_intl_language_set(e_config->language);
        _lc_check();
     }

   e_config_save_queue();
   return 1;
}

static int
_advanced_apply_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   if (cfdata->cur_language)
     {
	if (e_config->language) eina_stringshare_del(e_config->language);
        e_config->language = NULL;
        if ((cfdata->cur_language) && (cfdata->cur_language[0]))
           e_config->language = eina_stringshare_add(cfdata->cur_language);
	e_intl_language_set(e_config->language);
        _lc_check();
     }

   e_config_save_queue();
   return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ob, *ic;
   char *cur_sig_loc;
   Eina_List *next;
   int i = 0;
   char buf[PATH_MAX];
   
   cfdata->evas = evas;
   o = e_widget_table_add(evas, 0);
   of = e_widget_framelist_add(evas, _("Language Selector"), 0);
   ob = e_widget_ilist_add(evas, 16, 16, &(cfdata->cur_blang));
   e_widget_size_min_set(ob, 100, 80);
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

   e_prefix_data_snprintf(buf, sizeof(buf), "data/images/%s", "lang-system.png");
   ic = e_util_icon_add(buf, evas);
   e_widget_ilist_append(cfdata->gui.blang_list, ic, _("System Default"), NULL, NULL, "");
   if ((!cur_sig_loc) || (!cfdata->cur_language))
      e_widget_ilist_selected_set(cfdata->gui.blang_list, i);
   i++;
   
   for (next = cfdata->blang_list; next; next = next->next) 
     {
	E_Intl_Pair *pair;
	const char *key;
	const char *trans;

	pair = next->data;
	key = pair->locale_key;
	trans = _(pair->locale_translation);
	if (pair->locale_icon)
	  {
	     e_prefix_data_snprintf(buf, sizeof(buf), "data/images/%s", pair->locale_icon);
	     ic = e_util_icon_add(buf, evas);
	  }
        else
	  ic = NULL;
	e_widget_ilist_append(cfdata->gui.blang_list, ic, trans, NULL, NULL, key);
	if ((cur_sig_loc) && 
            (!strncmp(key, cur_sig_loc, strlen(cur_sig_loc))))
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
   e_widget_size_min_set(cfdata->gui.locale_entry, 100, 25);
   e_widget_frametable_object_append(of, cfdata->gui.locale_entry, 
				     1, 0, 1, 1, 1, 1, 1, 0);
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
   if (!cfdata->lang_list)
     eina_hash_foreach(cfdata->locale_hash, _lang_hash_cb, cfdata);

   if (cfdata->lang_list)
     {
	cfdata->lang_list =
          eina_list_sort(cfdata->lang_list, eina_list_count(cfdata->lang_list),
                         _lang_list_sort);
	_lang_list_load(cfdata);
     }

   e_widget_ilist_go(ob);
   e_widget_size_min_set(ob, 140, 200);
   e_widget_framelist_object_append(of, ob);
   e_widget_ilist_selected_set(ob, e_widget_ilist_selected_get(ob));

   /* Region List */
   ob = e_widget_ilist_add(evas, 0, 0, &(cfdata->cur_reg));
   cfdata->gui.reg_list = ob;

   e_widget_ilist_go(ob);
   e_widget_size_min_set(ob, 100, 100);
   e_widget_framelist_object_append(of, ob);
   e_widget_ilist_selected_set(ob, e_widget_ilist_selected_get(ob));

   /* Codeset List */
   ob = e_widget_ilist_add(evas, 0, 0, &(cfdata->cur_cs));
   cfdata->gui.cs_list = ob;

   e_widget_ilist_go(ob);
   e_widget_size_min_set(ob, 100, 100);
   e_widget_framelist_object_append(of, ob);
   
   /* Modified List */
   ob = e_widget_ilist_add(evas, 0, 0, &(cfdata->cur_mod));
   cfdata->gui.mod_list = ob;

   e_widget_ilist_go(ob);
   e_widget_size_min_set(ob, 100, 100);
   e_widget_framelist_object_append(of, ob);

   e_widget_table_object_append(o, of, 0, 0, 1, 1, 1, 1, 1, 1);
   
   /* Locale selector */
   of = e_widget_frametable_add(evas, _("Locale Selected"), 0);
   ob = e_widget_label_add(evas, _("Locale"));
   e_widget_frametable_object_append(of, ob, 0, 0, 1, 1, 1, 0, 1, 0);
   ob = e_widget_entry_add(evas, &(cfdata->cur_language), NULL, NULL, NULL);
   cfdata->gui.locale_entry = ob;
   e_widget_disabled_set(cfdata->gui.locale_entry, 1);
   e_widget_size_min_set(cfdata->gui.locale_entry, 100, 25);
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
_ilist_basic_language_cb_change(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   e_widget_entry_text_set(cfdata->gui.locale_entry, cfdata->cur_blang);
}

static void
_ilist_language_cb_change(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   _cfdata_language_go(cfdata->cur_lang, NULL, NULL, NULL, cfdata);
   e_widget_entry_text_set(cfdata->gui.locale_entry, cfdata->cur_lang);
   eina_stringshare_del(cfdata->cur_cs);
   eina_stringshare_del(cfdata->cur_mod);
}

static void
_ilist_region_cb_change(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data * cfdata;
   char locale[32];
    
   cfdata = data;
    
   _cfdata_language_go(cfdata->cur_lang, cfdata->cur_reg, NULL, NULL, cfdata);
   
   if ((cfdata->cur_lang) && (cfdata->cur_lang[0]))
     {
        sprintf(locale, "%s_%s", cfdata->cur_lang, cfdata->cur_reg);
        e_widget_entry_text_set(cfdata->gui.locale_entry, locale);
     }
   else
      e_widget_entry_text_set(cfdata->gui.locale_entry, "");
   eina_stringshare_del(cfdata->cur_cs);
   eina_stringshare_del(cfdata->cur_mod);
}

static void 
_ilist_codeset_cb_change(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data * cfdata;
   char locale[32];
   
   cfdata = data;
   
   if ((cfdata->cur_lang) && (cfdata->cur_lang[0]))
     {
        if (cfdata->cur_mod)
           sprintf(locale, "%s_%s.%s@%s", cfdata->cur_lang, cfdata->cur_reg, cfdata->cur_cs, cfdata->cur_mod);
        else
           sprintf(locale, "%s_%s.%s", cfdata->cur_lang, cfdata->cur_reg, cfdata->cur_cs);
        e_widget_entry_text_set(cfdata->gui.locale_entry, locale);
     }
   else
      e_widget_entry_text_set(cfdata->gui.locale_entry, "");
}

static void 
_ilist_modifier_cb_change(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data * cfdata;
   char locale[32];
   
   cfdata = data;
   
   if ((cfdata->cur_lang) && (cfdata->cur_lang[0]))
     {
        if (cfdata->cur_cs)
           sprintf(locale, "%s_%s.%s@%s", cfdata->cur_lang, cfdata->cur_reg, cfdata->cur_cs, cfdata->cur_mod);
        else
           sprintf(locale, "%s_%s@%s", cfdata->cur_lang, cfdata->cur_reg, cfdata->cur_mod);
        e_widget_entry_text_set(cfdata->gui.locale_entry, locale);
     }
   else
      e_widget_entry_text_set(cfdata->gui.locale_entry, "");
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

   if ((!lang) || (!lang[0]))
     {
        e_widget_ilist_clear(cfdata->gui.reg_list);
	e_widget_ilist_clear(cfdata->gui.cs_list);
	e_widget_ilist_clear(cfdata->gui.mod_list);
     }
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
		  if (!cs_trans)
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
   Evas_Object *ic;
   char buf[PATH_MAX];
   
   if (!data) return;

   cfdata = data;
   if (!cfdata->lang_list) return;

   evas_event_freeze(evas_object_evas_get(cfdata->gui.lang_list));
   edje_freeze();
   e_widget_ilist_freeze(cfdata->gui.lang_list);
   
   e_prefix_data_snprintf(buf, sizeof(buf), "data/images/%s", "lang-system.png");
   ic = e_util_icon_add(buf, cfdata->evas);
   e_widget_ilist_append(cfdata->gui.lang_list, ic, _("System Default"), NULL, NULL, "");
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
	     ic = e_icon_add(cfdata->evas);
	     e_util_icon_theme_set(ic, "dialog-ok-apply");
	  }
	else
	  ic = NULL;

	e_widget_ilist_append(cfdata->gui.lang_list, ic, trans, NULL, NULL, ln->lang_code);

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
