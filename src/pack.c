#include "e.h"

E_Pack_Object_Class classes[1];

void
e_pack_object_init(void)
{
}

E_Pack_Object *
e_pack_object_new(int type)
{
   E_Pack_Object *po;
   
   po = NEW(E_Pack_Object, 1);
   ZERO(po, E_Pack_Object, 1);
   po->type = type;
   po->class = classes[type];
   return po;
}
