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

typedef enum _E_Gadcon_Site
{
   E_GADCON_SITE_UNKNOWN = 0,        // when target site is unknown
   /* generic sities */
   E_GADCON_SITE_SHELF,
   E_GADCON_SITE_DESKTOP,
   E_GADCON_SITE_TOOLBAR,            // generic toolbar
   E_GADCON_SITE_EFM_TOOLBAR         // filemanager window toolbar
} E_Gadcon_Site;

#define E_GADCON_CLIENT_STYLE_PLAIN "plain"
#define E_GADCON_CLIENT_STYLE_INSET "inset"

typedef struct _E_Gadcon E_Gadcon;
typedef struct _E_Gadcon_Client E_Gadcon_Client;
typedef struct _E_Gadcon_Client_Class E_Gadcon_Client_Class;
typedef struct _E_Gadcon_Location E_Gadcon_Location;

#else
#ifndef E_GADCON_H
#define E_GADCON_H

#define E_GADCON_TYPE 0xE0b01006
#define E_GADCON_CLIENT_TYPE 0xE0b01007

struct _E_Gadcon
{
   E_Object e_obj_inherit;

   const char *name;
   int id;

   E_Gadcon_Layout_Policy layout_policy;

   struct 
     {
	Evas_Object *o_parent;
	const char *swallow_name;
     } edje;
   Ecore_Evas *ecore_evas;
   E_Zone *zone;

   E_Gadcon_Orient orient;

   Evas *evas;
   Evas_Object *o_container;
   Eina_List *clients;

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
   struct 
     {
	void (*func) (void *data, E_Gadcon *gc, const E_Gadcon_Client_Class *cc);
	void *data;
     } populate_class;
   struct 
     {
	void (*func) (void *data, int lock);
	void *data;
     } locked_set;
   struct 
     {
	void (*func) (void *data);
	void *data;
     } urgent_show;
   
   E_Config_Dialog *config_dialog;
   unsigned char editing : 1;
   Ecore_X_Window dnd_win, xdnd_win;
   E_Shelf *shelf;
   E_Toolbar *toolbar;
   E_Gadcon_Location *location;

   E_Drop_Handler *drop_handler;

   E_Config_Gadcon *cf;

   unsigned char instant_edit : 1;
};

#define GADCON_CLIENT_CLASS_VERSION 3
/* Version 3 add the *client_class param to icon(),label(),id_new(), id_del() */
/*           and the *orient param to orient() */
struct _E_Gadcon_Client_Class
{
   int   version;
   /* All members below are part of version 1 */
   const char *name;
   struct 
     {
	E_Gadcon_Client *(*init)     (E_Gadcon *gc, const char *name, const char *id, const char *style);
	void             (*shutdown) (E_Gadcon_Client *gcc);
	void             (*orient)   (E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
	char            *(*label)    (E_Gadcon_Client_Class *client_class);
	Evas_Object     *(*icon)     (E_Gadcon_Client_Class *client_class, Evas *evas);
	/* All members below are part of version 2 */
	/* Create new id, so that the gadcon client can refer to a config set inside the module */
	const char      *(*id_new)   (E_Gadcon_Client_Class *client_class);
	/* Del an id when a gadcon client is removed from the system */
	void             (*id_del)   (E_Gadcon_Client_Class *client_class, const char *id);
	/* All members below are part of version 3 */
	Eina_Bool        (*is_site)  (E_Gadcon_Site site);
     } func;
   char *default_style;
};

struct _E_Gadcon_Client
{
   E_Object e_obj_inherit;
   E_Gadcon *gadcon;
   const char *name;
   int id;
   Evas_Object *o_base;
   Evas_Object *o_box;
   Evas_Object *o_frame;
   Evas_Object *o_control;
   Evas_Object *o_event;
   const E_Gadcon_Client_Class *client_class;
   void *data;

   struct 
     {
	int pos, size, res;                 //gadcon
	double pos_x, pos_y, size_w, size_h;   //gadman
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
	Evas_Coord w, h;
     } pad, min, aspect;

   Ecore_Timer *scroll_timer;
   Ecore_Timer *instant_edit_timer;
   Ecore_Animator *scroll_animator;
   double scroll_pos, scroll_wanted;

   struct 
     {
	void *data;
	void (*func) (void *data);
     } scroll_cb;

   E_Menu *menu;
   const char *style;
   unsigned char autoscroll : 1;
   unsigned char resizable : 1;
   unsigned char moving : 1;
   unsigned char resizing : 1;
   unsigned char autoscroll_set : 1;
   Evas_Coord dx, dy;

   struct 
     {
	int x, y;
     } drag;

   unsigned char hidden : 1;

   E_Config_Gadcon_Client *cf;
};

/* defines usable gadget placements such as Desktop, Shelf #, etc */
/* next fields are mandatory (not NULL): name, add_gadget.func, remove_gadget.func */
struct _E_Gadcon_Location
{
   /* location name */
   const char * name;
   /* icon related to location, such as "preferences-desktop-shelf" for shelves, "preferences-desktop" for menus */
   const char * icon_name;
   E_Gadcon_Site site;
   /* adds gadcon client to location. Returns nonzero on success */
   struct 
     {
	int (*func) (void *data, const E_Gadcon_Client_Class *cc);
	void *data;
     } gadget_add;
   /* removes existing gadcon client from location */
   struct 
     {
	void (*func) (void *data, E_Gadcon_Client *gcc);
	void *data;
     } gadget_remove;
};

EINTERN int              e_gadcon_init(void);
EINTERN int              e_gadcon_shutdown(void);
EAPI void             e_gadcon_provider_register(const E_Gadcon_Client_Class *cc);
EAPI void             e_gadcon_provider_unregister(const E_Gadcon_Client_Class *cc);
EAPI Eina_List       *e_gadcon_provider_list(void);
EAPI E_Gadcon        *e_gadcon_swallowed_new(const char *name, int id, Evas_Object *obj, const char *swallow_name);
EAPI void             e_gadcon_custom_new(E_Gadcon *gc);
EAPI void             e_gadcon_custom_del(E_Gadcon *gc);
EAPI void             e_gadcon_swallowed_min_size_set(E_Gadcon *gc, Evas_Coord w, Evas_Coord h);
EAPI void             e_gadcon_min_size_request_callback_set(E_Gadcon *gc, void (*func) (void *data, E_Gadcon *gc, Evas_Coord w, Evas_Coord h), void *data);
EAPI void             e_gadcon_size_request_callback_set(E_Gadcon *gc, void (*func) (void *data, E_Gadcon *gc, Evas_Coord w, Evas_Coord h), void *data);
EAPI void             e_gadcon_frame_request_callback_set(E_Gadcon *gc, Evas_Object *(*func) (void *data, E_Gadcon_Client *gcc, const char *style), void *data);
EAPI void             e_gadcon_populate_callback_set(E_Gadcon *gc, void (*func) (void *data, E_Gadcon *gc, const E_Gadcon_Client_Class *cc), void *data);
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
EAPI void             e_gadcon_util_lock_func_set(E_Gadcon *gc, void (*func) (void *data, int lock), void *data);
EAPI void             e_gadcon_util_urgent_show_func_set(E_Gadcon *gc, void (*func) (void *data), void *data);
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
EAPI int              e_gadcon_client_viewport_geometry_get(E_Gadcon_Client *gcc, int *x, int *y, int *w, int *h);
EAPI E_Zone          *e_gadcon_client_zone_get(E_Gadcon_Client *gcc);
EAPI E_Menu          *e_gadcon_client_util_menu_items_append(E_Gadcon_Client *gcc, E_Menu *menu_gadget, int flags);
EAPI void             e_gadcon_client_util_menu_attach(E_Gadcon_Client *gcc);
EAPI void             e_gadcon_locked_set(E_Gadcon *gc, int lock);
EAPI void             e_gadcon_urgent_show(E_Gadcon *gc);

/* site helpers */

EAPI Eina_Bool        e_gadcon_site_is_shelf(E_Gadcon_Site site);
EAPI Eina_Bool        e_gadcon_site_is_desktop(E_Gadcon_Site site);
EAPI Eina_Bool        e_gadcon_site_is_efm_toolbar(E_Gadcon_Site site);

EAPI Eina_Bool        e_gadcon_site_is_any_toolbar(E_Gadcon_Site site); // all toolbar sities
EAPI Eina_Bool        e_gadcon_site_is_not_toolbar(E_Gadcon_Site site); // all non-toolbar sities

/* location helpers */

EAPI E_Gadcon_Location *
e_gadcon_location_new(const char * name, E_Gadcon_Site site,
		       int (*add_func) (void *data, const E_Gadcon_Client_Class *cc),
		       void *add_data,
		       void (*remove_func) (void *data, E_Gadcon_Client *cc),
		       void *remove_data);
EAPI void e_gadcon_location_free(E_Gadcon_Location *loc);
EAPI void e_gadcon_location_register (E_Gadcon_Location *loc);
EAPI void e_gadcon_location_unregister (E_Gadcon_Location *loc);
EAPI void e_gadcon_location_set_icon_name(E_Gadcon_Location *loc, const char *name);
EAPI void e_gadcon_client_add_location_menu(E_Gadcon_Client *gcc, E_Menu *menu);

#define GADCON_CLIENT_CONFIG_GET(_type, _items, _gc_class, _id)		\
  if (!_id)								\
    {									\
       char buf[128];							\
       int num = 0;							\
       _type *ci;							\
       if (_items)							\
	 {								\
	    const char *p;						\
	    ci = eina_list_last(_items)->data;				\
	    p = strrchr (ci->id, '.');					\
	    if (p) num = atoi (p + 1) + 1;				\
	 }								\
       snprintf (buf, sizeof (buf), "%s.%d", _gc_class.name, num);	\
       _id = buf;							\
    }									\
  else									\
    {									\
       Eina_List *l;							\
       _type *ci;							\
       EINA_LIST_FOREACH(_items, l, ci)					\
	 if ((ci->id) && (!strcmp(ci->id, id))) return ci;		\
    }

#endif
#endif
