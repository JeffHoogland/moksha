#ifdef E_TYPEDEFS

typedef enum _E_Gadman_Policy
{
   /* type */
   E_GADMAN_ANYWHERE = 0, 
   E_GADMAN_EDGES = 1, 
   E_GADMAN_LEFT_EDGE = 2, 
   E_GADMAN_RIGHT_EDGE = 3, 
   E_GADMAN_TOP_EDGE = 4, 
   E_GADMAN_BOTTOM_EDGE = 5, 
   /* extra flags */
   E_GADMAN_FIXED_ZONE = 1 << 8,
   E_GADMAN_HSIZE = 1 << 9,
   E_GADMAN_VSIZE = 1 << 10,
   E_GADMAN_HMOVE = 1 << 11,
   E_GADMAN_VMOVE = 1 << 12
} E_Gadman_Policy;

typedef struct _E_Gadman        E_Gadman;
typedef struct _E_Gadman_Client E_Gadman_Client;

#else
#ifndef E_GADMAN_H
#define E_GADMAN_H

struct _E_Gadman
{
   E_Object             e_obj_inherit;

   E_Container         *container;
   Evas_List           *clients;
};

struct _E_Gadman_Client
{
   E_Object             e_obj_inherit;

   E_Gadman            *gadman;
};

EAPI int              e_gadman_init(void);
EAPI int              e_gadman_shutdown(void);
EAPI E_Gadman        *e_gadman_new(E_Container *con);
EAPI E_Gadman_Client *e_gadman_client_new(E_Gadman *gm);
EAPI void             e_gadman_client_policy_set(E_Gadman_Client *gmc, E_Gadman_Policy pol);
EAPI void             e_gadman_client_min_size_set(E_Gadman_Client *gmc, Evas_Coord minw, Evas_Coord minh);
EAPI void             e_gadman_client_max_size_set(E_Gadman_Client *gmc, Evas_Coord minw, Evas_Coord minh);
EAPI void             e_gadman_client_default_align_set(E_Gadman_Client *gmc, double xalign, double yalign);
  
#endif
#endif
