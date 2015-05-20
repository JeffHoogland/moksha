#include "e.h"

#define TEXT_PRESS_KEY_SEQUENCE _("Please press key sequence,<br><br>" \
                                  "or <hilight>Escape</hilight> to abort.")
#define TEXT_PRESS_MOUSE_BINIDING_SEQUENCE _("Please hold any modifier you want<br>"             \
                                             "and press any button on your mouse,<br> or roll a" \
                                             " wheel, to assign mouse binding."                  \
                                             "<br>Press <hilight>Escape</highlight> to abort.")

static Eina_Bool
_e_grab_dialog_key_handler(void *data, int type __UNUSED__, Ecore_Event_Key *ev)
{
   E_Grab_Dialog *eg = data;

   if (ev->window != eg->grab_win) return ECORE_CALLBACK_RENEW;
   if (!strcmp(ev->key, "Escape") &&
       !(ev->modifiers & ECORE_EVENT_MODIFIER_SHIFT) &&
       !(ev->modifiers & ECORE_EVENT_MODIFIER_CTRL) &&
       !(ev->modifiers & ECORE_EVENT_MODIFIER_ALT) &&
       !(ev->modifiers & ECORE_EVENT_MODIFIER_WIN))
     {
        e_object_del(data);
        return ECORE_CALLBACK_RENEW;
     }
   
   if (eg->key)
     {
        e_object_ref(data);
        eg->key(eg->data ?: eg, type, ev);
        e_object_unref(data);
     }
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_e_grab_dialog_wheel_handler(void *data, int type __UNUSED__, Ecore_Event_Mouse_Wheel *ev)
{
   E_Grab_Dialog *eg = data;

   if (ev->window != eg->grab_win) return ECORE_CALLBACK_RENEW;

   if (eg->wheel)
     {
        e_object_ref(data);
        eg->wheel(eg->data ?: eg, type, ev);
        e_object_unref(data);
     }
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_e_grab_dialog_mouse_handler(void *data, int type __UNUSED__, Ecore_Event_Mouse_Button *ev)
{
   E_Grab_Dialog *eg = data;

   if (ev->window != eg->grab_win) return ECORE_CALLBACK_RENEW;

   if (eg->mouse)
     {
        e_object_ref(data);
        eg->mouse(eg->data ?: eg, type, ev);
        e_object_unref(data);
     }
   else
     e_object_del(data);
   return ECORE_CALLBACK_RENEW;
}

static void
_e_grab_dialog_free(E_Grab_Dialog *eg)
{
   if (eg->grab_win)
     {
        e_grabinput_release(eg->grab_win, eg->grab_win);
        ecore_x_window_free(eg->grab_win);
     }
   E_FREE_LIST(eg->handlers, ecore_event_handler_del);

   e_object_del(E_OBJECT(eg->dia));
   free(eg);
}

static void
_e_grab_dialog_delete(E_Win *win)
{
   E_Dialog *dia;
   E_Grab_Dialog *eg;

   dia = win->data;
   eg = dia->data;
   e_object_del(E_OBJECT(eg));
}

static void
_e_grab_dialog_dia_del(void *data)
{
   E_Dialog *dia = data;

   e_object_del(dia->data);
}

EAPI E_Grab_Dialog *
e_grab_dialog_show(E_Win *parent, Eina_Bool is_mouse, Ecore_Event_Handler_Cb key, Ecore_Event_Handler_Cb mouse, Ecore_Event_Handler_Cb wheel, const void *data)
{
   E_Manager *man;
   E_Container *con;
   E_Grab_Dialog *eg;
   Ecore_Event_Handler *eh;

   if (parent)
     {
        con = parent->container;
        man = con->manager;
        e_border_focus_set(parent->border, 0, 1);
     }
   else
     {
        man = e_manager_current_get();
        con = e_container_current_get(man);
     }

   eg = E_OBJECT_ALLOC(E_Grab_Dialog, E_GRAB_DIALOG_TYPE, _e_grab_dialog_free);
   if (!eg) return NULL;

   if (is_mouse)
     {
        eg->dia = e_dialog_new(con, "E", "_mousebind_getmouse_dialog");
        e_dialog_title_set(eg->dia, _("Mouse Binding Sequence"));
        e_dialog_icon_set(eg->dia, "preferences-desktop-mouse", 48);
        e_dialog_text_set(eg->dia, TEXT_PRESS_MOUSE_BINIDING_SEQUENCE);
     }
   else
     {
        eg->dia = e_dialog_new(con, "E", "_keybind_getkey_dialog");
        e_dialog_title_set(eg->dia, _("Key Binding Sequence"));
        e_dialog_icon_set(eg->dia, "preferences-desktop-keyboard-shortcuts", 48);
        e_dialog_text_set(eg->dia, TEXT_PRESS_KEY_SEQUENCE);
     }
   eg->dia->data = eg;
   e_win_centered_set(eg->dia->win, 1);
   e_win_borderless_set(eg->dia->win, 1);
   e_object_del_attach_func_set(E_OBJECT(eg->dia), _e_grab_dialog_dia_del);
   e_win_delete_callback_set(eg->dia->win, _e_grab_dialog_delete);

   eg->grab_win = ecore_x_window_input_new(man->root, 0, 0, 1, 1);
   ecore_x_window_show(eg->grab_win);
   e_grabinput_get(eg->grab_win, 0, eg->grab_win);

   eg->key = key;
   eg->mouse = mouse;
   eg->wheel = wheel;
   eg->data = (void*)data;

   eh = ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, (Ecore_Event_Handler_Cb)_e_grab_dialog_key_handler, eg);
   eg->handlers = eina_list_append(eg->handlers, eh);
   eh = ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_DOWN, (Ecore_Event_Handler_Cb)_e_grab_dialog_mouse_handler, eg);
   eg->handlers = eina_list_append(eg->handlers, eh);
   if (wheel)
     {
        eh = ecore_event_handler_add(ECORE_EVENT_MOUSE_WHEEL, (Ecore_Event_Handler_Cb)_e_grab_dialog_wheel_handler, eg);
        eg->handlers = eina_list_append(eg->handlers, eh);
     }
   e_dialog_show(eg->dia);
   e_border_layer_set(eg->dia->win->border, E_LAYER_ABOVE);
   if (parent)
     e_dialog_parent_set(eg->dia, parent);
   return eg;
}
