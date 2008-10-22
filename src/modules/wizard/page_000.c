/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

const char *intlfont = NULL;
/* A list of fonts to choose as the default, in order of preference. This list
 * can and probably will change over time with fine-tuning
 */
const char *preferred_fonts[] =
{
     "Sans",
     "DejaVu Sans",
     "Bitstream Vera Sans",
     "Arial",
     "Nice",
     "Verdana",
     "Lucida Sans"
};
/* negative numbers == keep theme set size but multiple by negative value
 * multiplied by -100 (so scale, 100 = 1:1 scaling)
 */
const int fontsize = -100;

EAPI int
wizard_page_init(E_Wizard_Page *pg)
{
   return 1;
}
EAPI int
wizard_page_shutdown(E_Wizard_Page *pg)
{
   return 1;
}
EAPI int
wizard_page_show(E_Wizard_Page *pg)
{
   Eina_List *fonts;
   Evas_Hash *fonts_hash;
   int i;
   
   fonts = evas_font_available_list(pg->evas);
   fonts_hash = e_font_available_list_parse(fonts);

   for (i = 0; i < (sizeof(preferred_fonts) / sizeof(char *)); i++)
     {
	E_Font_Properties *efp;
	
	efp =  evas_hash_find(fonts_hash, preferred_fonts[i]);
	printf("WIZ: page_000: FONT \"%s\" exists=", preferred_fonts[i]);
	if (efp) printf("yes\n");
	else printf("no\n");
	if ((!intlfont) && (efp))
	  intlfont = preferred_fonts[i];
     }
   
   if (!intlfont) printf("WIZ: page_000: No intl font found\n");
   else printf("WIZ: page_000: Chose \"%s\"\n", intlfont);

   if (intlfont)
     {
	const char *classes[] =
	  {
	       "title_bar",
	       "menu_item",
	       "menu_title",
	       "tb_plain",
	       "tb_light",
	       "tb_big",
	       "move_text",
	       "resize_text",
	       "winlist_title",
	       "configure",
	       "about_title",
	       "about_version",
	       "button_text",
	       "desklock_title",
	       "desklock_passwd",
	       "dialog_error",
	       "exebuf_command",
	       "init_title",
	       "init_text",
	       "init_version",    
	       "entry",
	       "frame",
	       "label",
	       "button",
	       "slider",
	       "radio_button",
	       "check_button",
	       "tlist",
	       "ilist_item",
	       "ilist_header", 
	       "fileman_typebuf",
	       "fileman_icon",
	       "module_small",
	       "module_normal",
	       "module_large",
	       "module_small_s",
	       "module_normal_s",
	       "module_large_s",
	       "wizard_title",
	       "wizard_button"
	       /* FIXME: this list needs to be extended as new text classes
		* appear - maybe we need to put the list of textclasses into
		* core E
		*/
	  };
	
	for (i = 0; i < (sizeof(classes) / sizeof(char *)); i++)
	  e_font_default_set(classes[i], intlfont, fontsize);

	e_font_apply();
     }
   
   e_font_available_hash_free(fonts_hash);
   evas_font_available_list_free(pg->evas, fonts);
   return 0; /* 1 == show ui, and wait for user, 0 == just continue */
}
EAPI int
wizard_page_hide(E_Wizard_Page *pg)
{
   return 1;
}
EAPI int
wizard_page_apply(E_Wizard_Page *pg)
{
   return 1;
}
