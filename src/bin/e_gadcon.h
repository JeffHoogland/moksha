/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef enum _E_Gadcon_Type
{
   E_GADCON_TYPE_FREEFLOAT,
   E_GADCON_TYPE_ARRANGE,
} E_Gadcon_Type;

typedef enum _E_Gadcon_Orient
{
   /* generic orientations */
   E_GADCON_ORIENT_HORIZ,
     E_GADCON_ORIENT_VERT,
     /* specific oreintations */
     E_GADCON_ORIENT_LEFT,
     E_GADCON_ORIENT_RIGHT,
     E_GADCON_ORIENT_TOP,
     E_GADCON_ORIENT_BOTTOM
} E_Gadcon_Orient;

typedef struct _E_Gadcon        E_Gadcon;
typedef struct _E_Gadcon_Client E_Gadcon_Client;

#else
#ifndef E_GADCON_H
#define E_GADCON_H

#define E_GADCON_TYPE 0xE0b01022
#define E_GADCON_CLIENT_TYPE 0xE0b01023

struct _E_Gadcon
{
   E_Object             e_obj_inherit;

   E_Gadcon_Type        type;
   struct {
      Evas_Object      *o_parent;
      char             *swallow_name;
      E_Gadcon_Orient   orient;
   } edje;
   struct {
      Ecore_Evas       *ee;
   } ecore_evas;
   
   Evas                *evas;
   Evas_Object         *o_container;
   Evas_List           *clients;
};

struct _E_Gadcon_Client
{
   E_Object             e_obj_inherit;
   E_Gadcon            *gadcon;
   
   Evas_Object         *o_base;
   struct {
      Evas_Coord        w, h;
   } min, max, asked, given;
};

EAPI int              e_gadcon_init(void);
EAPI int              e_gadcon_shutdown(void);
EAPI E_Gadcon        *e_gadcon_swallowed_new(Evas_Object *obj, char *swallow_name);
EAPI E_Gadcon        *e_gadcon_freefloat_new(Ecore_Evas *ee);
EAPI void             e_gadcon_resize(E_Gadcon *gc, int w, int h);
EAPI void             e_gadcon_orient_set(E_Gadcon *gc, E_Gadcon_Orient orient);



EAPI E_Gadcon_Client *e_gadcon_client_new(E_Gadcon *gc);

#endif
#endif
