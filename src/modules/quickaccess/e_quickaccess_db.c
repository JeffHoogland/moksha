#include "e_mod_main.h"

static const char *_e_qa_db[] =
{
   "XTerm",
   "URxvt",
   "terminology",
   NULL
};

static const char *_e_qa_arg_db[] =
{
   "-name",
   "-name",
   "--name",
   NULL
};

char *
e_qa_db_class_lookup(const char *class)
{
   int x;
   char buf[PATH_MAX];

   if (!class) return NULL;
   for (x = 0; _e_qa_db[x]; x++)
     {
        if (!strcmp(_e_qa_db[x], class))
          return strdup(_e_qa_arg_db[x]);
     }
   /* allows user-added name options for weird/obscure terminals */
   snprintf(buf, sizeof(buf), "%s/e-module-quickaccess.edj", e_module_dir_get(qa_mod->module));
   return edje_file_data_get(buf, class);
}
