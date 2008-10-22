/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

#define E_GADCON_CLIENT(x) ((E_Gadcon_Client *)(x))

/* different layout policies - only 1 supported for now */
typedef enum _E_Gadcon_Layout_Policy
{
   E_GADCON_LAYOUT_POLICY_PANEL
} E_Gadcon_Layout_Policy;

typedef enum _E_Gadcon_Orient
{
   /* generic orientations */
   E_GADCON_ORIENT_FLOAT,
     E_GADCON_ORIENT_HORIZ,
     E_GADCON_ORIENT_VERT,
     E_GADCON_ORIENT_LEFT,
     E_GADCON_ORIENT_RIGHT,
     E_GADCON_ORIENT_TOP,
     E_GADCON_ORIENT_BOTTOM,
     E_GADCON_ORIENT_CORNER_TL,
     E_GADCON_ORIENT_CORNER_TR,
     E_GADCON_ORIENT_CORNER_BL,
     E_GADCON_ORIENT_CORNER_BR,
     E_GADCON_ORIENT_CORNER_LT,
     E_GADCON_ORIENT_CORNER_RT,
     E_GADCON_ORIENT_CORNER_LB,
     E_GADCON_ORIENT_CORNER_RB
} E_Gadcon_Orient;

#define E_GADCON_CLIENT_STYLE_PLAIN "plain"
#define E_GADCON_CLIENT_STYLE_INSET "inset"

typedef struct _E_Gadcon              E_Gadcon;
typedef struct _E_Gadcon_Client       E_Gadcon_Client;
typedef struct _E_Gadcon_Client_Class E_Gadcon_Client_Class;

#else
#ifndef E_GADCON_H
#define E_GADCON_H

#define E_GADCON_TYPE 0xE0b01006
#define E_GADCON_CLIENT_TYPE 0xE0b01007

struct _E_Gadcon
{
   E_Object             e_obj_inherit;

   const char          *name;
   int                  id;
   
   E_Gadcon_Layout_Policy layout_policy;
   
   struct 
     {
	Evas_Object    *o_parent;
	const char     *swallow_name;
     } edje;
   Ecore_Evas          *ecore_evas;
   E_Zone              *zone;
   
   E_Gadcon_Orient      orient;
   
   Evas                *evas;
   Evas_Object         *o_container;
   Eina_List           *clients;
   
   struct 
     {
	void (*func) (void *data, E_Gadcon *gc, Evas_Coord w, Evas_Coord h);
	void *data;
     } resize_request, min_size_request;
   struct 
     {
	Evas_Object *(*func) (void *data, E_Gadcon_Client *gcc, const char *style);
	void *data;
     } frame_request;
   struct 
     {
	void (*func) (void *data, E_Gadcon_Client *gcc, E_Menu *menu);
	void *data;
     } menu_attach;
   
   E_Config_Dialog    *config_dialog;
   unsigned char       editing : 1;
   Ecore_X_Window      dnd_win;
   Ecore_X_Window      xdnd_win;
   E_Shelf            *shelf;
   E_Toolbar          *toolbar;

   E_Drop_Handler *drop_handler;

   E_Config_Gadcon *cf;

   unsigned char          instant_edit : 1;
};

#define GADCON_CLIENT_CLASS_VERSION 2
struct _E_Gadcon_Client_Class
{
   int   version;
   /* All members below are part of version 1 */
   const char *name;
   struct 
     {
	E_Gadcon_Client *(*init)     (E_Gadcon *gc, const char *name, const char *id, const char *style);
	void             (*shutdown) (E_Gadcon_Client *gcc);
	void             (*orient)   (E_Gadcon_Client *gcc);
	char            *(*label)    (void);
	Evas_Object     *(*icon)     (Evas *evas);
	/* All members below are part of version 2 */
	/* Create new id, so that the gadcon client can refer to a config set inside the module */
	const char      *(*id_new)   (void);
	/* Del an id when a gadcon client is removed from the system */
	void             (*id_del)   (const char *id);
	/* All members below are part of version 3 */
     } func;
   char *default_style;
};

struct _E_Gadcon_Client
{
   E_Object               e_obj_inherit;
   E_Gadcon              *gadcon;
   const char            *name;
   int                    id;
   Evas_Object           *o_base;
   Evas_Object           *o_box;
   Evas_Object           *o_frame;
   Evas_Object           *o_control;
   Evas_Object           *o_event;
   const E_Gadcon_Client_Class *client_class;
   void                  *data;
   struct 
     {
	int               pos, size, res;                 //gadcon
	double            pos_x, pos_y, size_w, size_h;   //gadman
     } config; 

   struct 
     { 
	int seq, flags; /* goes to save */
	int state, resist;
	int prev_pos, prev_size;
	int want_save : 1;
     } state_info;

   struct 
     {
	Evas_Coord        w, h;
     } pad, min, aspect;
   Ecore_Timer           *scroll_timer;
   Ecore_Timer           *instant_edit_timer;
   Ecore_Animator        *scroll_animator;
   double                 scroll_pos;
   double                 scroll_wanted;
   struct 
     {
	void *data;
	void (*func) (void *data);
     } scroll_cb;

   E_Menu                *menu;
   const char            *style;
   unsigned char          autoscroll : 1;
   unsigned char          resizable : 1;
   
   unsigned char          moving : 1;
   unsigned char          resizing : 1;
   Evas_Coord             dx, dy;

   struct 
     {
	int x, y;
     } drag;

   unsigned char       hidden : 1;

   E_Config_Gadcon_Client *cf;
};

EAPI int              e_gadcon_init(void);
EAPI int              e_gadcon_shutdown(void);
EAPI void             e_gadcon_provider_register(const E_Gadcon_Client_Class *cc);
EAPI void             e_gadcon_provider_unregister(const E_Gadcon_Client_Class *cc);
EAPI Eina_List       *e_gadcon_provider_list(void);
EAPI E_Gadcon        *e_gadcon_swallowed_new(const char *name, int id, Evas_Object *obj, char *swallow_name);
EAPI void             e_gadcon_swallowed_min_size_set(E_Gadcon *gc, Evas_Coord w, Evas_Coord h);
EAPI void             e_gadcon_min_size_request_callback_set(E_Gadcon *gc, void (*func) (void *data, E_Gadcon *gc, Evas_Coord w, Evas_Coord h), void *data);
EAPI void             e_gadcon_size_request_callback_set(E_Gadcon *gc, void (*func) (void *data, E_Gadcon *gc, Evas_Coord w, Evas_Coord h), void *data);
EAPI void             e_gadcon_frame_request_callback_set(E_Gadcon *gc, Evas_Object *(*func) (void *data, E_Gadcon_Client *gcc, const char *style), void *data);
EAPI void             e_gadcon_layout_policy_set(E_Gadcon *gc, E_Gadcon_Layout_Policy layout_policy);
EAPI void             e_gadcon_populate(E_Gadcon *gc);
EAPI void             e_gadcon_unpopulate(E_Gadcon *gc);
EAPI void             e_gadcon_populate_class(E_Gadcon *gc, const E_Gadcon_Client_Class *cc);
EAPI void             e_gadcon_orient(E_Gadcon *gc, E_Gadcon_Orient orient);
EAPI void             e_gadcon_edit_begin(E_Gadcon *gc);
EAPI void             e_gadcon_edit_end(E_Gadcon *gc);
EAPI void             e_gadcon_all_edit_begin(void);
EAPI void             e_gadcon_all_edit_end(void);
EAPI void             e_gadcon_zone_set(E_Gadcon *gc, E_Zone *zone);
EAPI E_Zone          *e_gadcon_zone_get(E_Gadcon *gc);
EAPI void             e_gadcon_ecore_evas_set(E_Gadcon *gc, Ecore_Evas *ee);
EAPI int              e_gadcon_canvas_zone_geometry_get(E_Gadcon *gc, int *x, int *y, int *w, int *h);
EAPI void             e_gadcon_util_menu_attach_func_set(E_Gadcon *gc, void (*func) (void *data, E_Gadcon_Client *gcc, E_Menu *menu), void *data);
EAPI void             e_gadcon_dnd_window_set(E_Gadcon *gc, Ecore_X_Window win);
EAPI Ecore_X_Window   e_gadcon_dnd_window_get(E_Gadcon *gc);
EAPI void             e_gadcon_xdnd_window_set(E_Gadcon *gc, Ecore_X_Window win);
EAPI Ecore_X_Window   e_gadcon_xdnd_window_get(E_Gadcon *gc);
EAPI void             e_gadcon_shelf_set(E_Gadcon *gc, E_Shelf *shelf);
EAPI E_Shelf         *e_gadcon_shelf_get(E_Gadcon *gc);
EAPI void             e_gadcon_toolbar_set(E_Gadcon *gc, E_Toolbar *toolbar);
EAPI E_Toolbar       *e_gadcon_toolbar_get(E_Gadcon *gc);
EAPI E_Config_Gadcon_Client *e_gadcon_client_config_new(E_Gadcon *gc, const char *name);
EAPI void             e_gadcon_client_config_del(E_Config_Gadcon *cf_gc, E_Config_Gadcon_Client *cf_gcc);
EAPI E_Gadcon_Client *e_gadcon_client_new(E_Gadcon *gc, const char *name, const char *id, const char *style, Evas_Object *base_obj);
EAPI void             e_gadcon_client_edit_begin(E_Gadcon_Client *gcc);
EAPI void             e_gadcon_client_edit_end(E_Gadcon_Client *gcc);
EAPI void             e_gadcon_client_show(E_Gadcon_Client *gcc);
EAPI void             e_gadcon_client_hide(E_Gadcon_Client *gcc);
EAPI void             e_gadcon_client_size_request(E_Gadcon_Client *gcc, Evas_Coord w, Evas_Coord h);
EAPI void             e_gadcon_client_min_size_set(E_Gadcon_Client *gcc, Evas_Coord w, Evas_Coord h);
EAPI void             e_gadcon_client_aspect_set(E_Gadcon_Client *gcc, int w, int h);
EAPI void             e_gadcon_client_autoscroll_set(E_Gadcon_Client *gcc, int autoscroll);
EAPI void             e_gadcon_client_autoscroll_update(E_Gadcon_Client *gcc, int mx, int my);
EAPI void             e_gadcon_client_autoscroll_cb_set(E_Gadcon_Client *gcc, void (*func)(void *data), void *data);
EAPI void             e_gadcon_client_resizable_set(E_Gadcon_Client *gcc, int resizable);
EAPI int	      e_gadcon_client_geometry_get(E_Gadcon_Client *gcc, int *x, int *y, int *w, int *h);
EAPI void             e_gadcon_client_util_menu_items_append(E_Gadcon_Client *gcc, E_Menu *menu, int flags);
EAPI void             e_gadcon_client_util_menu_attach(E_Gadcon_Client *gcc);

#endif
#endif
