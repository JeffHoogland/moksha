#ifndef E_FERITE_H
#define E_FERITE_H

#include <ferite.h>

void e_ferite_init(void);
void e_ferite_deinit(void);
void e_ferite_run( char *script );
void e_ferite_register( FeriteScript *script, FeriteNamespace *ns );
int  e_ferite_script_error( FeriteScript *script, char *errmsg, int val );
int  e_ferite_script_warning( FeriteScript *script, char *warnmsg );

#endif /* E_FERITE_H */
