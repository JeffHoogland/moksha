#include "e.h"

#define BODHI_VERSION "5.1.0"
#define BODHI_ABOUT_TITLE "About Bodhi"
#define BODHI_ABOUT_TEXT "Bodhi Linux - Enlightened Linux Desktop"
#define BODHI_ABOUT_AUTHORS \
    "Jeff Hoogland<br>" \
    "Joris 'aeonius' van Dijk<br>" \
    "Jason 'Tristam' Thomas<br>" \
    "Kristi Hoogland<br>" \
    "Rbt 'ylee' Wiley<br>" \
    "Stefan 'the waiter' Uram<br>" \
    "Stace 'sef' Fauske<br>" \
    "Kai 'kuuko' Huuhko<br>" \
    "Stephen 'okra' Houston<br>" \
    "Patrick Duckson<br>" \
    "Roger 'JollyRoger' Carter<br>" \
    "Jacob 'Oblio' Olson <br><br>" \
    "In memory:<br>" \
    "Charles 'Charles@Bodhi' van de Beek<br><br><br>" \
    "Past contributors:<br>" \
    "Ken 'trace'  LaBuda<br>" \
    "Christopher 'devilhorns' Michael<br>" \
    "Jason 'jarope' Peel<br>" \
    "Chris 'prolog' Seekamp<br>" \
    "Bob Eley<br>" \
    "Darren 'LostBumpkin' Dooley<br>" \
    "Anthony 'AntCer' Cervantes<br>" \
    "Kaleb 'belak' Elwert<br>" \
    "Jose 'Jose' Manimala<br>" \
    "Gareth 'hippytaff' Williams<br>" \
    "Micah 'M1C4HTRON13″ Denn<br>" \
    "Meji 'Meji_D' Dejsdóttir<br>" \
    "Víctor 'esmirlin' Parra García<br>" \
    "Mark 'ottermaton' Strawser<br>" \
    "Caerolle<br>" \
    "Gar Romero<br>" \
    "Doug 'Deepspeed' Yanez<br>" \
    "Reuben L. Lillie<br>" \
    "Timmy 'timmy' Larsson<br>" \
    "Dennis “DennisD'" \

/* externally accessible functions */

EAPI E_Bodhi_About *
e_bodhi_about_new(E_Container *con)
{
   E_Obj_Dialog *od;

   od =  e_obj_dialog_new(con, BODHI_ABOUT_TITLE, "E", "_about");
   if (!od) return NULL;
   e_obj_dialog_obj_theme_set(od, "base/theme/about", "e/widgets/about/main");
   e_obj_dialog_obj_part_text_set(od, "e.text.label", _("Close"));
   e_obj_dialog_obj_part_text_set(od, "e.text.title",
                                 BODHI_ABOUT_TITLE);
   e_obj_dialog_obj_part_text_set(od, "e.text.version",
                                  BODHI_VERSION);
   e_obj_dialog_obj_part_text_set(od, "e.textblock.about",
                                  BODHI_ABOUT_TEXT);
   e_obj_dialog_obj_part_text_set(od, "e.textblock.authors",
                                  BODHI_ABOUT_AUTHORS);

   return (E_Bodhi_About *)od;
}

EAPI void
e_bodhi_about_show(E_Bodhi_About *about)
{
   e_obj_dialog_show((E_Obj_Dialog *)about);
   e_obj_dialog_icon_set((E_Obj_Dialog *)about, "help-about");
}
