#include "debug.h"
#include "object.h"

void
e_object_init(E_Object *obj, E_Cleanup_Func cleanup_func)
{
  D_ENTER;
  
  if (!obj)
    D_RETURN;

  memset(obj, 0, sizeof(E_Object));
  obj->references = 1;
  obj->cleanup_func  = cleanup_func;

  D_RETURN;
}

void 
e_object_cleanup(E_Object *obj)
{
  D_ENTER;

  if (!obj)
    D_RETURN;

  free(obj);

  D_RETURN;
}


void
e_object_ref(E_Object *obj)
{
  D_ENTER;

  if (!obj)
    D_RETURN;

  obj->references++;
  D("++ refcount on %p, now %i\n", obj, obj->references);

  D_RETURN;
}

int
e_object_unref(E_Object *obj)
{
  D_ENTER;

  if (!obj)
    D_RETURN_(-1);

  obj->references--;

  D("-- refcount on %p, now %i\n", obj, obj->references);

  if (obj->references == 0 && obj->cleanup_func)
    {
      D("Refcount is zero, freeing.\n");
      obj->cleanup_func(obj);
      D_RETURN_(0);
    }

  D_RETURN_(obj->references);
}


int
e_object_get_usecount(E_Object *obj)
{
  D_ENTER;

  if (!obj)
    D_RETURN_(-1);

  D_RETURN_(obj->references);
}
