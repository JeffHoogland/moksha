/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* yes - i know. glibc specific... but i like being able to do my own */
/* backtraces! NB: you need CFLAGS="-rdynamic -g" LDFLAGS="-rdynamic -g" */
#ifdef OBJECT_PARANOIA_CHECK

/* local subsystem functions */
static void _e_object_segv(int sig);

/* local subsystem globals */
static sigjmp_buf _e_object_segv_buf;
#endif

/* externally accessible functions */
EAPI void *
e_object_alloc(int size, int type, E_Object_Cleanup_Func cleanup_func)
{
   E_Object *obj;
   
   obj = calloc(1, size);
   if (!obj) return NULL;
   obj->magic = E_OBJECT_MAGIC;
   obj->type = type;
   obj->references   = 1;
   obj->cleanup_func = cleanup_func;
   return obj;
}

EAPI void
e_object_del(E_Object *obj)
{
   E_OBJECT_CHECK(obj);
   if (obj->deleted) return;
   if (obj->del_att_func) obj->del_att_func(obj);
   if (obj->del_func) obj->del_func(obj);
   obj->deleted = 1;
   e_object_unref(obj);
}

EAPI int
e_object_is_del(E_Object *obj)
{
   E_OBJECT_CHECK_RETURN(obj, 1);
   return obj->deleted;
}

EAPI void
e_object_del_func_set(E_Object *obj, E_Object_Cleanup_Func del_func)
{
   E_OBJECT_CHECK(obj);
   obj->del_func = del_func;
}

EAPI void
e_object_type_set(E_Object *obj, int type)
{
   E_OBJECT_CHECK(obj);
   obj->type = type;
}

EAPI void
e_object_free(E_Object *obj)
{
   E_OBJECT_CHECK(obj);
   if (obj->free_att_func) obj->free_att_func(obj);
   /*
    * FIXME:
    * although this is good - if during cleanup the cleanup func calls
    * other generic funcs to do cleanups on the same object... we get bitching.
    * disable for now (the final free of the struct should probably happen after
    * the cleanup func and be done byt he object system - set the magic after
    * cleanup :)  
    */
#if 0
   obj->magic = E_OBJECT_MAGIC_FREED;
#endif
   obj->cleanup_func(obj);
}

EAPI int
e_object_ref(E_Object *obj)
{
   E_OBJECT_CHECK_RETURN(obj, -1);
   obj->references++;
   return obj->references;
}

EAPI int
e_object_unref(E_Object *obj)
{
   int ref;
   
   E_OBJECT_CHECK_RETURN(obj, -1);
   obj->references--;
   ref = obj->references;
   if (obj->references == 0) e_object_free(obj);
   return ref;
}

EAPI int
e_object_ref_get(E_Object *obj)
{
   E_OBJECT_CHECK_RETURN(obj, 0);
   return obj->references;
}

EAPI int
e_object_error(E_Object *obj)
{
#ifdef OBJECT_PARANOIA_CHECK   
   char buf[4096];
   char bt[8192];
   void *trace[128];
   char **messages = NULL;
   int i, trace_num;

   /* fetch stacktrace */
   trace_num = backtrace(trace, 128);
   messages = backtrace_symbols(trace, trace_num);

   /* build stacktrace */
   bt[0] = 0;
   if (messages)
     {
	for (i = 1; i < trace_num; i++)
	  {
	     strcat(bt, messages[i]);
	     strcat(bt, "\n");
	  }
	free(messages);
     }
   /* if NULL obj then dump stacktrace */
   if (!obj)
     {
	snprintf(buf, sizeof(buf),
		 "Object is NULL.\n"
		 "%s",
		 bt);
     }
   /* if obj pointer is non NULL, actually try an access and see if we segv */
   else if (obj)
     {
	struct sigaction act, oact;
	int magic = 0, segv = 0;
	
	/* setup segv handler */
	act.sa_handler = _e_object_segv;
	act.sa_flags   = SA_RESETHAND;
	sigemptyset(&act.sa_mask);
	sigaction(SIGSEGV, &act, &oact);
	/* set a longjump to be within this if statement. only called if we */
	/* segfault */
	if (sigsetjmp(_e_object_segv_buf, 1))
	  {
	     sigaction(SIGSEGV, &oact, NULL);
	     segv = 1;
	  }
	else
	  {
	     /* try access magic value */
	     magic = obj->magic;
	     /* if pointer is bogus we'd segv and so jump to the if () above */
	     /* contents, and thus reset segv handler and set segv flag. */
	     /* if not we just continue moving along here and reset handler */
	     sigaction(SIGSEGV, &oact, NULL);
	  }
	/* if we segfaulted above... */
	if (segv)
	  snprintf(buf, sizeof(buf),
		   "Object [%p] is an invalid/garbage pointer.\n"
		   "Segfault successfully avoided.\n"
		   "%s",
		   obj,
		   bt);
	else
	  {
	     /* valid ram then... if we freed this object before */
	     if (magic == E_OBJECT_MAGIC_FREED)
	       snprintf(buf, sizeof(buf),
			"Object [%p] is already freed.\n"
			"%s",
			obj,
			bt);
	     /* garbage magic value - pointer to non object */
	     else if (magic != E_OBJECT_MAGIC)
	       snprintf(buf, sizeof(buf),
			"Object [%p] has garbage magic (%x).\n"
			"%s",
			obj, magic,
			bt);
	     else if (obj->references < 0)
	       snprintf(buf, sizeof(buf),
			"Object [%p] has negative references (%i).\n"
			"%s",
			obj, obj->references,
			bt);
	     else if (obj->references > 100)
	       snprintf(buf, sizeof(buf),
			"Object [%p] has unusually high reference count (%i).\n"
			"%s",
			obj, obj->references,
			bt);
	     /* it's all ok! */
	     else
	       {
		  return 0;
	       }
	  }
     }
   /* display actual error message */
   e_error_message_show("%s", buf);
   return 1;
#else
   return 0;
#endif   
}

EAPI void
e_object_data_set(E_Object *obj, void *data)
{
   E_OBJECT_CHECK(obj);
   obj->data = data;
}

EAPI void *
e_object_data_get(E_Object *obj)
{
   E_OBJECT_CHECK_RETURN(obj, NULL);
   return obj->data;
}

EAPI void
e_object_free_attach_func_set(E_Object *obj, void (*func) (void *obj))
{
   E_OBJECT_CHECK(obj);
   obj->free_att_func = func;
}

EAPI void
e_object_del_attach_func_set(E_Object *obj, void (*func) (void *obj))
{
   E_OBJECT_CHECK(obj);
   obj->del_att_func = func;
}

/*
void
e_object_breadcrumb_add(E_Object *obj, char *crumb)
{
   E_OBJECT_CHECK(obj);
   obj->crumbs = evas_list_append(obj->crumbs, strdup(crumb));
}

void
e_object_breadcrumb_del(E_Object *obj, char *crumb)
{
   Evas_List *l;
   
   E_OBJECT_CHECK(obj);
   for (l = obj->crumbs; l; l = l->next)
     {
	if (!strcmp(crumb, l->data))
	  {
	     free(l->data);
	     obj->crumbs = evas_list_remove_list(obj->crumbs, l);
	     return;
	  }
     }
}

void
e_object_breadcrumb_debug(E_Object *obj)
{
   Evas_List *l;
   
   E_OBJECT_CHECK(obj);
   for (l = obj->crumbs; l; l = l->next)
     printf("CRUMB: %s\n", l->data);
}
*/

#ifdef OBJECT_PARANOIA_CHECK
/* local subsystem functions */
static void
_e_object_segv(int sig)
{
   siglongjmp(_e_object_segv_buf, 1);
}
#endif
