#include "e_ferite.h"
#include "e_ferite_gen_header.h"

void e_ferite_init(void)
{
   printf( "Initialising ferite....\n" );
   ferite_init( 0, NULL );
}

void e_ferite_deinit(void)
{
   printf( "Deinitialising ferite....\n" );
   ferite_deinit();
}

int e_ferite_script_error( FeriteScript *script, char *errmsg, int val )
{
   fprintf( stderr, "e17: ferite error: %s\n", errmsg );
   return 1;
}

int e_ferite_script_warning( FeriteScript *script, char *warnmsg )
{
   fprintf( stderr, "e17: ferite warning: %s\n", warnmsg );
   return 1;
}

void e_ferite_run( char *txt )
{
   FeriteScript *script = NULL;
   
   printf( "Compiling script `%s'\n", txt );
   script = __ferite_compile_string( txt );
   e_ferite_register( script, script->mainns );
   script->error_cb = e_ferite_script_error;
   script->warning_cb = e_ferite_script_warning;
   printf( "Executing script.\n" );
   ferite_script_execute( script );
   printf( "Cleaning up.\n" );
   ferite_script_delete( script );
}
