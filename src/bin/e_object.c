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
   obj->deleted = 1;
   if (obj->del_att_func) obj->del_att_func(obj);
   if (obj->del_func) obj->del_func(obj);
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
   obj->free_att_func = NULL;
   obj->walking_list++;
   while (obj->del_fn_list)
     {
        E_Object_Delfn *dfn = (E_Object_Delfn *)obj->del_fn_list;
        if (!dfn->delete_me) dfn->func(dfn->data, obj);
        obj->del_fn_list = eina_inlist_remove(obj->del_fn_list,
                                              EINA_INLIST_GET(dfn));
        free(dfn);
     }
   obj->walking_list--;
   if (obj->references) return;
   /*
    * FIXME:
    * although this is good - if during cleanup the cleanup func calls
    * other generic funcs to do cleanups on the same object... we get bitching.
    * disable for now (the final free of the struct should probably happen after
    * the cleanup func and be done by the object system - set the magic after
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
   if (!obj->references) return 0;
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

#if 0
EAPI void
e_bt(void)
{
   int i, trace_num;
   void *trace[128];
   char **messages = NULL;
   trace_num = backtrace(trace, 128);
   messages = backtrace_symbols(trace, trace_num);
   if (messages)
     {
        for (i = 1; i < trace_num; i++)
          {
             int j;

             for (j = 1; j < i; j++) putchar(' ');
             printf("%s\n", messages[i]);
          }
        free(messages);
     }
}
#endif

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
   if (!obj)
     {
	fprintf(stderr, "ERROR: Object is NULL. Add break point at %s:%d\n",
		__FILE__, __LINE__);
	return 1;
     }
   return 0;
#endif
}

EAPI void
e_object_data_set(E_Object *obj, const void *data)
{
   E_OBJECT_CHECK(obj);
   obj->data = (void*)data;
}

EAPI void *
e_object_data_get(E_Object *obj)
{
   E_OBJECT_CHECK_RETURN(obj, NULL);
   return obj->data;
}

EAPI void
e_object_free_attach_func_set(E_Object *obj, E_Object_Cleanup_Func func)
{
   E_OBJECT_CHECK(obj);
   obj->free_att_func = func;
}

EAPI void
e_object_del_attach_func_set(E_Object *obj, E_Object_Cleanup_Func func)
{
   E_OBJECT_CHECK(obj);
   obj->del_att_func = func;
}

EAPI void
e_object_delfn_clear(E_Object *obj)
{
   E_OBJECT_CHECK(obj);
   while (obj->del_fn_list)
     {
        E_Object_Delfn *dfn = (E_Object_Delfn *)obj->del_fn_list;
        if (obj->walking_list)
          dfn->delete_me = 1;
        else
          {
             obj->del_fn_list = eina_inlist_remove(obj->del_fn_list,
                                                   EINA_INLIST_GET(dfn));
             free(dfn);
          }
     }
}

EAPI E_Object_Delfn *
e_object_delfn_add(E_Object *obj, void (*func) (void *data, void *obj), void *data)
{
   E_Object_Delfn *dfn;
   E_OBJECT_CHECK_RETURN(obj, NULL);
   dfn = calloc(1, sizeof(E_Object_Delfn));
   if (!dfn) return NULL;
   dfn->func = func;
   dfn->data = data;
   obj->del_fn_list = eina_inlist_append(obj->del_fn_list, EINA_INLIST_GET(dfn));
   return dfn;
}

EAPI void
e_object_delfn_del(E_Object *obj, E_Object_Delfn *dfn)
{
   E_OBJECT_CHECK(obj);
   if (obj->walking_list)
     {
        dfn->delete_me = 1;
        return;
     }
   obj->del_fn_list = eina_inlist_remove(obj->del_fn_list, EINA_INLIST_GET(dfn));
   free(dfn);
}

/*
void
e_object_breadcrumb_add(E_Object *obj, char *crumb)
{
   E_OBJECT_CHECK(obj);
   obj->crumbs = eina_list_append(obj->crumbs, strdup(crumb));
}

void
e_object_breadcrumb_del(E_Object *obj, char *crumb)
{
   Eina_List *l;
   char *key;

   E_OBJECT_CHECK(obj);
   EINA_LIST_FOREACH(obj->crumbs, l, key)
     {
	if (!strcmp(crumb, key))
	  {
	     free(key);
	     obj->crumbs = eina_list_remove_list(obj->crumbs, l);
	     return;
	  }
     }
}

void
e_object_breadcrumb_debug(E_Object *obj)
{
   Eina_List *l;
   char *key;

   E_OBJECT_CHECK(obj);
   EINA_LISt_FOREACH(obj->crumbs, l, key)
     printf("CRUMB: %s\n", key);
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
