#include "e.h"

/* local subsystem functions */

/* local subsystem globals */

/* externally accessible functions */

EAPI E_About *
e_about_new(E_Container *con)
{
   E_Obj_Dialog *od;
   char buf[16384];

   od = e_obj_dialog_new(con, _("About Enlightenment"), "E", "_about");
   if (!od) return NULL;
   e_obj_dialog_obj_theme_set(od, "base/theme/about", "e/widgets/about/main");
   e_obj_dialog_obj_part_text_set(od, "e.text.label", _("Close"));
   e_obj_dialog_obj_part_text_set(od, "e.text.title", _("Enlightenment"));
   e_obj_dialog_obj_part_text_set(od, "e.text.version", VERSION);
   snprintf
     (buf, sizeof(buf), "%s%s",
     _(
       "<title>Copyright &copy; 2000-2012, by the Enlightenment "
       "Development Team</><br>"
       "<br>"
       "We hope you enjoy using this software as much as we enjoyed "
       "writing it.<br>"
       "<br>"
       "To contact us please visit:<br>"
       "<hilight>http://www.enlightenment.org</><br>"
       "<br>"
      ),
       "All rights reserved.<br>"
       "<br>"
       "Redistribution and use in source and binary forms, with or without "
       "modification, are permitted provided that the following conditions "
       "are met:<br>"
       "<br>"
       "1. Redistributions of source code must retain the above copyright "
       "notice, this list of conditions and the following disclaimer.<br>"
       "2. Redistributions in binary form must reproduce the above copyright "
       "notice, this list of conditions and the following disclaimer in the "
       "documentation and/or other materials provided with the "
       "distribution.<br>"
       "<br>"
       "<hilight>THIS SOFTWARE IS PROVIDED \"AS IS\" AND ANY EXPRESS OR "
       "IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED "
       "WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE "
       "ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR "
       "CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, "
       "SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT "
       "LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF "
       "USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED "
       "AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT "
       "LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN "
       "ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE "
       "POSSIBILITY OF SUCH DAMAGE.</><br>"
     );
   e_obj_dialog_obj_part_text_set(od, "e.textblock.about", buf);
   {
      FILE *f;
      char buf[4096];
      Eina_Strbuf *tbuf;

      e_prefix_data_concat_static(buf, "AUTHORS");
      f = fopen(buf, "r");
      if (f)
        {
           tbuf = eina_strbuf_new();
           eina_strbuf_append(tbuf, _("<title>The Team</><br><br>"));
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
                          eina_strbuf_append_printf(tbuf, "%s<br>", buf);
                       }
                  }
             }
           fclose(f);
           if (tbuf)
             {
                e_obj_dialog_obj_part_text_set(od, "e.textblock.authors",
                                               eina_strbuf_string_get(tbuf));
                eina_strbuf_free(tbuf);
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

