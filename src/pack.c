#include "e.h"

E_Pack_Object_Class classes[PACK_MAX];

void
e_pack_object_init(void)
{
   ZERO(classes, E_Pack_Object_Class, PACK_MAX);
   classes[PACK_ENTRY] = (E_Pack_Object_Class)
     {
	e_entry_new,
	e_entry_free,
	e_entry_show,
	e_entry_hide,
	e_entry_raise,
	e_entry_lower,
	e_entry_set_layer,
	e_entry_set_evas,
	e_entry_set_clip,
	e_entry_unset_clip,
	e_entry_move,
	e_entry_resize,
	e_entry_min_size,
	e_entry_max_size,
	e_entry_set_size
     };
}

E_Pack_Object *
e_pack_object_new(int type)
{
   E_Pack_Object *po;
   
   po = NEW(E_Pack_Object, 1);
   ZERO(po, E_Pack_Object, 1);
   po->type = type;
   po->class = classes[type];
   po->data.object = po->class.new();
   return po;
}
