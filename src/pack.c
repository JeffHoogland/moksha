#include "e.h"

E_Pack_Object_Class classes[PACK_MAX];

void
e_pack_object_init(void)
{
   /* all nulls */
   ZERO(classes, E_Pack_Object_Class, PACK_MAX);
   /* set up the entry widget class - we could pack them now */
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
   po->references = 0;
   if (po->class.new) po->data.object = po->class.new();
   return po;
}

void
e_pack_object_use(E_Pack_Object *po)
{
   po->references++;
}

void
e_pack_object_free(E_Pack_Object *po)
{
   Evas_List l;
   
   po->references--;
   if (po->references > 0) return;
   if (po->class.free) po->class.free(po->data.object);
   if (po->children)
     {
	for (l = po->children; l; l = l->next)
	  {
	     E_Pack_Object *child_po;
	     
	     child_po = l->data;
	     e_pack_object_free(child_po);
	  }
	po->children = evas_list_free(po->children);
     }
   free(po);
}

void
e_pack_object_set_evas(E_Pack_Object *po, Evas e)
{
   if (e == po->data.evas) return;
   po->data.evas = e;
   if (po->class.evas) po->class.evas(po->data.object, e);
   if (po->children)
     {
	Evas_List l;
	
	for (l = po->children; l; l = l->next)
	  {
	     E_Pack_Object *child_po;
	     
	     child_po = l->data;
	     e_pack_object_set_evas(child_po, e);
	  }
     }   
}

void
e_pack_object_show(E_Pack_Object *po)
{
}

void
e_pack_object_hide(E_Pack_Object *po)
{
}

void
e_pack_object_raise(E_Pack_Object *po)
{
}

void
e_pack_object_lower(E_Pack_Object *po)
{
}

void
e_pack_object_set_layer(E_Pack_Object *po, int l)
{
}

void
e_pack_object_clip(E_Pack_Object *po, Evas_Object clip)
{
}

void
e_pack_object_unclip(E_Pack_Object *po)
{
}

void
e_pack_object_move(E_Pack_Object *po, int x, int y)
{
}

void
e_pack_object_resize(E_Pack_Object *po, int w, int h)
{
}

void
e_pack_object_get_size(E_Pack_Object *po, int *w, int *h)
{
}
