/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */

/* local subsystem globals */

/* externally accessible functions */

EAPI E_About *
e_about_new(E_Container *con)
{
   E_Obj_Dialog *od;
   
   od = e_obj_dialog_new(con, _("About Enlightenment"), "E", "_about");
   if (!od) return NULL;
   e_obj_dialog_obj_theme_set(od, "base/theme/about", "e/widgets/about/main");
   e_obj_dialog_obj_part_text_set(od, "e.text.label", _("Close"));
   e_obj_dialog_obj_part_text_set(od, "e.text.title", _("Enlightenment"));
   e_obj_dialog_obj_part_text_set(od, "e.text.version", VERSION);
   e_obj_dialog_obj_part_text_set
     (od, "e.textblock.about",
      _(
	"Copyright &copy; 1999-2008, by the Enlightenment Development Team.<br>"
	"<br>"
	"We hope you enjoy using this software as much as we enjoyed "
	"writing it.<br>"
	"<br>"
	"This software is provided as-is with no explicit or implied "
	"warranty. This software is governed by licensing conditions, "
	"so please see the COPYING and COPYING-PLAIN licence files "
	"installed on your system.<br>"
	"<br>"
	"Enlightenment is under <hilight>HEAVY DEVELOPMENT</hilight> and it "
	"is not stable. Many features are incomplete or even non-existant "
	"yet and may have many bugs. You have been <hilight>WARNED!</hilight>"
	)
      );
   
     {
	FILE *f;
	char buf[4096], buf2[4096], *tbuf;

	snprintf(buf, sizeof(buf), "%s/AUTHORS", e_prefix_data_get());
	f = fopen(buf, "r");
	if (f)
	  {
	     tbuf = strdup(_("<title>The Team</title>"));
	     while (fgets(buf, sizeof(buf), f))
	       {
		  int len;

		  len = strlen(buf);
		  if (len > 0)
		    {  
		       if (buf[len - 1] == '\n')
			 {
			    buf[len - 1] = 0;
			    len--;
			 }
		       if (len > 0)
			 {
			    char *p;

			    do
			      {
				 p = strchr(buf, '<');
				 if (p) *p = 0;
			      }
			    while (p);
			    do
			      {
				 p = strchr(buf, '>');
				 if (p) *p = 0;
			      }
			    while (p);
			    snprintf(buf2, sizeof(buf2), "%s<br>", buf);
			    tbuf = realloc(tbuf, strlen(tbuf) + strlen(buf2) + 1);
			    strcat(tbuf, buf2);
			 }
		    }
	       }
	     fclose(f);
	     if (tbuf)
	       {
		  e_obj_dialog_obj_part_text_set
		    (od, "e.textblock.authors", tbuf);
		  free(tbuf);
	       }
	  }
     }
   return (E_About *)od;
}

EAPI void
e_about_show(E_About *about)
{
   e_obj_dialog_show((E_Obj_Dialog *)about);
   e_obj_dialog_icon_set((E_Obj_Dialog *)about, "help-about");
}
