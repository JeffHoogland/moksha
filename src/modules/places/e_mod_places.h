#ifndef E_MOD_PLACES_H
#define E_MOD_PLACES_H

typedef enum
{
   MOUNT_OP_NONE,
   MOUNT_OP_MOUNT,
   MOUNT_OP_UMOUNT,
   MOUNT_OP_EJECT
} Mount_Op;

typedef struct _Volume Volume;
struct _Volume
{
   const char *id;
   int perc_backup;
   
   const char *label;
   const char *icon;
   const char *device;
   const char *mount_point;
   const char *fstype;
   unsigned long long size;
   unsigned long long free_space;
   Eina_Bool mounted;


   const char *bus;
   const char *drive_type;
   const char *model;
   const char *vendor;
   const char *serial;
   unsigned char removable;
   unsigned char requires_eject;
   Eina_Bool unlocked;
   Eina_Bool encrypted;

   Eina_Bool valid;
   Eina_Bool to_mount;
   Eina_Bool force_open;
   Eina_List *objs;

   void (*mount_func)(Volume *vol, Eina_List *opts);
   void (*unmount_func)(Volume *vol, Eina_List *opts);
   void (*eject_func)(Volume *vol, Eina_List *opts);
};

void places_init(void);
void places_shutdown(void);

Volume *places_volume_add(const char *id, Eina_Bool first_time);
void places_volume_del(Volume *v);
void places_volume_update(Volume *vol);
Volume *places_volume_by_id_get(const char *id);
Eina_List *places_volume_list_get(void);
void places_volume_mount(Volume *vol);
void places_volume_unmount(Volume *vol);
void places_volume_eject(Volume *vol);

Evas_Object *places_main_obj_create(Evas *canvas);
void places_update_all_gadgets(void);
void places_fill_box(Evas_Object *box, Eina_Bool horiz);
void places_empty_box(Evas_Object *box);
void places_print_volume(Volume *v);

void places_generate_menu(void *data, E_Menu *em);
void places_augmentation(void *data, E_Menu *em);


#endif

