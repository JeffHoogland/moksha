#ifndef E_OBJECT_H
#define E_OBJECT_H

#include <Evas.h>
#include <Ecore.h>

#define E_OBJECT(x) ((E_Object*)(x))

typedef void (*E_Cleanup_Func) (void *object);

typedef struct _e_object
{
  int          references;
  E_Cleanup_Func  cleanup_func;
  
} E_Object;

/**
 * e_object_init - Initializes an E object
 * @obj:          The object to initalize
 * @cleanup_func: The destructor function for this object
 *
 * This function initializes an E object. It registers
 * the @cleanup_func that is to be called when the use
 * count on this object drops to zero. This destructor
 * then cleans up the object, from the derived classes up
 * the hierarchy to the highest ancestor, E_Object. You
 * MUST call the destructor of the base class at the end
 * of a destructor, or you're creating a leak. Moreover,
 * those destructors MUST NOT call free() on the object
 * itself, only clean up things that are pointed to etc.
 * The final free() call has to happen in the root class,
 * here, it happens in e_object_cleanup().
 */
void e_object_init(E_Object *obj, E_Cleanup_Func cleanup_func);

/**
 * e_object_cleanup - Cleanup function for E_Objects
 * @obj:     The object to clean up
 *
 * This is the most basic destructor function, the only
 * one which does not call another base class destructor
 * at the end. This is the place where the final free()
 * call occurs.
 */
void e_object_cleanup(E_Object *obj);

/**
 * e_object_ref - Increment the reference count of this object
 * @obj:     The object whose reference count to increase
 */
void e_object_ref(E_Object *obj);

/**
 * e_object_unref - Decrememnt the reference count of this object
 * @obj:     The object whose reference count to decrease
 *
 * This function decreases an object's reference counter. If
 * the counter drops to zero, the objects cleanup_func()
 * that was passed with e_object_init() is called. This function
 * is the destructor of the most derived class of this object,
 * and works its way back to the root class's destructor,
 * e_object_cleanup().
 */
int  e_object_unref(E_Object *obj);

/**
 * e_object_get_usecount - Returns the current use count
 * @obj:   The object whose use count to return
 *
 * This function returns the use count of an object. Use this
 * function when you want to perform tasks before an object
 * gets cleaned up by checking if the use count is one,
 * cleaning up, and then calling e_object_unref().
 */
int  e_object_get_usecount(E_Object *obj);

#endif
