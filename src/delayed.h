#ifndef E_DELAYED_H
#define E_DELAYED_H

#include "e.h"

typedef struct _E_Delayed_Action E_Delayed_Action;

struct _E_Delayed_Action {
	OBS_PROPERTIES;

	double delay;
	void (*delay_func)(int val, void *obj);
};

#define E_DELAYED_ACT_INIT(_e_da, _e_act, _e_delay, _e_act_cb) \
{ \
    OBS_INIT(_e_da, _e_act, e_delayed_action_start, e_delayed_action_free); \
    _e_da->delay = _e_delay; \
    _e_da->delay_func = _e_act_cb; \
}

void e_delayed_action_start(void *obs, void *obj);
void e_delayed_action_cancel(void *obs);
void e_delayed_action_free(void *obs);

#endif

