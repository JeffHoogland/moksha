#ifndef E_OBSERVER_H
#define E_OBSERVER_H

#include <Ecore.h>

#include "e.h"
#include "object.h"

#define E_OBSERVER(x) ((E_Observer*)(x))
#define E_OBSERVEE(x) ((E_Observee*)(x))

typedef struct _e_observer E_Observer;
typedef struct _e_observee E_Observee;

typedef enum _e_event_type
{
   /* basic event types */
   E_EVENT_BORDER_NEW = 1 << 0,
   E_EVENT_BORDER_DELETE = 1 << 1,
   E_EVENT_BORDER_FOCUS_IN = 1 << 2,
   E_EVENT_BORDER_FOCUS_OUT = 1 << 3,
   E_EVENT_BORDER_ICONIFY = 1 << 4,
   E_EVENT_BORDER_UNICONIFY = 1 << 5,
   E_EVENT_BORDER_MAXIMIZE = 1 << 6,
   E_EVENT_BORDER_UNMAXIMIZE = 1 << 7,
   E_EVENT_BORDER_MOVE = 1 << 8,
   E_EVENT_BORDER_RESIZE = 1 << 9,
   E_EVENT_BORDER_RAISE = 1 << 10,
   E_EVENT_BORDER_LOWER = 1 << 11,

   E_EVENT_DESKTOP_NEW = 1 << 12,
   E_EVENT_DESKTOP_DELETE = 1 << 13,
   E_EVENT_DESKTOP_SWITCH = 1 << 14,
   E_EVENT_DESKTOP_SCROLL = 1 << 15,

   E_EVENT_FILE_ADD = 1 << 16,
   E_EVENT_FILE_CHANGE = 1 << 17,
   E_EVENT_FILE_DELETE = 1 << 18,
   E_EVENT_FILE_INFO = 1 << 19,
   
   E_EVENT_BG_CHANGED = 1 << 20,
   E_EVENT_ICB_CHANGED = 1 << 21,
   E_EVENT_LAYOUT_CHANGED = 1 << 22,

   /* meta event types */
   E_EVENT_BORDER_ALL = E_EVENT_BORDER_NEW |
      E_EVENT_BORDER_DELETE | E_EVENT_BORDER_FOCUS_IN |
      E_EVENT_BORDER_ICONIFY | E_EVENT_BORDER_UNICONIFY |
      E_EVENT_BORDER_MAXIMIZE | E_EVENT_BORDER_UNMAXIMIZE,
   E_EVENT_DESKTOP_ALL = E_EVENT_DESKTOP_NEW |
      E_EVENT_DESKTOP_DELETE | E_EVENT_DESKTOP_SWITCH,

   /* ALL events */
   E_EVENT_MAX = 0xFFFFFFFF
}
E_Event_Type;

typedef void        (*E_Notify_Func) (E_Observer * observer,
				      E_Observee * observee,
				      E_Event_Type event,
				      void *data);

struct _e_observer
{
   E_Object            obj;

   Evas_List *           watched;	/* list<E_Observee> */
   E_Event_Type        event;
   E_Notify_Func       notify_func;
};

struct _e_observee
{
   E_Object            obj;

   Evas_List *           observers;	/* list<E_Observer> */
};

/**
 * e_observer_init - Initializes an observer
 * @obs:            The observer to initialize
 * @event:          The Ecore event for which this observer will be responsible
 * @notify_func:    The function the observer calls when it gets notified
 * @cleanup_func:   The destructor function to pass to the E_Object initializer
 *
 * This function initializes an observer. Observees can register observers,
 * which will call the given @notify_func when an observer issues an
 * e_observee_notify_observers() call. Observers are derived from
 * E_Objects, therefore, this function also handles E_Object initalization.
 */
void                e_observer_init(E_Observer * obs, E_Event_Type event,
				    E_Notify_Func notify_func,
				    E_Cleanup_Func cleanup_func);

/**
 * e_observer_cleanup - Cleans up an observer.
 * @obs:            The observer to clean up
 *
 * This function cleans up an observer by unregistering all observees.
 */
void                e_observer_cleanup(E_Observer * obs);

/**
 * e_observer_register_observee - Registers an observee
 * @observer:       The observer which registers the observee
 * @observee:       The observee which registers the observer
 *
 * This function registers the observer in the observee and vice versa.
 */
void                e_observer_register_observee(E_Observer * observer,
						 E_Observee * observee);

/**
 * e_observer_unregister_observee - Unregisters an observee
 * @observer:       The observer which unregisters the observee
 * @observee:       The observee which unregisters the observer
 *
 * This function unregisters the observer in the observee and vice versa.
 */
void                e_observer_unregister_observee(E_Observer * observer,
						   E_Observee * observee);

/**
 * e_observee_init - Initializes an observee.
 * @obs:            The observee to initialize
 * @cleanup_func:      The destructor function for cleaning this observee up
 *
 * This function initializes an observee. Observees are derived
 * from E_Objects, which is why this function gets the destructor
 * function as a parameter. It is passed on to e_object_init().
 */
void                e_observee_init(E_Observee * obs,
				    E_Cleanup_Func cleanup_func);

/**
 * e_observee_cleanup - Cleans up an observee.
 * @obs:            The observee to clean up
 *
 * This function cleans up an observee by unregistering it from all observers.
 */
void                e_observee_cleanup(E_Observee * obs);

/**
 * e_observee_notify_observers - Notify observers of a given Ecore event
 * @o:              The observee which notifies its observers
 * @event:          The event by which to filter the observers
 * @data:           arbitrary data attached to the event
 *
 * This function scans the registered observers in the observee
 * and calls the notify_func() of the observers that are
 * responsible for the given @event.
 */
void                e_observee_notify_observers(E_Observee * o,
						E_Event_Type event,
						void *data
						);

/**
 * e_observee_notify_all_observers - Notify all observers of a given E event
 * regardless of whether they are registered or not.
 * 
 * @o:              The observee which notifies the observers
 * @event:          The event by which to filter the observers
 * @data:           arbitrary data attached to the event
 *
 * This function scans ALL observers in the observee
 * and calls the notify_func() of the observers that are
 * responsible for the given @event. Useful for situations where the observee
 * is just being created and you want to notify observers of its existence.
 * If they are looking for this type of NEW event, then they can register
 * it as a legitimate observee.
 */
void                e_observee_notify_all_observers(E_Observee * o,
						    E_Event_Type event,
						    void *data);
#endif
