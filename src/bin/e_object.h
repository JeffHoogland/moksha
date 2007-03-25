/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

/* Object safety/debugging checks */
/* */
/* OBJECT_PARANOIA_CHECK is paranoid and checkes pointers and traps segv's */
/* in case they dont work... very paranoid and slow. NB for backtrace to */
/* work you need gcc, glibc and you need to compile with compile options of */
/* -g -rdynamic and link with them too */
/* OBJECT_CHECK is a simple null pointer and magic number check with no */
/* debug output */

#ifndef __GLIBC__
# ifdef OBJECT_PARANOIA_CHECK
#  warning "Your system doesn't have glibc. Paranoid object checking disabled."
#  undef OBJECT_PARANOIA_CHECK
# endif
#endif

#define E_OBJECT_MAGIC                  0xe0b9ec75
#define E_OBJECT_MAGIC_FREED            0xe0bf6eed
#define E_OBJECT(x)                     ((E_Object *)(x))
#define E_OBJECT_CLEANUP_FUNC(x)        ((E_Object_Cleanup_Func)(x))
#define E_OBJECT_ALLOC(x, type, cleanup_func) e_object_alloc(sizeof(x), (type), E_OBJECT_CLEANUP_FUNC(cleanup_func))
#define E_OBJECT_DEL_SET(x, del_func)   e_object_del_func_set(E_OBJECT(x), E_OBJECT_CLEANUP_FUNC(del_func))

#ifdef OBJECT_PARANOIA_CHECK
# define E_OBJECT_CHECK(x)                       do {if (e_object_error(E_OBJECT(x))) return;} while (0)
# define E_OBJECT_CHECK_RETURN(x, ret)           do {if (e_object_error(E_OBJECT(x))) return ret;} while (0)
#  define E_OBJECT_TYPE_CHECK(x, tp)             do {if ((E_OBJECT(x)->type) != (tp)) { fprintf(stderr, "Object type check failed in %s\n", __FUNCTION__); return;} } while (0)
#  define E_OBJECT_TYPE_CHECK_RETURN(x, tp, ret) do {if ((E_OBJECT(x)->type) != tp) { fprintf(stderr, "Object type check failed in %s\n", __FUNCTION__); return ret;} } while (0)
# define E_OBJECT_IF_NOT_TYPE(x, tp)             if (E_OBJECT(x)->type != (tp))
#else
# ifdef OBJECT_CHECK
#  define E_OBJECT_CHECK(x)                       do {if ((!E_OBJECT(x)) || (E_OBJECT(x)->magic != E_OBJECT_MAGIC)) return;} while (0)
#  define E_OBJECT_CHECK_RETURN(x, ret)           do {if ((!E_OBJECT(x)) || (E_OBJECT(x)->magic != E_OBJECT_MAGIC)) return ret;} while (0)
#  define E_OBJECT_TYPE_CHECK(x, tp)              do {if ((E_OBJECT(x)->type) != (tp)) { fprintf(stderr, "Object type check failed in %s\n", __FUNCTION__); return;} } while (0)
#  define E_OBJECT_TYPE_CHECK_RETURN(x, tp, ret)  do {if ((E_OBJECT(x)->type) != (tp)) { fprintf(stderr, "Object type check failed in %s\n", __FUNCTION__); return ret;} } while (0)
# define E_OBJECT_IF_NOT_TYPE(x, type)            if (E_OBJECT(x)->type != (type))
# else
#  define E_OBJECT_CHECK(x)               
#  define E_OBJECT_CHECK_RETURN(x, ret)   
#  define E_OBJECT_TYPE_CHECK(x, type)               
#  define E_OBJECT_TYPE_CHECK_RETURN(x, type, ret)   
# define E_OBJECT_IF_NOT_TYPE(x, type)
# endif
#endif

typedef void (*E_Object_Cleanup_Func) (void *obj);

typedef struct _E_Object E_Object;

#else
#ifndef E_OBJECT_H
#define E_OBJECT_H

struct _E_Object
{
   int                     magic;
   int                     type;
   int                     references;
   E_Object_Cleanup_Func   del_func;
   E_Object_Cleanup_Func   cleanup_func;
   void                  (*free_att_func) (void *obj);
   void                  (*del_att_func) (void *obj);
   void                   *data;
   unsigned char           deleted : 1;
};

EAPI void *e_object_alloc               (int size, int type, E_Object_Cleanup_Func cleanup_func);
EAPI void  e_object_del                 (E_Object *obj);
EAPI int   e_object_is_del              (E_Object *obj);
EAPI void  e_object_del_func_set        (E_Object *obj, E_Object_Cleanup_Func del_func);
EAPI void  e_object_type_set            (E_Object *obj, int type);
EAPI void  e_object_free                (E_Object *obj);
EAPI int   e_object_ref                 (E_Object *obj);
EAPI int   e_object_unref               (E_Object *obj);
EAPI int   e_object_ref_get             (E_Object *obj);
EAPI int   e_object_error               (E_Object *obj);
EAPI void  e_object_data_set            (E_Object *obj, void *data);
EAPI void *e_object_data_get            (E_Object *obj);
EAPI void  e_object_free_attach_func_set(E_Object *obj, void (*func) (void *obj));
EAPI void  e_object_del_attach_func_set (E_Object *obj, void (*func) (void *obj));

/*
EAPI void  e_object_breadcrumb_add      (E_Object *obj, char *crumb);
EAPI void  e_object_breadcrumb_del      (E_Object *obj, char *crumb);
EAPI void  e_object_breadcrumb_debug    (E_Object *obj);
*/

#endif
#endif
