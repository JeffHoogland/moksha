#ifndef E_OBSERVER_H
#define E_OBSERVER_H

#include <Ecore.h>

#include "e.h"
#include "object.h"

#define E_OBSERVER(x) ((E_Observer*)(x))
#define E_OBSERVEE(x) ((E_Observee*)(x))

typedef struct _e_observer E_Observer;
typedef struct _e_observee E_Observee;

typedef void(*E_Notify_Func)(E_Observer *observer, E_Observee *observee);

typedef enum _e_event_type
{
   E_EVENT_WINDOW_FOCUS_IN,
   E_EVENT_WINDOW_ICONIFY,
   E_EVENT_WINDOW_UNICONIFY,
   E_EVENT_WINDOW_MAXIMIZE,
   E_EVENT_DESKTOP_SWITCH,
   E_EVENT_MAX
} E_Event_Type;

struct _e_observer
{
  E_Object               obj;

  Evas_List              watched;      /* list<E_Observee> */
  E_Event_Type       event;
  E_Notify_Func          notify_func;
};

struct _e_observee
{
  E_Object               obj;

  Evas_List              observers;    /* list<E_Observer> */
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
void    e_observer_init(E_Observer *obs, E_Event_Type event,
			E_Notify_Func notify_func,
			E_Cleanup_Func cleanup_func);

/**
 * e_observer_cleanup - Cleans up an observer.
 * @obs:            The observer to clean up
 *
 * This function cleans up an observer by unregistering all observees.
 */
void    e_observer_cleanup(E_Observer *obs);

/**
 * e_observer_register_observee - Registers an observee
 * @observer:       The observer which registers the observee
 * @observee:       The observee which registers the observer
 *
 * This function registers the observer in the observee and vice versa.
 */
void    e_observer_register_observee(E_Observer *observer, E_Observee *observee);

/**
 * e_observer_unregister_observee - Unregisters an observee
 * @observer:       The observer which unregisters the observee
 * @observee:       The observee which unregisters the observer
 *
 * This function unregisters the observer in the observee and vice versa.
 */
void    e_observer_unregister_observee(E_Observer *observer, E_Observee *observee);

/**
 * e_observee_init - Initializes an observee.
 * @obs:            The observee to initialize
 * @cleanup_func:      The destructor function for cleaning this observee up
 *
 * This function initializes an observee. Observees are derived
 * from E_Objects, which is why this function gets the destructor
 * function as a parameter. It is passed on to e_object_init().
 */
void    e_observee_init(E_Observee *obs, E_Cleanup_Func cleanup_func);

/**
 * e_observee_cleanup - Cleans up an observee.
 * @obs:            The observee to clean up
 *
 * This function cleans up an observee by unregistering it from all observers.
 */
void    e_observee_cleanup(E_Observee *obs);

/**
 * e_observee_notify_observers - Notify observers of a given Ecore event
 * @o:              The observee which notifies its observers
 * @event:          The event by which to filter the observers
 *
 * This function scans the registered observers in the observee
 * and calls the notify_func() of the observers that are
 * responsible for the given @event.
 */
void    e_observee_notify_observers(E_Observee *o, E_Event_Type event);


#endif
