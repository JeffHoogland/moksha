#ifndef E_EPPLET_H
#define E_EPPLET_H

#include "e.h"
#include "view.h"
#include "observer.h"
#include "e_ferite.h"

typedef struct _E_Epplet E_Epplet;
typedef struct _E_Epplet_Context E_Epplet_Context;
typedef struct _E_Epplet_CB_Info E_Epplet_CB_Info;
typedef struct _Evas_Object_Wrapper Evas_Object_Wrapper;
typedef struct _E_Epplet_Observer E_Epplet_Observer;

struct _E_Epplet_Observer
{
   E_Observer o;

   FeriteScript *script;
   FeriteFunction *func;
   FeriteObject *data;
};

struct _E_Epplet_CB_Info
{
   FeriteScript *script;
   FeriteFunction  *func;
   FeriteObject *data;
   FeriteObject *data2;
};

struct _E_Epplet_Context
{
  char *name;
  E_View *view;
  FeriteScript *script;

  struct {
    double x, y;
    double w, h;
  } geom;
};
					 

struct _E_Epplet
{
	E_Object	o;
	
	E_Epplet_Context *context;

	char		*name;
	E_View		*view;
	Ebits_Object	 bits;

	FeriteVariable *fbits;

	struct {
		double	x, y;
		double w, h;
	} current, requested, offset;

	struct {
		int changed;
		int moving;
		struct {
			int up, down, left, right;
		}resizing;
	} state;

	void				*data;

};

struct _Evas_Object_Wrapper
{
   Evas evas;
   Evas_Object obj;
};

void e_epplet_load_from_layout(E_View *v);
E_Epplet_Context *e_epplet_get_context_from_script(FeriteScript *script);
void e_epplet_script_load(E_Epplet_Context *v, char *script_path);
void e_epplet_set_common_callbacks(E_Epplet *epp);
E_Epplet_CB_Info *e_epplet_cb_new( FeriteScript *script, char *func_name,
                  FeriteObject *data, FeriteObject *data2 );
void e_epplet_cb_cleanup( E_Epplet_CB_Info *cb);
void e_epplet_bits_cb (void *_data, Ebits_Object _o,  char *_c,
      int _b, int _x, int _y, int _ox, int _oy, int _ow, int _oh);
void e_epplet_evas_cb (void *_data, Evas _e, Evas_Object _o,
                       int _b, int _x, int _y);
void e_epplet_timer_func(int val, void *data);
E_Epplet_Observer *e_epplet_observer_new( FeriteScript *script,
                   char *func_name, FeriteObject *data, char *event_type);
void e_epplet_observer_register_desktops(E_Epplet_Observer *obs);
void e_epplet_desktop_observer_func(E_Observer *observer, E_Observee *observee);
/*void e_epplet_border_observer_func(E_Observer *observer, E_Observee *observee);*/

#endif
