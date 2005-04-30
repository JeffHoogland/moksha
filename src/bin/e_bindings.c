/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */
typedef struct _E_Binding_Mouse  E_Binding_Mouse;
typedef struct _E_Binding_Key    E_Binding_Key;
typedef struct _E_Binding_Signal E_Binding_Signal;

struct _E_Binding_Mouse
{
   E_Binding_Context ctxt;
   int button;
   E_Binding_Modifier mod;
   unsigned char any_mod : 1;
   char *action;
   char *params;
};

struct _E_Binding_Key
{
   E_Binding_Context ctxt;
   char *key;
   E_Binding_Modifier mod;
   unsigned char any_mod : 1;
   char *action;
   char *params;
};

struct _E_Binding_Signal
{
   E_Binding_Context ctxt;
   char *sig;
   char *src;
   char *action;
   char *params;
};

static void _e_bindings_mouse_free(E_Binding_Mouse *bind);
static void _e_bindings_key_free(E_Binding_Key *bind);
static int _e_bindings_context_match(E_Binding_Context bctxt, E_Binding_Context ctxt);

/* local subsystem globals */

static Evas_List *mouse_bindings = NULL;
static Evas_List *key_bindings = NULL;
static Evas_List *signal_bindings = NULL;

/* externally accessible functions */

int
e_bindings_init(void)
{
   e_bindings_mouse_add(E_BINDING_CONTEXT_BORDER,
			1, E_BINDING_MODIFIER_ALT, 0,
			"window_move", "");
   e_bindings_mouse_add(E_BINDING_CONTEXT_BORDER,
			2, E_BINDING_MODIFIER_ALT, 0,
			"window_resize", "");
   e_bindings_mouse_add(E_BINDING_CONTEXT_BORDER,
			3, E_BINDING_MODIFIER_ALT, 0,
			"window_menu", "");
   e_bindings_key_add(E_BINDING_CONTEXT_ANY,
		      "Left", E_BINDING_MODIFIER_SHIFT | E_BINDING_MODIFIER_ALT, 0,
		      "desk_flip_by", "-1 0");
   e_bindings_key_add(E_BINDING_CONTEXT_ANY,
		      "Right", E_BINDING_MODIFIER_SHIFT | E_BINDING_MODIFIER_ALT, 0,
		      "desk_flip_by", "1 0");
   e_bindings_key_add(E_BINDING_CONTEXT_ANY,
		      "Up", E_BINDING_MODIFIER_SHIFT | E_BINDING_MODIFIER_ALT, 0,
		      "desk_flip_by", "0 -1");
   e_bindings_key_add(E_BINDING_CONTEXT_ANY,
		      "Down", E_BINDING_MODIFIER_SHIFT | E_BINDING_MODIFIER_ALT, 0,
		      "desk_flip_by", "0 -1");
   e_bindings_key_add(E_BINDING_CONTEXT_ANY,
		      "Up", E_BINDING_MODIFIER_CTRL | E_BINDING_MODIFIER_ALT, 0,
		      "window_raise", "");
   e_bindings_key_add(E_BINDING_CONTEXT_ANY,
		      "Down", E_BINDING_MODIFIER_CTRL | E_BINDING_MODIFIER_ALT, 0,
		      "window_lower", "");
   e_bindings_key_add(E_BINDING_CONTEXT_ANY,
		      "x", E_BINDING_MODIFIER_CTRL | E_BINDING_MODIFIER_ALT, 0,
		      "window_close", "");
   e_bindings_key_add(E_BINDING_CONTEXT_ANY,
		      "Left", E_BINDING_MODIFIER_CTRL | E_BINDING_MODIFIER_ALT, 0,
		      "desk_linear_flip_by", "-1");
   e_bindings_key_add(E_BINDING_CONTEXT_ANY,
		      "Right", E_BINDING_MODIFIER_CTRL | E_BINDING_MODIFIER_ALT, 0,
		      "desk_linear_flip_by", "1");
   e_bindings_key_add(E_BINDING_CONTEXT_ANY,
		      "F1", E_BINDING_MODIFIER_ALT, 0,
		      "desk_linear_flip_to", "0");
   e_bindings_key_add(E_BINDING_CONTEXT_ANY,
		      "F2", E_BINDING_MODIFIER_ALT, 0,
		      "desk_linear_flip_to", "1");
   e_bindings_key_add(E_BINDING_CONTEXT_ANY,
		      "F3", E_BINDING_MODIFIER_ALT, 0,
		      "desk_linear_flip_to", "2");
   e_bindings_key_add(E_BINDING_CONTEXT_ANY,
		      "F4", E_BINDING_MODIFIER_ALT, 0,
		      "desk_linear_flip_to", "3");
   e_bindings_key_add(E_BINDING_CONTEXT_ANY,
		      "F5", E_BINDING_MODIFIER_ALT, 0,
		      "desk_linear_flip_to", "4");
   e_bindings_key_add(E_BINDING_CONTEXT_ANY,
		      "F6", E_BINDING_MODIFIER_ALT, 0,
		      "desk_linear_flip_to", "5");
   e_bindings_key_add(E_BINDING_CONTEXT_ANY,
		      "F7", E_BINDING_MODIFIER_ALT, 0,
		      "desk_linear_flip_to", "6");
   e_bindings_key_add(E_BINDING_CONTEXT_ANY,
		      "F8", E_BINDING_MODIFIER_ALT, 0,
		      "desk_linear_flip_to", "7");
   e_bindings_key_add(E_BINDING_CONTEXT_ANY,
		      "F9", E_BINDING_MODIFIER_ALT, 0,
		      "desk_linear_flip_to", "8");
   e_bindings_key_add(E_BINDING_CONTEXT_ANY,
		      "F10", E_BINDING_MODIFIER_ALT, 0,
		      "desk_linear_flip_to", "9");
   e_bindings_key_add(E_BINDING_CONTEXT_ANY,
		      "F11", E_BINDING_MODIFIER_ALT, 0,
		      "desk_linear_flip_to", "10");
   e_bindings_key_add(E_BINDING_CONTEXT_ANY,
		      "F12", E_BINDING_MODIFIER_ALT, 0,
		      "desk_linear_flip_to", "11");
   return 1;
}

int
e_bindings_shutdown(void)
{
   /* FIXME: free bindings */
   return 1;
}

void
e_bindings_mouse_add(E_Binding_Context ctxt, int button, E_Binding_Modifier mod, int any_mod, char *action, char *params)
{
   E_Binding_Mouse *bind;
   
   bind = calloc(1, sizeof(E_Binding_Mouse));
   bind->ctxt = ctxt;
   bind->button = button;
   bind->mod = mod;
   bind->any_mod = any_mod;
   bind->action = strdup(action);
   bind->params = strdup(params);
   mouse_bindings = evas_list_append(mouse_bindings, bind);
}

void
e_bindings_mouse_del(E_Binding_Context ctxt, int button, E_Binding_Modifier mod, int any_mod, char *action, char *params)
{
   Evas_List *l;
   
   for (l = mouse_bindings; l; l = l->next)
     {
	E_Binding_Mouse *bind;
	
	bind = l->data;
	if ((bind->ctxt == ctxt) &&
	    (bind->button == button) &&
	    (bind->mod == mod) &&
	    (bind->any_mod == any_mod) &&
	    (!strcmp(bind->action, action)) &&
	    (!strcmp(bind->params, params)))
	  {
	     _e_bindings_mouse_free(bind);
	     mouse_bindings = evas_list_remove_list(mouse_bindings, l);
	     break;
	  }
     }
}

void
e_bindings_mouse_grab(E_Binding_Context ctxt, Ecore_X_Window win)
{
   Evas_List *l;

   for (l = mouse_bindings; l; l = l->next)
     {
	E_Binding_Mouse *bind;
	
	bind = l->data;
	if (_e_bindings_context_match(bind->ctxt, ctxt))
	  {
	     int mod;
	     
	     mod = 0;
	     if (bind->mod & E_BINDING_MODIFIER_SHIFT) mod |= ECORE_X_MODIFIER_SHIFT;
	     if (bind->mod & E_BINDING_MODIFIER_CTRL) mod |= ECORE_X_MODIFIER_CTRL;
	     if (bind->mod & E_BINDING_MODIFIER_ALT) mod |= ECORE_X_MODIFIER_ALT;
	     if (bind->mod & E_BINDING_MODIFIER_WIN) mod |= ECORE_X_MODIFIER_WIN;
	     ecore_x_window_button_grab(win, bind->button, 
					ECORE_X_EVENT_MASK_MOUSE_DOWN | 
					ECORE_X_EVENT_MASK_MOUSE_UP | 
					ECORE_X_EVENT_MASK_MOUSE_MOVE, 
					mod, bind->any_mod);
	  }
     }
}

void
e_bindings_mouse_ungrab(E_Binding_Context ctxt, Ecore_X_Window win)
{
   Evas_List *l;
   
   for (l = mouse_bindings; l; l = l->next)
     {
	E_Binding_Mouse *bind;
	
	bind = l->data;
	if (_e_bindings_context_match(bind->ctxt, ctxt))
	  {
	     int mod;
	     
	     mod = 0;
	     if (bind->mod & E_BINDING_MODIFIER_SHIFT) mod |= ECORE_X_MODIFIER_SHIFT;
	     if (bind->mod & E_BINDING_MODIFIER_CTRL) mod |= ECORE_X_MODIFIER_CTRL;
	     if (bind->mod & E_BINDING_MODIFIER_ALT) mod |= ECORE_X_MODIFIER_ALT;
	     if (bind->mod & E_BINDING_MODIFIER_WIN) mod |= ECORE_X_MODIFIER_WIN;
	     ecore_x_window_button_ungrab(win, bind->button,
					  mod, bind->any_mod);
	  }
     }
}

int
e_bindings_mouse_down_event_handle(E_Binding_Context ctxt, E_Object *obj, Ecore_X_Event_Mouse_Button_Down *ev)
{
   E_Binding_Modifier mod = 0;
   Evas_List *l;
   
   if (ev->modifiers & ECORE_X_MODIFIER_SHIFT) mod |= E_BINDING_MODIFIER_SHIFT;
   if (ev->modifiers & ECORE_X_MODIFIER_CTRL) mod |= E_BINDING_MODIFIER_CTRL;
   if (ev->modifiers & ECORE_X_MODIFIER_ALT) mod |= E_BINDING_MODIFIER_ALT;
   if (ev->modifiers & ECORE_X_MODIFIER_WIN) mod |= E_BINDING_MODIFIER_WIN;
   for (l = mouse_bindings; l; l = l->next)
     {
	E_Binding_Mouse *bind;
	
	bind = l->data;
	if ((bind->button == ev->button) &&
	    ((bind->any_mod) || (bind->mod == mod)))
	  {
	     if (_e_bindings_context_match(bind->ctxt, ctxt))
	       {
		  E_Action *act;
		  
		  act = e_action_find(bind->action);
		  if (act)
		    {
		       if (act->func.go_mouse)
			 act->func.go_mouse(obj, bind->params, ev);
		       else if (act->func.go)
			 act->func.go(obj, bind->params);
		       return 1;
		    }
		  return 0;
	       }
	  }
     }
   return 0;
}

int
e_bindings_mouse_up_event_handle(E_Binding_Context ctxt, E_Object *obj, Ecore_X_Event_Mouse_Button_Up *ev)
{
   E_Binding_Modifier mod = 0;
   Evas_List *l;
   
   if (ev->modifiers & ECORE_X_MODIFIER_SHIFT) mod |= E_BINDING_MODIFIER_SHIFT;
   if (ev->modifiers & ECORE_X_MODIFIER_CTRL) mod |= E_BINDING_MODIFIER_CTRL;
   if (ev->modifiers & ECORE_X_MODIFIER_ALT) mod |= E_BINDING_MODIFIER_ALT;
   if (ev->modifiers & ECORE_X_MODIFIER_WIN) mod |= E_BINDING_MODIFIER_WIN;
   for (l = mouse_bindings; l; l = l->next)
     {
	E_Binding_Mouse *bind;
	
	bind = l->data;
	if ((bind->button == ev->button) &&
	    ((bind->any_mod) || (bind->mod == mod)))
	  {
	     if (_e_bindings_context_match(bind->ctxt, ctxt))
	       {
		  E_Action *act;
		  
		  act = e_action_find(bind->action);
		  if (act)
		    {
		       if (act->func.end_mouse)
			 act->func.end_mouse(obj, bind->params, ev);
		       else if (act->func.end)
			 act->func.end(obj, bind->params);
		       return 1;
		    }
		  return 0;
	       }
	  }
     }
   return 0;
}

/* FIXME: finish off key bindings */
void
e_bindings_key_add(E_Binding_Context ctxt, char *key, E_Binding_Modifier mod, int any_mod, char *action, char *params)
{
   E_Binding_Key *bind;
   
   bind = calloc(1, sizeof(E_Binding_Key));
   bind->ctxt = ctxt;
   bind->key = strdup(key);
   bind->mod = mod;
   bind->any_mod = any_mod;
   bind->action = strdup(action);
   bind->params = strdup(params);
   key_bindings = evas_list_append(key_bindings, bind);
}

void
e_bindings_key_del(E_Binding_Context ctxt, char *key, E_Binding_Modifier mod, int any_mod, char *action, char *params)
{
   Evas_List *l;
   
   for (l = key_bindings; l; l = l->next)
     {
	E_Binding_Key *bind;
	
	bind = l->data;
	if ((bind->ctxt == ctxt) &&
	    (!strcmp(bind->key, key)) &&
	    (bind->mod == mod) &&
	    (bind->any_mod == any_mod) &&
	    (!strcmp(bind->action, action)) &&
	    (!strcmp(bind->params, params)))
	  {
	     _e_bindings_key_free(bind);
	     key_bindings = evas_list_remove_list(key_bindings, l);
	     break;
	  }
     }
}

void
e_bindings_key_grab(E_Binding_Context ctxt, Ecore_X_Window win)
{
   Evas_List *l;

   printf("GRAB KEYS %p\n", key_bindings);
   for (l = key_bindings; l; l = l->next)
     {
	E_Binding_Key *bind;
	
	bind = l->data;
	if (_e_bindings_context_match(bind->ctxt, ctxt))
	  {
	     int mod;
	     
	     mod = 0;
	     if (bind->mod & E_BINDING_MODIFIER_SHIFT) mod |= ECORE_X_MODIFIER_SHIFT;
	     if (bind->mod & E_BINDING_MODIFIER_CTRL) mod |= ECORE_X_MODIFIER_CTRL;
	     if (bind->mod & E_BINDING_MODIFIER_ALT) mod |= ECORE_X_MODIFIER_ALT;
	     if (bind->mod & E_BINDING_MODIFIER_WIN) mod |= ECORE_X_MODIFIER_WIN;
	     printf("BIND %x %s %x %i\n", win, bind->key, mod, bind->any_mod);
	     ecore_x_window_key_grab(win, bind->key,
				     mod, bind->any_mod);
	  }
     }
}

void
e_bindings_key_ungrab(E_Binding_Context ctxt, Ecore_X_Window win)
{
   Evas_List *l;
   
   for (l = key_bindings; l; l = l->next)
     {
	E_Binding_Key *bind;
	
	bind = l->data;
	if (_e_bindings_context_match(bind->ctxt, ctxt))
	  {
	     int mod;
	     
	     mod = 0;
	     if (bind->mod & E_BINDING_MODIFIER_SHIFT) mod |= ECORE_X_MODIFIER_SHIFT;
	     if (bind->mod & E_BINDING_MODIFIER_CTRL) mod |= ECORE_X_MODIFIER_CTRL;
	     if (bind->mod & E_BINDING_MODIFIER_ALT) mod |= ECORE_X_MODIFIER_ALT;
	     if (bind->mod & E_BINDING_MODIFIER_WIN) mod |= ECORE_X_MODIFIER_WIN;
	     ecore_x_window_key_ungrab(win, bind->key,
				       mod, bind->any_mod);
	  }
     }
}

int
e_bindings_key_down_event_handle(E_Binding_Context ctxt, E_Object *obj, Ecore_X_Event_Key_Down *ev)
{
   E_Binding_Modifier mod = 0;
   Evas_List *l;
   
   if (ev->modifiers & ECORE_X_MODIFIER_SHIFT) mod |= E_BINDING_MODIFIER_SHIFT;
   if (ev->modifiers & ECORE_X_MODIFIER_CTRL) mod |= E_BINDING_MODIFIER_CTRL;
   if (ev->modifiers & ECORE_X_MODIFIER_ALT) mod |= E_BINDING_MODIFIER_ALT;
   if (ev->modifiers & ECORE_X_MODIFIER_WIN) mod |= E_BINDING_MODIFIER_WIN;
   for (l = key_bindings; l; l = l->next)
     {
	E_Binding_Key *bind;
	
	bind = l->data;
	if ((!strcmp(bind->key, ev->keyname)) &&
	    ((bind->any_mod) || (bind->mod == mod)))
	  {
	     if (_e_bindings_context_match(bind->ctxt, ctxt))
	       {
		  E_Action *act;
		  
		  act = e_action_find(bind->action);
		  if (act)
		    {
		       if (act->func.go_key)
			 act->func.go_key(obj, bind->params, ev);
		       else if (act->func.go)
			 act->func.go(obj, bind->params);
		       return 1;
		    }
		  return 0;
	       }
	  }
     }
   return 0;
}

int
e_bindings_key_up_event_handle(E_Binding_Context ctxt, E_Object *obj, Ecore_X_Event_Key_Up *ev)
{
   E_Binding_Modifier mod = 0;
   Evas_List *l;
   
   if (ev->modifiers & ECORE_X_MODIFIER_SHIFT) mod |= E_BINDING_MODIFIER_SHIFT;
   if (ev->modifiers & ECORE_X_MODIFIER_CTRL) mod |= E_BINDING_MODIFIER_CTRL;
   if (ev->modifiers & ECORE_X_MODIFIER_ALT) mod |= E_BINDING_MODIFIER_ALT;
   if (ev->modifiers & ECORE_X_MODIFIER_WIN) mod |= E_BINDING_MODIFIER_WIN;
   for (l = key_bindings; l; l = l->next)
     {
	E_Binding_Key *bind;
	
	bind = l->data;
	if ((!strcmp(bind->key, ev->keyname)) &&
	    ((bind->any_mod) || (bind->mod == mod)))
	  {
	     if (_e_bindings_context_match(bind->ctxt, ctxt))
	       {
		  E_Action *act;
		  
		  act = e_action_find(bind->action);
		  if (act)
		    {
		       if (act->func.end_key)
			 act->func.end_key(obj, bind->params, ev);
		       else if (act->func.end)
			 act->func.end(obj, bind->params);
		       return 1;
		    }
		  return 0;
	       }
	  }
     }
   return 0;
}

/* FIXME: finish off signal bindings */
int
e_bindings_signal_handle(E_Binding_Context ctxt, E_Object *obj, char *sig, char *src)
{
   Evas_List *l;
   
   return 0;
}

/* local subsystem functions */

static void
_e_bindings_mouse_free(E_Binding_Mouse *bind)
{
   E_FREE(bind->action);
   E_FREE(bind->params);
   free(bind);
}

static void
_e_bindings_key_free(E_Binding_Key *bind)
{
   E_FREE(bind->key);
   E_FREE(bind->action);
   E_FREE(bind->params);
   free(bind);
}

static int
_e_bindings_context_match(E_Binding_Context bctxt, E_Binding_Context ctxt)
{
   if (bctxt == E_BINDING_CONTEXT_ANY) return 1;
   if (ctxt == E_BINDING_CONTEXT_UNKNOWN) return 0;
   if (bctxt == ctxt) return 1;
   return 0;
}
