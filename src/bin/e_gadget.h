#ifdef E_TYPEDEFS

typedef struct _E_Gadget E_Gadget;
typedef struct _E_Gadget_Face E_Gadget_Face;
typedef struct _E_Gadget_Change E_Gadget_Change;
typedef struct _E_Gadget_Api E_Gadget_Api;

#else 
#ifndef E_GADGET_H
#define E_GADGET_H

#define E_GADGET_TYPE 0xE0b01021

struct _E_Gadget_Api
{
  E_Module *module;
  const char *name;
  int per_zone; /* 1 - one face per zone, 0 - one per container */
  void (*func_face_init) (void *data, E_Gadget_Face *gadget_face);
  void (*func_face_free) (void *data, E_Gadget_Face *gadget_face);
  void (*func_change) (void *data, E_Gadget_Face *gadget_face, E_Gadman_Client *gmc, E_Gadman_Change change);
  void (*func_menu_init) (void *data, E_Gadget *gadget);
  void (*func_face_menu_init) (void *data, E_Gadget_Face *gadget_face);
  void *data;
};

struct _E_Gadget
{
  E_Object e_obj_inherit;
  Evas_List *faces;

  E_Module *module;
  const char *name;
  E_Menu *menu;

  struct {
    void (*face_init) (void *data, E_Gadget_Face *gadget_face);
    void (*face_free) (void *data, E_Gadget_Face *gadget_face);
    void (*change) (void *data, E_Gadget_Face *gadget_face, E_Gadman_Client *gmc, E_Gadman_Change change);
    void (*menu_init) (void *data, E_Gadget *gadget);
    void (*face_menu_init) (void *data, E_Gadget_Face *gadget_face);
  } funcs;

  void *data;
};

struct _E_Gadget_Face
{
  E_Gadget *gad;
  E_Container *con;
  E_Zone *zone;
  Evas *evas;

  E_Menu *menu;
  void *conf;

  Evas_Object *main_obj;
  Evas_Object *event_obj;
  E_Gadman_Client *gmc;

  void *data;
};

struct _E_Gadget_Change
{
  E_Gadget *gadget;
  E_Gadget_Face *face;
};

E_Gadget *e_gadget_new(E_Gadget_Api *api);

void e_gadget_face_theme_set(E_Gadget_Face *face, char *category, char *group);

#endif
#endif
