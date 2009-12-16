#include "e.h"
#include "e_mod_main.h"
#include "e_mod_dnd.h"

/* local function prototypes */

/* local variables */
static Eina_List *handlers = NULL;

/* public functions */
EAPI int 
e_mod_dnd_init(void) 
{
   E_Zone *z;

   z = e_util_container_zone_number_get(0, 0);

   return 1;
}

EAPI int 
e_mod_dnd_shutdown(void) 
{
   return 1;
}

/* local functions */
