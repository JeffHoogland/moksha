/*#include "debug.h"*/
#include "e_ferite.h"
#include "e_ferite_gen_header.h"
#include "debug.h"

#if 0
#ifdef D			/* until ferite doesn't pullte the D(ebug) macro */
# undef D
# define D(x,...)
# define D_ENTER
# define D_RETURN
# define D_RETURN_(x)
#endif
#endif

void
e_ferite_init(void)
{
   D_ENTER;

   D("Initialising ferite....\n");
   ferite_init(0, NULL);

   D_RETURN;
}

void
e_ferite_deinit(void)
{
   D_ENTER;

   D("Deinitialising ferite....\n");
   ferite_deinit();

   D_RETURN;
}

int
e_ferite_script_error(FeriteScript * script, char *errmsg, int val)
{
   D_ENTER;

   fprintf(stderr, "e17: ferite error: %s\n", errmsg);

   D_RETURN_(1);
}

int
e_ferite_script_warning(FeriteScript * script, char *warnmsg)
{
   D_ENTER;

   fprintf(stderr, "e17: ferite warning: %s\n", warnmsg);

   D_RETURN_(1);
}

void
e_ferite_run(char *txt)
{
   FeriteScript       *script = NULL;

   D_ENTER;

   D("Ferite: Compiling script `%s'\n", txt);
   script = __ferite_compile_string(txt);
   e_ferite_register(script, script->mainns);
   script->error_cb = e_ferite_script_error;
   script->warning_cb = e_ferite_script_warning;
   D("Ferite: executing script.\n");
   ferite_script_execute(script);
   D("Ferite: Cleaning up.\n");
   ferite_script_delete(script);

   D_RETURN;
}
