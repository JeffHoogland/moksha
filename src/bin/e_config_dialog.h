/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef enum _E_Config_Dialog_CFData_Type
{
   E_CONFIG_DIALOG_CFDATA_TYPE_BASIC,
   E_CONFIG_DIALOG_CFDATA_TYPE_ADVANCED
} E_Config_Dialog_CFData_Type;

typedef struct _E_Config_Dialog      E_Config_Dialog;
typedef struct _E_Config_Dialog_View E_Config_Dialog_View;

#else
#ifndef E_CONFIG_DIALOG_H
#define E_CONFIG_DIALOG_H

#define E_CONFIG_DIALOG_TYPE 0xE0b01017

struct _E_Config_Dialog_View
{
   void        *(*create_cfdata)  (void *cfdata_other, E_Config_Dialog_CFData_Type type_other);
   void         (*free_cfdata)    (void *cfdata);
   void         (*apply_cfdata)   (void *cfdata);
   Evas_Object *(*create_widgets) (Evas *evas, void *cfdata);
};

struct _E_Config_Dialog
{
   E_Object                     e_obj_inherit;
   
   E_Config_Dialog_CFData_Type  view_type;
   E_Config_Dialog_View         basic, advanced;
   void                        *cfdata;
   E_Container                 *con;
   char                        *title;
   E_Dialog                    *dia;
};

EAPI E_Config_Dialog *e_config_dialog_new(E_Container *con, char *title, E_Config_Dialog_View *basic, E_Config_Dialog_View *advanced);

#endif
#endif
