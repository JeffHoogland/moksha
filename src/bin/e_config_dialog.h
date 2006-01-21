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
typedef struct _E_Config_Dialog_Data E_Config_Dialog_Data;

#else
#ifndef E_CONFIG_DIALOG_H
#define E_CONFIG_DIALOG_H

#define E_CONFIG_DIALOG_TYPE 0xE0b01017

struct _E_Config_Dialog_View
{
   void           *(*create_cfdata)     (E_Config_Dialog *cfd);
   void            (*free_cfdata)       (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
   /* Seems that every user of this structure allocates it on the stack and doesn't clear it, 
    * so I can't rely on this being NULL.  I currently set it to NULL in e_config_dialog_new()
    * and if you want to use it, set it in create_widgets();
    */
   int             (*close_cfdata)      (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
   struct {
      int          (*apply_cfdata)      (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
      Evas_Object *(*create_widgets)    (E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
   } basic, advanced;
};

struct _E_Config_Dialog
{
   E_Object                     e_obj_inherit;
   
   E_Config_Dialog_CFData_Type  view_type;
   E_Config_Dialog_View         view;
   E_Config_Dialog_Data        *cfdata;
   E_Container                 *con;
   char                        *title;
   char                        *icon;
   int                          icon_size;
   E_Dialog                    *dia;
   void                        *data;
   int                          view_dirty;
   int                          hide_buttons;
};

EAPI E_Config_Dialog *e_config_dialog_new(E_Container *con, char *title, char *icon, int icon_size, E_Config_Dialog_View *view, void *data);

#endif
#endif
