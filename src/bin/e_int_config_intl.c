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
static int  _lang_list_sort                 (void *data1, void *data2);
static void _lang_list_load                 (void *data);
static int  _region_list_sort               (void *data1, void *data2);
static void _region_list_load               (void *data);
static int  _basic_lang_list_sort           (void *data1, void *data2);

/* Fill the clear lists, fill with language, select */
/* Update lanague */
static void      _cfdata_language_go        (const char *lang, const char *region, const char *codeset, const char *modifier, E_Config_Dialog_Data *cfdata);
static Evas_Bool _lang_hash_cb              (Evas_Hash *hash, const char *key, void *data, void *fdata);
static Evas_Bool _region_hash_cb            (Evas_Hash *hash, const char *key, void *data, void *fdata);
static Evas_Bool _language_hash_free_cb     (Evas_Hash *hash, const char *key, void *data, void *fdata);
static Evas_Bool _region_hash_free_cb       (Evas_Hash *hash, const char *key, void *data, void *fdata);
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
   Evas_Hash  *region_hash;	        /* US->utf8 */
};

struct _E_Intl_Region_Node
{
   const char *region_code;		/* US */
   const char *region_name;		/* United States */
   Evas_List  *available_codesets;
   Evas_List  *available_modifiers;
};

struct _E_Config_Dialog_Data
{
   E_Config_Dialog *cfd;
   Evas *evas;
   
   /* Current data */
   char	*cur_language;

   char *cur_blang;
   
   char *cur_lang;
   char *cur_reg;
   char *cur_cs;
   char *cur_mod;

   int  lang_dirty;
   
   Evas_Hash *locale_hash;
   Evas_List *lang_list;
   Evas_List *region_list;
   Evas_List *blang_list;
   
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
     {"bg_BG.UTF-8", N_("Bulgarian")},
     {"zh_CN.UTF-8", N_("Chinese (Simplified)")},
     {"zh_TW.UTF-8", N_("Chinese (Traditional)")},
     {"cs_CZ.UTF-8", N_("Czech")},
     {"da_DK.UTF-8", N_("Danish")},
     {"nl_NL.UTF-8", N_("Dutch")},
     {"en_US.UTF-8", N_("English")},
     {"fi_FI.UTF-8", N_("Finnish")},
     {"fr_FR.UTF-8", N_("French")},
     {"de_DE.UTF-8", N_("German")},
     {"hu_HU.UTF-8", N_("Hungarian")},
     {"it_IT.UTF-8", N_("Italian")},
     {"ja_JP.UTF-8", N_("Japanese")},
     {"ko_KR.UTF-8", N_("Korean")},
     {"nb_NO.UTF-8", N_("Norwegian Bokmål")},
     {"pl_PL.UTF-8", N_("Polish")},
     {"pt_BR.UTF-8", N_("Portuguese")},
     {"ru_RU.UTF-8", N_("Russian")},
     {"sk_SK.UTF-8", N_("Slovak")},
     {"sl_SI.UTF-8", N_("Slovenian")},
     {"es_AR.UTF-8", N_("Spanish")},
     {"sv_SE.UTF-8", N_("Swedish")},
     { NULL, NULL }
};

const E_Intl_Pair language_predefined_pairs[ ] = {
       {"aa", N_("Afar")},
       {"af", N_("Afrikaans")},
       {"ak", N_("Akan")},
       {"am", N_("Amharic")},
       {"an", N_("Aragonese")},
       {"ar", N_("Arabic")},
       {"as", N_("Assamese")},
       {"az", N_("Azerbaijani")},
       {"be", N_("Belarusian")},
       {"bg", N_("Bulgarian")},
       {"bn", N_("Bengali")},
       {"br", N_("Breton")},
       {"bs", N_("Bosnian")},
       {"byn", N_("Blin")},
       {"ca", N_("Catalan")},
       {"cch", N_("Atsam")},
       {"cs", N_("Czech")},
       {"cy", N_("Welsh")},
       {"da", N_("Danish")},
       {"de", N_("German")},
       {"dv", N_("Divehi")},
       {"dz", N_("Dzongkha")},
       {"ee", N_("Ewe")},
       {"el", N_("Greek")},
       {"en", N_("English")},
       {"eo", N_("Esperanto")},
       {"es", N_("Spanish")},
       {"et", N_("Estonian")},
       {"eu", N_("Basque")},
       {"fa", N_("Persian")},
       {"fi", N_("Finnish")},
       {"fo", N_("Faroese")},
       {"fr", N_("French")},
       {"fur", N_("Friulian")},
       {"ga", N_("Irish")},
       {"gaa", N_("Ga")},
       {"gez", N_("Geez")},
       {"gl", N_("Galician")},
       {"gu", N_("Gujarati")},
       {"gv", N_("Manx")},
       {"ha", N_("Hausa")},
       {"haw", N_("Hawaiian")},
       {"he", N_("Hebrew")},
       {"hi", N_("Hindi")},
       {"hr", N_("Croatian")},
       {"hu", N_("Hungarian")},
       {"hy", N_("Armenian")},
       {"ia", N_("Interlingua")},
       {"id", N_("Indonesian")},
       {"ig", N_("Igbo")},
       {"is", N_("Icelandic")},
       {"it", N_("Italian")},
       {"iu", N_("Inuktitut")},
       {"iw", N_("Hebrew")},
       {"ja", N_("Japanese")},
       {"ka", N_("Georgian")},
       {"kaj", N_("Jju")},
       {"kam", N_("Kamba")},
       {"kcg", N_("Tyap")},
       {"kfo", N_("Koro")},
       {"kk", N_("Kazakh")},
       {"kl", N_("Kalaallisut")},
       {"km", N_("Khmer")},
       {"kn", N_("Kannada")},
       {"ko", N_("Korean")},
       {"kok", N_("Konkani")},
       {"ku", N_("Kurdish")},
       {"kw", N_("Cornish")},
       {"ky", N_("Kirghiz")},
       {"ln", N_("Lingala")},
       {"lo", N_("Lao")},
       {"lt", N_("Lithuanian")},
       {"lv", N_("Latvian")},
       {"mi", N_("Maori")},
       {"mk", N_("Macedonian")},
       {"ml", N_("Malayalam")},
       {"mn", N_("Mongolian")},
       {"mr", N_("Marathi")},
       {"ms", N_("Malay")},
       {"mt", N_("Maltese")},
       {"nb", N_("Norwegian Bokmål")},
       {"ne", N_("Nepali")},
       {"nl", N_("Dutch")},
       {"nn", N_("Norwegian Nynorsk")},
       {"no", N_("Norwegian")},
       {"nr", N_("South Ndebele")},
       {"nso", N_("Northern Sotho")},
       {"ny", N_("Nyanja; Chichewa; Chewa")},
       {"oc", N_("Occitan")},
       {"om", N_("Oromo")},
       {"or", N_("Oriya")},
       {"pa", N_("Punjabi")},
       {"pl", N_("Polish")},
       {"ps", N_("Pashto")},
       {"pt", N_("Portuguese")},
       {"ro", N_("Romanian")},
       {"ru", N_("Russian")},
       {"rw", N_("Kinyarwanda")},
       {"sa", N_("Sanskrit")},
       {"se", N_("Northern Sami")},
       {"sh", N_("Serbo-Croatian")},
       {"sid", N_("Sidamo")},
       {"sk", N_("Slovak")},
       {"sl", N_("Slovenian")},
       {"so", N_("Somali")},
       {"sq", N_("Albanian")},
       {"sr", N_("Serbian")},
       {"ss", N_("Swati")},
       {"st", N_("Southern Sotho")},
       {"sv", N_("Swedish")},
       {"sw", N_("Swahili")},
       {"syr", N_("Syriac")},
       {"ta", N_("Tamil")},
       {"te", N_("Telugu")},
       {"tg", N_("Tajik")},
       {"th", N_("Thai")},
       {"ti", N_("Tigrinya")},
       {"tig", N_("Tigre")},
       {"tl", N_("Tagalog")},
       {"tn", N_("Tswana")},
       {"tr", N_("Turkish")},
       {"ts", N_("Tsonga")},
       {"tt", N_("Tatar")},
       {"uk", N_("Ukrainian")},
       {"ur", N_("Urdu")},
       {"uz", N_("Uzbek")},
       {"ve", N_("Venda")},
       {"vi", N_("Vietnamese")},
       {"wa", N_("Walloon")},
       {"wal", N_("Walamo")},
       {"xh", N_("Xhosa")},
       {"yi", N_("Yiddish")},
       {"yo", N_("Yoruba")},
       {"zh", N_("Chinese")},
       {"zu", N_("Zulu")},
       { NULL, NULL}
};

const E_Intl_Pair region_predefined_pairs[ ] = {
       { "AF", N_("Afghanistan")},
       { "AX", N_("Åland Islands")},
       { "AL", N_("Albania")},
       { "DZ", N_("Algeria")},
       { "AS", N_("American Samoa")},
       { "AD", N_("Andorra")},
       { "AO", N_("Angola")},
       { "AI", N_("Anguilla")},
       { "AQ", N_("Antarctica")},
       { "AG", N_("Antigua and Barbuda")},
       { "AR", N_("Argentina")},
       { "AM", N_("Armenia")},
       { "AW", N_("Aruba")},
       { "AU", N_("Australia")},
       { "AT", N_("Austria")},
       { "AZ", N_("Azerbaijan")},
       { "BS", N_("Bahamas")},
       { "BH", N_("Bahrain")},
       { "BD", N_("Bangladesh")},
       { "BB", N_("Barbados")},
       { "BY", N_("Belarus")},
       { "BE", N_("Belgium")},
       { "BZ", N_("Belize")},
       { "BJ", N_("Benin")},
       { "BM", N_("Bermuda")},
       { "BT", N_("Bhutan")},
       { "BO", N_("Bolivia")},
       { "BA", N_("Bosnia and Herzegovina")},
       { "BW", N_("Botswana")},
       { "BV", N_("Bouvet Island")},
       { "BR", N_("Brazil")},
       { "IO", N_("British Indian Ocean Territory")},
       { "BN", N_("Brunei Darussalam")},
       { "BG", N_("Bulgaria")},
       { "BF", N_("Burkina Faso")},
       { "BI", N_("Burundi")},
       { "KH", N_("Cambodia")},
       { "CM", N_("Cameroon")},
       { "CA", N_("Canada")},
       { "CV", N_("Cape Verde")},
       { "KY", N_("Cayman Islands")},
       { "CF", N_("Central African Republic")},
       { "TD", N_("Chad")},
       { "CL", N_("Chile")},
       { "CN", N_("China")},
       { "CX", N_("Christmas Island")},
       { "CC", N_("Cocos (keeling) Islands")},
       { "CO", N_("Colombia")},
       { "KM", N_("Comoros")},
       { "CG", N_("Congo")},
       { "CD", N_("Congo")},
       { "CK", N_("Cook Islands")},
       { "CR", N_("Costa Rica")},
       { "CI", N_("Cote D'ivoire")},
       { "HR", N_("Croatia")},
       { "CU", N_("Cuba")},
       { "CY", N_("Cyprus")},
       { "CZ", N_("Czech Republic")},
       { "DK", N_("Denmark")},
       { "DJ", N_("Djibouti")},
       { "DM", N_("Dominica")},
       { "DO", N_("Dominican Republic")},
       { "EC", N_("Ecuador")},
       { "EG", N_("Egypt")},
       { "SV", N_("El Salvador")},
       { "GQ", N_("Equatorial Guinea")},
       { "ER", N_("Eritrea")},
       { "EE", N_("Estonia")},
       { "ET", N_("Ethiopia")},
       { "FK", N_("Falkland Islands (malvinas)")},
       { "FO", N_("Faroe Islands")},
       { "FJ", N_("Fiji")},
       { "FI", N_("Finland")},
       { "FR", N_("France")},
       { "GF", N_("French Guiana")},
       { "PF", N_("French Polynesia")},
       { "TF", N_("French Southern Territories")},
       { "GA", N_("Gabon")},
       { "GM", N_("Gambia")},
       { "GE", N_("Georgia")},
       { "DE", N_("Germany")},
       { "GH", N_("Ghana")},
       { "GI", N_("Gibraltar")},
       { "GR", N_("Greece")},
       { "GL", N_("Greenland")},
       { "GD", N_("Grenada")},
       { "GP", N_("Guadeloupe")},
       { "GU", N_("Guam")},
       { "GT", N_("Guatemala")},
       { "GG", N_("Guernsey")},
       { "GN", N_("Guinea")},
       { "GW", N_("Guinea-bissau")},
       { "GY", N_("Guyana")},
       { "HT", N_("Haiti")},
       { "HM", N_("Heard Island and Mcdonald Islands")},
       { "VA", N_("Holy See (vatican City State)")},
       { "HN", N_("Honduras")},
       { "HK", N_("Hong Kong")},
       { "HU", N_("Hungary")},
       { "IS", N_("Iceland")},
       { "IN", N_("India")},
       { "ID", N_("Indonesia")},
       { "IR", N_("Iran")},
       { "IQ", N_("Iraq")},
       { "IE", N_("Ireland")},
       { "IM", N_("Isle Of Man")},
       { "IL", N_("Israel")},
       { "IT", N_("Italy")},
       { "JM", N_("Jamaica")},
       { "JP", N_("Japan")},
       { "JE", N_("Jersey")},
       { "JO", N_("Jordan")},
       { "KZ", N_("Kazakhstan")},
       { "KE", N_("Kenya")},
       { "KI", N_("Kiribati")},
       { "KP", N_("Korea")},
       { "KR", N_("Korea")},
       { "KW", N_("Kuwait")},
       { "KG", N_("Kyrgyzstan")},
       { "LA", N_("Lao People's Democratic Republic")},
       { "LV", N_("Latvia")},
       { "LB", N_("Lebanon")},
       { "LS", N_("Lesotho")},
       { "LR", N_("Liberia")},
       { "LY", N_("Libyan Arab Jamahiriya")},
       { "LI", N_("Liechtenstein")},
       { "LT", N_("Lithuania")},
       { "LU", N_("Luxembourg")},
       { "MO", N_("Macao")},
       { "MK", N_("Macedonia")},
       { "MG", N_("Madagascar")},
       { "MW", N_("Malawi")},
       { "MY", N_("Malaysia")},
       { "MV", N_("Maldives")},
       { "ML", N_("Mali")},
       { "MT", N_("Malta")},
       { "MH", N_("Marshall Islands")},
       { "MQ", N_("Martinique")},
       { "MR", N_("Mauritania")},
       { "MU", N_("Mauritius")},
       { "YT", N_("Mayotte")},
       { "MX", N_("Mexico")},
       { "FM", N_("Micronesia")},
       { "MD", N_("Moldova")},
       { "MC", N_("Monaco")},
       { "MN", N_("Mongolia")},
       { "MS", N_("Montserrat")},
       { "MA", N_("Morocco")},
       { "MZ", N_("Mozambique")},
       { "MM", N_("Myanmar")},
       { "NA", N_("Namibia")},
       { "NR", N_("Nauru")},
       { "NP", N_("Nepal")},
       { "NL", N_("Netherlands")},
       { "AN", N_("Netherlands Antilles")},
       { "NC", N_("New Caledonia")},
       { "NZ", N_("New Zealand")},
       { "NI", N_("Nicaragua")},
       { "NE", N_("Niger")},
       { "NG", N_("Nigeria")},
       { "NU", N_("Niue")},
       { "NF", N_("Norfolk Island")},
       { "MP", N_("Northern Mariana Islands")},
       { "NO", N_("Norway")},
       { "OM", N_("Oman")},
       { "PK", N_("Pakistan")},
       { "PW", N_("Palau")},
       { "PS", N_("Palestinian Territory")},
       { "PA", N_("Panama")},
       { "PG", N_("Papua New Guinea")},
       { "PY", N_("Paraguay")},
       { "PE", N_("Peru")},
       { "PH", N_("Philippines")},
       { "PN", N_("Pitcairn")},
       { "PL", N_("Poland")},
       { "PT", N_("Portugal")},
       { "PR", N_("Puerto Rico")},
       { "QA", N_("Qatar")},
       { "RE", N_("Reunion")},
       { "RO", N_("Romania")},
       { "RU", N_("Russian Federation")},
       { "RW", N_("Rwanda")},
       { "SH", N_("Saint Helena")},
       { "KN", N_("Saint Kitts and Nevis")},
       { "LC", N_("Saint Lucia")},
       { "PM", N_("Saint Pierre and Miquelon")},
       { "VC", N_("Saint Vincent and the Grenadines")},
       { "WS", N_("Samoa")},
       { "SM", N_("San Marino")},
       { "ST", N_("Sao Tome and Principe")},
       { "SA", N_("Saudi Arabia")},
       { "SN", N_("Senegal")},
       { "CS", N_("Serbia and Montenegro")},
       { "SC", N_("Seychelles")},
       { "SL", N_("Sierra Leone")},
       { "SG", N_("Singapore")},
       { "SK", N_("Slovakia")},
       { "SI", N_("Slovenia")},
       { "SB", N_("Solomon Islands")},
       { "SO", N_("Somalia")},
       { "ZA", N_("South Africa")},
       { "GS", N_("South Georgia and the South Sandwich Islands")},
       { "ES", N_("Spain")},
       { "LK", N_("Sri Lanka")},
       { "SD", N_("Sudan")},
       { "SR", N_("Suriname")},
       { "SJ", N_("Svalbard and Jan Mayen")},
       { "SZ", N_("Swaziland")},
       { "SE", N_("Sweden")},
       { "CH", N_("Switzerland")},
       { "SY", N_("Syrian Arab Republic")},
       { "TW", N_("Taiwan")},
       { "TJ", N_("Tajikistan")},
       { "TZ", N_("Tanzania")},
       { "TH", N_("Thailand")},
       { "TL", N_("Timor-leste")},
       { "TG", N_("Togo")},
       { "TK", N_("Tokelau")},
       { "TO", N_("Tonga")},
       { "TT", N_("Trinidad and Tobago")},
       { "TN", N_("Tunisia")},
       { "TR", N_("Turkey")},
       { "TM", N_("Turkmenistan")},
       { "TC", N_("Turks and Caicos Islands")},
       { "TV", N_("Tuvalu")},
       { "UG", N_("Uganda")},
       { "UA", N_("Ukraine")},
       { "AE", N_("United Arab Emirates")},
       { "GB", N_("United Kingdom")},
       { "US", N_("United States")},
       { "UM", N_("United States Minor Outlying Islands")},
       { "UY", N_("Uruguay")},
       { "UZ", N_("Uzbekistan")},
       { "VU", N_("Vanuatu")},
       { "VE", N_("Venezuela")},
       { "VN", N_("Viet Nam")},
       { "VG", N_("Virgin Islands")},
       { "VI", N_("Virgin Islands")},
       { "WF", N_("Wallis and Futuna")},
       { "EH", N_("Western Sahara")},
       { "YE", N_("Yemen")},
       { "ZM", N_("Zambia")},
       { "ZW", N_("Zimbabwe")},
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
e_int_config_intl(E_Container *con)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "_config_intl_dialog")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);
   
   v->create_cfdata           = _create_data;
   v->free_cfdata             = _free_data;
   v->advanced.create_widgets    = _advanced_create_widgets;
   v->advanced.apply_cfdata      = _advanced_apply_data;
   v->basic.create_widgets    = _basic_create_widgets;
   v->basic.apply_cfdata      = _basic_apply_data;
   
   cfd = e_config_dialog_new(con,
			     _("Language Configuration"),
			    "E", "_config_intl_dialog",
			     "enlightenment/intl", 0, v, NULL);
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
   Evas_List    *e_lang_list;
   FILE		*output;
   
   e_lang_list = e_intl_language_list();
   
   /* Get list of all locales and start making map */
   output = popen("locale -a", "r");
   if ( output ) 
     {
	char line[32];
	while (fscanf(output, "%[^\n]\n", line) == 1)
	  {
	     char *language;
	     char *basic_language;

	     basic_language = e_intl_locale_canonic_get(line, E_INTL_LOC_LANG | E_INTL_LOC_REGION);
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
			    if (!evas_list_find(cfdata->blang_list, &basic_language_predefined_pairs[i]))
			       cfdata->blang_list = evas_list_append(cfdata->blang_list, &basic_language_predefined_pairs[i]);
			    break;
			 }
		       i++;
		    }
	       }
	     E_FREE(basic_language);
	     
	     language = e_intl_locale_canonic_get(line, E_INTL_LOC_LANG);
	     
	     /* If the language is a valid ll_RR[.enc[@mod]] locale add it to the hash */
	     if (language) 
	       {
		  E_Intl_Language_Node *lang_node;
		  E_Intl_Region_Node  *region_node;
		  char *region;
		  char *codeset;
		  char *modifier;

		  /* Separate out ll RR enc and mod parts */
		  region = e_intl_locale_canonic_get(line, E_INTL_LOC_REGION);
		  codeset = e_intl_locale_canonic_get(line, E_INTL_LOC_CODESET);
		  modifier = e_intl_locale_canonic_get(line, E_INTL_LOC_MODIFIER);
   
		  /* Add the language to the new locale properties to the hash */
		  /* First check if the LANGUAGE exists in there already */
		  lang_node  = evas_hash_find(cfdata->locale_hash, language);
		  if (lang_node == NULL) 
		    {
		       Evas_List *next;
		       int i;
		       
		       /* create new node */
		       lang_node = E_NEW(E_Intl_Language_Node, 1);

		       lang_node->lang_code = evas_stringshare_add(language);

		       /* Check if the language list exists */
		       /* Linear Search */
		       for (next = e_lang_list; next; next = next->next) 
			 {
			    char *e_lang;

			    e_lang = next->data;
			    if (!strncmp(e_lang, language, 2) || !strcmp("en", language)) 
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
			    if (!strcmp(language_predefined_pairs[i].locale_key, language))
			      {
				 lang_node->lang_name = _(language_predefined_pairs[i].locale_translation);
				 break;
			      }
			    i++;
			 }
		       
		       cfdata->locale_hash = evas_hash_add(cfdata->locale_hash, language, lang_node);
		    }

		  /* We now have the current language hash node, lets see if there is
		     region data that needs to be added. 
		   */
		  if (region)
		    {
		       region_node = evas_hash_find(lang_node->region_hash, region);

		       if (region_node == NULL)
			 {
			    int i;
			    
			    /* create new node */
			    region_node = E_NEW(E_Intl_Region_Node, 1);
			    region_node->region_code = evas_stringshare_add(region);
		       
			    /* Get the region translation */
			    /* Linear Search */
			    i = 0;
			    while (region_predefined_pairs[i].locale_key)
			      {
				 if (!strcmp(region_predefined_pairs[i].locale_key, region))
				   {
				      region_node->region_name = _(region_predefined_pairs[i].locale_translation);
				      break;
				   }
				 i++;
			      }
			    lang_node->region_hash = evas_hash_add(lang_node->region_hash, region, region_node);
			 }

		       /* We now have the current region hash node */
		       /* Add codeset to the region hash node if it exists */
		       if (codeset)
			 {
			    const char * cs;
			    const char * cs_trans;
			    
			    cs_trans = _intl_charset_upper_get(codeset);
			    if (cs_trans == NULL) 
			      cs = evas_stringshare_add(codeset);
			    else 
			      cs = evas_stringshare_add(cs_trans);
			    /* Exclusive */
			    /* Linear Search */
			    if (!evas_list_find(region_node->available_codesets, cs))
			      region_node->available_codesets = evas_list_append(region_node->available_codesets, cs);
			 }

		       /* Add modifier to the region hash node if it exists */
		       if (modifier)
			 {
			    const char * mod;
			    
			    /* Find only works here because we are using stringshare*/
			    mod = evas_stringshare_add(modifier);
			    /* Exclusive */
			    /* Linear Search */
			    if (!evas_list_find(region_node->available_modifiers, mod))
			      region_node->available_modifiers = evas_list_append(region_node->available_modifiers, mod);
			 }
		       free(region);
		    }
		  
		  E_FREE(codeset);
		  E_FREE(modifier);
		  free(language);
	       }
	  }
	
	/* Sort basic languages */	
	cfdata->blang_list = evas_list_sort(cfdata->blang_list, 
	      evas_list_count(cfdata->blang_list), 
	      _basic_lang_list_sort);

        while (e_lang_list)
	  {
	     free(e_lang_list->data);
	     e_lang_list = evas_list_remove_list(e_lang_list, e_lang_list);
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
   E_FREE(cfdata->cur_blang);
   E_FREE(cfdata->cur_lang);
   E_FREE(cfdata->cur_reg);
   E_FREE(cfdata->cur_cs);
   E_FREE(cfdata->cur_mod);

   evas_hash_foreach(cfdata->locale_hash, _language_hash_free_cb, NULL);
   evas_hash_free(cfdata->locale_hash);
   
   cfdata->lang_list = evas_list_free(cfdata->lang_list);
   cfdata->region_list = evas_list_free(cfdata->region_list);
   cfdata->blang_list = evas_list_free(cfdata->blang_list);
   
   free(cfdata);
}

static Evas_Bool 
_language_hash_free_cb(Evas_Hash *hash, const char *key, void *data, void *fdata)
{
   E_Intl_Language_Node *node;

   node = data;   
   if (node->lang_code) evas_stringshare_del(node->lang_code);
   evas_hash_foreach(node->region_hash, _region_hash_free_cb, NULL);
   evas_hash_free(node->region_hash);
   free(node); 
   
   return 1;
}

static Evas_Bool 
_region_hash_free_cb(Evas_Hash *hash, const char *key, void *data, void *fdata)
{ 
   E_Intl_Region_Node *node;

   node = data;   
   if (node->region_code) evas_stringshare_del(node->region_code);
   while (node->available_codesets) 
     {
	const char *str;

	str = node->available_codesets->data;
	if (str) evas_stringshare_del(str);
	node->available_codesets = evas_list_remove_list(node->available_codesets, node->available_codesets);
     }
    
   while (node->available_modifiers) 
     {
	const char *str;

	str = node->available_modifiers->data;
	if (str) evas_stringshare_del(str);
	node->available_modifiers = evas_list_remove_list(node->available_modifiers, node->available_modifiers);
     }
   
   free(node);  
   return 1;
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{	 
   if (cfdata->cur_language)
     {
	e_config->language = evas_stringshare_add(cfdata->cur_language);
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
	e_config->language = evas_stringshare_add(cfdata->cur_language);
	e_intl_language_set(e_config->language);
     }
   
   e_config_save_queue();
   
   return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ob, *ot;
   char *cur_sig_loc;
   Evas_List *next;
   int i;
   
   cfdata->evas = evas;
   
   o = e_widget_list_add(evas, 0, 0);
  
   of = e_widget_frametable_add(evas, _("Language Selector"), 1);
  
   /* Language List */ 
   ob = e_widget_ilist_add(evas, 16, 16, &(cfdata->cur_blang));
   e_widget_min_size_set(ob, 175, 175);
   e_widget_on_change_hook_set(ob, _ilist_basic_language_cb_change, cfdata);
   cfdata->gui.blang_list = ob;

   if (cfdata->cur_language)
     {
	cur_sig_loc = e_intl_locale_canonic_get(cfdata->cur_language, 
	      E_INTL_LOC_LANG | E_INTL_LOC_REGION);
     }
   else
     cur_sig_loc = NULL;

   i = 0;  
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
   e_widget_frametable_object_append(of, ob, 0, 0, 2, 6, 1, 1, 1, 1);
    
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   /* Locale selector */
   ot = e_widget_table_add(evas, 0);
   of = e_widget_framelist_add(evas, _("Locale Selected"), 0);
  
   ob = e_widget_label_add(evas, _("Locale"));
   e_widget_table_object_append(ot, ob, 
				0, 0, 1, 1,
				1, 1, 1, 1);
   
   cfdata->gui.locale_entry = e_widget_entry_add(evas, &(cfdata->cur_language));
   e_widget_disabled_set(cfdata->gui.locale_entry, 1);
   e_widget_min_size_set(cfdata->gui.locale_entry, 100, 25);
   e_widget_table_object_append(ot, cfdata->gui.locale_entry, 
				0, 1, 1, 1, 
				1, 1, 1, 1);
   e_widget_framelist_object_append(of, ot);
   e_widget_framelist_content_align_set(of, 0.0, 0.0);
   
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   e_dialog_resizable_set(cfd->dia, 1);
   return o;

}
   
static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ob, *ot;
   const char *lang, *reg, *cs, *mod;
   cfdata->evas = evas;
  
   _intl_current_locale_setup(cfdata);
   
   o = e_widget_list_add(evas, 0, 0);
  
   of = e_widget_frametable_add(evas, _("Language Selector"), 1);
  
   /* Language List */ 
   //ob = e_widget_label_add(evas, _("Language"));
   //e_widget_frametable_object_append(of, ob, 0, 0, 1, 1, 1, 1, 1, 1);
    
   ob = e_widget_ilist_add(evas, 16, 16, &(cfdata->cur_lang));
   cfdata->gui.lang_list = ob;

   /* If lang_list already loaded just use it */
   if (cfdata->lang_list == NULL) 
     {
	evas_hash_foreach(cfdata->locale_hash, _lang_hash_cb, cfdata);
	if (cfdata->lang_list) 
	  {
	     cfdata->lang_list = evas_list_sort(cfdata->lang_list, 
		   evas_list_count(cfdata->lang_list), 
		   _lang_list_sort);
	     
	     _lang_list_load(cfdata);
	  }
     }

   _lang_list_load(cfdata);
   
   e_widget_ilist_go(ob);
   e_widget_min_size_set(ob, 140, 200);
   e_widget_frametable_object_append(of, ob, 0, 0, 1, 4, 1, 1, 1, 1);
   
   /* Region List */
   //ob = e_widget_label_add(evas, _("Region"));
   //e_widget_frametable_object_append(of, ob, 1, 0, 1, 1, 1, 1, 1, 1); 

   ob = e_widget_ilist_add(evas, 0, 0, &(cfdata->cur_reg));
   cfdata->gui.reg_list = ob;
 
   e_widget_ilist_go(ob);
   e_widget_min_size_set(ob, 100, 100);
   e_widget_frametable_object_append(of, ob, 1, 0, 1, 4, 1, 1, 1, 1);
 
   /* Codeset List */
   //ob = e_widget_label_add(evas, _("Codeset"));
   //e_widget_frametable_object_append(of, ob, 2, 0, 1, 1, 1, 1, 1, 1); 

   ob = e_widget_ilist_add(evas, 0, 0, &(cfdata->cur_cs));
   cfdata->gui.cs_list = ob;

   e_widget_ilist_go(ob);
   e_widget_min_size_set(ob, 100, 100);
   e_widget_frametable_object_append(of, ob, 2, 0, 1, 4, 1, 1, 1, 1);
   
   /* Modified List */
   //ob = e_widget_label_add(evas, _("Modifier"));
   //e_widget_frametable_object_append(of, ob, 3, 0, 1, 1, 1, 1, 1, 1); 
   
   ob = e_widget_ilist_add(evas, 0, 0, &(cfdata->cur_mod));
   cfdata->gui.mod_list = ob;

   e_widget_ilist_go(ob);
   e_widget_min_size_set(ob, 100, 100);
   e_widget_frametable_object_append(of, ob, 3, 0, 1, 4, 1, 1, 1, 1);
    
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   /* Locale selector */
   ot = e_widget_table_add(evas, 0);
   of = e_widget_framelist_add(evas, _("Locale Selected"), 0);
  
   ob = e_widget_label_add(evas, _("Locale"));
   e_widget_table_object_append(ot, ob, 
				0, 0, 1, 1,
				1, 1, 1, 1);
   
   cfdata->gui.locale_entry = e_widget_entry_add(evas, &(cfdata->cur_language));
   e_widget_disabled_set(cfdata->gui.locale_entry, 1);
   e_widget_min_size_set(cfdata->gui.locale_entry, 100, 25);
   e_widget_table_object_append(ot, cfdata->gui.locale_entry, 
				0, 1, 1, 1, 
				1, 1, 1, 1);
   e_widget_framelist_object_append(of, ot);
   e_widget_framelist_content_align_set(of, 0.0, 0.0);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
  
   /* all these cur_* values are not guaranteed to be const so we need to
    * copy them. 
    */
   lang = NULL;
   reg = NULL;
   cs = NULL;
   mod = NULL;
   
   if (cfdata->cur_lang) lang = evas_stringshare_add(cfdata->cur_lang);
   if (cfdata->cur_reg) reg = evas_stringshare_add(cfdata->cur_reg);
   if (cfdata->cur_cs) cs = evas_stringshare_add(cfdata->cur_cs);
   if (cfdata->cur_mod) mod = evas_stringshare_add(cfdata->cur_mod);
   
   _cfdata_language_go(lang, reg, cs, mod, cfdata);
   
   if (lang) evas_stringshare_del(lang); 
   if (reg) evas_stringshare_del(reg); 
   if (cs) evas_stringshare_del(cs); 
   if (mod) evas_stringshare_del(mod); 
   
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
    E_FREE(cfdata->cur_cs);
    E_FREE(cfdata->cur_mod);
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
    E_FREE(cfdata->cur_cs);
    E_FREE(cfdata->cur_mod);
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
	lang_node = evas_hash_find(cfdata->locale_hash, lang);
	
	if (lang_node)
	  {
	     if (lang_update)
	       {
		  e_widget_ilist_clear(cfdata->gui.reg_list);
		   
		  cfdata->region_list = evas_list_free(cfdata->region_list);
		  evas_hash_foreach(lang_node->region_hash, _region_hash_cb, cfdata);
		  cfdata->region_list = evas_list_sort(cfdata->region_list, 
						     evas_list_count(cfdata->region_list), 
						     _region_list_sort);
		  _region_list_load(cfdata);
	       }
	     
	     if (region && region_update)
	       { 
		  E_Intl_Region_Node *reg_node;

		  reg_node = evas_hash_find(lang_node->region_hash, region);
		  if (reg_node)
		    {
		       Evas_List *next;
		       
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
	e_widget_ilist_go(cfdata->gui.reg_list);
     }
}

static Evas_Bool
_lang_hash_cb(Evas_Hash *hash, const char *key, void *data, void *fdata)
{
   E_Config_Dialog_Data *cfdata; 
   E_Intl_Language_Node *lang_node;
   
   cfdata = fdata;
   lang_node = data;
   
   cfdata->lang_list = evas_list_append(cfdata->lang_list, lang_node);
   
   return 1;
}

static Evas_Bool
_region_hash_cb(Evas_Hash *hash, const char *key, void *data, void *fdata)
{
   E_Config_Dialog_Data *cfdata; 
   E_Intl_Region_Node *reg_node;

   cfdata = fdata;
   reg_node = data;

   cfdata->region_list = evas_list_append(cfdata->region_list, reg_node);
   
   return 1;
}

void 
_intl_current_locale_setup(E_Config_Dialog_Data *cfdata)
{
   char *language;
   char *region;
   char *codeset;
   char *modifier;
   
   E_FREE(cfdata->cur_lang);
   E_FREE(cfdata->cur_reg);
   E_FREE(cfdata->cur_cs);
   E_FREE(cfdata->cur_mod);

   cfdata->cur_lang = NULL;
   cfdata->cur_reg = NULL;
   cfdata->cur_cs = NULL;
   cfdata->cur_mod = NULL;
 
   language = NULL;
   region = NULL;   
   codeset = NULL;
   modifier = NULL;
   
   if (cfdata->cur_language) 
     { 
	language = e_intl_locale_canonic_get(cfdata->cur_language, 
	      E_INTL_LOC_LANG);
	region = e_intl_locale_canonic_get(cfdata->cur_language, 
	      E_INTL_LOC_REGION);
     	codeset = e_intl_locale_canonic_get(cfdata->cur_language, 
	      E_INTL_LOC_CODESET);
     	modifier = e_intl_locale_canonic_get(cfdata->cur_language, 
     	      E_INTL_LOC_MODIFIER);
     }
   
   if (language) 
     cfdata->cur_lang = strdup(language);
   if (region) 
     cfdata->cur_reg = strdup(region);
   if (codeset) 
     {
	const char *cs_trans;
	
	cs_trans = _intl_charset_upper_get(codeset);
	if (cs_trans) cfdata->cur_cs = strdup(cs_trans);
     }
   if (modifier) 
     cfdata->cur_mod = strdup(modifier);
  
   E_FREE(language);
   E_FREE(region);
   E_FREE(codeset);
   E_FREE(modifier);

   cfdata->lang_dirty = 1;
}

static int 
_lang_list_sort(void *data1, void *data2) 
{
   E_Intl_Language_Node *ln1, *ln2;
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
   
   return (strcmp((const char *)trans1, (const char *)trans2));
}

static void 
_lang_list_load(void *data) 
{
   E_Config_Dialog_Data *cfdata;
   Evas_List *l;
   
   if (!data) return;

   cfdata = data;
   if (!cfdata->lang_list) return;
   
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
	
	     ic = edje_object_add(cfdata->evas);
	     e_util_edje_icon_set(ic, "enlightenment/e");
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
}

static int 
_region_list_sort(void *data1, void *data2) 
{
   E_Intl_Region_Node *rn1, *rn2;
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
   
   return (strcmp((const char *)trans1, (const char *)trans2));
}

static void 
_region_list_load(void *data) 
{
   E_Config_Dialog_Data *cfdata;
   Evas_List *l;
   
   if (!data) return;

   cfdata = data;
   if (!cfdata->region_list) return;

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
}

static int 
_basic_lang_list_sort(void *data1, void *data2) 
{
   E_Intl_Pair *ln1, *ln2;
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
   
   return (strcmp((const char *)trans1, (const char *)trans2));
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
